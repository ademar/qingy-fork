/***************************************************************************
                          textbox.h  -  description
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

#define BACKSPACE       8
#define TAB             9
#define RETURN         13
#define ESCAPE         27
#define DELETE        127
#define ARROW_LEFT  61440
#define ARROW_RIGHT 61441
#define ARROW_UP    61442
#define ARROW_DOWN  61443
#define HOME        61445
#define END         61446

typedef struct _TextBox
{
  /* properties */
	pthread_t events_thread;
	pthread_t cursor_thread;
	IDirectFBEventBuffer *events;
	IDirectFBDisplayLayer *layer;
	dfb_cursor_t     *cursor;
  char *text;
  color_t text_color;
  color_t cursor_color;
  unsigned int xpos, ypos;
  unsigned int width, height;
  int hasfocus;
	int ishidden;
  int mask_text;
  int hide_text;
  int position;
  IDirectFBWindow	*window;
  IDirectFBSurface *surface;
	pthread_mutex_t lock;
	void (*click_callback)(struct _TextBox *thiz);

  /* methods */
	void (*SetCursor) (struct _TextBox *thiz, IDirectFB *dfb, cursor_t *cursor_data, float x_ratio, float y_ratio);
  void (*SetFocus)(struct _TextBox *thiz, int focus);
  void (*SetTextColor)(struct _TextBox *thiz, color_t *text_color);
  void (*SetCursorColor)(struct _TextBox *thiz, color_t *cursor_color);
  void (*SetText)(struct _TextBox *thiz, char *text);
  void (*ClearText)(struct _TextBox *thiz);
	void (*HideText)(struct _TextBox *thiz, int hide);
	void (*MaskText)(struct _TextBox *thiz, int mask);
	void (*SetClickCallBack)  (struct _TextBox *thiz, void *callback);
  void (*Hide)(struct _TextBox *thiz);
  void (*Show)(struct _TextBox *thiz);
  void (*Destroy)(struct _TextBox *thiz);

} TextBox;

TextBox *TextBox_Create
(
	IDirectFBDisplayLayer *layer,
	IDirectFB *dfb,
	IDirectFBFont *font,	
	color_t *text_color,
	color_t *cursor_color,
	DFBWindowDescription *window_desc
);
