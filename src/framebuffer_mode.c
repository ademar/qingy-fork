/***************************************************************************
                     framebuffer_mode.c  -  description
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
#include "directfb_textbox.h"
#include "chvt.h"
#include "misc.h"
#include "session.h"
#include "load_settings.h"

#define POWEROFF 0
#define REBOOT   1
#define UP       2
#define DOWN    -2
#define YES      1
#define NO       0
#define DESTROY -1

DeviceInfo *devices = NULL;   /* the list of all input devices             */
IDirectFBEventBuffer *events; /* input interfaces: device and its buffer   */
IDirectFB *dfb;               /* the super interface                       */
IDirectFBSurface *primary;    /* surface of the primary layer              */
IDirectFBDisplayLayer *layer; /* the primary layer                         */
struct button *power, *reset; /* buttons                                   */
int mouse_x, mouse_y;         /* mouse coordinates                         */
int we_stopped_gpm;           /* wether this program stopped gpm or not    */
IDirectFBSurface *panel_image=NULL;/* background image                     */
IDirectFBWindow	 *button_window = NULL;/* window for displaying buttons    */
IDirectFBSurface *button_window_surface = NULL;/* surface of the above     */
unsigned int screen_width, screen_height;/* screen resolution              */
IDirectFBFont *font_small, *font_normal, *font_large;/* fonts              */
int font_small_height, font_normal_height, font_large_height;/* font sizes */
TextBox *username = NULL, *password = NULL;
int session_hasfocus = 0;
int workaround = -1;

void username_event(int *input, char *output, int action, __u8 opacity);
void password_event(int *input, char *output, int action, __u8 opacity);

void Draw_Background_Image()
{
	/* width and height of the background image */
	static unsigned int panel_width, panel_height;

	/* We clear the primary surface */
	primary->Clear (primary, 0, 0, 0, 0);
	if (!panel_image)
	{ /* we design the surface */
		panel_image = load_image (DATADIR "background.png", primary, dfb);
		if (panel_image != NULL)
			panel_image->GetSize (panel_image, (unsigned int *) &panel_width, (unsigned int *) &panel_height);
	}
	/* we put the backgound image in the center of the screen if it fits
	   the screen otherwise we stretch it to make it fit                    */
	if (!!panel_image)
	{
		if ( (panel_width <= screen_width) && (panel_height <= screen_height) )
			primary->Blit (primary, panel_image, NULL, (screen_width - panel_width)/2, (screen_height - panel_height)/2);
		else primary->StretchBlit (primary, panel_image, NULL, NULL);
	}

	/* we draw the surface, duplicating it */
	primary->Flip (primary, NULL, DSFLIP_BLIT);
}

void show_welcome_window(int action)
{
	static IDirectFBWindow	*window = NULL; /* window for welcome message */
	static IDirectFBSurface *window_surface= NULL; /* surface of the above       */

	if (!window && action != DESTROY)
	{
		DFBResult err;
		DFBWindowDescription window_desc;	/* description of the window above						*/
		/* This will contain Host name and welcome message */
		char *welcome_message;
		int len;

		welcome_message = (char *) calloc(MAX, sizeof(char));
		strncpy(welcome_message, "Welcome to ", MAX-1);
		len = strlen(welcome_message);
		gethostname(&(welcome_message[len]), MAX-len);

		/* All the necessary stuff to create a window */
		window_desc.flags  = ( DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS );
		window_desc.posx   = 0;
		window_desc.posy   = screen_height/8;
		window_desc.width  = screen_width;
		window_desc.height = 2*font_large_height;
		window_desc.caps   = DWCAPS_ALPHACHANNEL;
		DFBCHECK(layer->CreateWindow (layer, &window_desc, &window));
		window->SetOpacity(window, 0x00);
		window->GetSurface(window, &window_surface);
		window_surface->Clear(window_surface, 0x00, 0x00, 0x00, 0x00);
		window_surface->SetFont (window_surface, font_large);
		window_surface->SetColor (window_surface, MASK_TEXT_COLOR);
		window_surface->DrawString (window_surface, welcome_message, -1, window_desc.width/2, window_desc.height/2, DSTF_CENTER);
		window_surface->Flip(window_surface, NULL, 0);
		window->RequestFocus(window);
		window->RaiseToTop(window);
		free(welcome_message);
	}

	switch (action)
	{
		case YES:
			window->SetOpacity(window, WINDOW_OPACITY);
			break;
		case NO:
			window->SetOpacity(window, 0x00);
			break;
		case DESTROY:
			if (window_surface) window_surface->Release (window_surface);
			if (window) window->Release (window);
			window = NULL; window_surface = NULL;
			break;
	}
}

