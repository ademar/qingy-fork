/***************************************************************************
                      directfb_label.c  -  description
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <directfb.h>

#include "directfb_label.h"
#include "framebuffer_mode.h"
#include "load_settings.h"


void Plot(Label *thiz)
{
	if (!thiz || !thiz->surface) return;
	thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x00);
	if (thiz->text)
		switch (thiz->alignment)
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
	if (thiz->text)
	{
		free(thiz->text);
		thiz->text = NULL;
	}
	Plot(thiz);
}

void Label_SetText(Label *thiz, char *text, int alignment)
{
	if (!thiz || !text) return;
	if (thiz->text) Label_ClearText(thiz);
	thiz->text = (char *) calloc(strlen(text)+1, sizeof(char));
	strcpy(thiz->text, text);
	thiz->alignment = alignment;
	Plot(thiz);
}

void Label_SetFocus(Label *thiz, int focus)
{
	if (!thiz) return;

	if (focus)
	{
		thiz->window->RequestFocus(thiz->window);
		thiz->hasfocus=1;
		thiz->window->SetOpacity(thiz->window, SELECTED_WINDOW_OPACITY);
		return;
	}

	thiz->hasfocus = 0;
	thiz->window->SetOpacity(thiz->window, WINDOW_OPACITY);
	return;
}

void Label_Hide(Label *thiz)
{
	thiz->window->SetOpacity(thiz->window, 0x00);
}

void Label_Show(Label *thiz)
{
	if (thiz->hasfocus) thiz->window->SetOpacity(thiz->window, SELECTED_WINDOW_OPACITY);
	else thiz->window->SetOpacity(thiz->window, WINDOW_OPACITY);
}

void Label_Destroy(Label *thiz)
{
	if (!thiz) return;
	if (thiz->text) free(thiz->text);
	if (thiz->surface) thiz->surface->Release (thiz->surface);
	if (thiz->window) thiz->window->Release (thiz->window);
	free(thiz);
}

Label *Label_Create(IDirectFBDisplayLayer *layer, IDirectFBFont *font, DFBWindowDescription *window_desc)
{
	Label *newlabel = NULL;

	newlabel = (Label *) calloc(1, sizeof(Label));
	newlabel->text     = NULL;
	newlabel->xpos     = (unsigned int) window_desc->posx;
	newlabel->ypos     = (unsigned int) window_desc->posy;
	newlabel->width    = window_desc->width;
	newlabel->height   = window_desc->height;
	newlabel->hasfocus = 0;
	newlabel->alignment= LEFT;
	newlabel->window   = NULL;
	newlabel->surface  = NULL;
	newlabel->SetFocus = Label_SetFocus;
	newlabel->SetText  = Label_SetText;
	newlabel->ClearText= Label_ClearText;
	newlabel->Hide     = Label_Hide;
	newlabel->Show     = Label_Show;
	newlabel->Destroy  = Label_Destroy;

	if (layer->CreateWindow (layer, window_desc, &(newlabel->window)) != DFB_OK) return NULL;
	newlabel->window->SetOpacity(newlabel->window, 0x00 );
	newlabel->window->GetSurface(newlabel->window, &(newlabel->surface));
	newlabel->surface->Clear(newlabel->surface, 0x00, 0x00, 0x00, 0x00);
	newlabel->surface->Flip(newlabel->surface, NULL, 0);
	newlabel->surface->SetFont (newlabel->surface, font);
	newlabel->surface->SetColor (newlabel->surface, MASK_TEXT_COLOR_R, MASK_TEXT_COLOR_G, MASK_TEXT_COLOR_B, MASK_TEXT_COLOR_A);
	newlabel->window->RaiseToTop(newlabel->window);

	return newlabel;
}
