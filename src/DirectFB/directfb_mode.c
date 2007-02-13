/***************************************************************************
                      directfb_mode.c  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003-2006 by Noberasco Michele
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

/* system and DirectFB library stuff */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <directfb.h>
#include <directfb_keynames.h>
#include <signal.h>
#include <pthread.h>


#ifdef WANT_CRYPTO
#include "crypto.h"
#endif

/* misc stuff */
#include "memmgmt.h"
#include "vt.h"
#include "misc.h"
#include "session.h"
#include "load_settings.h"
#include "logger.h"
#include "keybindings.h"

/* my custom DirectFB stuff */
#include "utils.h"
#include "button.h"
#include "textbox.h"
#include "combobox.h"
#include "label.h"
#include "pm.h"


/* some super-structures */
typedef struct label_t
{
  Label *label;
  struct label_t *next;
} Label_list;
typedef struct button_t
{
  Button *button;
  struct button_t *next;
} Button_list;

#undef  exit
#define exit safe_exit


/*
 * Global variables... definitely too many!
 */

Label_list            *Labels             = NULL; /* This will contain all labels              */
Button_list           *Buttons            = NULL; /* This will contain all text boxes          */
IDirectFB             *dfb;                       /* the super interface                       */
IDirectFBDisplayLayer *layer;                     /* the primary layer                         */
IDirectFBSurface      *primary;                   /* surface of the primary layer              */
IDirectFBSurface      *panel_image        = NULL; /* background image                          */
IDirectFBSurface      *curs_surface       = NULL; /* main cursor shape                         */
IDirectFBEventBuffer  *events;                    /* all input events will be stored here      */
DeviceInfo            *devices            = NULL; /* the list of all input devices             */
IDirectFBFont         *font_tiny;                 /* fonts                                     */
IDirectFBFont         *font_smaller;  
IDirectFBFont         *font_small;
IDirectFBFont         *font_normal;
IDirectFBFont         *font_large;
TextBox               *username           = NULL; /* text boxes                                */
TextBox               *password           = NULL;
Label                 *username_label     = NULL; /* labels                                    */
Label                 *password_label     = NULL;
Label                 *session_label      = NULL;
Label                 *lock_key_statusA   = NULL;
Label                 *lock_key_statusB   = NULL;
Label                 *lock_key_statusC   = NULL;
Label                 *lock_key_statusD   = NULL;
ComboBox              *session            = NULL; /* combo boxes */
int                   screen_width;               /* screen resolution */
int                   screen_height;
int                   font_tiny_height;           /* font sizes */
int                   font_smaller_height;
int                   font_small_height;
int                   font_normal_height;
int                   font_large_height;
#ifdef USE_SCREEN_SAVERS
int                   screensaver_active    = 0; /* screensaver stuff */
int                   screensaver_countdown = 0;
#endif
float                 x_ratio               = 1; /* theme res. should be corrected by x_ratio */
float                 y_ratio               = 1; /* and y_ratio to match the actual res. */
int                   ppid                  = 0; /* process id of out parent */
int                   thread_action         = 0; /* whether one of our threads is doing anything significant */
pthread_mutex_t       lock_act              = PTHREAD_MUTEX_INITIALIZER; /* action lock */


void safe_exit(int exitstatus)
{
#undef exit

  unlock_tty_switching();
  exit(exitstatus);

#define exit safe_exit
}

void Draw_Background_Image(int do_the_drawing)
{
  /* width and height of the background image */
  static int panel_width, panel_height;
  /* We clear the primary surface */
  primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
  if (!panel_image)
	{ /* we design the surface */
		if (background)  panel_image = load_image (background, dfb, x_ratio, y_ratio);
		if (panel_image) panel_image->GetSize (panel_image, &panel_width, &panel_height);
	}
  /*	
   * we put the backgound image in the center of the screen if it fits
   * the screen otherwise we stretch it to make it fit
   */
  if (panel_image)
	{
		if ( (panel_width <= screen_width) && (panel_height <= screen_height) )
			primary->Blit (primary, panel_image, NULL, (screen_width - panel_width)/2, (screen_height - panel_height)/2);
		else primary->StretchBlit (primary, panel_image, NULL, NULL);
	}
  
  /* we draw the surface, duplicating it */
  if (do_the_drawing) primary->Flip (primary, NULL, DSFLIP_BLIT);
}

/*
 * plot previous session of <user> if it exists,
 * otherwise plot default vaule
 */
