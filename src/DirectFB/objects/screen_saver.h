/***************************************************************************
                       screen_saver.h  -  description
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


typedef enum _kinds
{
	PIXEL_SCREENSAVER,
	PHOTO_SCREENSAVER
} kinds;

typedef struct _screen_saver
{
	/* which screen saver do you want to run? */
	kinds kind;

	/* update rate of screen saver */
	unsigned int seconds;
	unsigned int milli_seconds;

	/* surface to run the screen saver into */
	IDirectFBSurface *surface;

	/* screensaver stops when there is an input event
	   and it returns it here...                      */
	IDirectFBEventBuffer *events;

} ScreenSaver;

void activate_screen_saver(ScreenSaver *thiz);
