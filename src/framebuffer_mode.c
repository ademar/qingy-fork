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
#include "directfb_button.h"
#include "directfb_utils.h"
#include "directfb_textbox.h"
#include "directfb_combobox.h"
#include "directfb_label.h"
#include "chvt.h"
#include "misc.h"
#include "session.h"
#include "load_settings.h"

#define POWEROFF 0
#define REBOOT   1

DeviceInfo *devices = NULL;   /* the list of all input devices             */
IDirectFBEventBuffer *events; /* input interfaces: device and its buffer   */
IDirectFB *dfb;               /* the super interface                       */
IDirectFBSurface *primary;    /* surface of the primary layer              */
IDirectFBDisplayLayer *layer; /* the primary layer                         */
Button *power, *reset;        /* buttons                                   */
int we_stopped_gpm;           /* wether this program stopped gpm or not    */
IDirectFBSurface *panel_image=NULL; /* background image                    */
int screen_width, screen_height; /* screen resolution                      */
IDirectFBFont *font_small, *font_normal, *font_large; /* fonts             */
int font_small_height, font_normal_height, font_large_height;/* font sizes */
Label *welcome_label = NULL, *username_label = NULL; /* text labels        */
Label *password_label = NULL, *session_label = NULL; /* more text labels   */
Label *lock_key_status = NULL;/* yet more text labels                      */
TextBox *username = NULL, *password = NULL; /* text boxes                  */
ComboBox *session = NULL;     /* combo boxes                               */
int username_area_mouse = 0;  /* sensible area for mouse cursor to be in   */
int password_area_mouse = 0;  /* sensible area for mouse cursor to be in   */
int session_area_mouse = 0;   /* sensible area for mouse cursor to be in   */
int workaround = -1;          /* do you black tty workaround? I do         */

void Draw_Background_Image()
{
	/* width and height of the background image */
	static int panel_width, panel_height;

	/* We clear the primary surface */
	primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
	if (!panel_image)
	{ /* we design the surface */
		if (BACKGROUND) panel_image = load_image (BACKGROUND, primary, dfb);
		if (panel_image != NULL) panel_image->GetSize (panel_image, &panel_width, &panel_height);
	}
	/* we put the backgound image in the center of the screen if it fits
	   the screen otherwise we stretch it to make it fit                    */
	if (panel_image)
	{
		if ( (panel_width <= screen_width) && (panel_height <= screen_height) )
			primary->Blit (primary, panel_image, NULL, (screen_width - panel_width)/2, (screen_height - panel_height)/2);
		else primary->StretchBlit (primary, panel_image, NULL, NULL);
	}

	/* we draw the surface, duplicating it */
	primary->Flip (primary, NULL, DSFLIP_BLIT);
}

/* plot previous session of <user> if it exists,
   otherwise plot default vaule                  */
void set_user_session(char *user)
{
	item *temp;
	char *user_session = get_last_session(user);
	if (!session || !session->items) return;

	if (!user_session)
	{
		temp = session->selected;
		while (strcmp(session->selected->name, "Text Console") != 0)
			session->selected = session->selected->next;
		if (session->selected != temp) session->KeyEvent(session, REDRAW);
	}
	else
	{
		temp = session->items;
		while (1)
		{
			if (strcmp(user_session, temp->name) == 0)
			{
				session->selected = temp;
				session->KeyEvent(session, REDRAW);
				return;
			}
			temp = temp->next;
			if (temp == session->items) break;
		}
	}
}

void close_framebuffer_mode (void)
{
	if (power) power->Destroy(power);
	if (reset) reset->Destroy(reset);
	if (panel_image) panel_image->Release (panel_image);
	welcome_label->Destroy(welcome_label);
	lock_key_status->Destroy(lock_key_status);
	username_label->Destroy(username_label);
	username->Destroy(username);
	password_label->Destroy(password_label);
	password->Destroy(password);
	session_label->Destroy(session_label);
	session->Destroy(session);
	font_small->Release (font_small);
	font_normal->Release (font_normal);
	font_large->Release (font_large);
	primary->Release (primary);
	events->Release (events);
	layer->Release (layer);
	while (devices)
	{
		DeviceInfo *next = devices->next;
		free (devices);
		devices = next;
	}
	dfb->Release (dfb);
	if (we_stopped_gpm) start_gpm();
}

