/***************************************************************************
                         listbox.c  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003-2005 by Noberasco Michele
    e-mail               : s4t4n@gentoo.org
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

#include "memmgmt.h"
#include "load_settings.h"
#include "listbox.h"
#include "directfb_mode.h"
#include "misc.h"


void ListBox_PlotEvent(ListBox *thiz, int flip)
{
  if (!thiz) return;

  thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x00);
  thiz->surface->DrawString (thiz->surface, thiz->selected->name, -1, 4, 0, DSTF_LEFT|DSTF_TOP);
  if (flip) thiz->surface->Flip(thiz->surface, NULL, 0);
}

void ListBox_MouseOver(ListBox *thiz, int status)
{
	if (!thiz) return;

	thiz->mouse = status;

	ListBox_PlotEvent(thiz, 0);
	if (status)
	{
		IDirectFBSurface *where;
		DFBRectangle dest1, dest2, dest;
		IDirectFBFont *font;
		char *text = thiz->selected->name;

		thiz->surface->GetFont(thiz->surface,&font);		
		font->GetStringExtents(font, text, 0, &dest1, NULL);
		font->GetStringExtents(font, text, strlen(text), &dest2, NULL);
		font->GetStringWidth (font, text, 0, &(dest.x));
		dest.y = 0;
		dest.h = dest1.h;
		dest.w = dest2.w - dest1.w + 4 + dest.h;
		thiz->surface->GetSubSurface(thiz->surface, &dest, &where);
		where->SetColor (where, thiz->text_color.R, thiz->text_color.G, thiz->text_color.B, thiz->text_color.A);
		where->DrawRectangle (where, 0, 0, dest.w, dest.h);
		where->FillTriangle (where, dest.w - dest.h + dest.h/3, dest.h/3, dest.w - dest.h/3, dest.h/3, dest.w - (dest.h/2), dest.h - dest.h/3);
	}

	thiz->surface->Flip(thiz->surface, NULL, 0);
}

void ListBox_resize(ListBox *thiz, item *selection)
{
	/* now we set the combobox width */
	DFBRectangle rect1, rect2, rect3;
	IDirectFBFont *font;
	char *text = selection->name;

	thiz->surface->GetFont(thiz->surface,&font);
	font->GetStringExtents(font, text, 0, &rect1, NULL);
	font->GetStringExtents(font, text, strlen(text), &rect2, NULL);
	font->GetStringWidth (font, text, 0, &(rect3.x));
	rect3.y = 0;
	rect3.h = rect1.h;
	rect3.w = rect2.w - rect1.w + 4 + rect3.h;

	thiz->width = rect3.w;
}

/* void ListBox_setHeightNYpos(ListBox *thiz, int n_items) */
/* { */
/* 		IDirectFBFont *font; */
/* 		DFBRectangle   rect; */

/* 		thiz->surface->GetFont(thiz->surface,&font); */
/* 		font->GetStringExtents(font, ".", 0, &rect, NULL); */
/* 		thiz->height = rect.h * n_items; */
/* } */

void ListBox_SelectItem(ListBox *thiz, item *selection)
{
	if (!thiz)      return;
	if (!selection) return;

	thiz->selected = selection;
	ListBox_resize(thiz, selection);
	ListBox_PlotEvent(thiz, 0);
	thiz->MouseOver(thiz, thiz->mouse);
}

void ListBox_Click(ListBox *thiz)
{
	char *text = "Not implemented, yet...";
	char *old_text;

  if (!thiz) return;

	old_text = thiz->selected->name;
	thiz->selected->name = text;
 
	ListBox_resize(thiz, thiz->selected);
  thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x00);
  thiz->surface->DrawString (thiz->surface, text, -1, 4, 0, DSTF_LEFT|DSTF_TOP);
  thiz->surface->Flip(thiz->surface, NULL, 0);
	sleep(1);

	thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x00);
	thiz->selected->name = old_text;
	thiz->SelectItem(thiz, thiz->selected);

/* 	/\* how many items do we have in this combobox? *\/ */
/* 	int   n_items = 1; */
/* 	item *largest = thiz->items; */
/* 	old_text = thiz->items->name; */
/* 	while (strcmp(thiz->items->next->name, old_text)); */
/* 	{ */
/* 		n_items++; */
/* 		thiz->items = thiz->items->next; */
/* 		/\* we also get the largest string *\/ */
/* 		if (strlen(thiz->items->name) > strlen(largest->name)) */
/* 			largest = thiz->items; */
/* 	} */

/* 	/\* resize the surface to hold all these items... *\/ */
/* 	ListBox_resize(thiz, largest); // width */
/* 	ListBox_setHeightNYpos(thiz, n_items); // height */
}

