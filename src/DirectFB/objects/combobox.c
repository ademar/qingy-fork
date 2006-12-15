/***************************************************************************
                         combobox.c  -  description
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
#include <pthread.h>
#include <directfb.h>
#include <directfb_keynames.h>

#include "memmgmt.h"
#include "load_settings.h"
#include "utils.h"
#include "combobox.h"
#include "misc.h"
#include "logger.h"


#define UPWARD   0
#define DOWNWARD 1

typedef struct _DropDown
{
	int   n_items;       /* n. of items displayed in the drop-down menu  */
	int   first_item;    /* index of first item that should be displayed */
  char *selected;      /* selected item                                */
	int   screen_width;  /* screen X resolution                          */
	int   screen_height; /* screen Y resolution                          */
	int   orig_y;        /* y position of the combobox before unfolding  */
	int   direction;     /* y position of the combobox before unfolding  */
	int   scrollbar;

} DropDown;


static void click(ComboBox *);


void ComboBox_SetSortFunction(ComboBox *thiz, void *sortfunc)
{
	pthread_mutex_lock(&(thiz->lock));
  thiz->sortfunc = sortfunc;
	pthread_mutex_unlock(&(thiz->lock));
}

void ComboBox_SortItems(ComboBox *thiz)
{
	if (!thiz) return;

	pthread_mutex_lock(&(thiz->lock));
	thiz->sortfunc(thiz->items, thiz->n_items);
	pthread_mutex_unlock(&(thiz->lock));
}

static void plotEvent(ComboBox *thiz, int flip)
{
  if (!thiz) return;

  thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x00);
  thiz->surface->DrawString (thiz->surface, thiz->selected, -1, 4, 0, DSTF_LEFT|DSTF_TOP);
  if (flip) thiz->surface->Flip(thiz->surface, NULL, 0);
}

static void DrawScrollBar(ComboBox *thiz)
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

