/***************************************************************************
                      directfb_button.h  -  description
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


/* available commands */
typedef enum
{
	HALT,
	REBOOT,
	SLEEP,
	SCREEN_SAVER
} Commands;

/* button structure */
typedef struct _Button
{
	/* properties */
	IDirectFBWindow  *window;    /* window that will contain the button 				*/
	IDirectFBSurface *surface;   /* surface of the above                        */
	IDirectFBSurface *normal;    /* normal button appearance                    */
	IDirectFBSurface *mouseover; /* button appearance when mouse is over it     */
	Commands command;            /* what should this button do?                 */
	int xpos;										 /* x position of the button                    */
	int ypos;										 /* y position of the button                    */
	unsigned int width;					 /* width of the button                         */
	unsigned int height;				 /* height of the button                        */
	int mouse;									 /* 1 if mouse is over button, 0 otherwise      */

	/* methods */
	void (*Destroy)(struct _Button *thiz);
	void (*MouseOver)(struct _Button *thiz, int status);
	void (*Show)(struct _Button *thiz);
	void (*Hide)(struct _Button *thiz);

} Button;

Button *Button_Create
(
	const char *normal,
	const char *mouseover,
	int xpos,  int ypos,
	IDirectFBDisplayLayer *layer,
	IDirectFBSurface *primary,
	IDirectFB *dfb
);

/* return a surface with an image loaded from disk */
IDirectFBSurface *load_image(const char *filename, IDirectFBSurface *primary, IDirectFB *dfb);
