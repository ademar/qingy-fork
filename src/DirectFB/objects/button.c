/***************************************************************************
                          button.c  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003-2006 by Noberasco Michele
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
#include <directfb_keynames.h>
#include <pthread.h>

#include "memmgmt.h"
#include "button.h"
#include "load_settings.h"
#include "misc.h"
#include "logger.h"
#include "utils.h"

static void mouseOver(Button *thiz, int status)
{
	if (!thiz) return;

	if (thiz->mouseOver == status) return;

	thiz->mouseOver = status;

	thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x00);

	if (status)
	{
		thiz->surface->Blit(thiz->surface, thiz->mouseover, NULL, 0, 0);
		thiz->surface->Flip(thiz->surface, NULL, 0);
	}
	else
	{
		thiz->surface->Blit(thiz->surface, thiz->normal, NULL, 0, 0);
		thiz->surface->Flip(thiz->surface, NULL, 0);
	}
	
}

void Button_Show(Button *thiz)
{
	if (!thiz || !thiz->window) return;

	thiz->window->SetOpacity(thiz->window, button_opacity);
}

void Button_Hide(Button *thiz)
{
	if (!thiz || !thiz->window) return;

	thiz->window->SetOpacity(thiz->window, 0x00);
}

void Button_Destroy(Button *thiz)
{
	if (!thiz) return;

	pthread_cancel(thiz->events_thread);
	pthread_join(thiz->events_thread, NULL);

/* 	if (thiz->normal)    thiz->normal->Release    (thiz->normal); */
/* 	if (thiz->mouseover) thiz->mouseover->Release (thiz->mouseover); */
/* 	if (thiz->surface)   thiz->surface->Release   (thiz->surface); */
/* 	if (thiz->window)    thiz->window->Release    (thiz->window); */

/* 	free (thiz); */
}

static int mouse_over_button(Button *thiz)
{
	int mouse_x, mouse_y;

	thiz->layer->GetCursorPosition (thiz->layer, &mouse_x, &mouse_y);

	if ( ((mouse_x >= thiz->xpos) && (mouse_x <= (thiz->xpos + (int) thiz->width)))  &&
			 ((mouse_y >= thiz->ypos) && (mouse_y <= (thiz->ypos + (int) thiz->height)))  )
		return 1;

	return 0;
}

static void button_thread(Button *thiz)
{
	DFBInputEvent evt;
	int status = 0;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,   NULL);
	pthread_setcanceltype (PTHREAD_CANCEL_DEFERRED, NULL);

	while (1)
	{
		pthread_testcancel();

		thiz->events->WaitForEventWithTimeout (thiz->events, 0, 100);
		while (thiz->events->HasEvent(thiz->events) == DFB_OK)
		{
			thiz->events->GetEvent (thiz->events, DFB_EVENT (&evt));
			switch (evt.type)
			{
				case DIET_AXISMOTION:
				{
					if (mouse_over_button(thiz))
						mouseOver(thiz, 1);
					else
						mouseOver(thiz, 0);

					break;
				}
				case DIET_BUTTONPRESS:
				{
					if (thiz->mouseOver)
						status=1;
					else
						status=0;

					break;
				}
				case DIET_BUTTONRELEASE:
				{
					if (thiz->mouseOver && status)
					{
						status = 0;
						thiz->callback(thiz);
					}
					else
						status = 0;

					break;
				}
				default: /* we do nothing here */
					break;
			}
		}
	}
}

Button *Button_Create(const char *normal, const char *mouseover, int xpos, int ypos, void (*callback)(struct _Button *thiz), IDirectFBDisplayLayer *layer, IDirectFBSurface *primary, IDirectFB *dfb, float x_ratio, float y_ratio)
{
	Button *but;
	IDirectFBWindow *window;
	IDirectFBSurface *surface;
	DFBWindowDescription window_desc;

	if (!normal || !mouseover || !layer || !primary || !dfb) return NULL;

	but = (Button *) calloc (1, sizeof (Button));
	but->normal = load_image(normal, dfb, x_ratio, y_ratio);
	if (!but->normal)
	{
		free(but);
		return NULL;
	}
	but->normal->GetSize (but->normal, (int *)&(but->width), (int *)&(but->height));
	but->mouseover = load_image(mouseover, dfb, x_ratio, y_ratio);
	if (!but->mouseover)
	{
		but->normal->Release (but->normal);
		free(but);
		return NULL;
	}
	but->xpos        = xpos;
	but->ypos        = ypos;
	but->Destroy     = Button_Destroy;
	but->Show        = Button_Show;
	but->Hide        = Button_Hide;
	but->callback    = callback;

	window_desc.flags  = ( DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS );
	window_desc.posx   = (unsigned int) but->xpos;
	window_desc.posy   = (unsigned int) but->ypos;
	window_desc.width  = but->width;
	window_desc.height = but->height;
	window_desc.caps   = DWCAPS_ALPHACHANNEL;

	if (layer->CreateWindow (layer, &window_desc, &window) != DFB_OK) return NULL;
	window->SetOpacity( window, 0x00 );
	window->RaiseToTop( window );
	window->GetSurface( window, &surface );
	window->SetOpacity( window, button_opacity );

	but->mouseOver = 1;
	but->surface = surface;
	but->window = window;
	mouseOver(but, 0);
	but->layer = layer;

	dfb->CreateInputEventBuffer (dfb, DICAPS_ALL, DFB_TRUE, &(but->events));
	pthread_create(&(but->events_thread), NULL, (void *) button_thread, but);

	return but;
}
