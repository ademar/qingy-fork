/***************************************************************************
                      directfb_utils.h  -  description
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


/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...)																					\
{																																\
	err = x;																											\
	if (err != DFB_OK)																						\
	{																															\
		fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );			\
		DirectFBErrorFatal( #x, err );															\
	}																															\
}

/* checks wether user has any of these locks active */
#define SCROLLLOCK	1
#define NUMLOCK			2
#define CAPSLOCK		3
int lock_is_pressed(DFBInputEvent *evt);

/* checks wether user is currently holding down any of these modifiers */
#define SHIFT		1
#define CONTROL	2
#define ALT			3
#define ALTGR		4
#define META		5
#define SUPER		6
#define HYPER		7
int modifier_is_pressed(DFBInputEvent *evt);

/* checks wether left mouse button is down */
int left_mouse_button_down(DFBInputEvent *evt);

/* return a surface with an image loaded from disk */
IDirectFBSurface *load_image(const char *filename, IDirectFBSurface *primary, IDirectFB *dfb);

/* two functions to create and destroy buttons */
struct button
{
	IDirectFBWindow  *window;    /* window that will contain the button 				*/
	IDirectFBSurface *surface;   /* surface of the above                        */
	IDirectFBSurface *normal;    /* normal button appearance                    */
	IDirectFBSurface *mouseover; /* button appearance when mouse is over it     */
	int xpos;										 /* x position of the button                    */
	int ypos;										 /* y position of the button                    */
	unsigned int width;					 /* width of the button                         */
	unsigned int height;				 /* height of the button                        */
	int mouse;									 /* 1 if mouse is over button, 0 otherwise      */
};

struct button *load_button (const char *normal, const char *mouseover, int relx, int rely, IDirectFBDisplayLayer *layer, IDirectFBSurface *primary, IDirectFB *dfb, __u8 opacity);
void destroy_button (struct button *button);

/* other stuff */
typedef struct _DeviceInfo DeviceInfo;
struct _DeviceInfo
{
	DFBInputDeviceID device_id;
	DFBInputDeviceDescription desc;
	DeviceInfo *next;
};
const char *get_device_name (DeviceInfo * devices, DFBInputDeviceID device_id);
DFBEnumerationResult enum_input_device	(DFBInputDeviceID device_id, DFBInputDeviceDescription desc, void *data);
static const DirectFBKeySymbolNames (keynames)
static const DirectFBKeyIdentifierNames (idnames)
int compare_symbol (const void *a, const void *b);
int compare_id (const void *a, const void *b);
