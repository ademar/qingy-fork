/***************************************************************************
                      directfb_mode.c  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003-2005 by Noberasco Michele
    e-mail               : s4t4n@gentoo.org
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

#ifdef WANT_CRYPTO
#include "crypto.h"
#endif

/* misc stuff */
#include "memmgmt.h"
#include "vt.h"
#include "misc.h"
#include "session.h"
#include "load_settings.h"
#include "keybindings.h"

/* my custom DirectFB stuff */
#include "directfb_mode.h"
#include "button.h"
#include "utils.h"
#include "textbox.h"
#include "combobox.h"
#include "label.h"

#ifdef USE_SCREEN_SAVERS
#include "screen_saver.h"
#endif

/* some super-structures */
typedef struct label_t
{
  Label *label;
  int    polltime;
  int    countdown;
  char  *content;
  char  *command;
  int    text_orientation;
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
ComboBox              *session            = NULL; /* combo boxes                               */
#ifdef USE_GPM_LOCK
int                   we_stopped_gpm;             /* wether this program stopped gpm or not    */
#endif
int                   screen_width;               /* screen resolution                         */
int                   screen_height;
int                   font_tiny_height;           /* font sizes                                */
int                   font_smaller_height;
int                   font_small_height;
int                   font_normal_height;
int                   font_large_height;
int                   username_area_mouse   = 0;  /* sensible areas for mouse cursor to be in  */
int                   password_area_mouse   = 0;
int                   session_area_mouse    = 0;
#ifdef USE_SCREEN_SAVERS
int                   screensaver_active    = 0;  /* screensaver stuff                         */
int                   screensaver_countdown = 0;
#endif
float                 x_ratio               = 1;  /* theme res. should be corrected by x_ratio */
float                 y_ratio               = 1;  /* and y_ratio to match the actual res.      */

void safe_exit(int exitstatus)
{
#undef exit

  /* we overwrite memory areas containing sensitive information */
	if (username) if (username->text)
		memset(username->text, '\0', sizeof(username->text));
	if (password) if (password->text)
		memset(password->text, '\0', sizeof(password->text));

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
		if (BACKGROUND)  panel_image = load_image (BACKGROUND, primary, dfb, x_ratio, y_ratio);
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

void close_framebuffer_mode (void)
{
	/*
	 * Once, all DirectFB interfaces were shut
	 * down gracefully here. Now no longer:
	 * DirectFB has not been nice to us recently,
	 * so we are not being nice to it any more!
	 *
	 * DIE, you BASTARD, DIE!
	 * And do it the HARD WAY!
	 */

  /* destroy all labels */
/*   while (Labels) */
/* 	{ */
/* 		Label_list *temp = Labels; */
/* 		Labels = Labels->next; */
/* 		if (temp->label) temp->label->Destroy(temp->label); */
/* 		temp->next = NULL; */
/* 		free(temp->content); */
/* 		free(temp->command); */
/* 		free(temp); */
/* 	} */

/*   /\* destroy all buttons *\/ */
/*   while (Buttons) */
/* 	{ */
/* 		Button_list *temp = Buttons; */
/* 		Buttons = Buttons->next; */
/* 		if (temp->button) temp->button->Destroy(temp->button); */
/* 		temp->next = NULL; */
/* 		free(temp); */
/* 	} */

/* 	/\* background image *\/ */
/*   if (panel_image) panel_image->Release (panel_image); */

/* 	/\* the silly messages that appear when you have your CAPS LOCK down *\/ */
/*   if (lock_key_statusA) lock_key_statusA->Destroy(lock_key_statusA); */
/*   if (lock_key_statusB) lock_key_statusB->Destroy(lock_key_statusB); */
/*   if (lock_key_statusC) lock_key_statusC->Destroy(lock_key_statusC); */
/*   if (lock_key_statusD) lock_key_statusD->Destroy(lock_key_statusD); */

/* 	/\* data input *\/ */
/*   if (username) username->Destroy(username); /\* nice: suicide *\/ */
/*   if (password) password->Destroy(password); */
/*   if (session)  session->Destroy (session); */

/* 	/\* fonts *\/ */
/*   if (font_tiny)    font_tiny->Release    (font_tiny); */
/*   if (font_smaller) font_smaller->Release (font_smaller); */
/*   if (font_small)   font_small->Release   (font_small); */
/*   if (font_normal)  font_normal->Release  (font_normal); */
/*   if (font_large)   font_large->Release   (font_large); */

/* 	/\* core DirectFB stuff *\/ */
/*   if (primary) primary->Release (primary); */
/*   if (events)  events->Release  (events); */
/*   if (layer)   layer->Release   (layer); */
/*   while (devices) */
/* 	{ */
/* 		DeviceInfo *next = devices->next; */
/* 		free (devices); */
/* 		devices = next; */
/* 	} */
/* 	if (dfb) dfb->Release (dfb); */

#ifdef USE_GPM_LOCK
  if (we_stopped_gpm) start_gpm();
#endif
}

void DirectFB_Error()
{
  fprintf(stderr, "Unrecoverable error: reverting to text mode!\n"); /* dammit! */
	close_framebuffer_mode();
}

/* mouse movement in buttons area */
void handle_buttons(int *mouse_x, int *mouse_y)
{
  Button_list *buttons       = Buttons;
  Button_list *other_buttons = Buttons;
  
  /* let's check wether mouse is over a button... */
	if (!session->isclicked)
		while (buttons)
		{
			if ((*mouse_x >= buttons->button->xpos) && (*mouse_x <= (buttons->button->xpos + (int) buttons->button->width)))
				if ((*mouse_y >= buttons->button->ypos) && (*mouse_y <= (buttons->button->ypos + (int) buttons->button->height)))
				{
					if (!buttons->button->mouse)
					{
						buttons->button->MouseOver(buttons->button, 1);
						while (other_buttons)
						{
							if (other_buttons->button->mouse && other_buttons->button != buttons->button) other_buttons->button->MouseOver(other_buttons->button, 0);
							other_buttons = other_buttons->next;
						}
						return;
					}
					else return; /* we already plotted this event */
				}
			buttons = buttons->next;
		}
  
  /* mouse not over any button */
  while (other_buttons)
	{
		if (other_buttons->button->mouse) other_buttons->button->MouseOver(other_buttons->button, 0);
		other_buttons = other_buttons->next;
	}
}

/* mouse movement in textboxes and comboboxes area */
void handle_text_combo_boxes(int *mouse_x, int *mouse_y)
{
	while (1)
	{
		/* mouse over session area */
		if ( (*mouse_x >= (int) session->xpos) && (*mouse_x <= (int) session->xpos + (int) session->width) )
			if ( (*mouse_y >= (int) session->ypos) && (*mouse_y <= (int) session->ypos + (int) session->height) )
			{
				session_area_mouse = 1;
				session->MouseOver(session, 1);
				return;
			}		

		/* mouse over username area */
		if ( (*mouse_x >= (int) username->xpos) && (*mouse_x <= (int) username->xpos + (int) username->width) )
			if ( (*mouse_y >= (int) username->ypos) && (*mouse_y <= (int) username->ypos + (int) username->height) )
			{
				username_area_mouse = 1;
				break;
			}
  
		/* mouse over password area */
		if ( (*mouse_x >= (int) password->xpos) && (*mouse_x <= (int) password->xpos + (int) password->width) )
			if ( (*mouse_y >= (int) password->ypos) && (*mouse_y <= (int) password->ypos + (int) password->height) )
			{
				password_area_mouse = 1;
				break;
			}
  
		break;
	}

	if (session->mouse) session->MouseOver(session, 0);
}

/* mouse movement in labels area */
void handle_labels(int *mouse_x, int *mouse_y)
{
	if (session->isclicked) return;

  /* mouse over username area */
  if (username_label)
    if ( (*mouse_x >= (int) username_label->xpos) && (*mouse_x <= (int) username_label->xpos + (int) username_label->width) )
      if ( (*mouse_y >= (int) username_label->ypos) && (*mouse_y <= (int) username_label->ypos + (int) username_label->height) )
			{
				username_area_mouse = 1;
				return;
			}
  
  /* mouse over password area */
  if (password_label)
    if ( (*mouse_x >= (int) password_label->xpos) && (*mouse_x <= (int) password_label->xpos + (int) password_label->width) )
      if ( (*mouse_y >= (int) password_label->ypos) && (*mouse_y <= (int) password_label->ypos + (int) password_label->height) )
			{
				password_area_mouse = 1;
				return;
			}
  
  /* mouse over session area */
  if (session_label)
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
  session_area_mouse  = 0;

  layer->GetCursorPosition (layer, &mouse_x, &mouse_y);
  handle_text_combo_boxes(&mouse_x, &mouse_y);
  handle_buttons(&mouse_x, &mouse_y);
  handle_labels(&mouse_x, &mouse_y);
}

void show_lock_key_status(DFBInputEvent *evt)
{
  if (lock_is_pressed(evt) == CAPSLOCK)
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
  show_lock_key_status(evt);
  handle_mouse_movement();
  layer->EnableCursor (layer, 1);
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
  primary->SetColor (primary, OTHER_TEXT_COLOR.R, OTHER_TEXT_COLOR.G, OTHER_TEXT_COLOR.B, OTHER_TEXT_COLOR.A);
}

void begin_shutdown_sequence (actions action)
{
  DFBInputEvent evt;
  char message[35];
  int countdown = 5;
  char *temp;

  clear_screen();
  
  /* First, we check shutdown policy */
  switch (SHUTDOWN_POLICY)
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
      if (!check_password("root", password->text))
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
		close_framebuffer_mode ();
		if (action == DO_POWEROFF) exit(EXIT_SHUTDOWN_H);
		if (action == DO_REBOOT)   exit(EXIT_SHUTDOWN_R);
		if (action == DO_SLEEP)    exit(EXIT_SLEEP);
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
  fprintf (stderr, "\nfatal error: unable to exec \"/sbin/shutdown\"!\n");
  if (!no_shutdown_screen) close_framebuffer_mode ();
  exit (QINGY_FAILURE);
}

void do_ctrl_alt_del(DFBInputEvent *evt)
{
  char *action = parse_inittab_file();
  
  if (!action) return;	
  if (!strcmp(action, "poweroff")) {free(action); begin_shutdown_sequence (DO_POWEROFF); return;}
  if (!strcmp(action, "reboot"))   {free(action); begin_shutdown_sequence (DO_REBOOT  ); return;}
  
  /* we display a message */
  clear_screen();
  primary->DrawString (primary, action, -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
  primary->Flip (primary, NULL, 0);
  sleep(2);
  reset_screen(evt);
  free(action);
}

void handle_mouse_event (DFBInputEvent *evt)
{
  static Button *button = NULL;
  static int status = 0;

  if (evt->type == DIET_AXISMOTION)
	{ /* we check wether there is movement on Z axis, aka mouse wheel */
		if (evt->axis == DIAI_Z)
		{
			int mouse_x, mouse_y;

			layer->GetCursorPosition (layer, &mouse_x, &mouse_y);

			if ( (mouse_x >= (int) session->xpos) && (mouse_x <= (int) session->xpos + (int) session->width) )
				if ( (mouse_y >= (int) session->ypos) && (mouse_y <= (int) session->ypos + (int) session->height) )
				{
					if (evt->axisrel == MOUSE_WHEEL_UP)
						session->KeyEvent(session, UP);
			
					if (evt->axisrel == MOUSE_WHEEL_DOWN)
						session->KeyEvent(session, DOWN);
				}
		}
		else handle_mouse_movement();
	}
  else
	{	/* mouse button press or release */
		if (left_mouse_button_down (evt))
		{ /*
			 * left mouse button is down:
			 * we check wether mouse pointer is over a specific area
			 */
			Button_list *buttons = Buttons;

			if (username_area_mouse) status = 1;
			if (password_area_mouse) status = 2;
			if (session_area_mouse)  status = 3;
			while (buttons)
	    {
	      if (buttons->button->mouse)
				{
					button = buttons->button;
					break;
				}
	      buttons = buttons->next;
	    }

			if (session->mouse)
			{
	      username->SetFocus(username, 0);
	      if (username_label) username_label->SetFocus(username_label, 0);
	      password->SetFocus(password, 0);
	      if (password_label) password_label->SetFocus(password_label, 0);
	      session->SetFocus(session, 1);
	      if (session_label) session_label->SetFocus(session_label, 1);
				session->Click(session);
			}
			else if (session->isclicked)
			{
				session->Click(session);
				handle_mouse_movement();
			}
		}
		else
		{	/* 
			 * left mouse button is up:
			 * if it was on a specific area when down we check if it is still there
			 */
			if (button)
				if (button->mouse)
					switch (button->action)
					{
						case DO_POWEROFF:
						case DO_REBOOT:
							begin_shutdown_sequence (button->action);
							break;
						case DO_SCREEN_SAVER:
#ifdef USE_SCREEN_SAVERS
							screensaver_countdown = 0;
							screensaver_active    = 1;
							clear_screen();
							primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
							primary->Flip  (primary, NULL, DSFLIP_BLIT);
#endif
							break;
						case DO_SLEEP:
							if (!SLEEP_CMD)
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

							begin_shutdown_sequence (DO_SLEEP);
							break;
						default: /* no action */
							break;
					}

			if (username_area_mouse && status == 1)
	    {	/* username area has been clicked! */
	      username->SetFocus(username, 1);
	      if (username_label) username_label->SetFocus(username_label, 1);
	      password->SetFocus(password, 0);
	      if (password_label) password_label->SetFocus(password_label, 0);
	      session->SetFocus(session, 0);
	      if (session_label) session_label->SetFocus(session_label, 0);
	    }
			if (password_area_mouse && status == 2)
	    {	/* password area has been clicked! */
	      username->SetFocus(username, 0);
	      if (username_label) username_label->SetFocus(username_label, 0);
	      password->SetFocus(password, 1);
	      if (password_label) password_label->SetFocus(password_label, 1);
	      session->SetFocus(session, 0);
	      if (session_label) session_label->SetFocus(session_label, 0);
	    }
			if (session_area_mouse && status == 3)
	    {	/* session area has been clicked! */
	      username->SetFocus(username, 0);
	      if (username_label) username_label->SetFocus(username_label, 0);
	      password->SetFocus(password, 0);
	      if (password_label) password_label->SetFocus(password_label, 0);

				if (!session->isclicked)
				{
					session->SetFocus(session, 1);
				}

	      if (session_label) session_label->SetFocus(session_label, 1);
	    }
			status = 0;		/* we reset click status because button went up */
			button = NULL;
		}
	}
}

void start_login_sequence(DFBInputEvent *evt)
{
  int  free_temp = 0;
  char *message;
  char *user_name;
  char *user_session;
  char *welcome_msg;
  char *temp;

  if (!strlen(username->text)) return;	
  message = StrApp((char**)NULL, "Logging in ", username->text, "...", (char*)NULL);
  clear_screen();
  primary->DrawString (primary, message, -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
  free(message);
  primary->Flip (primary, NULL, DSFLIP_BLIT);
  sleep(1);

  if (hide_last_user && !strcmp(username->text, "lastuser"))
	{
		temp = get_last_user();
		free_temp = 1;
	}
  else temp = username->text;
  if (!check_password(temp, password->text))
	{
		primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
		if (!clear_background) Draw_Background_Image(0);
		primary->DrawString (primary, "Login failed!", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
		primary->Flip (primary, NULL, DSFLIP_BLIT);
		sleep(2);
		password->ClearText(password);
		reset_screen(evt);
		if (free_temp) free(temp);
		return;
	}
  primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
  if (!clear_background) Draw_Background_Image(0);
  welcome_msg = get_welcome_msg(temp);
  primary->DrawString (primary, welcome_msg, -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
  primary->Flip (primary, NULL, DSFLIP_BLIT);
  free(welcome_msg);
  sleep(1);
  user_name = strdup(temp);  
  user_session = strdup(session->selected);
  if (free_temp) free(temp);

#ifdef WANT_CRYPTO
	encrypt_item(stdout, user_name);
	encrypt_item(stdout, password->text);
	encrypt_item(stdout, user_session);
	fflush(stdout);
#else
	fprintf(stdout, "%s\n%s\n%s\n", user_name, password->text, user_session);
#endif

  /* we overwrite memory areas containing sensitive information */
  memset(user_name,      '\0', sizeof(user_name     ));
  memset(password->text, '\0', sizeof(password->text));
  memset(user_session,   '\0', sizeof(user_session  ));

  free(user_name); free(user_session);

	close_framebuffer_mode();
  exit(EXIT_SUCCESS);
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
		if (!silent)
			fprintf(stderr, "You pressed '%s%s' and I will now %s...\n", print_modifier(modifier), print_key(ascii_code), print_action(action));
		
		switch (action)
		{
			case DO_POWEROFF:
			case DO_REBOOT:
				begin_shutdown_sequence (action);
				break;
			case DO_KILL: /* we kill out parent - the true qingy - then commit suicide */
				kill(getppid(), SIGINT);
				kill(getpid(),  SIGINT);
				break;
			case DO_SCREEN_SAVER:
				ascii_code            = 0;
				modifier              = 0;
				screensaver_countdown = 0;
				screensaver_active    = 1;
				clear_screen();
				primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
				primary->Flip  (primary, NULL, DSFLIP_BLIT);
				break;
			case DO_SLEEP:
				if (!SLEEP_CMD)
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
				begin_shutdown_sequence (DO_SLEEP);
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
					session->SetFocus(session, 0);
				}
	    }
			else
	    {
	      username->KeyEvent(username, ascii_code, modifier, 1);
	      set_user_session(username->text);
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
			else password->KeyEvent(password, ascii_code, modifier, 1);
		}

		/* session events */
		if (session->hasfocus && allow_tabbing)
		{
			if (ascii_code == RETURN)     session->KeyEvent(session, SELECT);
			if (ascii_code == ARROW_UP)   session->KeyEvent(session, UP);
			if (ascii_code == ARROW_DOWN) session->KeyEvent(session, DOWN);
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

		/* just in case we resized a combobox and
		 * the mouse cursor is no longer on top of it
		 */
		if (!session->isclicked)
			handle_mouse_movement();
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

void update_labels()
{
  char *message;
  Label_list *labels = Labels;

  for (; labels; labels = labels->next)
	{
		if (!labels->polltime) continue;
		if (labels->countdown)
		{
			labels->countdown--;
			continue;
		}
		if (!labels->command || !labels->content)
		{
			labels->polltime = 0;
			continue;
		}

		message = assemble_message(labels->content, labels->command);
		labels->label->SetText(labels->label, message, labels->text_orientation);
		free(message);
		labels->countdown = labels->polltime;
	}
}

int create_windows()
{
  DFBWindowDescription window_desc;
  IDirectFBFont *font;
  window_t *window = windowsList;

  window_desc.flags = ( DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS );
  window_desc.caps  = DWCAPS_ALPHACHANNEL;
  while (window)
	{
		window_desc.posx   = window->x      * screen_width  / THEME_XRES;
		window_desc.posy   = window->y      * screen_height / THEME_YRES;
		window_desc.width  = window->width  * screen_width  / THEME_XRES;
		window_desc.height = window->height * screen_height / THEME_YRES;	
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
				username = TextBox_Create(layer, font, window->text_color, window->cursor_color, &window_desc);
				if (!username) return 0;
				break;
			case PASSWORD:
				password = TextBox_Create(layer, font, window->text_color, window->cursor_color, &window_desc);
				if (!password) return 0;
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
				labels->label = Label_Create(layer, font, window->text_color, &window_desc);
				if (!labels->label) return 0;			
				labels->content          = strdup(window->content);
				labels->command          = strdup(window->command);
				labels->polltime         = window->polltime * 2;
				labels->text_orientation = window->text_orientation;
				labels->countdown        = 0;
				labels->next             = NULL;
				if (window->command)
	      {
					char *message = assemble_message(labels->content, labels->command);
					labels->label->SetText(labels->label, message, labels->text_orientation);
					free(message);
	      }
				else labels->label->SetText(labels->label, labels->content, window->text_orientation);
				labels->label->SetFocus(labels->label, 0);
				if (window->linkto)
	      {
					if (!strcmp(window->linkto, "login"))    username_label = labels->label;
					if (!strcmp(window->linkto, "password")) password_label = labels->label;
					if (!strcmp(window->linkto, "session"))  session_label  = labels->label;
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
				image1 = StrApp((char **)NULL, THEME_DIR, window->content, "_normal.png",    (char *)NULL);
				image2 = StrApp((char **)NULL, THEME_DIR, window->content, "_mouseover.png", (char *)NULL);
				buttons->button = Button_Create(image1, image2, window_desc.posx, window_desc.posy, layer, primary, dfb, x_ratio, y_ratio);
				if (!buttons->button) return 0;			
				buttons->next = NULL;
				buttons->button->MouseOver(buttons->button, 0);
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
				break;
			}
			case COMBO:
				if (window->type == COMBO && !strcmp(window->command, "sessions"))
				{
					session = ComboBox_Create(layer, font, window->text_color, &window_desc, screen_width, screen_height);
					if (!session) return 0;
				}
				break;
			default:
				return 0;
		}

		window = window->next;
	}
  destroy_windows_list(windowsList);

  /* Finally we create the four "CAPS LOCK is pressed" windows... */
  window_desc.posx   = 0;
  window_desc.posy   = screen_height - (font_small_height);
  window_desc.width  = screen_width/5;
  window_desc.height = font_small_height;
  lock_key_statusA = Label_Create(layer, font_small, &OTHER_TEXT_COLOR, &window_desc);
  if (!lock_key_statusA) return 0;
  lock_key_statusA->SetFocus(lock_key_statusA, 1);
  lock_key_statusA->Hide(lock_key_statusA);  
  lock_key_statusA->SetText(lock_key_statusA, "CAPS LOCK is pressed", CENTERBOTTOM);
  window_desc.posy = 0;
  lock_key_statusB = Label_Create(layer, font_small, &OTHER_TEXT_COLOR, &window_desc);
  if (!lock_key_statusB) return 0;
  lock_key_statusB->SetFocus(lock_key_statusB, 1);
  lock_key_statusB->Hide(lock_key_statusB);
  lock_key_statusB->SetText(lock_key_statusB, "CAPS LOCK is pressed", LEFT);
  window_desc.posx = screen_width - screen_width/5;
  window_desc.height = 2*font_small_height;
  lock_key_statusC = Label_Create(layer, font_small, &OTHER_TEXT_COLOR, &window_desc);
  if (!lock_key_statusC) return 0;
  lock_key_statusC->SetFocus(lock_key_statusC, 1);
  lock_key_statusC->Hide(lock_key_statusC);
  lock_key_statusC->SetText(lock_key_statusC, "CAPS LOCK is pressed", RIGHT);
  window_desc.posy = screen_height - (font_small_height);
  lock_key_statusD = Label_Create(layer, font_small, &OTHER_TEXT_COLOR, &window_desc);
  if (!lock_key_statusD) return 0;
  lock_key_statusD->SetFocus(lock_key_statusD, 1);
  lock_key_statusD->Hide(lock_key_statusD);
  lock_key_statusD->SetText(lock_key_statusD, "CAPS LOCK is pressed", RIGHT);

  return 1;
}

int set_font_sizes ()
{
  DFBFontDescription fdsc;
  const char *fontfile = FONT;

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

  /* load settings from file */
	initialize_variables();
	current_tty = ParseCMDLine(argc, argv, 0);
	if (!load_settings()) return QINGY_FAILURE;
  if (!disable_last_user) lastuser = get_last_user();

#ifdef WANT_CRYPTO
	restore_public_key(stdin);
#endif

#ifdef USE_GPM_LOCK
  /* Stop GPM if necessary */
  we_stopped_gpm = stop_gpm();
#endif

  /* we initialize directfb */
  if (silent) stderr_disable();
	result = DirectFBInit (&argc, &argv);
  if (result == DFB_OK) result = DirectFBSetOption("session","-1");
  if (result == DFB_OK) result = DirectFBCreate (&dfb);
  if (silent) stderr_enable();
  if (result == DFB_OK) result = dfb->EnumInputDevices (dfb, enum_input_device, &devices);
  if (result == DFB_OK) result = dfb->CreateInputEventBuffer (dfb, DICAPS_ALL, DFB_TRUE, &events);
  if (result == DFB_OK) result = dfb->GetDisplayLayer (dfb, DLID_PRIMARY, &layer);

  /* any errors so far? */
	if (result != DFB_OK)
	{
		DirectFB_Error();
		return QINGY_FAILURE;
	}

  /* more initialization */
  layer->SetCooperativeLevel (layer, DLSCL_ADMINISTRATIVE);
  layer->EnableCursor (layer, 0);
  sdsc.flags = DSDESC_CAPS;
  sdsc.caps  = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
  if (dfb->CreateSurface( dfb, &sdsc, &primary ) != DFB_OK)
	{
		DirectFB_Error();
		return QINGY_FAILURE;
	}
  primary->GetSize(primary, &screen_width, &screen_height);

	if (screen_width  != THEME_XRES) x_ratio = (float)screen_width/(float)THEME_XRES;
	if (screen_height != THEME_YRES) y_ratio = (float)screen_height/(float)THEME_YRES;

  if (!set_font_sizes ())
	{
		DirectFB_Error();
		return QINGY_FAILURE;
	}
  Draw_Background_Image(1);

  if (!create_windows())
	{
		DirectFB_Error();
		return QINGY_FAILURE;
	}
  if (!hide_password) password->mask_text = 1;
  else password->hide_text = 1;
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

  layer->EnableCursor (layer, 1);

#ifdef USE_SCREEN_SAVERS
  /* initialize screen saver stuff */
  screen_saver_kind    = SCREENSAVER_NAME;
  screen_saver_surface = primary;
  screen_saver_dfb     = dfb;
  screen_saver_events  = events;
#endif

  /* we go on for ever... or until the user does something in particular */
  while (returnstatus == -1)
	{
#ifdef USE_SCREEN_SAVERS
		if (!screensaver_countdown)
			screensaver_countdown = screensaver_timeout * 120;		
#endif
      
		/* we wait for an input event... */
#ifdef USE_SCREEN_SAVERS
		if (!screensaver_active) events->WaitForEventWithTimeout(events, 0, 500);
		else
		{
			primary->SetFont  (primary, font_large);
			primary->SetColor (primary, OTHER_TEXT_COLOR.R, OTHER_TEXT_COLOR.G, OTHER_TEXT_COLOR.B, OTHER_TEXT_COLOR.A);
			activate_screen_saver();
		}
#else  /* don't want screensavers */
		events->WaitForEventWithTimeout(events, 0, 500);
#endif /* screensaver stuff */
		
		if (events->HasEvent(events) == DFB_OK)
		{ /* ...got that! */
			events->GetEvent (events, DFB_EVENT (&evt));
#ifdef USE_SCREEN_SAVERS
			screensaver_countdown = screensaver_timeout * 120;			
			if (screensaver_active)
	    {
	      screensaver_active = 0;
	      reset_screen(&evt);
	    }
#endif
			switch (evt.type)
	    {
				case DIET_AXISMOTION:
				case DIET_BUTTONPRESS:
				case DIET_BUTTONRELEASE:
					handle_mouse_event (&evt);
					break;
				case DIET_KEYPRESS:
					returnstatus = handle_keyboard_event(&evt);
					break;
				default: /* we do nothing here */
					break;
	    }
		}
		else
		{ /* Let there be a flashing cursor! */
			static int flashing_cursor = 0;

#ifdef USE_SCREEN_SAVERS	  
			if (!screensaver_active)
	    {
	      if (username->hasfocus) username->KeyEvent(username, REDRAW, NONE, flashing_cursor);
	      if (password->hasfocus) password->KeyEvent(password, REDRAW, NONE, flashing_cursor);
	      flashing_cursor = !flashing_cursor;
	      if (use_screensaver) screensaver_countdown--;
	      update_labels();
	    }
			if (!screensaver_countdown)
	    {
	      screensaver_active = 1;
	      clear_screen();
	      primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
	      primary->Flip  (primary, NULL, DSFLIP_BLIT);
	    }
#else  /* don't want screensavers */
			if (username->hasfocus) username->KeyEvent(username, REDRAW, NONE, flashing_cursor);
			if (password->hasfocus) password->KeyEvent(password, REDRAW, NONE, flashing_cursor);
			flashing_cursor = !flashing_cursor;
			update_labels();
#endif /* screensaver stuff */
		}
	}

  close_framebuffer_mode ();
  exit(returnstatus);

	/* just to make gcc happy */
	return returnstatus;
}
