/***************************************************************************
                     framebuffer_mode.h  -  description
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
#include <string.h>
#include <unistd.h>
#include <directfb.h>
#include <directfb_keynames.h>

#include "framebuffer_mode.h"
#include "directfb_utils.h"
#include "chvt.h"
#include "misc.h"
#include "session.h"

#define POWEROFF 0
#define REBOOT 1
#define WINDOW_OPACITY 0x80
#define MASK_TEXT_COLOR  0xFF, 0x20, 0x20, 0xFF
#define OTHER_TEXT_COLOR 0x40, 0x40, 0x40, 0xFF

DeviceInfo *devices = NULL;		/* the list of all input devices									*/
IDirectFBEventBuffer *events;	/* input interfaces: device and its buffer				*/
IDirectFB *dfb;								/* the super interface														*/
IDirectFBSurface *primary;		/* the primary surface (surface of primary layer)	*/
IDirectFBDisplayLayer *layer;	/* the primary layer we use for mouse cursor			*/
struct button *power, *reset;	/* buttons																				*/
int mouse_x, mouse_y;					/* mouse coordinates															*/
IDirectFBSurface *panel_image;/* images																					*/
int we_stopped_gpm;						/* wether this program stopped gpm or not					*/
IDirectFBWindow	 *window;			/* the window we use for displaying text					*/
IDirectFBSurface *window_surface;	/* surface of the above												*/
DFBWindowDescription window_desc;	/* description of the window above						*/
unsigned int screen_width, screen_height;	/* screen resolution									*/
IDirectFBFont *font_small, *font_normal, *font_large;	/* fonts 									*/
char **sessions;
int n_sessions;

void DrawBase ()
{
	/* width and height of the background image */
	unsigned int panel_width, panel_height;

	/* We clear the primary surface */
	primary->Clear (primary, 0, 0, 0, 0);
	primary->Flip (primary, NULL, DSFLIP_BLIT);
	/* we design the surface */
	panel_image = load_image (DATADIR "background.png", primary, dfb);
	panel_image->GetSize (panel_image, (unsigned int *) &panel_width, (unsigned int *) &panel_height);

	/* we put the backgound image in the center of the screen if it fits
	   otherwise we stretch it to make it fit                            */
	if ( (panel_width <= screen_width) && (panel_height <= screen_height) )
		primary->Blit (primary, panel_image, NULL, (screen_width - panel_width)/2, (screen_height - panel_height)/2);
	else primary->StretchBlit (primary, panel_image, NULL, NULL);

	/* we draw power and reset buttons */
	primary->Blit (primary, power->normal, NULL, power->xpos, power->ypos);
	primary->Blit (primary, reset->normal, NULL, reset->xpos, reset->ypos);

	/* we draw the surface, duplicating it */
	primary->Flip (primary, NULL, DSFLIP_BLIT);
}

void CreateWindow(void)
{
	DFBResult err;
	/* This will contain Host name and welcome message */
	char *welcome_message;
	int len;

	welcome_message = (char *) calloc(MAX, sizeof(char));
	strncpy(welcome_message, "Welcome to ", MAX-1);
	len = strlen(welcome_message);
	gethostname(&(welcome_message[len]), MAX-len);

	/* All the necessary stuff to create a window */
	window_desc.flags  = ( DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS );
	window_desc.posx   = screen_width/8;
	window_desc.posy   = screen_height/8;
	window_desc.width  = 3*screen_width/4;
	window_desc.height = 3*screen_height/4;
	window_desc.caps   = DWCAPS_ALPHACHANNEL;
	DFBCHECK(layer->CreateWindow (layer, &window_desc, &window));
	window->GetSurface( window, &window_surface );
	window_surface->SetFont (window_surface, font_large);
	window_surface->SetColor (window_surface, MASK_TEXT_COLOR);
	window_surface->DrawString (window_surface, welcome_message, -1, window_desc.width / 2, 0, DSTF_TOP | DSTF_CENTER);
	window_surface->DrawString (window_surface, "login:", -1, window_desc.width/5, window_desc.height/3, DSTF_RIGHT);
	window_surface->DrawString (window_surface, "passwd:", -1, window_desc.width/5, window_desc.height/2, DSTF_RIGHT);
	window_surface->DrawString (window_surface, "session:", -1, window_desc.width/5, window_desc.height-window_desc.height/3, DSTF_RIGHT);
	window_surface->Flip( window_surface, NULL, 0 );
	window->SetOpacity( window, WINDOW_OPACITY );
	window->RequestFocus( window );
	window->RaiseToTop( window );

	free(welcome_message);
}