/* mouse movement in buttons area */
void handle_buttons(int *mouse_x, int *mouse_y)
{
	/* mouse over power button */
	if ((*mouse_x >= power->xpos) && (*mouse_x <= (power->xpos + (int) power->width)))
		if ((*mouse_y >= power->ypos) && (*mouse_y <= (power->ypos + (int) power->height)))
		{
			if (!power->mouse)
			{
				power->MouseOver(power, 1);
				if (reset->mouse) reset->MouseOver(reset, 0);
				return;
			}
			else return;		/* we already plotted this event */
		}

	/* mouse over reset button */
	if ((*mouse_x >= reset->xpos) && (*mouse_x <= (reset->xpos + (int) reset->width)))
		if ((*mouse_y >= reset->ypos) && (*mouse_y <= (reset->ypos + (int) reset->height)))
		{
			if (!reset->mouse)
			{
				reset->MouseOver(reset, 1);
				if (power->mouse) power->MouseOver(power, 0);
				return;
			}
			else return;		/* we already plotted this event */
		}

	/* mouse not over any power button */
	if (power->mouse) power->MouseOver(power, 0);
	if (reset->mouse) reset->MouseOver(reset, 0);
}

/* mouse movement in textboxes and comboboxes area */
void handle_text_combo_boxes(int *mouse_x, int *mouse_y)
{
	/* mouse over username area */
	if ( (*mouse_x >= (int) username->xpos) && (*mouse_x <= (int) username->xpos + (int) username->width) )
		if ( (*mouse_y >= (int) username->ypos) && (*mouse_y <= (int) username->ypos + (int) username->height) )
		{
			username_area_mouse = 1;
			return;
		}

	/* mouse over password area */
	if ( (*mouse_x >= (int) password->xpos) && (*mouse_x <= (int) password->xpos + (int) password->width) )
		if ( (*mouse_y >= (int) password->ypos) && (*mouse_y <= (int) password->ypos + (int) password->height) )
		{
			password_area_mouse = 1;
			return;
		}

	/* mouse over session area */
	if ( (*mouse_x >= (int) session->xpos) && (*mouse_x <= (int) session->xpos + (int) session->width) )
		if ( (*mouse_y >= (int) session->ypos) && (*mouse_y <= (int) session->ypos + (int) session->height) )
		{
			session_area_mouse = 1;
			return;
		}
}

/* mouse movement in labels area */
void handle_labels(int *mouse_x, int *mouse_y)
{
	/* mouse over username area */
	if ( (*mouse_x >= (int) username_label->xpos) && (*mouse_x <= (int) username_label->xpos + (int) username_label->width) )
		if ( (*mouse_y >= (int) username_label->ypos) && (*mouse_y <= (int) username_label->ypos + (int) username_label->height) )
		{
			username_area_mouse = 1;
			return;
		}

	/* mouse over password area */
	if ( (*mouse_x >= (int) password_label->xpos) && (*mouse_x <= (int) password_label->xpos + (int) password_label->width) )
		if ( (*mouse_y >= (int) password_label->ypos) && (*mouse_y <= (int) password_label->ypos + (int) password_label->height) )
		{
			password_area_mouse = 1;
			return;
		}

	/* mouse over session area */
	if ( (*mouse_x >= (int) session_label->xpos) && (*mouse_x <= (int) session_label->xpos + (int) session_label->width) )
		if ( (*mouse_y >= (int) session_label->ypos) && (*mouse_y <= (int) session_label->ypos + (int) session_label->height) )
		{
			session_area_mouse = 1;
			return;
		}
}

void handle_mouse_movement (void)
{
	int mouse_x, mouse_y;

	username_area_mouse = 0;
	password_area_mouse = 0;
	session_area_mouse = 0;

	layer->GetCursorPosition (layer, &mouse_x, &mouse_y);
	handle_buttons(&mouse_x, &mouse_y);
	handle_text_combo_boxes(&mouse_x, &mouse_y);
	handle_labels(&mouse_x, &mouse_y);
}

void show_lock_key_status(DFBInputEvent *evt)
{
	if (lock_is_pressed(evt) == 3) lock_key_status->Show(lock_key_status);
	else lock_key_status->Hide(lock_key_status);
}