static void mouseOver(ComboBox *thiz, int status)
{
	if (!thiz) return;

	thiz->mouse = status;

	if (!thiz->isclicked)
	{ /* drop-down menu is not visible atm */
		plotEvent(thiz, 0);

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
			where->Release(where);
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
		thiz->layer->GetCursorPosition(thiz->layer, &mouse_x, &mouse_y);

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

static void setWidth(ComboBox *thiz, char *selection)
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

static void setHeightNYpos(ComboBox *thiz, int n_items)
{
	int            min_items      = 10; /* minimum number of elements we would like on screen */
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

int mouse_over_combobox(ComboBox *thiz)
{
	int mouse_x, mouse_y;

	thiz->layer->GetCursorPosition (thiz->layer, &mouse_x, &mouse_y);

	if ( ((mouse_x >= (int)thiz->xpos) && (mouse_x <= ((int)thiz->xpos + (int)thiz->width)))  &&
			 ((mouse_y >= (int)thiz->ypos) && (mouse_y <= ((int)thiz->ypos + (int)thiz->height)))  )
		return 1;

	return 0;
}

static void selectItem(ComboBox *thiz, char *selection, int lock)
{
	if (!thiz)      return;
	if (!selection) return;

	if (lock) pthread_mutex_lock(&(thiz->lock));

	thiz->selected = selection;
	plotEvent(thiz, 0);
	setWidth(thiz, selection);
	thiz->mouse = mouse_over_combobox(thiz);
	mouseOver(thiz, thiz->mouse);

	if (lock) pthread_mutex_unlock(&(thiz->lock));
}

void ComboBox_SelectItem(ComboBox *thiz, char *selection)
{
	if (!thiz)      return;
	if (!selection) return;

	selectItem(thiz, selection, 1);
}

static void keyEvent(ComboBox *thiz, int direction)
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
							selectItem(thiz, thiz->items[i], 0);
						else
							while (i < dropDown->first_item)
								dropDown->first_item--;
					}
					else
					{
						dropDown->selected = thiz->items[thiz->n_items - 1];
						if (!thiz->isclicked)
							selectItem(thiz, thiz->items[thiz->n_items - 1], 0);
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
							selectItem(thiz, thiz->items[i], 0);
						else
							while (i > (dropDown->first_item + dropDown->n_items - 1))
								dropDown->first_item++;
					}
					else
					{
						dropDown->selected = thiz->items[0];
						if (!thiz->isclicked)
							selectItem(thiz, thiz->items[0], 0);
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
				click(thiz);
			}
			break;
		case REDRAW:
			plotEvent(thiz, 1);
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

	pthread_mutex_lock(&(thiz->lock));

	thiz->text_color.R = text_color->R;
	thiz->text_color.G = text_color->G;
	thiz->text_color.B = text_color->B;
	thiz->text_color.A = text_color->A;
	thiz->surface->SetColor (thiz->surface, text_color->R, text_color->G, text_color->B, text_color->A);

	pthread_mutex_unlock(&(thiz->lock));
}

void setFocus(ComboBox *thiz, int focus, int lock)
{
  if (!thiz) return;

	if (lock) pthread_mutex_lock(&(thiz->lock));

  if (focus)
  {
    thiz->window->RequestFocus(thiz->window);
    thiz->hasfocus=1;
    thiz->window->SetOpacity(thiz->window, selected_window_opacity);
		mouseOver(thiz, thiz->mouse);
		if (lock) pthread_mutex_unlock(&(thiz->lock));
    return;
  }

	if (thiz->isclicked)
	{
		setWidth(thiz, thiz->selected);
		setHeightNYpos(thiz, 1);
		thiz->isclicked = 0;
	}

  thiz->hasfocus = 0;
  thiz->window->SetOpacity(thiz->window, window_opacity);
	mouseOver(thiz, thiz->mouse);

	if (lock) pthread_mutex_unlock(&(thiz->lock));
}

void ComboBox_SetFocus(ComboBox *thiz, int focus)
{
  if (!thiz) return;

	setFocus(thiz, focus, 1);
}

static void scrollBarClick (ComboBox *thiz, int mouse_y)
{
	DropDown *dropDown = (DropDown *) thiz->extraData;

	/* let's see if upper triangle was clicked... */
	if (mouse_y <= (int)(thiz->ypos+dropDown->scrollbar))
		if (dropDown->first_item > 0)
		{
			dropDown->first_item--;
			mouseOver(thiz, 1);
			return;
		}

	/* ...or the lower one */
	if (mouse_y >= (int)(thiz->ypos+thiz->height-dropDown->scrollbar))
		if ((dropDown->first_item + dropDown->n_items) < thiz->n_items)
		{
			dropDown->first_item++;
			mouseOver(thiz, 1);
			return;
		}

	/* we check wether the upper half of the scrollbar was clicked... */
	if (mouse_y <= (int)((float)thiz->ypos+(float)((float)thiz->height/(float)2)))
		if (dropDown->first_item > 0)
		{
			dropDown->first_item -= dropDown->n_items;
			if (dropDown->first_item < 0) dropDown->first_item = 0;
			mouseOver(thiz, 1);
			return;
		}

	/* ...or the lower half */
	if (mouse_y >= (int)((float)thiz->ypos+(float)((float)thiz->height/(float)2)))
		if ((dropDown->first_item + dropDown->n_items) < thiz->n_items)
		{
			dropDown->first_item += dropDown->n_items;
			if ((dropDown->first_item + dropDown->n_items) >= thiz->n_items)
				dropDown->first_item = thiz->n_items - dropDown->n_items;
			mouseOver(thiz, 1);
			return;
		}
}

static void click(ComboBox *thiz)
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
		thiz->layer->GetCursorPosition(thiz->layer, &mouse_x, &mouse_y);

		/* let's see wether this is a scrollbar click */
		if ( thiz->n_items > dropDown->n_items                                &&
				 mouse_x <= (int)(thiz->xpos + thiz->width)                       &&
				 mouse_x >= (int)(thiz->xpos + thiz->width - dropDown->scrollbar) &&
				 mouse_y >= (int) thiz->ypos                                      &&
				 mouse_y <= (int)(thiz->ypos + thiz->height)
			 )
		{
			scrollBarClick(thiz, mouse_y);
			return;
		}

		/* we fold the combobox */

		setHeightNYpos(thiz, 1);
		thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x00);

		if (thiz->mouse) thiz->selected = dropDown->selected;

		thiz->isclicked = 0;
 		selectItem(thiz, thiz->selected, 0);

		/* let's check wether mouse is still over this item */
		if (mouse_x < (int)thiz->xpos || mouse_x > (int)(thiz->xpos + thiz->width ) ||
				mouse_y < (int)thiz->ypos || mouse_y > (int)(thiz->ypos + thiz->height)   )
			mouseOver(thiz, 0);

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
	setWidth(thiz, largest);

	/* set combobox height and Y position... */
	setHeightNYpos(thiz, thiz->n_items);

	/* prepare drop-down menu data */	
	dropDown->selected   = thiz->selected;
	dropDown->first_item = 0;
	thiz->isclicked      = 1;

	/* draw items on screen and put a nice box around them */
	mouseOver(thiz, thiz->mouse);
}