void ListBox_KeyEvent(ListBox *thiz, int direction)
{
  if (!thiz || !thiz->selected) return;

  switch (direction)
  {
		case UP:
			if (thiz->selected != thiz->selected->prev)
				thiz->SelectItem(thiz, thiz->selected->prev);
			break;
		case DOWN:
			if (thiz->selected != thiz->selected->next)
				thiz->SelectItem(thiz, thiz->selected->next);
			break;
		case REDRAW:
			ListBox_PlotEvent(thiz, 1);
			break;
  }
}

void ListBox_SetTextColor(ListBox *thiz, color_t *text_color)
{
	if (!thiz)       return;
	if (!text_color) return;

	thiz->text_color.R = text_color->R;
	thiz->text_color.G = text_color->G;
	thiz->text_color.B = text_color->B;
	thiz->text_color.A = text_color->A;
	thiz->surface->SetColor (thiz->surface, text_color->R, text_color->G, text_color->B, text_color->A);
}

void ListBox_SetFocus(ListBox *thiz, int focus)
{
  if (!thiz) return;

  if (focus)
  {
    thiz->window->RequestFocus(thiz->window);
    thiz->hasfocus=1;
    thiz->window->SetOpacity(thiz->window, SELECTED_WINDOW_OPACITY);    
		thiz->MouseOver(thiz, thiz->mouse);
    return;
  }

  thiz->hasfocus = 0;
  thiz->window->SetOpacity(thiz->window, WINDOW_OPACITY);
	thiz->MouseOver(thiz, thiz->mouse);
  return;
}

void ListBox_AddItem(ListBox *thiz, char *object)
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
    thiz->SelectItem(thiz, thiz->items);
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

void ListBox_ClearItems(ListBox *thiz)
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

void ListBox_Hide(ListBox *thiz)
{
  thiz->window->SetOpacity(thiz->window, 0x00);
}

void ListBox_Show(ListBox *thiz)
{
  if (thiz->hasfocus) thiz->window->SetOpacity(thiz->window, SELECTED_WINDOW_OPACITY);
  else thiz->window->SetOpacity(thiz->window, WINDOW_OPACITY);
}

void ListBox_Destroy(ListBox *thiz)
{
  if (!thiz) return;
  ListBox_ClearItems(thiz);
  if (thiz->surface) thiz->surface->Release (thiz->surface);
  if (thiz->window) thiz->window->Release (thiz->window);
  free(thiz);
}

ListBox *ListBox_Create(IDirectFBDisplayLayer *layer, IDirectFBFont *font, color_t *text_color, DFBWindowDescription *window_desc)
{
  ListBox *newbox = NULL;

  newbox = (ListBox *) calloc(1, sizeof(ListBox));
  newbox->items        = NULL;
  newbox->selected     = NULL;
  newbox->xpos         = (unsigned int) window_desc->posx;
  newbox->ypos         = (unsigned int) window_desc->posy;
  newbox->width        = window_desc->width;
  newbox->height       = window_desc->height;
  newbox->hasfocus     = 0;
  newbox->position     = 0;
	newbox->mouse        = 0;
  newbox->window       = NULL;
  newbox->surface      = NULL;
	newbox->text_color.R = text_color->R;
	newbox->text_color.G = text_color->G;
	newbox->text_color.B = text_color->B;
	newbox->text_color.A = text_color->A;
	newbox->SetTextColor = ListBox_SetTextColor;
	newbox->MouseOver    = ListBox_MouseOver;
  newbox->KeyEvent     = ListBox_KeyEvent;
	newbox->Click        = ListBox_Click;
  newbox->SetFocus     = ListBox_SetFocus;
  newbox->AddItem      = ListBox_AddItem;
  newbox->ClearItems   = ListBox_ClearItems;
	newbox->SelectItem   = ListBox_SelectItem;
  newbox->Hide         = ListBox_Hide;
  newbox->Show         = ListBox_Show;
  newbox->Destroy      = ListBox_Destroy;

  if (layer->CreateWindow (layer, window_desc, &(newbox->window)) != DFB_OK) return NULL;
  newbox->window->SetOpacity(newbox->window, 0x00 );
  newbox->window->GetSurface(newbox->window, &(newbox->surface));
  newbox->surface->Clear(newbox->surface, 0x00, 0x00, 0x00, 0x00);
  newbox->surface->Flip(newbox->surface, NULL, 0);
  newbox->surface->SetFont (newbox->surface, font);
  newbox->surface->SetColor (newbox->surface, newbox->text_color.R, newbox->text_color.G, newbox->text_color.B, newbox->text_color.A);
  newbox->window->RaiseToTop(newbox->window);

  return newbox;
}