void set_user_session(char *user)
{
  char *user_session = NULL;
	int   i            = 0;
  
  if (!session || !session->items)
		return;

	user_session = get_last_session(user);
  
  if (user_session)
	{
		for (; i<session->n_items; i++)
			if (!strcmp(session->items[i], user_session))
			{
				if (session->items[i] != session->selected)
				{
					session->SelectItem(session, session->items[i]);
					free(user_session);
					return;
				}
				else return;
			}

		free(user_session);
	}
		
	for (i=0; i<session->n_items; i++)
		if (!strcmp(session->items[i], "Text: Console"))
			if (session->items[i] != session->selected)
				session->SelectItem(session, session->items[i]);
}

void close_framebuffer_mode (int exit_status)
{
	/* write our exit status */
	fprintf(stdout, "%d\n", exit_status);
	fflush(stdout);
	kill (ppid, SIGUSR2);

	/* since we are shutting down, there is no point in deallocating all stuff nicely, as
	 * it is only a waste of time and CPU cycles.
	 */

	writelog(DEBUG,"Starting GUI shutdown...\n");

	/* disable bogus error messages on DirectFB exit */
  stderr_disable();
	if (dfb) dfb->Release (dfb);
/* 	stderr_enable(&current_tty); */

	unlock_tty_switching();

	writelog(DEBUG,"GUI shutdown complete\n");
}

void DirectFB_Error()
{
  writelog(ERROR, "Unrecoverable DirectFB error: reverting to text mode!\n"); /* dammit! */
  writelog(ERROR, "This usually means that you have an issue with DirectFB, not with qingy...\n");
  writelog(ERROR, "Most likely you don't have your console framebuffer properly set up.\n");
  writelog(ERROR, "Please turn on DEBUG log level in qingy to see exactly why DirectFB is letting you down.\n");
	close_framebuffer_mode(GUI_FAILURE);
}

void show_lock_key_status(DFBInputEvent *evt)
{
	static int status = 0;

	if (evt)
		status = (lock_is_pressed(evt) == CAPSLOCK) ? 1 : 0;

  if (status)
	{ /* CAPS lock is active */
		lock_key_statusA->Show(lock_key_statusA);
		lock_key_statusB->Show(lock_key_statusB);
		lock_key_statusC->Show(lock_key_statusC);
		lock_key_statusD->Show(lock_key_statusD);
	}
  else
	{ /* CAPS lock is not active */
		lock_key_statusA->Hide(lock_key_statusA);
		lock_key_statusB->Hide(lock_key_statusB);
		lock_key_statusC->Hide(lock_key_statusC);
		lock_key_statusD->Hide(lock_key_statusD);
	}
}

/* this redraws the login screen */
void reset_screen(DFBInputEvent *evt)
{
  Label_list  *labels  = Labels;
  Button_list *buttons = Buttons;
  
  Draw_Background_Image (1);
  
  /* redraw all labels */
  while (labels)
	{
		labels->label->Show(labels->label);
		labels = labels->next;
	}
 
	/* redraw all buttons */
  while (buttons)
	{
		buttons->button->Show(buttons->button);
		buttons = buttons->next;
	}
  
  username->Show(username);
  password->Show(password);
  session->Show(session);

	if (evt->type == DIET_KEYPRESS || evt->type == DIET_KEYRELEASE)
		show_lock_key_status(evt);
	else
		show_lock_key_status(NULL);

	if (!cursor)
		layer->EnableCursor (layer, 1);
	else
		layer->EnableCursor (layer, cursor->enable);
}

/* this clears the screen */
void clear_screen(void)
{
  Label_list  *labels  = Labels;
  Button_list *buttons = Buttons;
  
  /* hide all labels */
  while (labels)
	{
		labels->label->Hide(labels->label);
		labels = labels->next;
	}

  /* hide all buttons */
  while (buttons)
	{
		buttons->button->Hide(buttons->button);
		buttons = buttons->next;
	}
  
  username->Hide(username);
  password->Hide(password);
  lock_key_statusA->Hide(lock_key_statusA);
  lock_key_statusB->Hide(lock_key_statusB);
  lock_key_statusC->Hide(lock_key_statusC);
  lock_key_statusD->Hide(lock_key_statusD);
  session->Hide(session);
  layer->EnableCursor (layer, 0);
  primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
	if (!clear_background) Draw_Background_Image(1);
	else primary->Flip (primary, NULL, DSFLIP_BLIT);
	primary->SetFont (primary, font_large);
  primary->SetColor (primary, other_text_color.R, other_text_color.G, other_text_color.B, other_text_color.A);
}

