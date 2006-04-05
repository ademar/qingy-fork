/***************************************************************************
                      directfb_label.c  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003-2005 by Noberasco Michele
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
#include "directfb_mode.h"
#include "misc.h"


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

void Label_ClearText(Label *thiz)
{
	if (!thiz) return;
	free(thiz->text);
	Plot(thiz);
}

static int *label_thread(Label *thiz)
{

	fprintf(stderr, "thread id: %d\n", (int)(thiz->thread_id));

	while (1)
	{
		char *message = assemble_message(thiz->content, thiz->command);
		thiz->SetText(thiz, message);
		free(message);
		sleep(thiz->polltime);
	}
}

void Label_SetAction(Label *thiz, int polltime, char *content, char *command)
{
	if (!thiz) return;

	thiz->polltime = polltime;
	thiz->content  = strdup(content);
	thiz->command  = strdup(command);

	if (!polltime)
	{
		char *message = assemble_message(thiz->content, thiz->command);
		thiz->SetText(thiz, message);
		free(message);
		return;
	}

	pthread_create(&(thiz->thread_id), NULL, (void *) label_thread, thiz);
}

void Label_SetText(Label *thiz, char *text)
{
	if (!thiz || !text) return;
	free(thiz->text);
	thiz->text = strdup(text);
	Plot(thiz);
}

void Label_SetTextOrientation(Label *thiz, int orientation)
{
	if (!thiz) return;
	thiz->text_orientation = orientation;
}

void Label_SetTextColor(Label *thiz, color_t *text_color)
{
	if (!thiz)       return;
	if (!text_color) return;

	thiz->text_color.R = text_color->R;
	thiz->text_color.G = text_color->G;
	thiz->text_color.B = text_color->B;
	thiz->text_color.A = text_color->A;
	thiz->surface->SetColor (thiz->surface, text_color->R, text_color->G, text_color->B, text_color->A);	
}

void Label_SetFocus(Label *thiz, int focus)
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
	return;
}

void Label_Hide(Label *thiz)
{
	thiz->window->SetOpacity(thiz->window, 0x00);
}

void Label_Show(Label *thiz)
{
	if (thiz->hasfocus) thiz->window->SetOpacity(thiz->window, selected_window_opacity);
	else thiz->window->SetOpacity(thiz->window, window_opacity);
}

void Label_Destroy(Label *thiz)
{
	if (!thiz) return;
	if (thiz->text) free(thiz->text);
	if (thiz->surface) thiz->surface->Release (thiz->surface);
	if (thiz->window) thiz->window->Release (thiz->window);
	free(thiz);
}

Label *Label_Create(IDirectFBDisplayLayer *layer, IDirectFBFont *font, color_t *text_color, DFBWindowDescription *window_desc)
{
	Label *newlabel = NULL;

	newlabel = (Label *) calloc(1, sizeof(Label));
	newlabel->text               = NULL;
	newlabel->xpos               = (unsigned int) window_desc->posx;
	newlabel->ypos               = (unsigned int) window_desc->posy;
	newlabel->width              = window_desc->width;
	newlabel->height             = window_desc->height;
	newlabel->hasfocus           = 0;
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
	newlabel->ClearText          = Label_ClearText;
	newlabel->Hide               = Label_Hide;
	newlabel->Show               = Label_Show;
	newlabel->Destroy            = Label_Destroy;
	newlabel->SetAction          = Label_SetAction;
	newlabel->thread_id          = 0;

	if (layer->CreateWindow (layer, window_desc, &(newlabel->window)) != DFB_OK) return NULL;
	newlabel->window->SetOpacity(newlabel->window, 0x00 );
	newlabel->window->GetSurface(newlabel->window, &(newlabel->surface));
	newlabel->surface->Clear(newlabel->surface, 0x00, 0x00, 0x00, 0x00);
	newlabel->surface->Flip(newlabel->surface, NULL, 0);
	newlabel->surface->SetFont (newlabel->surface, font);
	newlabel->surface->SetColor (newlabel->surface, newlabel->text_color.R, newlabel->text_color.G, newlabel->text_color.B, newlabel->text_color.A);
	newlabel->window->RaiseToTop(newlabel->window);

	return newlabel;
}