void show_login_window(int action, __u8 opacity)
{
	static IDirectFBWindow	*window = NULL;
	static IDirectFBSurface *window_surface= NULL;

	if (!window && action != DESTROY)
	{
		DFBResult err;
		DFBWindowDescription window_desc;

		window_desc.flags  = ( DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS );
		window_desc.posx   = screen_width/8;
		window_desc.posy   = 3*screen_height/8-font_large_height;
		window_desc.width  = 3*screen_width/20;
		window_desc.height = 2*font_large_height;
		window_desc.caps   = DWCAPS_ALPHACHANNEL;

		DFBCHECK(layer->CreateWindow (layer, &window_desc, &window));
		window->SetOpacity( window, 0x00 );
		window->GetSurface( window, &window_surface );
		window_surface->Clear(window_surface, 0x00, 0x00, 0x00, 0x00);
		window_surface->SetFont (window_surface, font_large);
		window_surface->SetColor (window_surface, MASK_TEXT_COLOR);
		window_surface->DrawString (window_surface, "login:", -1, window_desc.width, window_desc.height/2, DSTF_RIGHT);
		window_surface->Flip(window_surface, NULL, 0);
		window->RequestFocus(window);
		window->RaiseToTop(window);
	}

	switch (action)
	{
		case YES:
			window->SetOpacity( window, opacity);;
			break;
		case NO:
			window->SetOpacity( window, 0x00 );;
			break;
		case DESTROY:
			if (window_surface) window_surface->Release (window_surface);
			if (window) window->Release (window);
			window = NULL; window_surface = NULL;
			break;
	}
}

void show_passwd_window(int action, __u8 opacity)
{
	static IDirectFBWindow	*window = NULL;
	static IDirectFBSurface *window_surface= NULL;

	if (!window && action != DESTROY)
	{
		DFBResult err;
		DFBWindowDescription window_desc;

		window_desc.flags  = ( DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS );
		window_desc.posx   = screen_width/8;
		window_desc.posy   = 4*screen_height/8-font_large_height;
		window_desc.width  = 3*screen_width/20;
		window_desc.height = 2*font_large_height;
		window_desc.caps   = DWCAPS_ALPHACHANNEL;

		DFBCHECK(layer->CreateWindow (layer, &window_desc, &window));
		window->SetOpacity( window, 0x00 );
		window->GetSurface( window, &window_surface );
		window_surface->Clear(window_surface, 0x00, 0x00, 0x00, 0x00);
		window_surface->SetFont (window_surface, font_large);
		window_surface->SetColor (window_surface, MASK_TEXT_COLOR);
		window_surface->DrawString (window_surface, "passwd:", -1, window_desc.width, window_desc.height/2, DSTF_RIGHT);
		window_surface->Flip( window_surface, NULL, 0 );
		window->RequestFocus( window );
		window->RaiseToTop( window );
	}

	switch (action)
	{
		case YES:
			window->SetOpacity( window, opacity);;
			break;
		case NO:
			window->SetOpacity( window, 0x00 );;
			break;
		case DESTROY:
			if (window_surface) window_surface->Release (window_surface);
			if (window) window->Release (window);
			window = NULL; window_surface = NULL;
			break;
	}
}

