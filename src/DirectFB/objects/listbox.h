/***************************************************************************
                         listbox.h  -  description
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

#define UP       2
#define DOWN    -2

#ifndef HAVE_STRUCT_ITEM
typedef struct _item
{
	char         *name;
	struct _item *next;
	struct _item *prev;
} item;
#define HAVE_STRUCT_ITEM 1
#endif

typedef struct _ListBox
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
	int               position;
	IDirectFBWindow	 *window;
	IDirectFBSurface *surface;
	int               mouse;    /* 1 if mouse is over listbox, 0 otherwise */

	/* methods */
	void (*SetTextColor)(struct _ListBox *thiz, color_t *text_color);
	void (*KeyEvent)    (struct _ListBox *thiz, int      direction );
	void (*MouseOver)   (struct _ListBox *thiz, int      status    );
	void (*SetFocus)    (struct _ListBox *thiz, int      focus     );
	void (*AddItem)     (struct _ListBox *thiz, char    *item      );
	void (*SelectItem)  (struct _ListBox *thiz, item    *selection );
	void (*ClearItems)  (struct _ListBox *thiz);
	void (*Click)       (struct _ListBox *thiz);
	void (*Hide)        (struct _ListBox *thiz);
	void (*Show)        (struct _ListBox *thiz);
	void (*Destroy)     (struct _ListBox *thiz);

} ListBox;

ListBox *ListBox_Create
(
	IDirectFBDisplayLayer *layer,
	IDirectFBFont *font,
	color_t *text_color,
	DFBWindowDescription *window_desc
);
