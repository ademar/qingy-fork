/***************************************************************************
                       screen_saver.c  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003 by Noberasco Michele
    e-mail               : noberasco.gnu@educ.disi.unige.it
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


#include <stdlib.h>
#include <directfb.h>
#include "screen_saver.h"

int screen_width;
int screen_height;


void pixel_screen_saver(IDirectFBSurface *surface);
void photo_screen_saver(IDirectFBSurface *surface);

void activate_screen_saver(ScreenSaver *thiz)
{
	void (*do_screen_saver)(IDirectFBSurface *surface);

	if (!thiz) return;
	if (!thiz->surface) return;
	if (!thiz->events) return;

	thiz->surface->GetSize(thiz->surface, &screen_width, &screen_height);

	switch (thiz->kind)
	{
		case PHOTO_SCREENSAVER:
			do_screen_saver = photo_screen_saver;
			break;
		case PIXEL_SCREENSAVER: /* fall to default */
		default:
			do_screen_saver = pixel_screen_saver;
	}

	/* do screen saver until an input event arrives */
	while (1)
	{
		thiz->events->WaitForEventWithTimeout(thiz->events, thiz->seconds, thiz->milli_seconds);
		if (thiz->events->HasEvent(thiz->events) == DFB_OK) break;
		do_screen_saver(thiz->surface);
	}
}



void pixel_screen_saver(IDirectFBSurface *surface)
{
	static int toggle = 1;
	int width  = screen_width  / 200;
	int height = screen_height / 300;
	int posx   = rand() % screen_width;
	int posy   = rand() % screen_height;

	if (!surface) return;

	if (toggle)
	{
		surface->Clear (surface, 0x00, 0x00, 0x00, 0xFF);
		surface->SetColor (surface, 0x50, 0x50, 0x50, 0xFF);
		surface->FillRectangle (surface, posx, posy, width, height);
		surface->Flip (surface, NULL, DSFLIP_BLIT);
		toggle = 0;
	}
	else
	{
		surface->Clear (surface, 0x00, 0x00, 0x00, 0xFF);
		surface->Flip (surface, NULL, DSFLIP_BLIT);
		toggle = 1;
	}
}

void photo_screen_saver(IDirectFBSurface *surface)
{
	/* not implemented yet... */
	if (!surface) return;
	return;
}