void show_session_window(int action, __u8 opacity)
{
	static IDirectFBWindow	*window = NULL;
	static IDirectFBSurface *window_surface= NULL;

	if (!window && action != DESTROY)
	{
		DFBResult err;
		DFBWindowDescription window_desc;

		window_desc.flags  = ( DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS );
		window_desc.posx   = screen_width/8;
		window_desc.posy   = 5*screen_height/8-font_large_height;
		window_desc.width  = 3*screen_width/20;
		window_desc.height = 2*font_large_height;
		window_desc.caps   = DWCAPS_ALPHACHANNEL;

		DFBCHECK(layer->CreateWindow (layer, &window_desc, &window));
		window->SetOpacity( window, 0x00 );
		window->GetSurface( window, &window_surface );
		window_surface->Clear(window_surface, 0x00, 0x00, 0x00, 0x00);
		window_surface->SetFont (window_surface, font_large);
		window_surface->SetColor (window_surface, MASK_TEXT_COLOR);
		window_surface->DrawString (window_surface, "session:", -1, window_desc.width, window_desc.height-window_desc.height/2, DSTF_RIGHT);
		window_surface->Flip( window_surface, NULL, 0 );
		window->RequestFocus( window );
		window->RaiseToTop( window );
	}

	switch (action)
	{
		case YES:
			window->SetOpacity( window, opacity);;
			break;
		case NO:
			window->SetOpacity( window, 0x00 );;
			break;
		case DESTROY:
			if (window_surface) window_surface->Release (window_surface);
			if (window) window->Release (window);
			window = NULL; window_surface = NULL;
			break;
	}
}

int print_session_name(int action, char *user, __u8 opacity)
{
	static struct session *session = NULL;
	static IDirectFBWindow	*window = NULL;
	static IDirectFBSurface *window_surface= NULL;
	static DFBWindowDescription window_desc;

	if (!session)
	{ /* Get info about available sessions */
		get_sessions();
		session = &sessions;
	}
	if (!window && action != DESTROY)
	{
		DFBResult err;

		window_desc.flags  = ( DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS );
		window_desc.posx   = 3*screen_width/10;
		window_desc.posy   = 5*screen_height/8-font_large_height;
		window_desc.width  = 3*screen_width/4 - window_desc.posx;
		window_desc.height = 2*font_large_height;

		window_desc.caps   = DWCAPS_ALPHACHANNEL;
		DFBCHECK(layer->CreateWindow (layer, &window_desc, &window));
		window->SetOpacity( window, 0x00 );
		window->GetSurface( window, &window_surface );
		window_surface->Clear(window_surface, 0x00, 0x00, 0x00, 0x00);
		window_surface->SetFont (window_surface, font_large);
		window_surface->SetColor (window_surface, MASK_TEXT_COLOR);
		window_surface->DrawString (window_surface, session->name, -1, 0, window_desc.height/2, DSTF_LEFT);
		window_surface->Flip( window_surface, NULL, 0 );
		window->RequestFocus( window );
		window->RaiseToTop( window );
	}

	switch (action)
	{
		case YES:
			if (user != NULL)
			{
				struct session *first = session;
				char *user_session = get_last_session(user);

				if (!!user_session)
				{
					while (strcmp(session->name, user_session) != 0)
					{
						session = session->next;
						if (session == first) break;
					}
					if (session != first)
					{
						window_surface->Clear(window_surface, 0x00, 0x00, 0x00, 0x00);
						window_surface->DrawString (window_surface, session->name, -1, 0, window_desc.height/2, DSTF_LEFT);
						window_surface->Flip( window_surface, NULL, 0 );
					}
				}
			}
			window->SetOpacity( window, opacity);
			break;
		case NO:
			window->SetOpacity( window, 0x00);
			break;
		case DESTROY:
			if (window_surface) window_surface->Release (window_surface);
			if (window) window->Release (window);
			window = NULL; window_surface = NULL;
			break;
		case UP:
			session= session->prev;
			window_surface->Clear(window_surface, 0x00, 0x00, 0x00, 0x00);
			window_surface->DrawString (window_surface, session->name, -1, 0, window_desc.height/2, DSTF_LEFT);
			window_surface->Flip( window_surface, NULL, 0 );
			break;
		case DOWN:
			session= session->next;
			window_surface->Clear(window_surface, 0x00, 0x00, 0x00, 0x00);
			window_surface->DrawString (window_surface, session->name, -1, 0, window_desc.height/2, DSTF_LEFT);
			window_surface->Flip( window_surface, NULL, 0 );
			break;
	}

	return session->id;
}

