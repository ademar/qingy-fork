/***************************************************************************
                            pm.c  -  description
                            --------------------
    begin                : Sep 08 2006
    copyright            : (C) 2006 by Noberasco Michele
    e-mail               : michele.noberasco@tiscali.it
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.              *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <directfb.h>

#include "memmgmt.h"
#include "load_settings.h"
#include "logger.h"

IDirectFB *dfb;

DFBEnumerationResult getScreens (DFBScreenID screen_id, DFBScreenDescription desc, void *callbackdata)
{
	IDirectFBScreen ***screens = (IDirectFBScreen ***)callbackdata;
	static int i=0;

	/* just to make gcc happy */
	if (callbackdata) {}

	if (!i)
		*screens = (IDirectFBScreen **) calloc(2, sizeof(IDirectFBScreen *));
	else
	{
		IDirectFBScreen **temp = (IDirectFBScreen **) realloc(*screens, (i+2)*sizeof(IDirectFBScreen *));
		if (!temp)
		{
			writelog(ERROR, "Memory allocation failure!\n");
			abort();
		}
		*screens = temp;
	}

	if (dfb->GetScreen(dfb, screen_id, &((*screens)[i])) != DFB_OK)
		writelog(DEBUG, "Could not get screen instance...\n");
	else
	{
		WRITELOG(DEBUG, "Found new screen (%d so far): %s, ", ++i, desc.name);
		if (desc.caps & DSCCAPS_POWER_MANAGEMENT)
			writelog(DEBUG, "has power management support\n");
		else
		{
			writelog(DEBUG, "has no power management support\n");
			i--;
		}
	}

	(*screens)[i] = NULL;


	return DFB_OK;
}

void screen_pm_thread(IDirectFB *directfb)
{
	IDirectFBScreen      **screens = NULL;
	IDirectFBEventBuffer  *events;
	int                    status  = 0;

	if (!directfb) return;

	dfb = directfb;

	/* get available screens with power management capabilities */
	dfb->EnumScreens(dfb, getScreens, &screens);

	/* get an input event buffer */
	dfb->CreateInputEventBuffer (dfb, DICAPS_ALL, DFB_TRUE, &events);

	while (1)
	{
		int i;
		int hasevents=0;
		int timeout=screen_power_management_timeout*60;

		events->WaitForEventWithTimeout (events, timeout, 0);
		while (events->HasEvent(events) == DFB_OK)
		{
			hasevents=1;
			events->Reset(events);
		}

		if (!hasevents)
		{
			if (!status)
			{
				for (i=0; screens[i]; i++)
					screens[i]->SetPowerMode(screens[i], DSPM_OFF);
				status=1;
			}
		}
		else
		{
			if (status)
			{
				for (i=0; screens[i]; i++)
					screens[i]->SetPowerMode(screens[i], DSPM_ON);
				status=0;
			}
		}

	}
/* 	DSPM_STANDBY */
/* 	DSPM_SUSPEND */
}
