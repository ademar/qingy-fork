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
#include "chvt.h"
#include "misc.h"
#include "session.h"

#define POWEROFF 0
#define REBOOT   1
#define CURRENT  0
#define UP   		 1
#define DOWN 	  -1

DeviceInfo *devices = NULL;		/* the list of all input devices									*/
IDirectFBEventBuffer *events;	/* input interfaces: device and its buffer				*/
IDirectFB *dfb;								/* the super interface														*/
IDirectFBSurface *primary;		/* the primary surface (surface of primary layer)	*/
IDirectFBDisplayLayer *layer;	/* the primary layer we use for mouse cursor			*/
struct button *power, *reset;	/* buttons																				*/
int mouse_x, mouse_y;					/* mouse coordinates															*/
IDirectFBSurface *panel_image=NULL;/* background image                          */
int we_stopped_gpm;						/* wether this program stopped gpm or not					*/
IDirectFBWindow	 *window;			/* the window we use for displaying text					*/
IDirectFBSurface *window_surface;	/* surface of the above												*/
DFBWindowDescription window_desc;	/* description of the window above						*/
IDirectFBWindow	 *lock_window = NULL;/* window for displaying CAPS LOCK status  */
IDirectFBSurface *lock_window_surface = NULL;/* surface of the above						*/
IDirectFBWindow	 *button_window = NULL;/* window for displaying buttons         */
IDirectFBSurface *button_window_surface = NULL;/* surface of the above					*/
unsigned int screen_width, screen_height;	/* screen resolution									*/
IDirectFBFont *font_small, *font_normal, *font_large;	/* fonts 									*/
int font_small_height, font_normal_height, font_large_height; /* font sizes     */
int username_hasfocus = 0;		/* if username section has input focus            */
int password_hasfocus = 0;		/* if password section has input focus            */
int session_hasfocus  = 0;		/* if session section has input focus             */
int workaround = -1;

void DrawBase ()
{
	/* width and height of the background image */
	static unsigned int panel_width, panel_height;

	/* We clear the primary surface */
	primary->Clear (primary, 0, 0, 0, 0);
	if (!panel_image)
	{ /* we design the surface */
		panel_image = load_image (DATADIR "background.png", primary, dfb);
		panel_image->GetSize (panel_image, (unsigned int *) &panel_width, (unsigned int *) &panel_height);
	}
	/* we put the backgound image in the center of the screen if it fits
	   the screen otherwise we stretch it to make it fit                    */
	if ( (panel_width <= screen_width) && (panel_height <= screen_height) )
		primary->Blit (primary, panel_image, NULL, (screen_width - panel_width)/2, (screen_height - panel_height)/2);
	else primary->StretchBlit (primary, panel_image, NULL, NULL);

	/* we draw the surface, duplicating it */
	primary->Flip (primary, NULL, DSFLIP_BLIT);

	/* we draw power and reset buttons */
	power->surface->Blit (power->surface, power->normal, NULL, 0, 0);
	power->surface->Flip(power->surface, NULL, 0);
	reset->surface->Blit (reset->surface, reset->normal, NULL, 0, 0);
	reset->surface->Flip(reset->surface, NULL, 0);
}

