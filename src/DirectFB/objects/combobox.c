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


#define UPWARD   0
#define DOWNWARD 1


typedef struct _DropDown
{
	IDirectFBDisplayLayer *layer;         /* don't tell our constructor we remember this  */
	int                    n_items;       /* n. of items displayed in the drop-down menu  */
	int                    first_item;    /* index of first item that should be displayed */
  char                  *selected;      /* selected item                                */
	int                    screen_width;  /* screen X resolution                          */
	int                    screen_height; /* screen Y resolution                          */
	int                    orig_y;        /* y position of the combobox before unfolding  */
	int                    direction;     /* y position of the combobox before unfolding  */
	int                    scrollbar;

} DropDown;


void ComboBox_SortItems(ComboBox *thiz)
{
	int i          = 0;
	int x_sessions = 0;

	// We divide X and text sessions ...
	for (; i<(thiz->n_items-1); i++)
	{
		int j = i + 1;
		for (; j<thiz->n_items; j++)
			if (!strncmp(thiz->items[i], "Text: ", 6))
				if (strncmp(thiz->items[j], "Text: ", 6))
				{
					char *swap = thiz->items[i];
					thiz->items[i] = thiz->items[j];
					thiz->items[j] = swap;
					break;
				}

		if (strncmp(thiz->items[i], "Text: ", 6))
			x_sessions++;
	}

	// ... we sort X sessions ...
	for (i=0; i<(x_sessions-1); i++)
	{
		int j = i + 1;
		for (; j<x_sessions; j++)
			if (strcasecmp(thiz->items[i], thiz->items[j]) > 0)
			{
				char *swap = thiz->items[i];
				thiz->items[i] = thiz->items[j];
				thiz->items[j] = swap;
			}
	}

	// ... and text ones
	for (i=x_sessions; i<(thiz->n_items-1); i++)
	{
		int j = i + 1;
		for (; j<thiz->n_items; j++)
			if (strcasecmp(thiz->items[i], thiz->items[j]) > 0)
			{
				char *swap = thiz->items[i];
				thiz->items[i] = thiz->items[j];
				thiz->items[j] = swap;
			}
	}
}

void ComboBox_PlotEvent(ComboBox *thiz, int flip)
{
  if (!thiz) return;

  thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x00);
  thiz->surface->DrawString (thiz->surface, thiz->selected, -1, 4, 0, DSTF_LEFT|DSTF_TOP);
  if (flip) thiz->surface->Flip(thiz->surface, NULL, 0);
}

void DrawScrollBar(ComboBox *thiz)
{
	DropDown      *dropDown;
	IDirectFBFont *font;
	int            unit;
	int            x1, y1, x2, y2, x3, y3;

	dropDown = (DropDown *) thiz->extraData;

	thiz->surface->GetFont(thiz->surface, &font);
	font->GetHeight(font, &unit);
	unit /= (float)1.5;

	/* we draw the main, upper and lower rectangles */
	thiz->surface->DrawRectangle(thiz->surface, thiz->width - unit, 0, unit, thiz->height);
	thiz->surface->DrawRectangle(thiz->surface, thiz->width - unit, 0, unit, unit);
	thiz->surface->DrawRectangle(thiz->surface, thiz->width - unit, thiz->height - unit, unit, unit);

	/* we draw the upper triangle... */
	x1 = thiz->width - unit + 4;
	y1 = unit - 4;
	x2 = thiz->width - 4;
	y2 = y1;
	x3 = ((float)x1 + (float)x2) / (float)2;
	y3 = 4;
	thiz->surface->FillTriangle (thiz->surface, x1, y1, x2, y2, x3, y3);

	/* ...and the lower one */
	y1 = thiz->height - unit + 4;
	y2 = y1;
	y3 = thiz->height - 4;
	thiz->surface->FillTriangle (thiz->surface, x1, y1, x2, y2, x3, y3);

	/* we draw the relative position rectangle */
	y1 = (float)(dropDown->first_item) * (float)(thiz->height - (2*unit)) / (float)thiz->n_items;
	y2 = (float)(thiz->height - (2*unit)) * (float)dropDown->n_items / (float)thiz->n_items;
	thiz->surface->FillRectangle(thiz->surface, thiz->width - unit + 4, unit + y1 + 4, unit - 8, y2 - 8);

	dropDown->scrollbar = unit;
}

