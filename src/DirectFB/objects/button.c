/***************************************************************************
                      directfb_button.c  -  description
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

#include <stdio.h>
#include <stdlib.h>
#include <directfb.h>
#include <unistd.h>

#include "memmgmt.h"
#include "directfb_mode.h"
#include "button.h"
#include "load_settings.h"
#include "misc.h"


void Button_MouseOver(Button *thiz, int status)
{
	if (!thiz) return;

	thiz->mouse = status;
	thiz->surface->Clear (thiz->surface, 0x00, 0x00, 0x00, 0x00);
	if (status) thiz->surface->Blit(thiz->surface, thiz->mouseover, NULL, 0, 0);
	else thiz->surface->Blit(thiz->surface, thiz->normal, NULL, 0, 0);
	thiz->surface->Flip(thiz->surface, NULL, 0);
}

void Button_Show(Button *thiz)
{
	if (!thiz || !thiz->window) return;
	thiz->window->SetOpacity(thiz->window, BUTTON_OPACITY);
}

void Button_Hide(Button *thiz)
{
	if (!thiz || !thiz->window) return;
	thiz->window->SetOpacity(thiz->window, 0x00);
}

void Button_Destroy(Button *button)
{
	if (!button) return;
	if (button->normal) button->normal->Release (button->normal);
	if (button->mouseover) button->mouseover->Release (button->mouseover);
	if (button->surface) button->surface->Release (button->surface);
	if (button->window) button->window->Release (button->window);

	free (button);
}

IDirectFBSurface *load_image(const char *filename, IDirectFBSurface *primary, IDirectFB *dfb)
{
	IDirectFBImageProvider *provider;
	IDirectFBSurface *tmp = NULL;
	IDirectFBSurface *surface = NULL;
	DFBSurfaceDescription dsc;
	DFBResult err;

	if (access(filename, R_OK))
	{
		if (!silent) fprintf (stderr, "Cannot load image: file '%s' does not exist!\n", filename);
		return NULL;
	}

	err = dfb->CreateImageProvider (dfb, filename, &provider);
	if (err != DFB_OK)
	{
		if (!silent) fprintf (stderr, "Couldn't load image from file '%s': %s\n", filename, DirectFBErrorString (err));
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
			surface->Clear (surface, 0x00, 0x00, 0x00, 0x00);
			surface->SetBlittingFlags (surface, DSBLIT_BLEND_ALPHACHANNEL);			
			surface->Blit (surface, tmp, NULL, 0, 0);
		}
		tmp->Release (tmp);
	}

	return surface;
}

Button *Button_Create(const char *normal, const char *mouseover, int xpos, int ypos, IDirectFBDisplayLayer *layer, IDirectFBSurface *primary, IDirectFB *dfb)
{
	Button *but;
	IDirectFBWindow *window;
	IDirectFBSurface *surface;
	DFBWindowDescription window_desc;

	if (!normal || !mouseover || !layer || !primary || !dfb) return NULL;

	but = (Button *) calloc (1, sizeof (Button));
	but->normal = load_image (normal, primary, dfb);
	if (!but->normal)
	{
		free(but);
		return NULL;
	}
	but->normal->GetSize (but->normal, (int *)&(but->width), (int *)&(but->height));
	but->mouseover = load_image (mouseover, primary, dfb);
	if (!but->mouseover)
	{
		but->normal->Release (but->normal);
		free(but);
		return NULL;
	}
	but->xpos      = xpos;
	but->ypos      = ypos;
	but->Destroy   = Button_Destroy;
	but->MouseOver = Button_MouseOver;
	but->Show      = Button_Show;
	but->Hide      = Button_Hide;

	window_desc.flags  = ( DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS );
	window_desc.posx   = (unsigned int) but->xpos;
	window_desc.posy   = (unsigned int) but->ypos;
	window_desc.width  = but->width;
	window_desc.height = but->height;
	window_desc.caps   = DWCAPS_ALPHACHANNEL;

	if (layer->CreateWindow (layer, &window_desc, &window) != DFB_OK) return NULL;
	window->SetOpacity( window, 0x00 );
	window->RaiseToTop( window );
	window->GetSurface( window, &surface );
	surface->SetBlittingFlags (surface, DSBLIT_BLEND_ALPHACHANNEL);
	window->SetOpacity( window, BUTTON_OPACITY );
	but->mouse = 0;
	but->surface = surface;
	but->window = window;

	/* We need to add REAL transparency in button PNGs! */
	surface->SetSrcColorKey(surface, 0x00, 0x00, 0x00);
	surface->SetBlittingFlags(surface, DSBLIT_SRC_COLORKEY);

	return but;
}