void reset_screen(DFBInputEvent *evt)
{
	Draw_Background_Image ();
	power->Show(power);
	reset->Show(reset);
	welcome_label->Show(welcome_label);
	username_label->Show(username_label);
	password_label->Show(password_label);
	session_label->Show(session_label);
	username->Show(username);
	password->Show(password);
	session->Show(session);
	show_lock_key_status(evt);
	handle_mouse_movement();
	layer->EnableCursor (layer, 1);
}

void clear_screen(void)
{
	power->Hide(power);
	reset->Hide(reset);
	welcome_label->Hide(welcome_label);
	username->Hide(username);
	password->Hide(password);
	username_label->Hide(username_label);
	password_label->Hide(password_label);
	session_label->Hide(session_label);
	lock_key_status->Hide(lock_key_status);
	session->Hide(session);
	layer->EnableCursor (layer, 0);
	primary->Clear (primary, 0, 0, 0, 0);
	primary->Flip (primary, NULL, DSFLIP_BLIT);
	primary->SetFont (primary, font_large);
	primary->SetColor (primary, OTHER_TEXT_COLOR_R, OTHER_TEXT_COLOR_G, OTHER_TEXT_COLOR_B, OTHER_TEXT_COLOR_A);
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
			   we check wether mouse pointer is over a specific area */
			if (power->mouse) status = 1;
			if (reset->mouse) status = 2;
			if (username_area_mouse) status = 3;
			if (password_area_mouse) status = 4;
			if (session_area_mouse) status = 5;
		}
		else
		{	/* left mouse button is up:
			   if it was on a specific area when down we check if it is still there */
			if ((power->mouse) && (status == 1))	/* power button has been clicked! */
				begin_shutdown_sequence (POWEROFF);
			if ((reset->mouse) && (status == 2))	/* reset button has been clicked! */
				begin_shutdown_sequence (REBOOT);
			if (username_area_mouse && status == 3)
			{	/* username area has been clicked! */
				username->SetFocus(username, 1);
				username_label->SetFocus(username_label, 1);
				password->SetFocus(password, 0);
				password_label->SetFocus(password_label, 0);
				session->SetFocus(session, 0);
				session_label->SetFocus(session_label, 0);
			}
			if (password_area_mouse && status == 4)
			{	/* password area has been clicked! */
				username->SetFocus(username, 0);
				username_label->SetFocus(username_label, 0);
				password->SetFocus(password, 1);
				password_label->SetFocus(password_label, 1);
				session->SetFocus(session, 0);
				session_label->SetFocus(session_label, 0);
			}
			if (session_area_mouse && status == 5)
			{	/* session area has been clicked! */
				username->SetFocus(username, 0);
				username_label->SetFocus(username_label, 0);
				password->SetFocus(password, 0);
				password_label->SetFocus(password_label, 0);
				session->SetFocus(session, 1);
				session_label->SetFocus(session_label, 1);
			}
			status = 0;		/* we reset click status because button went up */
		}
	}
}