void ComboBox_MouseOver(ComboBox *thiz, int status)
{
	if (!thiz) return;

	thiz->mouse = status;

	if (!thiz->isclicked)
	{ /* drop-down menu is not visible atm */
		ComboBox_PlotEvent(thiz, 0);

		if (status)
		{ /* mouse is over combobox, draw a nice box around it */
			IDirectFBSurface *where;
			DFBRectangle dest1, dest2, dest;
			IDirectFBFont *font;
			char *text = thiz->selected;

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
	{ /* drop-down menu is visible, we draw it... */
		DropDown *dropDown = (DropDown *) thiz->extraData;
		int       y_step   = thiz->height / dropDown->n_items;
		int       index    = dropDown->first_item;
		int       i        = 0;
		int       y        = 0;
		int       margin   = 0;
		int       mouse_x;
		int       mouse_y;

		thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x77);
		dropDown->layer->GetCursorPosition(dropDown->layer, &mouse_x, &mouse_y);

		/* draw lateral scrollbar if needed */
		dropDown->scrollbar = 0;
		if (thiz->n_items > dropDown->n_items)
		{
			DrawScrollBar(thiz);
			margin = dropDown->scrollbar;
		}

		/* draw items on screen */
		for (; i<dropDown->n_items; i++)
		{
			int startXpos = thiz->xpos;
			int endXpos   = thiz->xpos + thiz->width;
			int startYpos = thiz->ypos + y;
			int endYpos   = thiz->ypos + y + y_step;

			/* if mouse is over a particular item, we highlight it */
			if ( (mouse_x >= (int) startXpos) && (mouse_x < (int) (endXpos - margin)) &&
					 (mouse_y >= (int) startYpos) && (mouse_y < (int)       endYpos     )    )
			{
				thiz->surface->FillRectangle (thiz->surface, 0, y + 1, thiz->width - margin, y_step + 1);
				thiz->surface->SetColor (thiz->surface, 0, 0, 0, thiz->text_color.A);
				thiz->surface->DrawString (thiz->surface, thiz->items[index], -1, 4, y, DSTF_LEFT|DSTF_TOP);
				thiz->surface->SetColor (thiz->surface, thiz->text_color.R, thiz->text_color.G, thiz->text_color.B, thiz->text_color.A);
				dropDown->selected = thiz->items[index];
			}
			else
				thiz->surface->DrawString (thiz->surface, thiz->items[index], -1, 4, y, DSTF_LEFT|DSTF_TOP);

			index++;
			y += y_step;
		}

		/* draw a nice rectangle around combobox items */
		thiz->surface->SetColor (thiz->surface, thiz->text_color.R, thiz->text_color.G, thiz->text_color.B, thiz->text_color.A);
		thiz->surface->DrawRectangle (thiz->surface, 0, 0, thiz->width, thiz->height);
	}

	thiz->surface->Flip(thiz->surface, NULL, 0);
}

void ComboBox_setWidth(ComboBox *thiz, char *selection)
{
	/* we set the combobox width */
	DFBRectangle rect1, rect2, rect3;
	IDirectFBFont *font;

	thiz->surface->GetFont(thiz->surface,&font);
	font->GetStringExtents(font, selection, 0, &rect1, NULL);
	font->GetStringExtents(font, selection, strlen(selection), &rect2, NULL);
	font->GetStringWidth (font, selection, 0, &(rect3.x));
	rect3.y = 0;
	rect3.h = rect1.h;
	rect3.w = rect2.w - rect1.w + 4 + rect3.h;
	thiz->width = rect3.w;

	thiz->window->Resize(thiz->window, thiz->width, thiz->height);
}

void ComboBox_setHeightNYpos(ComboBox *thiz, int n_items)
{//FIXME!!!!!!!!!!!!!
	int            min_items      = 5; /* minimum number of elements we would like on screen */
	int            fitting_down;
	int            fitting_up;
	IDirectFBFont *font;
	DFBRectangle   rect;
	DropDown      *dropDown       = (DropDown *) thiz->extraData;

	if (n_items < min_items) min_items = n_items;

	thiz->surface->GetFont(thiz->surface,&font);
	font->GetStringExtents(font, ".", 0, &rect, NULL);

	/* don't do useless calculations if we only have to display one item */
	if (min_items == 1)
	{
		thiz->height = rect.h * min_items;
		thiz->window->Resize(thiz->window, thiz->width, thiz->height);
		dropDown->n_items = min_items;

		if ((int)thiz->ypos != dropDown->orig_y)
		{
			thiz->ypos = dropDown->orig_y;
			thiz->window->MoveTo(thiz->window, thiz->xpos, thiz->ypos);
		}

		return;
	}

	/* how many items fit on screen? */
	for (fitting_down=thiz->n_items; ((int)thiz->ypos + (fitting_down * rect.h)) > dropDown->screen_height; fitting_down--); /* downward */
	for (fitting_up=thiz->n_items; ((int)thiz->ypos - (fitting_up * rect.h)) < 0; fitting_up--); /* upward */

	/* if there is not enough space to fit min_items, we adjust it */
	if (fitting_down < min_items && fitting_up < min_items)
	{
		if (fitting_down >= fitting_up)
			min_items = fitting_down;
		else
			min_items = fitting_up;
	}

	dropDown->n_items      = min_items;
	dropDown->orig_y       = thiz->ypos;

	if (fitting_down >= min_items)
		dropDown->direction  = DOWNWARD;
	else
	{
		dropDown->direction  = UPWARD;
		thiz->ypos          -= rect.h * (min_items - 1);
		thiz->window->MoveTo(thiz->window, thiz->xpos, thiz->ypos);
	}

	thiz->height = rect.h * min_items;	
	thiz->window->Resize(thiz->window, thiz->width, thiz->height);
}

void ComboBox_SelectItem(ComboBox *thiz, char *selection)
{
	if (!thiz)      return;
	if (!selection) return;

	thiz->selected = selection;
	ComboBox_PlotEvent(thiz, 0);
	ComboBox_setWidth(thiz, selection);
	thiz->MouseOver(thiz, thiz->mouse);
}

void ComboBox_KeyEvent(ComboBox *thiz, int direction)
{
	DropDown *dropDown;
	int       i;

  if (!thiz          ) return;
	if (!thiz->selected) return;
	
	dropDown = (DropDown *) thiz->extraData;

	if (!thiz->isclicked)
		dropDown->selected = thiz->selected;

  switch (direction)
  {
		case UP:
			for (i=0; i<thiz->n_items; i++)
				if (thiz->items[i] == dropDown->selected)
				{
					if (i > 0)
					{
						i--;
						dropDown->selected = thiz->items[i];
						if (!thiz->isclicked)
							thiz->SelectItem(thiz, thiz->items[i]);
						else
							while (i < dropDown->first_item)
								dropDown->first_item--;
					}
					else
					{
						dropDown->selected = thiz->items[thiz->n_items - 1];
						if (!thiz->isclicked)
							thiz->SelectItem(thiz, thiz->items[thiz->n_items - 1]);
						else
						{
							dropDown->first_item = thiz->n_items - dropDown->n_items;
							if (dropDown->first_item < 0) dropDown->first_item = 0;
						}
					}
					break;
				}
			break;
		case DOWN:
			for (i=0; i<thiz->n_items; i++)
				if (thiz->items[i] == dropDown->selected)
				{
					if (i < (thiz->n_items - 1))
					{
						i++;
						dropDown->selected = thiz->items[i];
						if (!thiz->isclicked)
							thiz->SelectItem(thiz, thiz->items[i]);
						else
							while (i > (dropDown->first_item + dropDown->n_items - 1))
								dropDown->first_item++;
					}
					else
					{
						dropDown->selected = thiz->items[0];
						if (!thiz->isclicked)
							thiz->SelectItem(thiz, thiz->items[0]);
						else
							dropDown->first_item = 0;
					}
					break;
				}
			break;
		case SELECT:
			if (thiz->isclicked)
			{
				thiz->selected = dropDown->selected;
				thiz->Click(thiz);
			}
			break;
		case REDRAW:
			ComboBox_PlotEvent(thiz, 1);
			break;
  }

	if (thiz->isclicked)
	{
		int y_step = thiz->height / dropDown->n_items;
		int index  = dropDown->first_item;
		int y      = 0;
		int margin = 0;

		thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x77);

		/* draw lateral scrollbar if needed */
		dropDown->scrollbar = 0;
		if (thiz->n_items > dropDown->n_items)
		{
			DrawScrollBar(thiz);
			margin = dropDown->scrollbar;
		}

		/* draw items on screen */
		for (i=0; i<dropDown->n_items; i++)
		{
			if (thiz->items[index] == dropDown->selected)
			{
				thiz->surface->FillRectangle (thiz->surface, 0, y + 1, thiz->width - margin, y_step + 1);
				thiz->surface->SetColor (thiz->surface, 0, 0, 0, thiz->text_color.A);
				thiz->surface->DrawString (thiz->surface, thiz->items[index], -1, 4, y, DSTF_LEFT|DSTF_TOP);
				thiz->surface->SetColor (thiz->surface, thiz->text_color.R, thiz->text_color.G, thiz->text_color.B, thiz->text_color.A);
			}
			else
				thiz->surface->DrawString (thiz->surface, thiz->items[index], -1, 4, y, DSTF_LEFT|DSTF_TOP);

			index++;
			y += y_step;
		}

		/* draw a nice rectangle around combobox items */
		thiz->surface->SetColor (thiz->surface, thiz->text_color.R, thiz->text_color.G, thiz->text_color.B, thiz->text_color.A);
		thiz->surface->DrawRectangle (thiz->surface, 0, 0, thiz->width, thiz->height);

		thiz->surface->Flip(thiz->surface, NULL, 0);
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

void ComboBox_ScrollBarClick (ComboBox *thiz, int mouse_y)
{
	DropDown *dropDown = (DropDown *) thiz->extraData;

	/* let's see if upper triangle was clicked... */
	if (mouse_y <= (int)(thiz->ypos+dropDown->scrollbar))
		if (dropDown->first_item > 0)
		{
			dropDown->first_item--;
			thiz->MouseOver(thiz, 1);
			return;
		}

	/* ...or the lower one */
	if (mouse_y >= (int)(thiz->ypos+thiz->height-dropDown->scrollbar))
		if ((dropDown->first_item + dropDown->n_items) < thiz->n_items)
		{
			dropDown->first_item++;
			thiz->MouseOver(thiz, 1);
			return;
		}

	/* we check wether the upper half of the scrollbar was clicked... */
	if (mouse_y <= (int)((float)thiz->ypos+(float)((float)thiz->height/(float)2)))
		if (dropDown->first_item > 0)
		{
			dropDown->first_item -= dropDown->n_items;
			if (dropDown->first_item < 0) dropDown->first_item = 0;
			thiz->MouseOver(thiz, 1);
			return;
		}

	/* ...or the lower half */
	if (mouse_y >= (int)((float)thiz->ypos+(float)((float)thiz->height/(float)2)))
		if ((dropDown->first_item + dropDown->n_items) < thiz->n_items)
		{
			dropDown->first_item += dropDown->n_items;
			if ((dropDown->first_item + dropDown->n_items) >= thiz->n_items)
				dropDown->first_item = thiz->n_items - dropDown->n_items;
			thiz->MouseOver(thiz, 1);
			return;
		}
}

void ComboBox_Click(ComboBox *thiz)
{
	DropDown *dropDown;
	char     *largest;
	int       i;

  if (!thiz) return;

	dropDown = (DropDown *) thiz->extraData;
	largest  = thiz->items[0];

	/* is this combobox folded or unfolded? */

	if (thiz->isclicked) /* unfolded, let's see what we should do about it... */
	{
		int       mouse_x;
		int       mouse_y;

		if (!dropDown) return;
		dropDown->layer->GetCursorPosition(dropDown->layer, &mouse_x, &mouse_y);

		/* let's see wether this is a scrollbar click */
		if ( thiz->n_items > dropDown->n_items                                &&
				 mouse_x <= (int)(thiz->xpos + thiz->width)                       &&
				 mouse_x >= (int)(thiz->xpos + thiz->width - dropDown->scrollbar) &&
				 mouse_y >= (int) thiz->ypos                                      &&
				 mouse_y <= (int)(thiz->ypos + thiz->height)
			 )
		{
			ComboBox_ScrollBarClick(thiz, mouse_y);
			return;
		}

		/* we fold the combobox */

		ComboBox_setHeightNYpos(thiz, 1);
		thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x00);

		if (thiz->mouse) thiz->selected = dropDown->selected;

		thiz->isclicked = 0;
 		ComboBox_SelectItem(thiz, thiz->selected);

		/* let's check wether mouse is still over this item */
		if (mouse_x < (int)thiz->xpos || mouse_x > (int)(thiz->xpos + thiz->width ) ||
				mouse_y < (int)thiz->ypos || mouse_y > (int)(thiz->ypos + thiz->height)   )
			thiz->MouseOver(thiz, 0);

		return;
	}

	/* we unfold the combobox */

	thiz->window->RaiseToTop(thiz->window);

	/* we get the largest item of this combobox */
	for (i=1; i<thiz->n_items; i++)
	{
		if (strlen(thiz->items[i]) > strlen(largest))
			largest = thiz->items[i];
	}

	/* we set the combobox width to that of its largest item */
	ComboBox_setWidth(thiz, largest);

	/* set combobox height and Y position... */
	ComboBox_setHeightNYpos(thiz, thiz->n_items);

	/* prepare drop-down menu data */	
	dropDown->selected   = thiz->selected;
	dropDown->first_item = 0;
	thiz->isclicked      = 1;

	/* draw items on screen and put a nice box around them */
	thiz->MouseOver(thiz, thiz->mouse);
}

void ComboBox_AddItem(ComboBox *thiz, char *object)
{
  if (!thiz || !object) return;
  if (!thiz->items)
  {
		thiz->items    = (char **) calloc(1, sizeof(char *));
		thiz->items[0] = strdup(object);
		thiz->n_items  = 1;
		thiz->SelectItem(thiz, thiz->items[0]);

		return;
  }

	thiz->items = (char **) realloc(thiz->items, (thiz->n_items + 1) * sizeof(char *));
	thiz->items[thiz->n_items] = strdup(object);
	thiz->n_items++;
}

void ComboBox_ClearItems(ComboBox *thiz)
{
	int i = 0;

  if (!thiz || !thiz->items) return;
  thiz->selected = NULL;

	for (; i<thiz->n_items; i++)
		free(thiz->items[i]);

	free(thiz->items);
	thiz->n_items = 0;
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
  if (thiz->window)  thiz->window->Release (thiz->window);
	if (thiz->extraData) free(thiz->extraData);
  free(thiz);
}

ComboBox *ComboBox_Create(IDirectFBDisplayLayer *layer, IDirectFBFont *font, color_t *text_color, DFBWindowDescription *window_desc, int screen_width, int screen_height)
{
  ComboBox *newbox = NULL;

  newbox = (ComboBox *) calloc(1, sizeof(ComboBox));
	newbox->n_items      = 0;
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

	DropDown *dropDown      = (DropDown *) calloc(1, sizeof(DropDown));
	dropDown->layer         = layer;
	dropDown->screen_width  = screen_width;
	dropDown->screen_height = screen_height;
	newbox->extraData       = dropDown;

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
