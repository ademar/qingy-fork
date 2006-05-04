/***************************************************************************
                          label.h  -  description
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

/* text alignment */
#define LEFT         0
#define CENTER       1
#define RIGHT        2
#define CENTERBOTTOM 3

/* modes of displaying */
#define NORMAL 0
#define SOLID  1

typedef struct _Label
{
	/* properties */
	pthread_t events_thread;
	pthread_t update_thread;
	IDirectFBEventBuffer  *events;
	IDirectFBDisplayLayer *layer;
	pthread_mutex_t        lock;
	char *text;
	color_t text_color;
	unsigned int xpos, ypos;
	unsigned int width, height;
	int hasfocus;
	int ishidden;
	IDirectFBWindow	*window;
	IDirectFBSurface *surface;
	int polltime;
  char *content;
  char *command;
  int   text_orientation;
	void (*click_callback)(struct _Label *thiz);

	/* methods */
	void (*SetFocus)          (struct _Label *thiz, int focus);
	void (*SetTextColor)      (struct _Label *thiz, color_t *text_color);
	void (*SetText)           (struct _Label *thiz, char *text);
	void (*SetTextOrientation)(struct _Label *thiz, int orientation);
	void (*SetClickCallBack)  (struct _Label *thiz, void *callback);
	void (*ClearText)         (struct _Label *thiz);
	void (*Hide)              (struct _Label *thiz);
	void (*Show)              (struct _Label *thiz);
	void (*Destroy)           (struct _Label *thiz);
	void (*SetAction)         (struct _Label *thiz, int polltime, char *content, char *command);

} Label;

Label *Label_Create
(
	IDirectFBDisplayLayer *layer,
	IDirectFB *dfb,
	IDirectFBFont *font,
	color_t *text_color,
	DFBWindowDescription *window_desc
);
