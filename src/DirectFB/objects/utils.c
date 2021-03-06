/***************************************************************************
                          utils.c  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003-2005 by Noberasco Michele
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

#include <stdio.h>
#include <stdlib.h>
#include <directfb.h>
#include <directfb_keynames.h>
#include <unistd.h>

#include "logger.h"
#include "load_settings.h"
#include "utils.h"
#include "misc.h"

int lock_is_pressed(DFBInputEvent *evt)
{
	struct
	{
		DFBInputDeviceLockState lock;
		const char *name;
		int x;
	} locks[] =	{
								{DILS_SCROLL, "ScrollLock", 0},
								{DILS_NUM,    "NumLock",    0},
								{DILS_CAPS,   "CapsLock",   0},
							};
  int n_locks = sizeof (locks) / sizeof (locks[0]);
	int i;

	for(i=0; i<n_locks; i++)
  	if (evt->locks & locks[i].lock)
			return (i+1);

	return 0;
}

modifiers modifier_is_pressed(DFBInputEvent *evt)
{
	int result = NONE;
	struct
	{
		DFBInputDeviceModifierMask  modifier;
		const char                 *name;
		int                         x;
	} modifiers[] =
		{
			{ DIMM_SHIFT,   "Shift", 0 },
			{ DIMM_CONTROL, "Ctrl",  0 },
			{ DIMM_ALT,     "Alt",   0 },
			{ DIMM_ALTGR,   "AltGr", 0 }
/*		{ DIMM_META,    "Meta",  0 }, I need these as keys, */
/*		{ DIMM_SUPER,   "Super", 0 }, not as modifiers      */
/*		{ DIMM_HYPER,   "Hyper", 0 }                        */
		};
	int n_modifiers = sizeof (modifiers) / sizeof (modifiers[0]);
	int i;

	if (!(evt->flags & DIEF_MODIFIERS)) return 0;
	for (i=0; i<n_modifiers; i++)
		if (evt->modifiers & modifiers[i].modifier)
		{
			if (!result) result = i+1;
			else if (result == CONTROL && (i+1) == ALT) result = CTRLALT;
		}

	return result;
}

int left_mouse_button_down(DFBInputEvent *evt)
{
	static struct
	{
		DFBInputDeviceButtonMask  mask;
		const char               *name;
		int                       x;
	} buttons[] = {
									{ DIBM_LEFT,   "Left",   0 },
									{ DIBM_MIDDLE, "Middle", 0 },
									{ DIBM_RIGHT,  "Right",  0 },
								};

	if (evt->buttons & buttons[0].mask) return 1;
	return 0;
}

int compare_symbol (const void *a, const void *b)
{
	DFBInputDeviceKeySymbol *symbol = (DFBInputDeviceKeySymbol *) a;
	struct DFBKeySymbolName *symname = (struct DFBKeySymbolName *) b;

	return *symbol - symname->symbol;
}

int compare_id (const void *a, const void *b)
{
	DFBInputDeviceKeyIdentifier *id     = (DFBInputDeviceKeyIdentifier *) a;
	struct DFBKeyIdentifierName *idname = (struct DFBKeyIdentifierName *) b;

	return *id - idname->identifier;
}

const char *get_device_name (DeviceInfo * devices, DFBInputDeviceID device_id)
{
	while (devices)
	{
		if (devices->device_id == device_id) return devices->desc.name;
		devices = devices->next;
	}

	return "<unknown>";
}

DFBEnumerationResult enum_input_device(DFBInputDeviceID device_id, DFBInputDeviceDescription desc, void *data)
{
	DeviceInfo **devices = data;
	DeviceInfo *device;

	device = malloc (sizeof (DeviceInfo));
	device->device_id = device_id;
	device->desc = desc;
	device->next = *devices;
	*devices = device;

	return DFENUM_OK;
}

IDirectFBSurface *load_image(const char *filename, IDirectFB *dfb, float x_ratio, float y_ratio)
{
	IDirectFBImageProvider *provider;
	IDirectFBSurface *image = NULL;
	DFBSurfaceDescription dsc;
	DFBResult err;

	if (access(filename, R_OK))
	{
		WRITELOG(ERROR, "Cannot load image: file '%s' does not exist!\n", filename);
		return NULL;
	}

	err = dfb->CreateImageProvider (dfb, filename, &provider);
	if (err != DFB_OK)
	{
		WRITELOG(ERROR, "Couldn't load image from file '%s': %s\n", filename, DirectFBErrorString (err));
		return NULL;
	}

	provider->GetSurfaceDescription (provider, &dsc);
	dsc.flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
	dsc.pixelformat = DSPF_ARGB;
	dsc.width       = (float)dsc.width  * (float)x_ratio;
	dsc.height      = (float)dsc.height * (float)y_ratio;
	if (dfb->CreateSurface (dfb, &dsc, &image) == DFB_OK) provider->RenderTo (provider, image, NULL);

	return image;
}

/* generic function to load a cursor shape */
void SetCursor(dfb_cursor_t **cursor, IDirectFB *dfb, cursor_t *cursor_data, float x_ratio, float y_ratio)
{
	IDirectFBSurface *temp_surf;
	dfb_cursor_t     *temp_curs;
	char             *my_path;

	if (!cursor)      return;
	if (!cursor_data) return;

	if (!cursor_data->enable)
	{
		if (*cursor)
		{
			free((*cursor)->surface);
			free(*cursor);
			*cursor = NULL;
		}
		return;
	}

	my_path = StrApp((char**)NULL, theme_dir, "/", cursor_data->path, (char*)NULL);

	temp_surf = load_image (my_path, dfb, x_ratio, y_ratio);
	free(my_path);

	if (temp_surf)
	{
		temp_curs = (dfb_cursor_t *)calloc(1, sizeof(dfb_cursor_t));
		temp_curs->surface = temp_surf;
		temp_curs->x_off   = cursor_data->x_off;
		temp_curs->y_off   = cursor_data->y_off;
		temp_curs->locked  = 0;
		
		*cursor = temp_curs;
	}
}