void ComboBox_AddItem(ComboBox *thiz, char *object)
{
  if (!thiz || !object) return;

	pthread_mutex_lock(&(thiz->lock));

  if (!thiz->items)
  {
		thiz->items    = (char **) calloc(1, sizeof(char *));
		thiz->items[0] = strdup(object);
		thiz->n_items  = 1;
		selectItem(thiz, thiz->items[0], 0);

		pthread_mutex_unlock(&(thiz->lock));
		return;
  }

	thiz->items = (char **) realloc(thiz->items, (thiz->n_items + 1) * sizeof(char *));
	thiz->items[thiz->n_items] = strdup(object);
	thiz->n_items++;

	pthread_mutex_unlock(&(thiz->lock));
}

void ComboBox_ClearItems(ComboBox *thiz)
{
	int i = 0;

  if (!thiz || !thiz->items) return;

	pthread_mutex_lock(&(thiz->lock));

  thiz->selected = NULL;

	for (; i<thiz->n_items; i++)
		free(thiz->items[i]);

	free(thiz->items);
	thiz->n_items = 0;

	pthread_mutex_unlock(&(thiz->lock));
}

void ComboBox_Hide(ComboBox *thiz)
{
	pthread_mutex_lock(&(thiz->lock));
  thiz->window->SetOpacity(thiz->window, 0x00);
	pthread_mutex_unlock(&(thiz->lock));
}

void ComboBox_Show(ComboBox *thiz)
{
	pthread_mutex_lock(&(thiz->lock));
  if (thiz->hasfocus) thiz->window->SetOpacity(thiz->window, selected_window_opacity);
  else thiz->window->SetOpacity(thiz->window, window_opacity);
	pthread_mutex_unlock(&(thiz->lock));
}

void ComboBox_Destroy(ComboBox *thiz)
{
  if (!thiz) return;

	pthread_cancel(thiz->events_thread);
	pthread_join(thiz->events_thread, NULL);

/*   ComboBox_ClearItems(thiz); */
/*   if (thiz->surface) thiz->surface->Release (thiz->surface); */
/*   if (thiz->window)  thiz->window->Release (thiz->window); */
/* 	if (thiz->extraData) free(thiz->extraData); */
/*   free(thiz); */
}

void ComboBox_SetCursor(ComboBox *thiz, IDirectFB *dfb, cursor_t *cursor_data, float x_ratio, float y_ratio)
{
	if (!thiz)        return;
	if (!cursor_data) return;

	SetCursor(&(thiz->cursor), dfb, cursor_data, x_ratio, y_ratio);
}