void show_lock_key_status(DFBInputEvent *evt, int action)
{
	static IDirectFBWindow	*window = NULL;
	static IDirectFBSurface *window_surface = NULL;
	__u8 opacity;

	if (!window_surface)
	{
		DFBWindowDescription window_desc;
		DFBResult err;

		window_desc.flags  = ( DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS );
		window_desc.posx   = 0;
		window_desc.posy   = screen_height - (font_small_height);
		window_desc.width  = screen_width/5;
		window_desc.height = font_small_height;
		window_desc.caps   = DWCAPS_ALPHACHANNEL;
		DFBCHECK(layer->CreateWindow (layer, &window_desc, &window));
		window->GetSurface(window, &window_surface );
		window_surface->SetFont (window_surface, font_small);
		window_surface->SetColor (window_surface, MASK_TEXT_COLOR);
		window_surface->DrawString (window_surface, "CAPS LOCK is pressed", -1, window_desc.width/2, window_desc.height+1, DSTF_CENTER|DSTF_BOTTOM );
		window_surface->Flip(window_surface, NULL, 0);
		window->SetOpacity(window, 0x00);
		window->RaiseToTop(window);
	}

	switch (action)
	{
		case DESTROY:
			if (window_surface) window_surface->Release (window_surface);
			if (window) window->Release (window);
			break;
		case NO:
			window->SetOpacity(window, 0x00);
			break;
		case YES:
			window->GetOpacity(window, &opacity);
			if (lock_is_pressed(evt) == 3)
			{
				if (opacity == 0x00) window->SetOpacity(window, WINDOW_OPACITY);
			}
			else
			{
				if (opacity != 0x00) window->SetOpacity(window, 0x00);
			}
			break;
	}
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
	show_welcome_window(DESTROY);
	show_lock_key_status(NULL, DESTROY);
	print_session_name(DESTROY, NULL, 0x00);
	show_login_window(DESTROY, 0x00);
	TextBox_Destroy(username); username = NULL;
	show_passwd_window(DESTROY, 0x00);
	TextBox_Destroy(password); password = NULL;
	show_session_window(DESTROY, 0x00);
	primary->Release (primary);
	events->Release (events);
	layer->Release (layer);
	dfb->Release (dfb);
	if (we_stopped_gpm) start_gpm();
}

void handle_buttons(void)
{
	/* mouse over power button */
	if ((mouse_x >= power->xpos) && (mouse_x <= (power->xpos + (int) power->width)))
		if ((mouse_y >= power->ypos) && (mouse_y <= (power->ypos + (int) power->height)))
		{
			if (!power->mouse)
			{
				power->mouse = 1;
				power->surface->Clear (power->surface, 0x00, 0x00, 0x00, 0x00);
				power->surface->Blit(power->surface, power->mouseover, NULL, 0, 0);
				power->surface->Flip(power->surface, NULL, 0);
				if (reset->mouse)
				{
					reset->mouse = 0;
					reset->surface->Clear (reset->surface, 0x00, 0x00, 0x00, 0x00);
					reset->surface->Blit(reset->surface, reset->normal, NULL, 0, 0);
					reset->surface->Flip(reset->surface, NULL, 0);
				}
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
				reset->surface->Clear (reset->surface, 0x00, 0x00, 0x00, 0x00);
				reset->surface->Blit(reset->surface, reset->mouseover, NULL, 0, 0);
				reset->surface->Flip(reset->surface, NULL, 0);
				if (power->mouse)
				{
					power->mouse = 0;
					power->surface->Clear (power->surface, 0x00, 0x00, 0x00, 0x00);
					power->surface->Blit(power->surface, power->normal, NULL, 0, 0);
					power->surface->Flip(power->surface, NULL, 0);
				}
				return;
			}
			else return;		/* we already plotted this event */
		}

	/* mouse not over any power button */
	if (power->mouse)
	{
		power->mouse = 0;
		power->surface->Clear (power->surface, 0x00, 0x00, 0x00, 0x00);
		power->surface->Blit(power->surface, power->normal, NULL, 0, 0);
		power->surface->Flip(power->surface, NULL, 0);
	}
	if (reset->mouse)
	{
		reset->mouse = 0;
		reset->surface->Clear (reset->surface, 0x00, 0x00, 0x00, 0x00);
		reset->surface->Blit(reset->surface, reset->normal, NULL, 0, 0);
		reset->surface->Flip(reset->surface, NULL, 0);
	}
}