void begin_shutdown_sequence (actions action, IDirectFBEventBuffer *events)
{
  DFBInputEvent evt;
  char message[35];
  int countdown = 5;
  char *temp;

  clear_screen();
  
  /* First, we check shutdown policy */
  switch (shutdown_policy)
	{
    case NOONE: /* no one is allowed to shut down the system */
			if (action == DO_SLEEP)
				primary->DrawString (primary, "Putting this machine in sleep mode is not allowed!", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
			else
				primary->DrawString (primary, "Shutting down this machine is not allowed!", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
      primary->Flip (primary, NULL, 0);
      sleep(2);
      events->GetEvent(events, DFB_EVENT (&evt));
      reset_screen(&evt);
      return;
    case ROOT: /* only root can shutdown the system */
      if (!gui_check_password("root", password->text, session->selected, ppid))
			{
				if (action == DO_SLEEP)
					primary->DrawString (primary, "You must enter root password to put this machine to sleep!", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
				else
					primary->DrawString (primary, "You must enter root password to shut down this machine!", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
				primary->Flip (primary, NULL, 0);
				sleep(2);
				events->GetEvent(events, DFB_EVENT (&evt));
				reset_screen(&evt);
				return;
			}
      break;
    case EVERYONE: /* everyone can shutdown, so we do nothing here */
      break;
	}
  
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
			case DO_POWEROFF:
				strcat (message, "shutdown");
				break;
			case DO_REBOOT:
				strcat (message, "restart");
				break;
			case DO_SLEEP:
				strcat (message, "will fall asleep");
				break;
			default: /* other actions do not concern us */
				break;
		}
		primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
		if (!clear_background) Draw_Background_Image(0);
		strcat (message, " in ");
		temp = int_to_str (countdown);
		strcat (message, temp);
		free(temp);
		strcat (message, " seconds");
		primary->DrawString (primary, "Press ESC key to abort", -1, 0, screen_height, DSTF_LEFT | DSTF_BOTTOM);
		primary->DrawString (primary, message, -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
		primary->Flip (primary, NULL, 0);
		sleep (1);
		countdown--;
	}
  if (no_shutdown_screen || (action == DO_SLEEP))
	{
		if (action == DO_POWEROFF) close_framebuffer_mode (EXIT_SHUTDOWN_H);
		if (action == DO_REBOOT)   close_framebuffer_mode (EXIT_SHUTDOWN_R);
		if (action == DO_SLEEP)    close_framebuffer_mode (EXIT_SLEEP);
		exit(EXIT_SUCCESS);
	}
  else
	{
		primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
		Draw_Background_Image(0);
	}
  if (action == DO_POWEROFF)
	{
		if (!no_shutdown_screen)
		{
			primary->DrawString (primary, "shutting down system...", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
			primary->Flip (primary, NULL, 0);
		}
		execl ("/sbin/shutdown", "/sbin/shutdown", "-h", "now", (char*)NULL);
	}
  if (action == DO_REBOOT)
	{
		if (!no_shutdown_screen)
		{
			primary->DrawString (primary, "rebooting system...", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
			primary->Flip (primary, NULL, 0);
		}
		execl ("/sbin/shutdown", "/sbin/shutdown", "-r", "now", (char*)NULL);
	}
  
  /* we should never get here unless call to /sbin/shutdown fails */
  writelog(ERROR, "Unable to exec \"/sbin/shutdown\"!\n");
  if (!no_shutdown_screen) close_framebuffer_mode (GUI_FAILURE);
  exit (GUI_FAILURE);
}

void do_ctrl_alt_del(DFBInputEvent *evt)
{
  char *action = parse_inittab_file();
  
  if (!action) return;	
  if (!strcmp(action, "poweroff")) {free(action); begin_shutdown_sequence (DO_POWEROFF, events); return;}
  if (!strcmp(action, "reboot"))   {free(action); begin_shutdown_sequence (DO_REBOOT,   events); return;}
  
  /* we display a message */
  clear_screen();
  primary->DrawString (primary, action, -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
  primary->Flip (primary, NULL, 0);
  sleep(2);
  reset_screen(evt);
  free(action);
}

/* callback function to handle label clicks */
void label_click(Label *label)
{
	if (!label)          return;
	if (label->hasfocus) return;

	if (label == username_label)
	{	/* username area has been clicked! */
		username->SetFocus(username, 1);
		password->SetFocus(password, 0);
		if (password_label) password_label->SetFocus(password_label, 0);
		session->SetFocus(session, 0);
		if (session_label) session_label->SetFocus(session_label, 0);
	}

	if (label == password_label)
	{	/* password area has been clicked! */
		username->SetFocus(username, 0);
		if (username_label) username_label->SetFocus(username_label, 0);
		password->SetFocus(password, 1);
		session->SetFocus(session, 0);
		if (session_label) session_label->SetFocus(session_label, 0);
	}

	if (label == session_label)
	{	/* session area has been clicked! */
		username->SetFocus(username, 0);
		if (username_label) username_label->SetFocus(username_label, 0);
		password->SetFocus(password, 0);
		if (password_label) password_label->SetFocus(password_label, 0);
		session->SetFocus(session, 1);
	}
}

/* callback function to handle textbox clicks */
void textbox_click(TextBox *textbox)
{
	if (!textbox)          return;
	if (textbox->hasfocus) return;

	if (textbox == username)
	{	/* username area has been clicked! */
		if (username_label) username_label->SetFocus(username_label, 1);
		password->SetFocus(password, 0);
		if (password_label) password_label->SetFocus(password_label, 0);
		session->SetFocus(session, 0);
		if (session_label) session_label->SetFocus(session_label, 0);
	}

	if (textbox == password)
	{	/* password area has been clicked! */
		username->SetFocus(username, 0);
		if (username_label) username_label->SetFocus(username_label, 0);
		if (password_label) password_label->SetFocus(password_label, 1);
		session->SetFocus(session, 0);
		if (session_label) session_label->SetFocus(session_label, 0);
	}
}

/* callback function to handle button clicks */
void button_click(Button *button)
{
	int doit = 0;

	if (!button) return;

	/* main process is forbidden to perform actions until we are done,
	 * unless, of course, it is already doing something, in which case
	 * we wait for it to finish
	 */
	while (!doit)
	{
		pthread_mutex_lock(&lock_act);
		if (!thread_action)
		{
			thread_action = 1;
			doit          = 1;
		}
		if (!doit)
		{
			pthread_mutex_unlock(&lock_act);
			struct timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec = 10000000;
			sleep(1);
		}
	}
	pthread_mutex_unlock(&lock_act);

	switch (button->action)
	{
		case DO_POWEROFF:
		case DO_REBOOT:
			begin_shutdown_sequence (button->action, button->events);
			break;
		case DO_SCREEN_SAVER:
#ifdef USE_SCREEN_SAVERS
			do_ss(dfb, primary, font_large, 0);
#endif
			break;
		case DO_SLEEP:
			if (!sleep_cmd)
			{
				DFBInputEvent evt;
					
				clear_screen();
				primary->DrawString (primary, "You must define sleep command in settings file!", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
				primary->Flip (primary, NULL, 0);
				sleep(2);
				events->GetEvent (events, DFB_EVENT (&evt));
				reset_screen(&evt);
				break;
			}

			begin_shutdown_sequence (DO_SLEEP, button->events);
			break;
		default: /* no action */
			break;
	}

	pthread_mutex_lock(&lock_act);
	thread_action = 0;
	pthread_mutex_unlock(&lock_act);
}

void start_login_sequence(DFBInputEvent *evt)
{
	int   result;
  int   free_temp = 0;
  char *message;
  char *welcome_msg;
  char *temp;

  if (!strlen(username->text)) return;	
  message = StrApp((char**)NULL, "Logging in ", username->text, "...", (char*)NULL);
  clear_screen();
  primary->DrawString (primary, message, -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
  free(message);
  primary->Flip (primary, NULL, DSFLIP_BLIT);

  if (hide_last_user && !strcmp(username->text, "lastuser"))
	{
		temp = get_last_user();
		free_temp = 1;
	}
  else temp = username->text;

	result = gui_check_password(temp, password->text, session->selected, ppid);
	switch (result)
	{
		case 1: /* login success */
			primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
			if (!clear_background) Draw_Background_Image(0);
			welcome_msg = get_welcome_msg(temp);
			primary->DrawString (primary, welcome_msg, -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
			primary->Flip (primary, NULL, DSFLIP_BLIT);
			free(welcome_msg);
			sleep(1);
			if (free_temp) free(temp);

			close_framebuffer_mode(EXIT_SUCCESS);
			exit(EXIT_SUCCESS);
			break;

		case 0: /* login failure */
			message = StrApp((char**)NULL, "Login failed!", (char*)NULL);
			break;

		default: /* crypto error occurred */
			message = StrApp((char**)NULL, "Crypto error - regenerate your keys!", (char*)NULL);
			break;
	}

	primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
	if (!clear_background) Draw_Background_Image(0);
	primary->DrawString (primary, message, -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
	primary->Flip (primary, NULL, DSFLIP_BLIT);
	sleep(2);
	password->ClearText(password);
	reset_screen(evt);
	if (free_temp) free(temp);
}

void handle_mouse_event()
{
	if (curs_surface)
		if (!pthread_mutex_trylock(lock_mouse_cursor))
		{
			layer->SetCursorShape (layer, curs_surface, cursor->x_off, cursor->y_off);
			pthread_mutex_unlock(lock_mouse_cursor);
		}
}

int handle_keyboard_event(DFBInputEvent *evt)
{
  struct DFBKeySymbolName *symbol_name; /* we store here the symbol name of the last key the user pressed */
	actions    action;
  int        returnstatus   =       -1;
  int        allow_tabbing  =        1;
  modifiers  modifier       =        NONE;
  int        ascii_code     = (int)  evt->key_symbol;

  show_lock_key_status(evt);
  symbol_name = bsearch (&(evt->key_symbol), keynames, 
												 sizeof (keynames) / sizeof (keynames[0]) - 1,
												 sizeof (keynames[0]), compare_symbol);

	modifier = modifier_is_pressed(evt);

	/* Let's check wether our user pressed some of our allowed key bindings... */
	action = search_keybindings(modifier, ascii_code);
	if (action != DO_NOTHING)
	{
		WRITELOG(DEBUG, "You pressed '%s%s' and I will now %s...\n", print_modifier(modifier), print_key(ascii_code), print_action(action));
		
		switch (action)
		{
			case DO_POWEROFF:
			case DO_REBOOT:
				begin_shutdown_sequence (action, events);
				break;
			case DO_KILL:
				close_framebuffer_mode(EXIT_RESPAWN);
				exit(EXIT_SUCCESS);
				break;
			case DO_SCREEN_SAVER:
				do_ss(dfb, primary, font_large, 0);
				break;
			case DO_SLEEP:
				if (!sleep_cmd)
				{
					DFBInputEvent evt;

					clear_screen();
					primary->DrawString (primary, "You must define sleep command in settings file!", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
					primary->Flip (primary, NULL, 0);
					sleep(2);
					events->GetEvent (events, DFB_EVENT (&evt));
					reset_screen(&evt);
					break;
				}
				begin_shutdown_sequence (DO_SLEEP, events);
				break;
			case DO_NEXT_TTY:
				return (current_tty + 1);
				break;
			case DO_PREV_TTY:
				return (current_tty - 1);
				break;
			case DO_TEXT_MODE:
				return EXIT_TEXT_MODE;
				break;
			case DO_NOTHING: /* Guess what, we do nothing here! */
				break;
		}
	}

	if (modifier == ALT || modifier == CTRLALT)
	{ 
		if (symbol_name)
		{
			/* we check if the user is pressing ctrl-alt-del... */
			if (!strcmp(symbol_name->name, "DELETE") && modifier == CTRLALT)
				do_ctrl_alt_del(evt);
			/*
			 * ... or [CTRL-]ALT-number with 1 <= number <= 12
			 * in this case we close directfb mode and send him to that tty
			 */
			if (!strncmp(symbol_name->name, "F", 1) && strlen (symbol_name->name) <= 3)
			{
				int temp = atoi (symbol_name->name + 1);
				if ((temp > 0) && (temp < 13))
					if (current_tty != temp)
						return temp;
			}
		}
		return returnstatus;
	}

  if (symbol_name)
	{
		/* ctr-j is alias for RETURN; */
		if (modifier == CONTROL && ascii_code == 'j') ascii_code = RETURN;

		/* Rock'n Roll! */
		if (!username->hasfocus && !session->isclicked && ascii_code == RETURN) start_login_sequence(evt);

		/* user name events */
		if (username->hasfocus && allow_tabbing)
		{
			if (ascii_code == TAB || ascii_code == RETURN)
	    {
	      allow_tabbing = 0;
	      if (username_label) username_label->SetFocus(username_label, 0);
	      username->SetFocus(username, 0);
	      if (modifier_is_pressed(evt) != SHIFT)
				{
					if (password_label) password_label->SetFocus(password_label, 1);
					password->SetFocus(password, 1);
				}
	      else
				{
					if (session_label) session_label->SetFocus(session_label, 1);
					session->SetFocus(session, 1);
				}
	    }
			else
	    {
				pthread_mutex_lock(&(username->lock));
				set_user_session(username->text);
				pthread_mutex_unlock(&(username->lock));
	    }
		}

		/* password events */
		if (password->hasfocus && allow_tabbing)
		{
			if (ascii_code == TAB)
	    {
	      allow_tabbing = 0;
	      if (password_label) password_label->SetFocus(password_label, 0);
	      password->SetFocus(password, 0);
	      if (modifier_is_pressed(evt) != SHIFT)
				{
					if (session_label) session_label->SetFocus(session_label, 1);
					session->SetFocus(session, 1);
				}
	      else
				{
					if (username_label) username_label->SetFocus(username_label, 1);
					username->SetFocus(username, 1);
				}
	    }
		}

		/* session events */
		if (session->hasfocus && allow_tabbing)
		{
			if (ascii_code == TAB)
	    {
	      allow_tabbing = 0;
	      if (session_label) session_label->SetFocus(session_label, 0);
	      session->SetFocus(session, 0);
	      if (modifier_is_pressed(evt) != SHIFT)
				{
					if (username_label) username_label->SetFocus(username_label, 1);
					username->SetFocus(username, 1);
				}
	      else
				{
					if (password_label) password_label->SetFocus(password_label, 1);
					password->SetFocus(password, 1);
				}
	    }
		}
	}

  return returnstatus;
}

void load_sessions(ComboBox *session)
{
  char *temp;

	/* we get all available sessions... */
  while ((temp = get_sessions()) != NULL)
	{
		session->AddItem(session, temp);
		free(temp);
	}

	/* ...and order them */
	session->SortItems(session);
}

int create_windows()
{
  DFBWindowDescription window_desc;
  IDirectFBFont *font;
  window_t *window = windowsList;
	char *message = "CAPS LOCK is pressed";
	int width;

  window_desc.flags = ( DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS );
  window_desc.caps  = DWCAPS_ALPHACHANNEL;

  while (window)
	{
		window_desc.posx   = window->x      * screen_width  / theme_xres;
		window_desc.posy   = window->y      * screen_height / theme_yres;
		window_desc.width  = window->width  * screen_width  / theme_xres;
		window_desc.height = window->height * screen_height / theme_yres;	

		switch(window->text_size)
		{
			case TINY:
				font = font_tiny;
				break;
			case SMALLER:
				font = font_smaller;
				break;
			case SMALL:
				font = font_small;
				break;
			case MEDIUM:
				font = font_normal;
				break;
			case LARGE: /* fall to default */
			default:
				font = font_large;
				break;
		}

		/* what kind of window are we going to create? */
		switch (window->type)
		{
			case LOGIN:
				username = TextBox_Create(layer, dfb, font, window->text_color, window->cursor_color, &window_desc);
				if (!username) return 0;
				if (window->cursor) username->SetCursor(username, dfb, window->cursor, x_ratio, y_ratio);
				username->SetClickCallBack(username, textbox_click);
				break;
			case PASSWORD:
				password = TextBox_Create(layer, dfb, font, window->text_color, window->cursor_color, &window_desc);
				if (!password) return 0;
				if (window->cursor) password->SetCursor(password, dfb, window->cursor, x_ratio, y_ratio);
				password->SetClickCallBack(password, textbox_click);
				break;
			case LABEL:
			{
				static Label_list *labels = NULL;

				if (!labels)
	      {
					labels = (Label_list *) calloc(1, sizeof(Label_list));
					Labels = labels;
	      }
				else
	      {
					labels->next = (Label_list *) calloc(1, sizeof(Label_list));
					labels = labels->next;
	      }
				labels->label = Label_Create(layer, dfb, font, window->text_color, &window_desc);
				if (!labels->label) return 0;
				labels->label->SetTextOrientation(labels->label, window->text_orientation);
				labels->label->SetAction(labels->label, window->polltime, window->content, window->command);
				labels->label->SetFocus(labels->label, 0);
				labels->next = NULL;
				if (window->linkto)
	      {
					if (!strcmp(window->linkto, "login"))    username_label = labels->label;
					if (!strcmp(window->linkto, "password")) password_label = labels->label;
					if (!strcmp(window->linkto, "session"))  session_label  = labels->label;
					labels->label->SetClickCallBack(labels->label, label_click);
	      }
				else
				{
					labels->label->window->SetOpacity(labels->label->window, selected_window_opacity);
				}

				break;
			}
			case BUTTON:
			{
				static Button_list *buttons = NULL;
				char *image1, *image2;

				if (!buttons)
	      {
					buttons = (Button_list *) calloc(1, sizeof(Button_list));
					Buttons = buttons;
	      }
				else
	      {
					buttons->next = (Button_list *) calloc(1, sizeof(Button_list));
					buttons = buttons->next;
	      }
				image1 = StrApp((char **)NULL, theme_dir, window->content, "_normal.png",    (char *)NULL);
				image2 = StrApp((char **)NULL, theme_dir, window->content, "_mouseover.png", (char *)NULL);
				buttons->button = Button_Create(image1, image2, window_desc.posx, window_desc.posy, button_click, layer, primary, dfb, x_ratio, y_ratio);
				if (!buttons->button) return 0;
				buttons->next = NULL;
				free(image1); free(image2);
				if (!window->command)
					buttons->button->action = DO_NOTHING;
				else
				{
					if (!strcmp(window->command, "halt"       )) buttons->button->action = DO_POWEROFF;
					if (!strcmp(window->command, "reboot"     )) buttons->button->action = DO_REBOOT;
					if (!strcmp(window->command, "sleep"      )) buttons->button->action = DO_SLEEP;
					if (!strcmp(window->command, "screensaver")) buttons->button->action = DO_SCREEN_SAVER;
				}
				if (window->cursor) buttons->button->SetCursor(buttons->button, dfb, window->cursor, x_ratio, y_ratio);

				break;
			}
			case COMBO:
				if (window->type == COMBO && !strcmp(window->command, "sessions"))
				{
					session = ComboBox_Create(layer, dfb, font, window->text_color, &window_desc, screen_width, screen_height);
					if (!session) return 0;
					session->SetSortFunction(session, sort_sessions);
					if (window->cursor) session->SetCursor(session, dfb, window->cursor, x_ratio, y_ratio);
				}

				break;
			default:
				return 0;
		}

		window = window->next;
	}
  destroy_windows_list(windowsList);

  /* Finally we create the four "CAPS LOCK is pressed" windows... */
	font_small->GetStringWidth (font_small, message, -1, &width);
  window_desc.posx   = 0;
  window_desc.posy   = screen_height - (font_small_height);
  window_desc.width  = width;
  window_desc.height = font_small_height;
  lock_key_statusA = Label_Create(layer, dfb, font_small, &other_text_color, &window_desc);
  if (!lock_key_statusA) return 0;
  lock_key_statusA->SetFocus(lock_key_statusA, 1);
  lock_key_statusA->Hide(lock_key_statusA);  
	lock_key_statusA->SetTextOrientation(lock_key_statusA, CENTERBOTTOM);
  lock_key_statusA->SetText(lock_key_statusA, message);
  window_desc.posy = 0;
  lock_key_statusB = Label_Create(layer, dfb, font_small, &other_text_color, &window_desc);
  if (!lock_key_statusB) return 0;
  lock_key_statusB->SetFocus(lock_key_statusB, 1);
  lock_key_statusB->Hide(lock_key_statusB);
	lock_key_statusB->SetTextOrientation(lock_key_statusB, LEFT);
  lock_key_statusB->SetText(lock_key_statusB, message);
  window_desc.posx = screen_width - width;
  window_desc.height = 2*font_small_height;
  lock_key_statusC = Label_Create(layer, dfb, font_small, &other_text_color, &window_desc);
  if (!lock_key_statusC) return 0;
  lock_key_statusC->SetFocus(lock_key_statusC, 1);
  lock_key_statusC->Hide(lock_key_statusC);
	lock_key_statusC->SetTextOrientation(lock_key_statusC, RIGHT);
  lock_key_statusC->SetText(lock_key_statusC, message);
  window_desc.posy = screen_height - (font_small_height);
  lock_key_statusD = Label_Create(layer, dfb, font_small, &other_text_color, &window_desc);
  if (!lock_key_statusD) return 0;
  lock_key_statusD->SetFocus(lock_key_statusD, 1);
  lock_key_statusD->Hide(lock_key_statusD);
	lock_key_statusD->SetTextOrientation(lock_key_statusD, RIGHT);
  lock_key_statusD->SetText(lock_key_statusD, message);

  return 1;
}

int set_font_sizes ()
{
  DFBFontDescription fdsc;
  const char *fontfile = font;

  fdsc.flags = DFDESC_HEIGHT;
  fdsc.height = screen_width / 30;
  if (dfb->CreateFont (dfb, fontfile, &fdsc, &font_large) != DFB_OK) return 0;
  font_large_height = fdsc.height;
  fdsc.height = screen_width / 40;
  if (dfb->CreateFont (dfb, fontfile, &fdsc, &font_normal) != DFB_OK) return 0;
  font_normal_height = fdsc.height;
  fdsc.height = screen_width / 50;
  if (dfb->CreateFont (dfb, fontfile, &fdsc, &font_small) != DFB_OK) return 0;
  font_small_height = fdsc.height;
  fdsc.height = screen_width / 60;
  if (dfb->CreateFont (dfb, fontfile, &fdsc, &font_smaller) != DFB_OK) return 0;
  font_smaller_height = fdsc.height;
  fdsc.height = screen_width / 70;
  if (dfb->CreateFont (dfb, fontfile, &fdsc, &font_tiny) != DFB_OK) return 0;
  font_tiny_height = fdsc.height;

  return 1;
}

int main (int argc, char *argv[])
{
  int returnstatus = -1;        /* return value of this function...         */
  DFBSurfaceDescription sdsc;   /* description for the primary surface      */
  DFBInputEvent evt;            /* generic input events will be stored here */
  DFBResult result;             /* we store eventual errors here            */
  char *lastuser=NULL;          /* latest user who logged in                */
	pthread_t pm_thread;

  /* load settings from file */
	initialize_variables();
	current_tty = ParseCMDLine(argc, argv, 0);
	if (!load_settings()) return GUI_FAILURE;
  if (!disable_last_user) lastuser = get_last_user();

#ifdef WANT_CRYPTO
	restore_public_key(stdin);
#endif

	/* get the pid of our father process */
	ppid = atoi(argv[argc-1]);

	log_stderr();

  /* we initialize directfb */
	result = DirectFBInit (&argc, &argv);
  if (result == DFB_OK) result = DirectFBSetOption("session","-1");
  if (result == DFB_OK) result = DirectFBCreate (&dfb);
  if (result == DFB_OK) result = dfb->EnumInputDevices (dfb, enum_input_device, &devices);
  if (result == DFB_OK) result = dfb->CreateInputEventBuffer (dfb, DICAPS_ALL, DFB_TRUE, &events);
  if (result == DFB_OK) result = dfb->GetDisplayLayer (dfb, DLID_PRIMARY, &layer);

  /* any errors so far? */
	if (result != DFB_OK)
	{
		DirectFB_Error();
		return GUI_FAILURE;
	}

  /* more initialization */
  layer->SetCooperativeLevel (layer, DLSCL_ADMINISTRATIVE);
  layer->EnableCursor (layer, 0);
  sdsc.flags = DSDESC_CAPS;
  sdsc.caps  = DSCAPS_PRIMARY | DSCAPS_FLIPPING;

  if (dfb->CreateSurface( dfb, &sdsc, &primary ) != DFB_OK)
	{
		DirectFB_Error();
		return GUI_FAILURE;
	}

  primary->GetSize(primary, &screen_width, &screen_height);

	if (screen_width  != theme_xres) x_ratio = (float)screen_width/(float)theme_xres;
	if (screen_height != theme_yres) y_ratio = (float)screen_height/(float)theme_yres;

  if (!set_font_sizes ())
	{
		DirectFB_Error();
		return GUI_FAILURE;
	}

  Draw_Background_Image(1);

	lock_mouse_cursor = NULL;
	if (cursor)
		if (cursor->path)
		{
			char *path = StrApp((char**)NULL, theme_dir, "/", cursor->path, (char*)NULL);
			curs_surface = load_image (path, dfb, x_ratio, y_ratio);
			free(path);
			layer->SetCursorShape (layer, curs_surface, cursor->x_off, cursor->y_off);

			lock_mouse_cursor = (pthread_mutex_t *)calloc(1,sizeof(pthread_mutex_t));
			if (pthread_mutex_init(lock_mouse_cursor, NULL))
			{
				writelog(ERROR, "Could not initialize cursor management mutex\n");
				free(lock_mouse_cursor);
				lock_mouse_cursor = NULL;
			}
		}

  if (!create_windows())
	{
		DirectFB_Error();
		return GUI_FAILURE;
	}

  if (!hide_password) password->MaskText(password, 1);
  else password->HideText(password, 1);
  load_sessions(session);
  if (lastuser)
	{
		if (username_label) username_label->SetFocus(username_label, 0);
		if (password_label) password_label->SetFocus(password_label, 1);
		if (!hide_last_user) username->SetText(username, lastuser);
		else username->SetText(username, "lastuser");
		username->SetFocus(username, 0);
		password->SetFocus(password, 1);
		set_user_session(lastuser);
		free(lastuser);
		lastuser = NULL;
	}
  else
	{ 
		if (username_label) username_label->SetFocus(username_label, 1);
		if (password_label) password_label->SetFocus(password_label, 0);
		username->SetFocus(username, 1);
	}
  if (session_label) session_label->SetFocus(session_label, 0);
  session->SetFocus(session, 0);

	if (!cursor)
		layer->EnableCursor (layer, 1);
	else
		layer->EnableCursor (layer, cursor->enable);

	if (use_screen_power_management)
	{
		pthread_create(&pm_thread, NULL, (void *) screen_pm_thread, dfb);
	}

#ifdef USE_SCREEN_SAVERS
	if (use_screensaver)
		do_ss(dfb, primary, font_large, 1);
#endif

  /* we go on for ever... or until the user does something in particular */
  while (returnstatus == -1)
	{
		/* we wait for an input event... */
		events->WaitForEventWithTimeout(events, 0, 500);

		if (events->HasEvent(events) == DFB_OK) /* ...got that! */
		{
			int do_action = 1;

			events->GetEvent (events, DFB_EVENT (&evt));

			/* we perform our action only if allowed to */
			pthread_mutex_lock(&lock_act);
			if (thread_action)
				do_action = 0;
			else
				thread_action = 1;
			pthread_mutex_unlock(&lock_act);

			if (do_action)
			{
				switch (evt.type)
				{
					case DIET_KEYPRESS:
						returnstatus = handle_keyboard_event(&evt);
						break;
					case DIET_AXISMOTION:
						handle_mouse_event();
						break;
					default: /* we do nothing here */
						break;
				}

				pthread_mutex_lock(&lock_act);
				thread_action = 0;
				pthread_mutex_unlock(&lock_act);
			}
		}
	}

  close_framebuffer_mode (returnstatus);
	return EXIT_SUCCESS;
}
