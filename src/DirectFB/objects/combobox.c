/***************************************************************************
                         combobox.c  -  description
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
#include "combobox.h"
#include "directfb_mode.h"
#include "misc.h"



typedef struct _DropDown
{
	int   n_items;    /* n. of items displayed in the drop-down menu */
	item *first_item; /* first item displayed                        */
  item *selected;   /* selected item                               */

} DropDown;


void ComboBox_SortItems(ComboBox *thiz)
{
	/*item *browse = thiz->items;
	int   i      = 0;

	if (!thiz)        return;
	if (!thiz->items) return;*/

	/* we sort X sessions... */
	
	// TO BE IMPLEMENTED

	/* display stuff */

	// NOT REALLY
	/*for (i=0; i<(*(thiz->items->n_items)); i++)
	{
		fprintf(stderr, "%s\n", browse->name);
		browse = browse->next;
		}*/
}

void ComboBox_PlotEvent(ComboBox *thiz, int flip)
{
  if (!thiz) return;

  thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x00);
  thiz->surface->DrawString (thiz->surface, thiz->selected->name, -1, 4, 0, DSTF_LEFT|DSTF_TOP);
  if (flip) thiz->surface->Flip(thiz->surface, NULL, 0);
}

void ComboBox_MouseOver(ComboBox *thiz, int status)
{
	if (!thiz) return;

	thiz->mouse = status;

	if (!thiz->isclicked)
	{
		ComboBox_PlotEvent(thiz, 0);

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
	else
	{ /* drop-down menu is visible, we draw a square around it... */
		DropDown *dropDown = (DropDown *) thiz->extraData;

		/* draw items on screen */
		item *run = dropDown->selected;
		int   y   = 0;
		int   i;

		for (i=0; i<(*(run->n_items)); i++)
		{
			/* if mouse is over a particular item, we highlight it */
			/*if (1 == 2)
			{
			}
			else*/
				thiz->surface->DrawString (thiz->surface, run->name, -1, 4, y, DSTF_LEFT|DSTF_TOP);
			y = y + (thiz->height / (*(run->n_items)));
			run = run->next;
		}

		thiz->surface->SetColor (thiz->surface, thiz->text_color.R, thiz->text_color.G, thiz->text_color.B, thiz->text_color.A);
		thiz->surface->DrawRectangle (thiz->surface, 0, 0, thiz->width, thiz->height);
	}
}

void ComboBox_setWidth(ComboBox *thiz, item *selection)
{
	/* we set the combobox width */
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

	thiz->window->Resize(thiz->window, thiz->width, thiz->height);
}

void ComboBox_setHeightNYpos(ComboBox *thiz, int n_items)
{
	IDirectFBFont *font;
	DFBRectangle   rect;

	thiz->surface->GetFont(thiz->surface,&font);
	font->GetStringExtents(font, ".", 0, &rect, NULL);
	thiz->height = rect.h * n_items;
	
	thiz->window->Resize(thiz->window, thiz->width, thiz->height);
}

void ComboBox_SelectItem(ComboBox *thiz, item *selection)
{
	if (!thiz)      return;
	if (!selection) return;

	thiz->selected = selection;
	ComboBox_setWidth(thiz, selection);
	ComboBox_PlotEvent(thiz, 0);
	thiz->MouseOver(thiz, thiz->mouse);
}

void ComboBox_KeyEvent(ComboBox *thiz, int direction)
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
			ComboBox_PlotEvent(thiz, 1);
			break;
  }
}

void ComboBox_SetTextColor(ComboBox *thiz, color_t *text_color)
{
	if (!thiz)       return;
	if (!text_color) return;

	thiz->text_color.R = text_color->R;
	thiz->text_color.G = text_color->G;
	thiz->text_color.B = text_color->B;
	thiz->text_color.A = text_color->A;
	thiz->surface->SetColor (thiz->surface, text_color->R, text_color->G, text_color->B, text_color->A);
}

void ComboBox_SetFocus(ComboBox *thiz, int focus)
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

	if (thiz->isclicked)
	{
		ComboBox_setWidth(thiz, thiz->selected);
		ComboBox_setHeightNYpos(thiz, 1);
		thiz->isclicked = 0;
	}

  thiz->hasfocus = 0;
  thiz->window->SetOpacity(thiz->window, WINDOW_OPACITY);
	thiz->MouseOver(thiz, thiz->mouse);
  return;
}