void handle_textboxes(void)
{
	/* mouse over username area */
	if ( (mouse_x >= (int) username->xpos) && (mouse_x <= (int) username->xpos + (int) username->width) )
		if ( (mouse_y >= (int) username->ypos) && (mouse_y <= (int) username->ypos + (int) username->height) )
		{
			username->mouse = 1;
			password->mouse = 0;
			return;
		}

	/* mouse over password area */
	if ( (mouse_x >= (int) password->xpos) && (mouse_x <= (int) password->xpos + (int) password->width) )
		if ( (mouse_y >= (int) password->ypos) && (mouse_y <= (int) password->ypos + (int) password->height) )
		{
			username->mouse = 0;
			password->mouse = 1;
			return;
		}

	/* mouse not over any textbox */
	username->mouse = 0;
	password->mouse = 0;
}

void handle_mouse_movement (void)
{
	handle_buttons();
	handle_textboxes();
}

void reset_screen(DFBInputEvent *evt)
{
	Draw_Background_Image ();
	power->mouse = 0;
	reset->mouse = 0;
	power->window->SetOpacity(power->window, BUTTON_OPACITY);
	reset->window->SetOpacity(reset->window, BUTTON_OPACITY);
	show_welcome_window(YES);
	show_login_window(YES, (username->hasfocus) ? SELECTED_WINDOW_OPACITY : WINDOW_OPACITY);
	TextBox_Show(username);
	show_passwd_window(YES, (password->hasfocus) ? SELECTED_WINDOW_OPACITY : WINDOW_OPACITY);
	TextBox_Show(password);
	show_session_window(YES, (session_hasfocus) ? SELECTED_WINDOW_OPACITY : WINDOW_OPACITY);
	print_session_name(YES, NULL, (session_hasfocus) ? SELECTED_WINDOW_OPACITY : WINDOW_OPACITY);
	show_lock_key_status(evt, YES);
	layer->GetCursorPosition (layer, &mouse_x, &mouse_y);
	handle_mouse_movement();
	layer->EnableCursor (layer, 1);
}

void clear_screen(void)
{
	power->window->SetOpacity(power->window, 0x00 );
	reset->window->SetOpacity(reset->window, 0x00 );
	show_login_window(NO, 0x00);
	TextBox_Hide(username);
	TextBox_Hide(password);
	show_passwd_window(NO, 0x00);
	show_session_window(NO, 0x00);
	show_welcome_window(NO);
	show_lock_key_status(NULL, NO);
	print_session_name(NO, NULL, 0x00);
	layer->EnableCursor (layer, 0);
	primary->Clear (primary, 0, 0, 0, 0);
	primary->Flip (primary, NULL, DSFLIP_BLIT);
	primary->SetFont (primary, font_large);
	primary->SetColor (primary, OTHER_TEXT_COLOR);
}

