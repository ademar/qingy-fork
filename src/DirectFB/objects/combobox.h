/***************************************************************************
                         combobox.h  -  description
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

#define UP       2
#define DOWN    -2
#define SELECT  -4

#define RETURN         13
#define ARROW_UP    61442
#define ARROW_DOWN  61443

typedef struct _ComboBox
{
	/* properties */
	pthread_t              events_thread;
	IDirectFBEventBuffer  *events;
	IDirectFBDisplayLayer *layer;
	pthread_mutex_t        lock;
	dfb_cursor_t          *cursor;
	int                    n_items;
	char                 **items;
	char                  *selected;
	color_t                text_color;
	unsigned int           xpos;
	unsigned int           ypos;
	unsigned int           width;
	unsigned int           height;
	int                    hasfocus;
	int                    isclicked;
	int                    position;
	IDirectFBWindow	      *window;
	IDirectFBSurface      *surface;
	int                    mouse;     /* 1 if mouse is over combobox, 0 otherwise */
	void                  *extraData; /* internal use only                        */
	void (*sortfunc)      (char **items, int n_items);
	void (*click_callback)(struct _ComboBox *thiz);

	/* public methods */
	void (*SetCursor)       (struct _ComboBox *thiz, IDirectFB *dfb, cursor_t *cursor_data, float x_ratio, float y_ratio);
	void (*SetTextColor)    (struct _ComboBox *thiz, color_t *text_color);
	void (*SetFocus)        (struct _ComboBox *thiz, int      focus     );
	void (*AddItem)         (struct _ComboBox *thiz, char    *item      );
	void (*SelectItem)      (struct _ComboBox *thiz, char    *selection );
	void (*SetSortFunction) (struct _ComboBox *thiz, void    *sortfunc  );
	void (*SortItems)       (struct _ComboBox *thiz);
	void (*ClearItems)      (struct _ComboBox *thiz);
	void (*SetClickCallBack)(struct _ComboBox *thiz, void *callback);
	void (*Hide)            (struct _ComboBox *thiz);
	void (*Show)            (struct _ComboBox *thiz);
	void (*Destroy)         (struct _ComboBox *thiz);

} ComboBox;

ComboBox *ComboBox_Create
(
	IDirectFBDisplayLayer *layer,
	IDirectFB *dfb,
	IDirectFBFont *font,
	color_t *text_color,
	DFBWindowDescription *window_desc,
	int screen_width,
	int screen_height
);