void start_login_sequence(DFBInputEvent *evt)
{
	char message[MAX];
	char *user_name;
	char *user_session;

	if (strlen(username->text) == 0) return;
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
		password->ClearText(password);
		reset_screen(evt);
		return;
	}
	primary->Clear (primary, 0, 0, 0, 0);
	primary->DrawString (primary, "Starting selected session...", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
	primary->Flip (primary, NULL, DSFLIP_BLIT);
	sleep(1);
	user_name = (char *) calloc(strlen(username->text)+1, sizeof(char));
	strcpy(user_name, username->text);
	user_session = (char *) calloc(strlen(session->selected->name)+1, sizeof(char));
	strcpy(user_session, session->selected->name);
	close_framebuffer_mode();
	start_session(user_name, user_session, workaround);

	/* The above never returns, so... */
	free(user_name); user_name = NULL;
	free(user_session); user_session = NULL;
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
	int modifier = 0;
	int ascii_code = (int) evt->key_symbol;

	show_lock_key_status(evt);
	/* If user presses ESC two times, we go back to text mode */
	if ((last_symbol == ESCAPE) && (ascii_code == ESCAPE)) return TEXT_MODE;
	/* We store the previous keystroke */
	last_symbol = ascii_code;
	symbol_name = bsearch (&(evt->key_symbol), keynames, sizeof (keynames) / sizeof (keynames[0]) - 1, sizeof (keynames[0]), compare_symbol);
	modifier = modifier_is_pressed(evt);
	if (modifier)
	{
		if (modifier == ALT)
		{	/* we check if user press ALT-p or ALT-r to start shutdown/reboot sequence */
			if ((ascii_code == 'P')||(ascii_code == 'p')) begin_shutdown_sequence (POWEROFF);
			if ((ascii_code == 'R')||(ascii_code == 'r')) begin_shutdown_sequence (REBOOT);
		}
		if (modifier == ALT || modifier == CTRLALT)
		{ /* we check if the user is pressing [CTRL-]ALT-number with 1 <= number <= 12
			   if so we close directfb mode and send him to that tty              */
			if (symbol_name)
				if ((strlen (symbol_name->name) <= 3) && (strncmp (symbol_name->name, "F", 1) == 0))
				{
					temp = atoi (symbol_name->name + 1);
					if ((temp > 0) && (temp < 13))
						if (get_active_tty () != temp)
							return temp;
				}
			return returnstatus;
		}
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
				username_label->SetFocus(username_label, 0);
				username->SetFocus(username, 0);
				if (modifier_is_pressed(evt) != SHIFT)
				{
					password_label->SetFocus(password_label, 1);
					password->SetFocus(password, 1);
				}
				else
				{
					session_label->SetFocus(session_label, 1);
					session->SetFocus(session, 1);
				}
			}
			else
			{
				username->KeyEvent(username, ascii_code, 1);
				set_user_session(username->text);
			}
		}

		/* password events */
		if (password->hasfocus && allow_tabbing)
		{
			if (ascii_code == TAB)
			{
				allow_tabbing = 0;
				password_label->SetFocus(password_label, 0);
				password->SetFocus(password, 0);
				if (modifier_is_pressed(evt) != SHIFT)
				{
					session_label->SetFocus(session_label, 1);
					session->SetFocus(session, 1);
				}
				else
				{
					username_label->SetFocus(username_label, 1);
					username->SetFocus(username, 1);
				}
			}
			else password->KeyEvent(password, ascii_code, 1);
		}

		/* session events */
		if (session->hasfocus && allow_tabbing)
		{
			if (ascii_code == ARROW_UP) session->KeyEvent(session, UP);
			if (ascii_code == ARROW_DOWN) session->KeyEvent(session, DOWN);
			if (ascii_code == TAB)
			{
				allow_tabbing = 0;
				session_label->SetFocus(session_label, 0);
				session->SetFocus(session, 0);
				if (modifier_is_pressed(evt) != SHIFT)
				{
					username_label->SetFocus(username_label, 1);
					username->SetFocus(username, 1);
				}
				else
				{
					password_label->SetFocus(password_label, 1);
					password->SetFocus(password, 1);
				}
			}
		}
	}

	return returnstatus;
}