void begin_shutdown_sequence (int action)
{
	DFBInputEvent evt;
	char message[35];
	int countdown = 5;

	clear_screen();

	/* we wait for <countdown> seconds */
	while (countdown >= 0)
	{
		while ((events->GetEvent (events, DFB_EVENT (&evt))) == DFB_OK)
			if (evt.type == DIET_KEYPRESS)
				if (evt.key_symbol == DIKS_ESCAPE)
				{ /* user aborted sequence */
					reset_screen(&evt);
					return;
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
				Draw_Background_Image ();
				return;
		}
		primary->Clear (primary, 0, 0, 0, 0);
		strcat (message, " in ");
		strcat (message, int_to_str (countdown));
		strcat (message, " seconds");
		primary->DrawString (primary, "Press ESC key to abort", -1, 0, screen_height, DSTF_LEFT | DSTF_BOTTOM);
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
		primary->DrawString (primary, "rebooting system...", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
		primary->Flip (primary, NULL, 0);
		execl ("/sbin/shutdown", "/sbin/shutdown", "-r", "now", (char *) 0);
	}

	/* we should never get here unless call to /sbin/shutdown fails */
	fprintf (stderr, "\nfatal error: unable to exec \"/sbin/shutdown\"!\n");
	close_framebuffer_mode ();
	exit (EXIT_FAILURE);
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
			if (username->mouse) status = 3;
			if (password->mouse) status = 4;
		}
		else
		{	/* left mouse button is up:
			   if it was on a button when down we check if it is still there */
			if ((power->mouse) && (status == 1))	/* power button has been clicked! */
				begin_shutdown_sequence (POWEROFF);
			if ((reset->mouse) && (status == 2))	/* reset button has been clicked! */
				begin_shutdown_sequence (REBOOT);
			if ((username->mouse) && (status == 3))	/* username textbox has been clicked! */
			{
				session_hasfocus  = 0;
				TextBox_SetFocus(username, 1);
				show_login_window(YES, SELECTED_WINDOW_OPACITY);
				TextBox_SetFocus(password, 0);
				show_passwd_window(YES, WINDOW_OPACITY);
				show_session_window(YES, WINDOW_OPACITY);
				print_session_name(YES, NULL, WINDOW_OPACITY);
			}
			if ((password->mouse) && (status == 4))	/* username textbox has been clicked! */
			{
				session_hasfocus  = 0;
				TextBox_SetFocus(username, 0);
				show_login_window(YES, WINDOW_OPACITY);
				TextBox_SetFocus(password, 1);
				show_passwd_window(YES, SELECTED_WINDOW_OPACITY);
				show_session_window(YES, WINDOW_OPACITY);
				print_session_name(YES, NULL, WINDOW_OPACITY);
			}

			status = 0;		/* we reset click status because button went up */
		}
	}
}

