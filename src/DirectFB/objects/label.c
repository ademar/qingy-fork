/***************************************************************************
                      directfb_label.c  -  description
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
#include <string.h>
#include <directfb.h>
#include <unistd.h>
#include <pthread.h>

#include "memmgmt.h"
#include "load_settings.h"
#include "label.h"
#include "misc.h"
#include "logger.h"


static void Plot(Label *thiz)
{
	if (!thiz || !thiz->surface) return;

	thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x00);

	if (thiz->text)
		switch (thiz->text_orientation)
		{
			case LEFT:
				thiz->surface->DrawString (thiz->surface, thiz->text, -1, 0, 0, DSTF_LEFT|DSTF_TOP);
				break;
			case CENTER:
				thiz->surface->DrawString (thiz->surface, thiz->text, -1, thiz->width/2, thiz->height/2, DSTF_CENTER);
				break;
			case RIGHT:
				thiz->surface->DrawString (thiz->surface, thiz->text, -1, thiz->width, thiz->height/2, DSTF_RIGHT);
				break;
			case CENTERBOTTOM:
				thiz->surface->DrawString (thiz->surface, thiz->text, -1, thiz->width/2, thiz->height+1, DSTF_CENTER|DSTF_BOTTOM );
				break;
		}

	thiz->surface->Flip(thiz->surface, NULL, 0);
}

void Label_SetClickCallBack(Label *thiz, void *callback)
{
	if (!thiz) return;

	pthread_mutex_lock(&(thiz->lock));

	thiz->click_callback = callback;

	pthread_mutex_unlock(&(thiz->lock));
}

void Label_ClearText(Label *thiz)
{
	if (!thiz) return;

	pthread_mutex_lock(&(thiz->lock));

	free(thiz->text);
	Plot(thiz);

	pthread_mutex_unlock(&(thiz->lock));
}

static void setText(Label *thiz, char *text)
{
	if (!thiz || !text) return;

	free(thiz->text);
	thiz->text = strdup(text);
	Plot(thiz);
}

void Label_SetAction(Label *thiz, int polltime, char *content, char *command)
{
	char *message;

	if (!thiz) return;

	pthread_mutex_lock(&(thiz->lock));
	thiz->polltime = polltime;
	thiz->content  = strdup(content);
	thiz->command  = strdup(command);
	message = assemble_message(thiz->content, thiz->command);
	setText(thiz, message);
	free(message);
	pthread_mutex_unlock(&(thiz->lock));
}

void Label_SetText(Label *thiz, char *text)
{
	if (!thiz || !text) return;

	pthread_mutex_lock(&(thiz->lock));

	setText(thiz, text);

	pthread_mutex_unlock(&(thiz->lock));
}

void Label_SetTextOrientation(Label *thiz, int orientation)
{
	if (!thiz) return;

	pthread_mutex_lock(&(thiz->lock));
	thiz->text_orientation = orientation;
	pthread_mutex_unlock(&(thiz->lock));
}

void Label_SetTextColor(Label *thiz, color_t *text_color)
{
	if (!thiz)       return;
	if (!text_color) return;

	pthread_mutex_lock(&(thiz->lock));
	thiz->text_color.R = text_color->R;
	thiz->text_color.G = text_color->G;
	thiz->text_color.B = text_color->B;
	thiz->text_color.A = text_color->A;
	thiz->surface->SetColor (thiz->surface, text_color->R, text_color->G, text_color->B, text_color->A);	
	pthread_mutex_unlock(&(thiz->lock));
}

static void setFocus(Label *thiz, int focus)
{
	if (!thiz) return;

	if (focus)
	{
		thiz->window->RequestFocus(thiz->window);
		thiz->hasfocus=1;
		thiz->window->SetOpacity(thiz->window, selected_window_opacity);
		return;
	}

	thiz->hasfocus = 0;
	thiz->window->SetOpacity(thiz->window, window_opacity);
}

void Label_SetFocus(Label *thiz, int focus)
{
	if (!thiz) return;

	pthread_mutex_lock(&(thiz->lock));
	setFocus(thiz, focus);
	pthread_mutex_unlock(&(thiz->lock));
}

void Label_Hide(Label *thiz)
{
	if (!thiz) return;

	pthread_mutex_lock(&(thiz->lock));
	thiz->ishidden = 1;
	thiz->window->SetOpacity(thiz->window, 0x00);
	pthread_mutex_unlock(&(thiz->lock));
}

void Label_Show(Label *thiz)
{
	if (!thiz) return;

	pthread_mutex_lock(&(thiz->lock));
	thiz->ishidden = 0;
	if (thiz->hasfocus) thiz->window->SetOpacity(thiz->window, selected_window_opacity);
	else thiz->window->SetOpacity(thiz->window, window_opacity);
	pthread_mutex_unlock(&(thiz->lock));
}

void Label_Destroy(Label *thiz)
{
	if (!thiz) return;

	pthread_cancel(thiz->events_thread);
	pthread_join(thiz->events_thread, NULL);

	pthread_cancel(thiz->update_thread);
	pthread_join(thiz->update_thread, NULL);

/* 	if (thiz->surface) thiz->surface->Release(thiz->surface); */
/* 	if (thiz->window)  thiz->window->Release (thiz->window); */
/* 	free(thiz); */
}