int print_session_name(int direction)
{
	static struct session *session = NULL;
	static IDirectFBSurface *session_surface = NULL;
	DFBRectangle *session_area;

	if (!session)
	{ /* Get info about available sessions */
		get_sessions();
		session = &sessions;
	}
	if (direction ==  UP ) session= session->prev;
	if (direction == DOWN) session= session->next;

	if (!session_surface)
	{
		session_area = (DFBRectangle *) calloc(1, sizeof(DFBRectangle));
		session_area->x = window_desc.width/5 + font_large_height;
		session_area->y = window_desc.height-window_desc.height/3 - font_large_height;
		session_area->w = window_desc.width - session_area->x;
		session_area->h = font_large_height;
		window_surface->GetSubSurface (window_surface, session_area, &session_surface);
	}

	session_surface->Clear (session_surface, 0x00, 0x00, 0x00, 0xff);
	session_surface->SetColor (session_surface, MASK_TEXT_COLOR);
	session_surface->SetFont  (session_surface, font_large);
	session_surface->DrawString (session_surface, session->name, -1, 0, 0, DSTF_LEFT|DSTF_TOP);
	session_surface->Flip (session_surface, NULL, DSFLIP_BLIT);

	return session->id;
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
	window_surface->Clear (window_surface, 0x00, 0x00, 0x00, 0xff);
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
	print_session_name(CURRENT);
	username_hasfocus = 1;

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
	if (lock_window_surface) lock_window_surface->Release (lock_window_surface);
	if (lock_window) lock_window->Release (lock_window);
	window_surface->Release (window_surface);
	window->Release (window);
	primary->Release (primary);
	events->Release (events);
	layer->Release (layer);
	dfb->Release (dfb);
}

