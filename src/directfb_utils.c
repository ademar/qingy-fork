/***************************************************************************
                      directfb_utils.c  -  description
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


#include <stdio.h>
#include <stdlib.h>
#include <directfb.h>
#include <directfb_keynames.h>
#include "directfb_utils.h"

int lock_is_pressed(DFBInputEvent *evt)
{
	struct
	{
		DFBInputDeviceLockState lock;
		const char *name;
		int x;
	} locks[] =	{
								{DILS_SCROLL, "ScrollLock", 0},
								{DILS_NUM, "NumLock", 0},
								{DILS_CAPS, "CapsLock", 0},
							};
  int n_locks = sizeof (locks) / sizeof (locks[0]);
	int i;

	for(i=0; i<n_locks; i++)
  	if (evt->locks & locks[i].lock)
			return (i+1);

	return 0;
}

int modifier_is_pressed(DFBInputEvent *evt)
{
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
			{ DIMM_ALTGR,   "AltGr", 0 },
			{ DIMM_META,    "Meta",  0 },
			{ DIMM_SUPER,   "Super", 0 },
			{ DIMM_HYPER,   "Hyper", 0 }
		};
	int n_modifiers = sizeof (modifiers) / sizeof (modifiers[0]);
	int i;

	if (!(evt->flags & DIEF_MODIFIERS)) return 0;
	for (i=0; i<n_modifiers; i++)
		if (evt->modifiers & modifiers[i].modifier)
			return (i+1);

	return 0;
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

IDirectFBSurface *load_image(const char *filename, IDirectFBSurface *primary, IDirectFB *dfb)
{
	IDirectFBImageProvider *provider;
	IDirectFBSurface *tmp = NULL;
	IDirectFBSurface *surface = NULL;
	DFBSurfaceDescription dsc;
	DFBResult err;

	err = dfb->CreateImageProvider (dfb, filename, &provider);
	if (err != DFB_OK)
	{
		fprintf (stderr, "Couldn't load image from file '%s': %s\n", filename, DirectFBErrorString (err));
		return NULL;
	}

	provider->GetSurfaceDescription (provider, &dsc);
	dsc.flags = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
	dsc.pixelformat = DSPF_ARGB;
	if (dfb->CreateSurface (dfb, &dsc, &tmp) == DFB_OK) provider->RenderTo (provider, tmp, NULL);

	provider->Release (provider);

	if (tmp)
	{
		primary->GetPixelFormat (primary, &dsc.pixelformat);
		if (dfb->CreateSurface (dfb, &dsc, &surface) == DFB_OK)
		{
			surface->Clear (surface, 0, 0, 0, 0xFF);
			surface->SetBlittingFlags (surface, DSBLIT_BLEND_ALPHACHANNEL);
			surface->Blit (surface, tmp, NULL, 0, 0);
		}
		tmp->Release (tmp);
	}

	return surface;
}

struct button *load_button (const char *normal, const char *mouseover, int relx, int rely, IDirectFBDisplayLayer *layer, IDirectFBSurface *primary, IDirectFB *dfb, __u8 opacity)
{
	struct button *but;
	IDirectFBWindow *window;
	IDirectFBSurface *surface;
	DFBWindowDescription window_desc;
	DFBResult err;

	but = (struct button *) calloc (1, sizeof (struct button));
	but->normal = load_image (normal, primary, dfb);
	but->normal->GetSize (but->normal, &(but->width), &(but->height));
	but->mouseover = load_image (mouseover, primary, dfb);
	but->xpos = relx - but->width;
	but->ypos = rely - but->height;

	window_desc.flags  = ( DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS );
	window_desc.posx   = but->xpos;
	window_desc.posy   = but->ypos;
	window_desc.width  = but->width;
	window_desc.height = but->height;
	window_desc.caps   = DWCAPS_ALPHACHANNEL;

	DFBCHECK(layer->CreateWindow (layer, &window_desc, &window));
	window->SetOpacity( window, 0x00 );
	window->RaiseToTop( window );
	window->GetSurface( window, &surface );
	window->SetOpacity( window, opacity );
	but->mouse = 0;
	but->surface = surface;
	but->window = window;

	return but;
}

void destroy_button (struct button *button)
{
	if (!button) return;
	if (button->normal) button->normal->Release (button->normal);
	if (button->mouseover) button->mouseover->Release (button->mouseover);
	if (button->surface) button->surface->Release (button->surface);
	if (button->window) button->window->Release (button->window);

	free (button);
	button = NULL;
}

int compare_symbol (const void *a, const void *b)
{
	DFBInputDeviceKeySymbol *symbol = (DFBInputDeviceKeySymbol *) a;
	struct DFBKeySymbolName *symname = (struct DFBKeySymbolName *) b;

	return *symbol - symname->symbol;
}

int compare_id (const void *a, const void *b)
{
	DFBInputDeviceKeyIdentifier *id = (DFBInputDeviceKeyIdentifier *) a;
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