void close_framebuffer_mode (void)
{
	/* release our interfaces to shutdown DirectFB */
	while (devices)
	{
		DeviceInfo *next = devices->next;
		free (devices);
		devices = next;
	}
	if (power) destroy_button (power);
	if (reset) destroy_button (reset);
	if (panel_image) panel_image->Release (panel_image);
	font_small->Release (font_small);
	font_normal->Release (font_normal);
	font_large->Release (font_large);
	window_surface->Release (window_surface);
	window->Release (window);
	primary->Release (primary);
	events->Release (events);
	layer->Release (layer);
	dfb->Release (dfb);
}

void begin_shutdown_sequence (int action)
{
	DFBInputEvent evt;
	char message[35];
	int countdown = 5;

	/* We clear the double buffered surface */
	window->SetOpacity( window, 0x00 );
	primary->Clear (primary, 0, 0, 0, 0);
	primary->Flip (primary, NULL, DSFLIP_BLIT);
	primary->SetFont (primary, font_large);
	primary->SetColor (primary, OTHER_TEXT_COLOR);

	/* we wait for <countdown> seconds */
	while (countdown >= 0)
	{
		while ((events->GetEvent (events, DFB_EVENT (&evt))) == DFB_OK)
			if (evt.type == DIET_KEYPRESS)
			{
				layer->EnableCursor (layer, 0);
				layer->EnableCursor (layer, 1);
				if (evt.key_symbol == DIKS_ESCAPE)
				{
					DrawBase ();
					window->SetOpacity( window, WINDOW_OPACITY );
					return;
				}
			}
		if (!countdown) break;
		strcpy (message, "system ");
		switch (action)
		{
			case POWEROFF:
				strcat (message, "shutdown");
				break;
			case REBOOT:
				strcat (message, "restart");
				break;
			default:
				DrawBase ();
				return;
		}
		primary->Clear (primary, 0, 0, 0, 0);
		strcat (message, " in ");
		strcat (message, int_to_str (countdown));
		strcat (message, " seconds");
		primary->DrawString (primary, "Press ESC key to abort", -1, 0,
		screen_height, DSTF_LEFT | DSTF_BOTTOM);
		primary->DrawString (primary, message, -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
		primary->Flip (primary, NULL, 0);
		sleep (1);
		countdown--;
	}
	primary->Clear (primary, 0, 0, 0, 0);
	if (action == POWEROFF)
	{
		primary->DrawString (primary, "shutting down system...", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
		primary->Flip (primary, NULL, 0);
		execl ("/sbin/shutdown", "/sbin/shutdown", "-h", "now", (char *) 0);
	}
	if (action == REBOOT)
	{
		primary->DrawString (primary, "rebooting system...", -1,
		screen_width / 2, screen_height / 2, DSTF_CENTER);
		primary->Flip (primary, NULL, 0);
		execl ("/sbin/shutdown", "/sbin/shutdown", "-r", "now", (char *) 0);
	}

	/* we should never get here unless call to /sbin/shutdown fails */
	fprintf (stderr, "\nfatal error: unable to exec \"/sbin/shutdown\"!\n");
	close_framebuffer_mode ();
	exit (EXIT_FAILURE);
}

void handle_mouse_movement (void)
{
	/* mouse over power button */
	if ((mouse_x >= power->xpos) && (mouse_x <= (power->xpos + (int) power->width)))
		if ((mouse_y >= power->ypos) && (mouse_y <= (power->ypos + (int) power->height)))
		{
			if (!power->mouse)
			{
				power->mouse = 1;
				primary->Blit (primary, power->mouseover, NULL, power->xpos, power->ypos);
				primary->Blit (primary, reset->normal, NULL, reset->xpos, reset->ypos);
				primary->Flip (primary, NULL, DSFLIP_BLIT);
				return;
			}
			else return;		/* we already plotted this event */
		}

	/* mouse over reset button */
	if ((mouse_x >= reset->xpos) && (mouse_x <= (reset->xpos + (int) reset->width)))
		if ((mouse_y >= reset->ypos) && (mouse_y <= (reset->ypos + (int) reset->height)))
		{
			if (!reset->mouse)
			{
				reset->mouse = 1;
				primary->Blit (primary, power->normal, NULL, power->xpos, power->ypos);
				primary->Blit (primary, reset->mouseover, NULL, reset->xpos, reset->ypos);
				primary->Flip (primary, NULL, DSFLIP_BLIT);
				return;
			}
			else return;		/* we already plotted this event */
		}

	/* mouse not over any power button */
	if ((power->mouse) || (reset->mouse))
	{
		power->mouse = 0;
		reset->mouse = 0;
		primary->Blit (primary, power->normal, NULL, power->xpos, power->ypos);
		primary->Blit (primary, reset->normal, NULL, reset->xpos, reset->ypos);
		primary->Flip (primary, NULL, DSFLIP_BLIT);
	}
}

void handle_mouse_event (DFBInputEvent *evt)
{
	static int status = 0;

	if (evt->type == DIET_AXISMOTION)
		handle_mouse_movement ();
	else
	{	/* mouse button press or release */
		if (left_mouse_button_down (evt))
		{	/* left mouse button is down:
			   we check wether mouse pointer is over a button */
			if (power->mouse) status = 1;
			if (reset->mouse) status = 2;
		}
		else
		{	/* left mouse button is up:
			   if it was on a button when down we check if it is still there */
			if ((power->mouse) && (status == 1))	/* power button has been clicked! */
				begin_shutdown_sequence (POWEROFF);
			if ((reset->mouse) && (status == 2))	/* reset button has been clicked! */
				begin_shutdown_sequence (REBOOT);
			status = 0;		/* we reset click status because button went up */
		}
	}
}

int handle_keyboard_event(DFBInputEvent *evt)
{
	int temp;	/* yuk! we have got a temp variable! */
  /* we store here the symbol name of the last key the user pressed ...	*/
	struct DFBKeySymbolName *symbol_name;
	/* ... and here the previous one */
	static DFBInputDeviceKeySymbol last_symbol = DIKS_NULL;
	struct DFBKeyIdentifierName *id_name;
	int returnstatus = -1;

	/* If user presses ESC two times, we bo back to text mode */
	if (last_symbol == DIKS_ESCAPE && evt->key_symbol == DIKS_ESCAPE) return TEXT_MODE;
	/* We store the previous keystroke */
	last_symbol = evt->key_symbol;
	symbol_name = bsearch (&(evt->key_symbol), keynames, sizeof (keynames) / sizeof (keynames[0]) - 1, sizeof (keynames[0]), compare_symbol);
	id_name = bsearch(&(evt->key_id), idnames, sizeof(idnames) / sizeof(idnames[0]) - 1, sizeof(idnames[0]), compare_id );
	/* we check if the user is pressing ALT-number with 1 <= number <= 12
		 if so we close directfb mode and send him to that tty              */
	if (modifier_is_pressed(evt) == ALT)
	{
		if (symbol_name)
		{
			if ((strlen (symbol_name->name) <= 3) && (strncmp (symbol_name->name, "F", 1) == 0))
			{
				temp = atoi (symbol_name->name + 1);
				if ((temp > 0) && (temp < 13))
					if (get_active_tty () != temp)
					{
						return temp;
					}
			}
		}
		/* we check if user press ALT-p or ALT-r to start shutdown/reboot sequence */
		if (id_name)
		{
			char test= (id_name->name)[0];
			if ((test == 'P')||(test == 'p')) begin_shutdown_sequence (POWEROFF);
			if ((test == 'R')||(test == 'r')) begin_shutdown_sequence (REBOOT);
		}
	}
	/* Sometimes mouse cursor disappears after a keystroke,
	   we make it visible again                             */
	layer->EnableCursor (layer, 0);
	layer->EnableCursor (layer, 1);

	return returnstatus;
}

void set_font_sizes ()
{
	DFBResult err;
	DFBFontDescription fdsc;
	const char *fontfile = FONT;

	fdsc.flags = DFDESC_HEIGHT;
	fdsc.height = screen_width / 30;
	DFBCHECK (dfb->CreateFont (dfb, fontfile, &fdsc, &font_large));
	fdsc.height = screen_width / 40;
	DFBCHECK (dfb->CreateFont (dfb, fontfile, &fdsc, &font_normal));
	fdsc.height = screen_width / 50;
	DFBCHECK (dfb->CreateFont (dfb, fontfile, &fdsc, &font_small));
}

int framebuffer_mode (int argc, char *argv[])
{
	int returnstatus = -1;			/* return value of this function...											*/
	DFBResult err;							/* used by that bloody macro to check for errors				*/
	DFBSurfaceDescription sdsc;	/* this will be the description for the primary surface	*/
	DFBInputEvent evt;					/* generic input events will be stored here							*/
	int i;

	we_stopped_gpm= stop_gpm();

	n_sessions=how_many_sessions();
	sessions = get_sessions(n_sessions);
	for(i=0;i<n_sessions;i++)
	  fprintf(stderr, "%s\n", sessions[i]);

	/* we initialize directfb */
	DFBCHECK (DirectFBInit (&argc, &argv));
	DFBCHECK (DirectFBCreate (&dfb));
	/* create a list of input devices */
	dfb->EnumInputDevices (dfb, enum_input_device, &devices);
	/* create an event buffer for all devices */
	DFBCHECK (dfb->CreateInputEventBuffer (dfb, DICAPS_ALL, DFB_TRUE, &events));
	DFBCHECK (dfb->GetDisplayLayer (dfb, DLID_PRIMARY, &layer));
	layer->SetCooperativeLevel (layer, DLSCL_ADMINISTRATIVE);
	layer->EnableCursor (layer, 1);
	sdsc.flags = DSDESC_CAPS;
	sdsc.caps  = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &primary ));
	primary->GetSize (primary, &screen_width, &screen_height);
	mouse_x = screen_width / 2;
	mouse_y = screen_height / 2;
	set_font_sizes ();
	power = load_button (DATADIR "power_normal.png", DATADIR "power_mouseover.png", screen_width, screen_height, primary, dfb);
	reset = load_button (DATADIR "reset_normal.png", DATADIR "reset_mouseover.png", power->xpos - 10, screen_height, primary, dfb);
	DrawBase();
	CreateWindow();

	/* we go on for ever... or until the user does something in particular */
	while (returnstatus == -1)
	{
		/* we wait for an input event */
		events->WaitForEvent (events);
		events->GetEvent (events, DFB_EVENT (&evt));
		if ((evt.type == DIET_AXISMOTION) || (evt.type == DIET_BUTTONPRESS) || (evt.type == DIET_BUTTONRELEASE))
		{	/* handle mouse */
			layer->GetCursorPosition (layer, &mouse_x, &mouse_y);
			handle_mouse_event (&evt);
		}
		if (evt.type == DIET_KEYPRESS)
		{	/* manage keystrokes */
			returnstatus= handle_keyboard_event(&evt);
		}
	}

	close_framebuffer_mode ();
	if (we_stopped_gpm) start_gpm();
	return returnstatus;
}
