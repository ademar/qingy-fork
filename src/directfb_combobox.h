/***************************************************************************
                     directfb_combobox.h  -  description
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


#define UP       2
#define DOWN    -2

typedef struct _item
{
	char *name;
	struct _item *next;
	struct _item *prev;
} item;

typedef struct _ComboBox
{
	/* properties */
	item *items;
	item *selected;
	unsigned int xpos, ypos;
	unsigned int width, height;
	int hasfocus;
	int position;
	IDirectFBWindow	*window;
	IDirectFBSurface *surface;

	/* methods */
	void (*KeyEvent)(struct _ComboBox *thiz, int direction);
	void (*SetFocus)(struct _ComboBox *thiz, int focus);
	void (*AddItem)(struct _ComboBox *thiz, char *item);
	void (*ClearItems)(struct _ComboBox *thiz);
	void (*Hide)(struct _ComboBox *thiz);
	void (*Show)(struct _ComboBox *thiz);
	void (*Destroy)(struct _ComboBox *thiz);

} ComboBox;

ComboBox *ComboBox_Create
(
	IDirectFBDisplayLayer *layer,
	IDirectFBFont *font,
	DFBWindowDescription *window_desc
);