void handle_mouse_movement (void)
{
	static DFBRegion *buttons_area = NULL;

	/* Why redraw all the screen if we need to modify only the buttons area? */
	if (!buttons_area)
	{
		buttons_area = (DFBRegion *) calloc(1, sizeof(DFBRegion));
		buttons_area->x1 = reset->xpos;
		buttons_area->y1 = reset->ypos;
		buttons_area->x2 = screen_width;
		buttons_area->y2 = screen_height;
	}

	/* mouse over power button */
	if ((mouse_x >= power->xpos) && (mouse_x <= (power->xpos + (int) power->width)))
		if ((mouse_y >= power->ypos) && (mouse_y <= (power->ypos + (int) power->height)))
		{
			if (!power->mouse)
			{
				power->mouse = 1;
				power->surface->Blit(power->surface, power->mouseover, NULL, 0, 0);
				power->surface->Flip(power->surface, NULL, 0);
				if (reset->mouse)
				{
					reset->mouse = 0;
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
				reset->surface->Blit(reset->surface, reset->mouseover, NULL, 0, 0);
				reset->surface->Flip(reset->surface, NULL, 0);
				if (power->mouse)
				{
					power->mouse = 0;
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
		power->surface->Blit(power->surface, power->normal, NULL, 0, 0);
		power->surface->Flip(power->surface, NULL, 0);
	}
	if (reset->mouse)
	{
		reset->mouse = 0;
		reset->surface->Blit(reset->surface, reset->normal, NULL, 0, 0);
		reset->surface->Flip(reset->surface, NULL, 0);
	}
}

void show_lock_key_status(DFBInputEvent *evt, int force_draw)
{
	DFBWindowDescription lock_window_desc;
	static int caps_lock_warning = 0;
	DFBResult err;

	if (!lock_window_surface)
	{	/* All the necessary stuff to create a lock_window */
		lock_window_desc.flags  = ( DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS );
		lock_window_desc.posx   = 0;
		lock_window_desc.posy   = screen_height - (font_small_height);
		lock_window_desc.width  = screen_width/5;
		lock_window_desc.height = font_small_height;
		lock_window_desc.caps   = DWCAPS_ALPHACHANNEL;
		DFBCHECK(layer->CreateWindow (layer, &lock_window_desc, &lock_window));
		lock_window->GetSurface( lock_window, &lock_window_surface );
		lock_window_surface->Clear (lock_window_surface, 0x00, 0x00, 0x00, 0xff);
		lock_window_surface->SetFont (lock_window_surface, font_small);
		lock_window_surface->SetColor (lock_window_surface, MASK_TEXT_COLOR);
		lock_window_surface->DrawString (lock_window_surface, "CAPS LOCK is pressed", -1, lock_window_desc.width/2, lock_window_desc.height+1, DSTF_CENTER|DSTF_BOTTOM );
		lock_window_surface->Flip( lock_window_surface, NULL, 0 );
		lock_window->SetOpacity( lock_window, 0x00 );
		lock_window->RaiseToTop( lock_window );
	}

	if (lock_is_pressed(evt) == 3)
		if (!caps_lock_warning || force_draw)
		{
			lock_window->SetOpacity( lock_window, WINDOW_OPACITY );
			caps_lock_warning = 1;
		}
	if (lock_is_pressed(evt) != 3)
		if (caps_lock_warning || force_draw)
		{
			lock_window->SetOpacity( lock_window, 0x00 );
			caps_lock_warning = 0;
		}
}

void reset_screen(DFBInputEvent *evt)
{
	DrawBase ();
	power->mouse = 0;
	reset->mouse = 0;
	power->window->SetOpacity(power->window, WINDOW_OPACITY );
	reset->window->SetOpacity(reset->window, WINDOW_OPACITY );
	layer->GetCursorPosition (layer, &mouse_x, &mouse_y);
	handle_mouse_movement();
	show_lock_key_status(evt, 1);
	window->SetOpacity( window, WINDOW_OPACITY );
}

void begin_shutdown_sequence (int action)
{
	DFBInputEvent evt;
	char message[35];
	int countdown = 5;

	/* We clear the double buffered surface */
	window->SetOpacity( window, 0x00 );
	power->window->SetOpacity(power->window, 0x00 );
	reset->window->SetOpacity(reset->window, 0x00 );
	if (lock_window) lock_window->SetOpacity( lock_window, 0x00 );
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
				{ /* user aborted sequence */
					reset_screen(&evt);
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

int handle_keyboard_input(int *input, char *buffer, int *length)
{

	if (*input == BACKSPACE)
	{
		if ( !(*length) ) return 0;
		(*length)--;
		buffer[*length] = '\0';
		return 1;
	}

	if ( (*input >= 32) && (*input <=255) ) /* ASCII */
	{
		if (*length == MAX) return 0;
		buffer[*length] = *input;
		(*length)++;
		buffer[*length] = '\0';
		return 1;
	}

	/*if (strcmp(input, "CURSOR_LEFT") == 0)*/
	/*if (strcmp(input, "CURSOR_RIGHT") == 0)*/
	return 0;
}

void username_event(int *input, char *output)
{
	static char buffer[MAX];
	static int length = 0;
	static IDirectFBSurface *username_surface = NULL;
	DFBRectangle *username_area;

	if (!length) buffer[0] = '\0';
	if (!username_surface)
	{
		username_area = (DFBRectangle *) calloc(1, sizeof(DFBRectangle));
		username_area->x = window_desc.width/5 + font_large_height;
		username_area->y = window_desc.height/3 - font_large_height;
		username_area->w = window_desc.width - username_area->x;
		username_area->h = font_large_height;
		window_surface->GetSubSurface (window_surface, username_area, &username_surface);
	}

	if (input != NULL)
		if (handle_keyboard_input(input, &(buffer[0]), &length))
		{
			username_surface->Clear (username_surface, 0x00, 0x00, 0x00, 0xff);
			username_surface->SetColor (username_surface, MASK_TEXT_COLOR);
			username_surface->SetFont  (username_surface, font_large);
			username_surface->DrawString (username_surface, buffer, -1, 0, 0, DSTF_LEFT|DSTF_TOP);
			username_surface->Flip (username_surface, NULL, DSFLIP_BLIT);
		}

	if (output != NULL) strcpy(output, buffer);
}

void password_event(int *input, char *output)
{
	static char buffer[MAX];
	static int length = 0;
	static IDirectFBSurface *password_surface = NULL;
	DFBRectangle *password_area;
	char *mask = NULL;
	int i;

	if (!length) buffer[0] = '\0';
	if (!password_surface)
	{
		password_area = (DFBRectangle *) calloc(1, sizeof(DFBRectangle));
		password_area->x = window_desc.width/5 + font_large_height;
		password_area->y = window_desc.height/2 - font_large_height;
		password_area->w = window_desc.width - password_area->x;
		password_area->h = font_large_height;
		window_surface->GetSubSurface (window_surface, password_area, &password_surface);
	}

	if (input != NULL)
		if (handle_keyboard_input(input, &(buffer[0]), &length))
		{
			mask = (char *) calloc(strlen(buffer)+1, sizeof(char));
			for (i=0; i<length; i++) mask[i] = '*';
			mask[length] = '\0';
			password_surface->Clear (password_surface, 0x00, 0x00, 0x00, 0xff);
			password_surface->SetColor (password_surface, MASK_TEXT_COLOR);
			password_surface->SetFont  (password_surface, font_large);
			password_surface->DrawString (password_surface, mask, -1, 0, 0, DSTF_LEFT|DSTF_TOP);
			password_surface->Flip (password_surface, NULL, DSFLIP_BLIT);
			free(mask);
		}

	if (output != NULL) strcpy(output, buffer);
}

void start_login_sequence(DFBInputEvent *evt)
{
	int session_id;
	char message[MAX];

	username_event(NULL, &(username[0]));
	if (strlen(username) == 0) return;
	password_event(NULL, &(password[0]));
	session_id = print_session_name(CURRENT);
	strncpy(message, "Logging in ", MAX);
	strncat(message, username, MAX-strlen(message));
	strncat(message, "...", MAX-strlen(message));

	window->SetOpacity( window, 0x00 );
	if (lock_window) lock_window->SetOpacity( lock_window, 0x00 );
	power->window->SetOpacity(power->window, 0x00 );
	reset->window->SetOpacity(reset->window, 0x00 );
	primary->Clear (primary, 0, 0, 0, 0);
	primary->SetFont (primary, font_large);
	primary->SetColor (primary, OTHER_TEXT_COLOR);
	primary->DrawString (primary, message, -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
	primary->Flip (primary, NULL, DSFLIP_BLIT);
	sleep(1);

	if (!check_password())
	{
		primary->Clear (primary, 0, 0, 0, 0);
		primary->DrawString (primary, "Login failed!", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
		primary->Flip (primary, NULL, DSFLIP_BLIT);
		sleep(1);
		reset_screen(evt);
		return;
	}
	primary->Clear (primary, 0, 0, 0, 0);
	primary->DrawString (primary, "Starting selected session...", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
	primary->Flip (primary, NULL, DSFLIP_BLIT);
	sleep(1);
	close_framebuffer_mode();
	if (we_stopped_gpm) start_gpm();
	start_session(session_id, workaround);
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

	fprintf(stderr, "U pressed '%d', and that is '%c'\n", ascii_code, ascii_code);
	show_lock_key_status(evt, 0);
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
		if (username_hasfocus && allow_tabbing)
		{
			if (ascii_code == TAB)
			{
				username_hasfocus = 0;
				password_hasfocus = 1;
				allow_tabbing = 0;
			}
			else username_event(&ascii_code, NULL);
		}

		/* password events */
		if (password_hasfocus && allow_tabbing)
		{
			if (ascii_code == TAB)
			{
				password_hasfocus = 0;
				session_hasfocus  = 1;
				allow_tabbing = 0;
			}
			else password_event(&ascii_code, NULL);
		}

		/* session events */
		if (session_hasfocus && allow_tabbing)
		{
			if (strcmp(symbol_name->name, "CURSOR_UP"  ) == 0) print_session_name(UP);
			if (strcmp(symbol_name->name, "CURSOR_DOWN") == 0) print_session_name(DOWN);
			if (ascii_code == TAB)
			{
				session_hasfocus  = 0;
				username_hasfocus = 1;
				allow_tabbing = 0;
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

	if (do_workaround != -1) workaround = do_workaround;

	/* Stop GPM if necessary */
	we_stopped_gpm= stop_gpm();

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
	mouse_x = screen_width  / 2;
	mouse_y = screen_height / 2;
	set_font_sizes ();
	power = load_button (DATADIR "power_normal.png", DATADIR "power_mouseover.png", screen_width, screen_height, layer, primary, dfb, WINDOW_OPACITY );
	reset = load_button (DATADIR "reset_normal.png", DATADIR "reset_mouseover.png", power->xpos - 10, screen_height, layer, primary, dfb, WINDOW_OPACITY );
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
