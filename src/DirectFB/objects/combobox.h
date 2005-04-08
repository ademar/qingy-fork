/***************************************************************************
                         combobox.h  -  description
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

#define UP       2
#define DOWN    -2

#ifndef HAVE_STRUCT_ITEM
typedef struct _item
{
	char         *name;
	int          *n_items;
	struct _item *next;
	struct _item *prev;
} item;
#define HAVE_STRUCT_ITEM 1
#endif

typedef struct _ComboBox
{
	/* properties */
	item             *items;
	item             *selected;
	color_t           text_color;
	unsigned int      xpos;
	unsigned int      ypos;
	unsigned int      width;
	unsigned int      height;
	int               hasfocus;
	int               isclicked;
	int               position;
	IDirectFBWindow	 *window;
	IDirectFBSurface *surface;
	int               mouse;    /* 1 if mouse is over combobox, 0 otherwise */

	/* methods */
	void (*SetTextColor)(struct _ComboBox *thiz, color_t *text_color);
	void (*KeyEvent)    (struct _ComboBox *thiz, int      direction );
	void (*MouseOver)   (struct _ComboBox *thiz, int      status    );
	void (*SetFocus)    (struct _ComboBox *thiz, int      focus     );
	void (*AddItem)     (struct _ComboBox *thiz, char    *item      );
	void (*SelectItem)  (struct _ComboBox *thiz, item    *selection );
	void (*SortItems)   (struct _ComboBox *thiz);
	void (*ClearItems)  (struct _ComboBox *thiz);
	void (*Click)       (struct _ComboBox *thiz);
	void (*Hide)        (struct _ComboBox *thiz);
	void (*Show)        (struct _ComboBox *thiz);
	void (*Destroy)     (struct _ComboBox *thiz);

} ComboBox;

ComboBox *ComboBox_Create
(
	IDirectFBDisplayLayer *layer,
	IDirectFBFont *font,
	color_t *text_color,
	DFBWindowDescription *window_desc
);
