/***************************************************************************
                     directfb_combobox.c  -  description
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

#include "directfb_combobox.h"
#include "framebuffer_mode.h"
#include "load_settings.h"

void PlotEvent(ComboBox *thiz)
{
	if (!thiz) return;

	thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x00);
	thiz->surface->DrawString (thiz->surface, thiz->selected->name, -1, 0, 0, DSTF_LEFT|DSTF_TOP);
	thiz->surface->Flip(thiz->surface, NULL, 0);
}

void ComboBox_KeyEvent(ComboBox *thiz, int direction)
{
	if (!thiz || !thiz->selected) return;

	switch (direction)
	{
		case UP:
			if (thiz->selected != thiz->selected->prev)
			{
				thiz->selected = thiz->selected->prev;
				PlotEvent(thiz);
			}
			break;
		case DOWN:
			if (thiz->selected != thiz->selected->next)
			{
				thiz->selected = thiz->selected->next;
				PlotEvent(thiz);
			}
			break;
		case REDRAW:
			PlotEvent(thiz);
			break;
	}
}

void ComboBox_SetFocus(ComboBox *thiz, int focus)
{
	if (!thiz) return;

	if (focus)
	{
		thiz->window->RequestFocus(thiz->window);
		thiz->hasfocus=1;
		thiz->window->SetOpacity(thiz->window, SELECTED_WINDOW_OPACITY);
		ComboBox_KeyEvent(thiz, REDRAW);
		return;
	}

	thiz->hasfocus = 0;
	thiz->window->SetOpacity(thiz->window, WINDOW_OPACITY);
	ComboBox_KeyEvent(thiz, REDRAW);
	return;
}

void ComboBox_AddItem(ComboBox *thiz, char *object)
{
	item *curr;

	if (!thiz || !object) return;
	if (!thiz->items)
	{
		thiz->items = (item *) calloc(1, sizeof(item));
		thiz->items->next = thiz->items;
		thiz->items->prev = thiz->items;
		thiz->items->name = (char *) calloc(strlen(object)+1, sizeof(char));
		strcpy(thiz->items->name, object);
		thiz->selected = thiz->items;
		return;
	}
	curr = thiz->items;
	while(curr->next != thiz->items) curr = curr->next;
	curr->next = (item *) calloc(1, sizeof(item));
	curr->next->next = thiz->items;
	curr->next->prev = curr;
	thiz->items->prev = curr->next;
	curr->next->name = (char *) calloc(strlen(object)+1, sizeof(char));
	strcpy(curr->next->name, object);
}

void ComboBox_ClearItems(ComboBox *thiz)
{
	item *curr;

	if (!thiz || !thiz->items) return;
	thiz->selected = NULL;
	curr = thiz->items->prev;
	while (curr != thiz->items)
	{
		free(curr->name); curr->name = NULL;
		curr->next = NULL;
		curr = curr->prev;
		curr->next->prev = NULL;
		free(curr->next);
	}
	curr->next = NULL;
	curr->prev = NULL;
	free(curr->name); curr->name = NULL;
	curr = NULL;
	free(thiz->items); thiz->items = NULL;
}

void ComboBox_Hide(ComboBox *thiz)
{
	thiz->window->SetOpacity(thiz->window, 0x00);
}

void ComboBox_Show(ComboBox *thiz)
{
	if (thiz->hasfocus) thiz->window->SetOpacity(thiz->window, SELECTED_WINDOW_OPACITY);
	else thiz->window->SetOpacity(thiz->window, WINDOW_OPACITY);
}

void ComboBox_Destroy(ComboBox *thiz)
{
	if (!thiz) return;
	ComboBox_ClearItems(thiz);
	if (thiz->surface) thiz->surface->Release (thiz->surface);
	if (thiz->window) thiz->window->Release (thiz->window);
	free(thiz);
}

ComboBox *ComboBox_Create(IDirectFBDisplayLayer *layer, IDirectFBFont *font, DFBWindowDescription *window_desc)
{
	ComboBox *newbox = NULL;
	DFBResult err;

	newbox = (ComboBox *) calloc(1, sizeof(ComboBox));
	newbox->items      = NULL;
	newbox->selected   = NULL;
	newbox->xpos       = (unsigned int) window_desc->posx;
	newbox->ypos       = (unsigned int) window_desc->posy;
	newbox->width      = window_desc->width;
	newbox->height     = window_desc->height;
	newbox->hasfocus   = 0;
	newbox->position   = 0;
	newbox->window     = NULL;
	newbox->surface    = NULL;
	newbox->KeyEvent   = ComboBox_KeyEvent;
	newbox->SetFocus   = ComboBox_SetFocus;
	newbox->AddItem    = ComboBox_AddItem;
	newbox->ClearItems = ComboBox_ClearItems;
	newbox->Hide       = ComboBox_Hide;
	newbox->Show       = ComboBox_Show;
	newbox->Destroy    = ComboBox_Destroy;

	DFBCHECK(layer->CreateWindow (layer, window_desc, &(newbox->window)));
	newbox->window->SetOpacity(newbox->window, 0x00 );
	newbox->window->GetSurface(newbox->window, &(newbox->surface));
	newbox->surface->Clear(newbox->surface, 0x00, 0x00, 0x00, 0x00);
	newbox->surface->Flip(newbox->surface, NULL, 0);
	newbox->surface->SetFont (newbox->surface, font);
	newbox->surface->SetColor (newbox->surface, MASK_TEXT_COLOR_R, MASK_TEXT_COLOR_G, MASK_TEXT_COLOR_B, MASK_TEXT_COLOR_A);
	newbox->window->RaiseToTop(newbox->window);

	return newbox;
}
