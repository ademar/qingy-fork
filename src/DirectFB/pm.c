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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <directfb.h>
#include <pthread.h>

#include "memmgmt.h"
#include "load_settings.h"
#include "logger.h"
#include "directfb_mode.h"
#include "pm.h"

#ifdef USE_SCREEN_SAVERS
#include "screen_saver.h"
typedef struct _ss_data
{
	char             *ss_name;
	IDirectFB        *ss_dfb;
	IDirectFBSurface *ss_surface;
	IDirectFBFont    *ss_font;
	pthread_mutex_t  *ss_lock;
	int               ss_timeout;
} ss_data;
#endif

IDirectFB *dfb;

static DFBEnumerationResult getScreens (DFBScreenID screen_id, DFBScreenDescription desc, void *callbackdata)
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
	int                    timeout = screen_power_management_timeout * 60;
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
}

#ifdef USE_SCREEN_SAVERS
static void screen_saver_thread(ss_data *data)
{
	IDirectFBEventBuffer  *events;
	int                    timeout;

	if (!data) return;

	timeout              = data->ss_timeout * 60;
	screen_saver_kind    = data->ss_name;
	screen_saver_surface = data->ss_surface;
	screen_saver_dfb     = data->ss_dfb;

	/* get an input event buffer */
	data->ss_dfb->CreateInputEventBuffer (data->ss_dfb, DICAPS_ALL, DFB_TRUE, &events);

	while (1)
	{
		int hasevents = 0;

		events->WaitForEventWithTimeout (events, timeout, 0);
		if (events->HasEvent(events) == DFB_OK)
		{
			hasevents = 1;
			events->Reset(events);
		}

		if (!hasevents)
			if (!pthread_mutex_trylock(data->ss_lock))
			{
				DFBInputEvent evt;

	      clear_screen();
	      data->ss_surface->Clear    (data->ss_surface, 0x00, 0x00, 0x00, 0xFF);
	      data->ss_surface->Flip     (data->ss_surface, NULL, DSFLIP_BLIT);
				data->ss_surface->SetFont  (data->ss_surface, data->ss_font);
				data->ss_surface->SetColor (data->ss_surface, other_text_color.R, other_text_color.G, other_text_color.B, other_text_color.A);
				activate_screen_saver(events);

				events->GetEvent (events, DFB_EVENT (&evt));
				reset_screen(&evt);

				pthread_mutex_unlock(data->ss_lock);
			}
	}
}

void do_ss(IDirectFB *dfb, IDirectFBSurface *surface, IDirectFBFont *font, int wait)
{
	pthread_t               ss_thread;
	static pthread_mutex_t  lock;
	ss_data                *data;
	static int              i = 0;

	if (!i)
	{
		pthread_mutex_init(&lock, NULL);
		i = 1;
	}

	if (!wait)
	{
		if (!pthread_mutex_trylock(&lock))
		{
			IDirectFBEventBuffer *events;
			DFBInputEvent         evt;

			dfb->CreateInputEventBuffer (dfb, DICAPS_ALL, DFB_TRUE, &events);

			clear_screen();
			surface->Clear    (surface, 0x00, 0x00, 0x00, 0xFF);
			surface->Flip     (surface, NULL, DSFLIP_BLIT);
			surface->SetFont  (surface, font);
			surface->SetColor (surface, other_text_color.R, other_text_color.G, other_text_color.B, other_text_color.A);
			activate_screen_saver(events);

			events->GetEvent (events, DFB_EVENT (&evt));
			reset_screen(&evt);

			pthread_mutex_unlock(&lock);
		}
		return;
	}

	data = (ss_data*)calloc(1, sizeof(ss_data));
	data->ss_name    = screensaver_name;
	data->ss_surface = surface;
	data->ss_dfb     = dfb;
	data->ss_font    = font;
	data->ss_timeout = screensaver_timeout;
	data->ss_lock    = &lock;

	pthread_create(&ss_thread, NULL, (void *) screen_saver_thread, data);
}
#endif
