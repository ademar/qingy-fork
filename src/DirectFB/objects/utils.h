/***************************************************************************
                      directfb_utils.h  -  description
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <keybindings.h>

#define MOUSE_WHEEL_UP   -1
#define MOUSE_WHEEL_DOWN  1

/* checks wether user has any of these locks active */
#define SCROLLLOCK	1
#define NUMLOCK			2
#define CAPSLOCK		3
int lock_is_pressed(DFBInputEvent *evt);

/* checks wether user is currently holding down any key modifiers */
modifiers modifier_is_pressed(DFBInputEvent *evt);

/* checks wether left mouse button is down */
int left_mouse_button_down(DFBInputEvent *evt);

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
