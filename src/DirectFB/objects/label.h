/***************************************************************************
                      directfb_label.h  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003 by Noberasco Michele
    e-mail               : noberasco.gnu@disi.unige.it
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


#define LEFT         0
#define CENTER       1
#define RIGHT        2
#define CENTERBOTTOM 3

typedef struct _Label
{
	/* properties */
	char *text;
	unsigned int xpos, ypos;
	unsigned int width, height;
	int hasfocus;
	int alignment;
	IDirectFBWindow	*window;
	IDirectFBSurface *surface;

	/* methods */
	void (*SetFocus)(struct _Label *thiz, int focus);
	void (*SetText)(struct _Label *thiz, char *text, int alignment);
	void (*ClearText)(struct _Label *thiz);
	void (*Hide)(struct _Label *thiz);
	void (*Show)(struct _Label *thiz);
	void (*Destroy)(struct _Label *thiz);

} Label;

Label *Label_Create
(
	IDirectFBDisplayLayer *layer,
	IDirectFBFont *font,
	DFBWindowDescription *window_desc
);