void ComboBox_Click(ComboBox *thiz)
{
  if (!thiz) return;

	if (thiz->isclicked)
	{
		ComboBox_setHeightNYpos(thiz, 1); // height
		thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x00);
		thiz->SelectItem(thiz, thiz->selected);

		thiz->isclicked = 0;
		return;
	}

	thiz->window->RaiseToTop(thiz->window);
  thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x00);

	/* we get the largest item of this combobox */
	item *largest  = thiz->items;
  item *run      = thiz->items;
	int   i        = 0;

	for (; i<(*(thiz->items->n_items)); i++)
	{
		run = run->next;
		/* we also get the largest string */
		if (strlen(run->name) > strlen(largest->name))
			largest = run;
	}
	ComboBox_setWidth(thiz, largest); // width

	/* resize the surface to hold all these items... */
	ComboBox_setHeightNYpos(thiz, *(thiz->items->n_items)); // height

	/* prepare drop-down menu data */
	DropDown *dropDown = (DropDown *) calloc(1, sizeof(DropDown));
	thiz->extraData = dropDown;
	dropDown->first_item =  thiz->selected;
	dropDown->selected   =  thiz->selected;
	dropDown->n_items    = *thiz->items->n_items;
	thiz->isclicked      =  1;

	/* draw items on screen and put a nice box around them */
	thiz->MouseOver(thiz, thiz->mouse);

	thiz->surface->Flip(thiz->surface, NULL, 0);
}

void ComboBox_AddItem(ComboBox *thiz, char *object)
{
  item *curr;

  if (!thiz || !object) return;
  if (!thiz->items)
  {
    thiz->items = (item *) calloc(1, sizeof(item));
		thiz->items->n_items = (int *) calloc(1, sizeof(int));
		*(thiz->items->n_items) = 1;
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
	curr->next->n_items = thiz->items->n_items;
	thiz->items->prev = curr->next;
	curr->next->name = (char *) calloc(strlen(object)+1, sizeof(char));
	strcpy(curr->next->name, object);
	(*(thiz->items->n_items))++;
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
    curr->next       = NULL;
		curr->n_items    = NULL;
    curr             = curr->prev;
    curr->next->prev = NULL;
    free(curr->next);
  }
  curr->next = NULL;
  curr->prev = NULL;
  free(curr->name); curr->name = NULL;
  curr = NULL;
	free(thiz->items->n_items); thiz->items->n_items = NULL;
  free(thiz->items);          thiz->items          = NULL;
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
	if (thiz->extraData) free(thiz->extraData);
  free(thiz);
}

ComboBox *ComboBox_Create(IDirectFBDisplayLayer *layer, IDirectFBFont *font, color_t *text_color, DFBWindowDescription *window_desc)
{
  ComboBox *newbox = NULL;

  newbox = (ComboBox *) calloc(1, sizeof(ComboBox));
  newbox->items        = NULL;
  newbox->selected     = NULL;
  newbox->xpos         = (unsigned int) window_desc->posx;
  newbox->ypos         = (unsigned int) window_desc->posy;
  newbox->width        = window_desc->width;
  newbox->height       = window_desc->height;
  newbox->hasfocus     = 0;
  newbox->isclicked    = 0;
  newbox->position     = 0;
	newbox->mouse        = 0;
	newbox->extraData    = NULL;
  newbox->window       = NULL;
  newbox->surface      = NULL;
	newbox->text_color.R = text_color->R;
	newbox->text_color.G = text_color->G;
	newbox->text_color.B = text_color->B;
	newbox->text_color.A = text_color->A;
	newbox->SetTextColor = ComboBox_SetTextColor;
	newbox->MouseOver    = ComboBox_MouseOver;
  newbox->KeyEvent     = ComboBox_KeyEvent;
	newbox->Click        = ComboBox_Click;
  newbox->SetFocus     = ComboBox_SetFocus;
  newbox->AddItem      = ComboBox_AddItem;
	newbox->SortItems    = ComboBox_SortItems;
  newbox->ClearItems   = ComboBox_ClearItems;
	newbox->SelectItem   = ComboBox_SelectItem;
  newbox->Hide         = ComboBox_Hide;
  newbox->Show         = ComboBox_Show;
  newbox->Destroy      = ComboBox_Destroy;

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