static void combobox_thread(ComboBox *thiz)
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
				case DIET_AXISMOTION:
				{
					pthread_mutex_lock(&(thiz->lock));
					if (mouse_over_combobox(thiz))
					{
						if (evt.axis == DIAI_Z)
						{
							if (evt.axisrel == MOUSE_WHEEL_UP)
								keyEvent(thiz, UP);
			
							if (evt.axisrel == MOUSE_WHEEL_DOWN)
								keyEvent(thiz, DOWN);
						}
						if (thiz->cursor)
							if (!thiz->cursor->locked)
							{
								thiz->cursor->locked = 1;
								pthread_mutex_lock(lock_mouse_cursor);
								thiz->layer->SetCursorShape (thiz->layer, thiz->cursor->surface, thiz->cursor->x_off, thiz->cursor->y_off);
							}
						mouseOver(thiz, 1);
					}
					else
					{
						if (thiz->cursor)
							if (thiz->cursor->locked)
							{
								thiz->cursor->locked = 0;
								pthread_mutex_unlock(lock_mouse_cursor);
							}
						mouseOver(thiz, 0);
					}
					pthread_mutex_unlock(&(thiz->lock));

					break;
				}
				case DIET_BUTTONPRESS:
				case DIET_BUTTONRELEASE:
				{
					pthread_mutex_lock(&(thiz->lock));
					if (mouse_over_combobox(thiz))
					{
						setFocus(thiz, 1, 0);
						click(thiz);
					}
					pthread_mutex_unlock(&(thiz->lock));

					break;
				}
				case DIET_KEYPRESS:
				{
					struct DFBKeySymbolName *symbol_name;
					modifiers modifier;
					actions   action;
					int       ascii_code;

					pthread_mutex_lock(&(thiz->lock));

					if (thiz->hasfocus)
					{
						modifier   = modifier_is_pressed(&evt);
						ascii_code = (int)evt.key_symbol;
						symbol_name = bsearch (&(evt.key_symbol), keynames, 
																	 sizeof (keynames) / sizeof (keynames[0]) - 1,
																	 sizeof (keynames[0]), compare_symbol);

						action = search_keybindings(modifier, ascii_code);

						if (action == DO_NOTHING)
							if (symbol_name)
								switch (ascii_code)
								{
									case RETURN:
										keyEvent(thiz, SELECT);
										break;
									case ARROW_UP:
										keyEvent(thiz, UP);
										break;
									case ARROW_DOWN:
										keyEvent(thiz, DOWN);
										break;
									default:
										/* if user is typing a char, we allow him to select a session by typing the first char of its name */
										if (ascii_code >= 32 && ascii_code <= 127 && !thiz->isclicked)
										{
											int done  = 0;
											int found = 0;
											int i;

											ascii_code = to_upper (ascii_code);
											if (to_upper (thiz->selected[0]) == ascii_code)
											{
												for (i=0; i<thiz->n_items; i++)
												{
													if (thiz->items[i] == thiz->selected)
													{
														found = 1;
														continue;
													}
													if (found)
													{
														if (to_upper (thiz->items[i][0]) == ascii_code)
														{
															selectItem (thiz, thiz->items[i], 0);
															done = 1;
															break;
														}
													}
												}
											}
											if (!done)
											{
												for (i = 0; i < thiz->n_items; i++)
												{
													if (to_upper (thiz->items[i][0]) == ascii_code)
													{
														selectItem (thiz, thiz->items[i], 0);
														break;
													}
												}
											}
										}
										break;
								}
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

ComboBox *ComboBox_Create(IDirectFBDisplayLayer *layer, IDirectFB *dfb, IDirectFBFont *font, color_t *text_color, DFBWindowDescription *window_desc, int screen_width, int screen_height)
{
  ComboBox *newbox = NULL;

  newbox = (ComboBox *) calloc(1, sizeof(ComboBox));
	newbox->n_items         = 0;
  newbox->items           = NULL;
  newbox->selected        = NULL;
  newbox->xpos            = (unsigned int) window_desc->posx;
  newbox->ypos            = (unsigned int) window_desc->posy;
  newbox->width           = window_desc->width;
  newbox->height          = window_desc->height;
  newbox->hasfocus        = 0;
  newbox->isclicked       = 0;
  newbox->position        = 0;
	newbox->mouse           = 0;
  newbox->window          = NULL;
  newbox->surface         = NULL;
	newbox->cursor          = NULL;
	newbox->text_color.R    = text_color->R;
	newbox->text_color.G    = text_color->G;
	newbox->text_color.B    = text_color->B;
	newbox->text_color.A    = text_color->A;
	newbox->SetTextColor    = ComboBox_SetTextColor;
  newbox->SetFocus        = ComboBox_SetFocus;
  newbox->AddItem         = ComboBox_AddItem;
	newbox->SetSortFunction = ComboBox_SetSortFunction;
	newbox->SortItems       = ComboBox_SortItems;
  newbox->ClearItems      = ComboBox_ClearItems;
	newbox->SelectItem      = ComboBox_SelectItem;
  newbox->Hide            = ComboBox_Hide;
  newbox->Show            = ComboBox_Show;
  newbox->Destroy         = ComboBox_Destroy;
	newbox->SetCursor       = ComboBox_SetCursor;

	DropDown *dropDown      = (DropDown *) calloc(1, sizeof(DropDown));
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
	newbox->layer = layer;

	dfb->CreateInputEventBuffer (dfb, DICAPS_ALL, DFB_TRUE, &(newbox->events));
	pthread_mutex_init(&(newbox->lock), NULL);
	pthread_create(&(newbox->events_thread), NULL, (void *) combobox_thread, newbox);

  return newbox;
}