void Create_Labels_TextBoxes_ComboBoxes(void)
{
	DFBWindowDescription window_desc;

	window_desc.flags  = ( DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS );
	window_desc.posx   = 0;
	window_desc.posy   = screen_height/8;
	window_desc.width  = screen_width;
	window_desc.height = 2*font_large_height;
	window_desc.caps   = DWCAPS_ALPHACHANNEL;
	welcome_label = Label_Create(layer, font_large, &window_desc);
	welcome_label->SetText(welcome_label, print_welcome_message("Welcome to ", NULL), CENTER);
	window_desc.posx = screen_width/8;
	window_desc.posy   = 3*screen_height/8-font_large_height;
	window_desc.width  = 3*screen_width/20;
	username_label = Label_Create(layer, font_large, &window_desc);
	username_label->SetText(username_label, "login:", RIGHT);
	window_desc.posy   = 4*screen_height/8-font_large_height;
	password_label = Label_Create(layer, font_large, &window_desc);
	password_label->SetText(password_label, "passwd:", RIGHT);
	window_desc.posy   = 5*screen_height/8-font_large_height;
	session_label = Label_Create(layer, font_large, &window_desc);
	session_label->SetText(session_label, "session:", RIGHT);
	window_desc.posx   = 3*screen_width/10;
	window_desc.posy   = 3*screen_height/8-font_large_height;
	window_desc.width  = screen_width - window_desc.posx;
	window_desc.height = 2*font_large_height;
	username = TextBox_Create(layer, font_large, &window_desc);
	window_desc.posy   = 4*screen_height/8-font_large_height;
	password = TextBox_Create(layer, font_large, &window_desc);
	window_desc.posy = 5*screen_height/8-font_large_height;
	session = ComboBox_Create(layer, font_large, &window_desc);
	window_desc.posx   = 0;
	window_desc.posy   = screen_height - (font_small_height);
	window_desc.width  = screen_width/5;
	window_desc.height = font_small_height;
	lock_key_status = Label_Create(layer, font_small, &window_desc);
	lock_key_status->SetText(lock_key_status, "CAPS LOCK is pressed", CENTERBOTTOM);
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
	int returnstatus = -1;      /* return value of this function...             */
	DFBResult err;              /* the bloody macro uses it to check for errors */
	DFBSurfaceDescription sdsc; /* description for the primary surface          */
	DFBInputEvent evt;          /* generic input events will be stored here     */
	char *lastuser = get_last_user(); /* last user logged in                    */

	if (do_workaround != -1) workaround = do_workaround;

	/* Stop GPM if necessary */
	we_stopped_gpm= stop_gpm();

	/* load settings from file */
	load_settings();

	/* we initialize directfb */
	stderr_disable();
	DFBCHECK (DirectFBInit (&argc, &argv));
	stderr_enable();

	DFBCHECK (DirectFBCreate (&dfb));
	dfb->EnumInputDevices (dfb, enum_input_device, &devices);
	DFBCHECK (dfb->CreateInputEventBuffer (dfb, DICAPS_ALL, DFB_TRUE, &events));
	DFBCHECK (dfb->GetDisplayLayer (dfb, DLID_PRIMARY, &layer));
	layer->SetCooperativeLevel (layer, DLSCL_ADMINISTRATIVE);
	layer->EnableCursor (layer, 0);
	sdsc.flags = DSDESC_CAPS;
	sdsc.caps  = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &primary ));
	primary->GetSize (primary, &screen_width, &screen_height);

	set_font_sizes ();
	Draw_Background_Image();

	/* we create buttons */
	power = Button_Create(stringCombine(THEME_DIR, "power_normal.png"), stringCombine(THEME_DIR, "power_mouseover.png"), screen_width, screen_height, layer, primary, dfb);
	reset = Button_Create(stringCombine(THEME_DIR, "reset_normal.png"), stringCombine(THEME_DIR, "reset_mouseover.png"), power->xpos - 10, screen_height, layer, primary, dfb);
	power->MouseOver(power, 0);
	reset->MouseOver(reset, 0);

	/* we create labels, textboxes and comboboxes */
	Create_Labels_TextBoxes_ComboBoxes();
	password->mask_text = 1;
	get_sessions(session);
	if (lastuser)
	{
		username_label->SetFocus(username_label, 0);
		password_label->SetFocus(password_label, 1);
		username->SetText(username, lastuser);
		username->SetFocus(username, 0);
		password->SetFocus(password, 1);
		set_user_session(lastuser);
		free(lastuser);
		lastuser = NULL;
	}
	else
	{
		username_label->SetFocus(username_label, 1);
		password_label->SetFocus(password_label, 0);
		username->SetFocus(username, 1);
	}
	welcome_label->SetFocus(welcome_label, 0);
	session_label->SetFocus(session_label, 0);
	session->SetFocus(session, 0);

	layer->EnableCursor (layer, 1);
	/* we go on for ever... or until the user does something in particular */
	while (returnstatus == -1)
	{
		/* we wait for an input event... */
		events->WaitForEventWithTimeout(events, 0, 500);
		if (events->HasEvent(events) == DFB_OK)
		{ /* ...got that! */
			events->GetEvent (events, DFB_EVENT (&evt));
			if ((evt.type == DIET_AXISMOTION) || (evt.type == DIET_BUTTONPRESS) || (evt.type == DIET_BUTTONRELEASE))
			{	/* handle mouse */
				handle_mouse_event (&evt);
			}
			if (evt.type == DIET_KEYPRESS)
			{	/* manage keystrokes */
				returnstatus= handle_keyboard_event(&evt);
			}
		}
		else
		{ /* Let there be a flashing cursor! */
			static int flashing_cursor = 0;
			if (username->hasfocus)
				username->KeyEvent(username, REDRAW, flashing_cursor);
			if (password->hasfocus)
				password->KeyEvent(password, REDRAW, flashing_cursor);
			flashing_cursor = !flashing_cursor;
		}
	}

	close_framebuffer_mode ();
	return returnstatus;
}