void start_login_sequence(DFBInputEvent *evt)
{
	int session_id;
	char message[MAX];
	char *user_name;

	if (strlen(username->text) == 0) return;
	session_id = print_session_name(YES, NULL, WINDOW_OPACITY);
	strncpy(message, "Logging in ", MAX);
	strncat(message, username->text, MAX-strlen(message));
	strncat(message, "...", MAX-strlen(message));
	clear_screen();
	primary->DrawString (primary, message, -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
	primary->Flip (primary, NULL, DSFLIP_BLIT);
	sleep(1);

	if (!check_password(username->text, password->text))
	{
		primary->Clear (primary, 0, 0, 0, 0);
		primary->DrawString (primary, "Login failed!", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
		primary->Flip (primary, NULL, DSFLIP_BLIT);
		sleep(1);
		TextBox_ClearText(password);
		reset_screen(evt);
		return;
	}
	primary->Clear (primary, 0, 0, 0, 0);
	primary->DrawString (primary, "Starting selected session...", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
	primary->Flip (primary, NULL, DSFLIP_BLIT);
	sleep(1);
	user_name = (char *) calloc(strlen(username->text)+1, sizeof(char));
	strcpy(user_name, username->text);
	close_framebuffer_mode();
	start_session(user_name, session_id, workaround);

	/* The above never returns, so... */
	free(user_name); user_name = NULL;
	fprintf(stderr, "Go tell my creator his brains went pop!\n");
	exit(0);
}

int handle_keyboard_event(DFBInputEvent *evt)
{
	int temp;	/* yuk! we have got a temp variable! */
  /* we store here the symbol name of the last key the user pressed ...	*/
	struct DFBKeySymbolName *symbol_name;
	/* ... and here the previous one */
	static int last_symbol;
	int returnstatus = -1;
 	int allow_tabbing = 1;
	int ascii_code = (int) evt->key_symbol;

	show_lock_key_status(evt, YES);
	/* If user presses ESC two times, we bo back to text mode */
	if ((last_symbol == ESCAPE) && (ascii_code == ESCAPE)) return TEXT_MODE;
	/* We store the previous keystroke */
	last_symbol = ascii_code;
	symbol_name = bsearch (&(evt->key_symbol), keynames, sizeof (keynames) / sizeof (keynames[0]) - 1, sizeof (keynames[0]), compare_symbol);
	if (modifier_is_pressed(evt) == ALT)
	{ /* we check if the user is pressing ALT-number with 1 <= number <= 12
		   if so we close directfb mode and send him to that tty              */
		if (symbol_name)
			if ((strlen (symbol_name->name) <= 3) && (strncmp (symbol_name->name, "F", 1) == 0))
			{
				temp = atoi (symbol_name->name + 1);
				if ((temp > 0) && (temp < 13))
					if (get_active_tty () != temp)
						return temp;
			}
		/* we check if user press ALT-p or ALT-r to start shutdown/reboot sequence */
		if ((ascii_code == 'P')||(ascii_code == 'p')) begin_shutdown_sequence (POWEROFF);
		if ((ascii_code == 'R')||(ascii_code == 'r')) begin_shutdown_sequence (REBOOT);
		/* we get here only if user aborted shutdown */
		layer->EnableCursor (layer, 1);
		return returnstatus;
	}

	if (symbol_name)
	{
		/* Rock'n Roll! */
		if (ascii_code == RETURN) start_login_sequence(evt);

		/* user name events */
		if (username->hasfocus && allow_tabbing)
		{
			if (ascii_code == TAB)
			{
				allow_tabbing = 0;
				TextBox_SetFocus(username, 0);
				show_login_window(YES, WINDOW_OPACITY);
				if (modifier_is_pressed(evt) != SHIFT)
				{
					TextBox_SetFocus(password, 1);
					show_passwd_window(YES, SELECTED_WINDOW_OPACITY);
				}
				else
				{
					session_hasfocus  = 1;
					show_session_window(YES, SELECTED_WINDOW_OPACITY);
					print_session_name(YES, NULL, SELECTED_WINDOW_OPACITY);
				}
			}
			else
			{
				char old_user[MAX];

				strncpy(old_user, username->text, MAX-1);
				TextBox_KeyEvent(username, ascii_code, 1);
				if (strcmp(old_user,username->text) != 0)
					print_session_name(YES, username->text, WINDOW_OPACITY);				
			}
		}

		/* password events */
		if (password->hasfocus && allow_tabbing)
		{
			if (ascii_code == TAB)
			{
				allow_tabbing = 0;
				TextBox_SetFocus(password, 0);
				show_passwd_window(YES, WINDOW_OPACITY);
				if (modifier_is_pressed(evt) != SHIFT)
				{
					session_hasfocus  = 1;
					show_session_window(YES, SELECTED_WINDOW_OPACITY);
					print_session_name(YES, NULL, SELECTED_WINDOW_OPACITY);
				}
				else
				{
					TextBox_SetFocus(username, 1);
					show_login_window(YES, SELECTED_WINDOW_OPACITY);
				}
			}
			else TextBox_KeyEvent(password, ascii_code, 1);
		}

		/* session events */
		if (session_hasfocus && allow_tabbing)
		{
			if (ascii_code == ARROW_UP) print_session_name(UP, NULL, WINDOW_OPACITY);
			if (ascii_code == ARROW_DOWN) print_session_name(DOWN, NULL, WINDOW_OPACITY);
			if (ascii_code == TAB)
			{
				session_hasfocus  = 0;
				allow_tabbing = 0;
				show_session_window(YES, WINDOW_OPACITY);
				print_session_name(YES, NULL, WINDOW_OPACITY);
				if (modifier_is_pressed(evt) != SHIFT)
				{
					TextBox_SetFocus(username, 1);
					show_login_window(YES, SELECTED_WINDOW_OPACITY);
				}
				else
				{
					TextBox_SetFocus(password, 1);
					show_passwd_window(YES, SELECTED_WINDOW_OPACITY);
				}
			}
		}
	}

	/* Sometimes mouse cursor disappears after a keystroke, we make it visible again */
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
	font_large_height = fdsc.height;
	fdsc.height = screen_width / 40;
	DFBCHECK (dfb->CreateFont (dfb, fontfile, &fdsc, &font_normal));
	font_normal_height = fdsc.height;
	fdsc.height = screen_width / 50;
	DFBCHECK (dfb->CreateFont (dfb, fontfile, &fdsc, &font_small));
	font_small_height = fdsc.height;
}

int framebuffer_mode (int argc, char *argv[], int do_workaround)
{
	int returnstatus = -1;			/* return value of this function...											*/
	DFBResult err;							/* used by that bloody macro to check for errors				*/
	DFBSurfaceDescription sdsc;	/* this will be the description for the primary surface	*/
	DFBInputEvent evt;					/* generic input events will be stored here							*/
	char *lastuser = NULL;

	if (do_workaround != -1) workaround = do_workaround;

	/* Stop GPM if necessary */
	we_stopped_gpm= stop_gpm();

	/* we initialize directfb */
	DFBCHECK (DirectFBInit (&argc, &argv));
	DFBCHECK (DirectFBCreate (&dfb));
	dfb->EnumInputDevices (dfb, enum_input_device, &devices);
	DFBCHECK (dfb->CreateInputEventBuffer (dfb, DICAPS_ALL, DFB_TRUE, &events));
	DFBCHECK (dfb->GetDisplayLayer (dfb, DLID_PRIMARY, &layer));
	layer->SetCooperativeLevel (layer, DLSCL_ADMINISTRATIVE);
	layer->EnableCursor (layer, 1);
	sdsc.flags = DSDESC_CAPS;
	sdsc.caps  = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &primary ));
	primary->GetSize (primary, &screen_width, &screen_height);

	set_font_sizes ();
	Draw_Background_Image();

	/* we load and draw buttons */
	power = load_button (DATADIR "power_normal.png", DATADIR "power_mouseover.png", screen_width, screen_height, layer, primary, dfb, BUTTON_OPACITY );
	reset = load_button (DATADIR "reset_normal.png", DATADIR "reset_mouseover.png", power->xpos - 10, screen_height, layer, primary, dfb, BUTTON_OPACITY );
	power->surface->Clear(power->surface, 0x00, 0x00, 0x00, 0x00);
	reset->surface->Clear(reset->surface, 0x00, 0x00, 0x00, 0x00);
	power->surface->Blit (power->surface, power->normal, NULL, 0, 0);
	reset->surface->Blit (reset->surface, reset->normal, NULL, 0, 0);
	power->surface->Flip(power->surface, NULL, 0);
	reset->surface->Flip(reset->surface, NULL, 0);

	lastuser = get_last_user();

	/* we show windows */
	show_welcome_window(YES);
	if (!!lastuser)
	{
		show_login_window(YES, WINDOW_OPACITY);
		show_passwd_window(YES, SELECTED_WINDOW_OPACITY);
	}
	else
	{
		show_login_window(YES, SELECTED_WINDOW_OPACITY);
		show_passwd_window(YES, WINDOW_OPACITY);
	}
	show_session_window(YES, WINDOW_OPACITY);
	print_session_name(YES, lastuser, WINDOW_OPACITY);

	/* we create textboxes */
	if (!username)
	{
		DFBWindowDescription window_desc;

		window_desc.flags  = ( DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS );
		window_desc.posx   = 3*screen_width/10;
		window_desc.posy   = 3*screen_height/8-font_large_height;
		window_desc.width  = screen_width - window_desc.posx;
		window_desc.height = 2*font_large_height;
		window_desc.caps   = DWCAPS_ALPHACHANNEL;
		username = TextBox_Create(layer, font_large, &window_desc);
		window_desc.posy   = 4*screen_height/8-font_large_height;
		password = TextBox_Create(layer, font_large, &window_desc);
		password->mask_text = 1;
		if (!!lastuser)
		{
			TextBox_SetText(username, lastuser);
			username->hasfocus = 1;
			TextBox_SetFocus(username, 0);
			TextBox_SetFocus(password, 1);
		}
		else TextBox_SetFocus(username, 1);
	}

	if (!!lastuser)
	{
		free(lastuser);
		lastuser = NULL;
	}

	/* we go on for ever... or until the user does something in particular */
	while (returnstatus == -1)
	{
		/* we wait for an input event */
		events->WaitForEventWithTimeout(events, 0, 500);
		if (events->HasEvent(events) == DFB_OK)
		{
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
		else
		{
			static int do_switch = 0;
			if (username->hasfocus)
			{
				TextBox_KeyEvent(username, REDRAW, do_switch);
				do_switch = !do_switch;
			}
			if (password->hasfocus)
			{
				TextBox_KeyEvent(password, REDRAW, do_switch);
				do_switch = !do_switch;
			}
		}
	}

	close_framebuffer_mode ();
	return returnstatus;
}