static int *label_update_thread(Label *thiz)
{
	char *message;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,   NULL);
	pthread_setcanceltype (PTHREAD_CANCEL_DEFERRED, NULL);

	while (1)
	{
		int seconds = 1;

		pthread_testcancel();

		pthread_mutex_lock(&(thiz->lock));

		if (thiz->content && thiz->polltime)
		{
			message = assemble_message(thiz->content, thiz->command);
			setText(thiz, message);
			free(message);
		}

		if (thiz->polltime)
			seconds = thiz->polltime;

		pthread_mutex_unlock(&(thiz->lock));

		sleep(seconds);
	}
}

int mouse_over_label(Label *thiz)
{
	int mouse_x, mouse_y;

	thiz->layer->GetCursorPosition (thiz->layer, &mouse_x, &mouse_y);

	if ( ((mouse_x >= (int)thiz->xpos) && (mouse_x <= ((int)thiz->xpos + (int)thiz->width)))  &&
			 ((mouse_y >= (int)thiz->ypos) && (mouse_y <= ((int)thiz->ypos + (int)thiz->height)))  )
		return 1;

	return 0;
}

static int *label_events_thread(Label *thiz)
{
	DFBInputEvent evt;

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
				case DIET_BUTTONPRESS:
				{
					pthread_mutex_lock(&(thiz->lock));
				
					if (!(thiz->ishidden))
						if (mouse_over_label(thiz))
							if (thiz->click_callback)
							{
								thiz->click_callback(thiz);
								setFocus(thiz, 1);
							}

					pthread_mutex_unlock(&(thiz->lock));

					break;
				}
				default: /* we do nothing here */
					break;
			}
		}
	}
}

Label *Label_Create(IDirectFBDisplayLayer *layer, IDirectFB *dfb, IDirectFBFont *font, color_t *text_color, DFBWindowDescription *window_desc)
{
	Label *newlabel = NULL;

	newlabel = (Label *) calloc(1, sizeof(Label));
	newlabel->text               = NULL;
	newlabel->xpos               = (unsigned int) window_desc->posx;
	newlabel->ypos               = (unsigned int) window_desc->posy;
	newlabel->width              = window_desc->width;
	newlabel->height             = window_desc->height;
	newlabel->hasfocus           = 0;
	newlabel->ishidden           = 0;
	newlabel->text_orientation   = LEFT;
	newlabel->window             = NULL;
	newlabel->surface            = NULL;
	newlabel->text_color.R       = text_color->R;
	newlabel->text_color.G       = text_color->G;
	newlabel->text_color.B       = text_color->B;
	newlabel->text_color.A       = text_color->A;
	newlabel->SetFocus           = Label_SetFocus;
	newlabel->SetTextColor       = Label_SetTextColor;
	newlabel->SetText            = Label_SetText;
	newlabel->SetTextOrientation = Label_SetTextOrientation;
	newlabel->SetClickCallBack   = Label_SetClickCallBack;
	newlabel->click_callback     = NULL;
	newlabel->ClearText          = Label_ClearText;
	newlabel->Hide               = Label_Hide;
	newlabel->Show               = Label_Show;
	newlabel->Destroy            = Label_Destroy;
	newlabel->SetAction          = Label_SetAction;
	newlabel->events_thread      = 0;
	newlabel->update_thread      = 0;
	newlabel->layer              = layer;
	newlabel->content            = NULL;
	newlabel->command            = NULL;
	newlabel->polltime           = 0;

	if (layer->CreateWindow (layer, window_desc, &(newlabel->window)) != DFB_OK) return NULL;
	newlabel->window->SetOpacity(newlabel->window, 0x00 );
	newlabel->window->GetSurface(newlabel->window, &(newlabel->surface));
	newlabel->surface->Clear(newlabel->surface, 0x00, 0x00, 0x00, 0x00);
	newlabel->surface->Flip(newlabel->surface, NULL, 0);
	newlabel->surface->SetFont (newlabel->surface, font);
	newlabel->surface->SetColor (newlabel->surface, newlabel->text_color.R, newlabel->text_color.G, newlabel->text_color.B, newlabel->text_color.A);
	newlabel->window->RaiseToTop(newlabel->window);

	dfb->CreateInputEventBuffer (dfb, DICAPS_ALL, DFB_TRUE, &(newlabel->events));
	pthread_mutex_init(&(newlabel->lock), NULL);
	pthread_create(&(newlabel->update_thread), NULL, (void *) label_update_thread, newlabel);
	pthread_create(&(newlabel->events_thread), NULL, (void *) label_events_thread, newlabel);

	return newlabel;
}
