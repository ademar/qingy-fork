/***************************************************************************
                      directfb_mode.c  -  description
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


/* system and DirectFB library stuff */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <directfb.h>
#include <directfb_keynames.h>

/* misc stuff */
#include "chvt.h"
#include "misc.h"
#include "session.h"
#include "load_settings.h"

/* my custom DirectFB stuff */
#include "directfb_mode.h"
#include "button.h"
#include "utils.h"
#include "textbox.h"
#include "combobox.h"
#include "label.h"
#include "screen_saver.h"


#define POWEROFF 0
#define REBOOT   1


/*
 * Global variables... definitely too many!
 */

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
Label_list *Labels = NULL;

typedef struct button_t
{
  Button *button;
  struct button_t *next;
} Button_list;
Button_list *Buttons = NULL;

IDirectFB             *dfb;                       /* the super interface                       */
IDirectFBDisplayLayer *layer;                     /* the primary layer                         */
IDirectFBSurface      *primary,                   /* surface of the primary layer              */
  *panel_image        = NULL; /* background image                          */
IDirectFBEventBuffer  *events;                    /* all input events will be stored here      */
DeviceInfo            *devices            = NULL; /* the list of all input devices             */
IDirectFBFont         *font_small,                /* fonts                                     */
  *font_normal,
  *font_large;  
TextBox               *username           = NULL, /* text boxes                                */
  *password           = NULL;
Label                 *username_label     = NULL, /* labels                                    */
  *password_label     = NULL,
  *session_label      = NULL,
  *lock_key_statusA   = NULL,
  *lock_key_statusB   = NULL,
  *lock_key_statusC   = NULL,
  *lock_key_statusD   = NULL;
ComboBox              *session            = NULL; /* combo boxes                               */
int                   we_stopped_gpm,             /* wether this program stopped gpm or not    */
  screen_width,               /* screen resolution                         */
  screen_height,
  font_small_height,          /* font sizes                                */
  font_normal_height,
  font_large_height,
  username_area_mouse   = 0,  /* sensible areas for mouse cursor to be in  */
  password_area_mouse   = 0,
  session_area_mouse    = 0,
  screensaver_active    = 0,  /* screensaver stuff                         */
  screensaver_countdown = 0;


void Draw_Background_Image(int do_the_drawing)
{
  /* width and height of the background image */
  static int panel_width, panel_height;
  /* We clear the primary surface */
  primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
  if (!panel_image)
    { /* we design the surface */
      if (BACKGROUND)  panel_image = load_image (BACKGROUND, primary, dfb);
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
  item *temp;
  char *user_session = get_last_session(user);
  
  if (!session || !session->items)
    {
      free(user_session);
      return;
    }
  
  if (!user_session)
    {
      temp = session->selected;
      while (strcmp(session->selected->name, "Text: Console"))
	session->selected = session->selected->next;
      if (session->selected != temp) session->KeyEvent(session, REDRAW);
      return;
    }
  
  temp = session->items;
  while (1)
    {
      if (!strcmp(user_session, temp->name))
	{
	  session->selected = temp;
	  session->KeyEvent(session, REDRAW);
	  free(user_session);
	  return;
	}
      temp = temp->next;
      if (temp == session->items) break;
    }
  free(user_session);
}

void close_framebuffer_mode (void)
{
  /* destroy all labels */
  while (Labels)
    {
      Label_list *temp = Labels;
      Labels = Labels->next;
      if (temp->label) temp->label->Destroy(temp->label);
      temp->next = NULL;
      free(temp->content);
      free(temp->command);
      free(temp);
    }
  /* destroy all buttons */
  while (Buttons)
    {
      Button_list *temp = Buttons;
      Buttons = Buttons->next;
      if (temp->button) temp->button->Destroy(temp->button);
      temp->next = NULL;
      free(temp);
    }
  
  if (panel_image) panel_image->Release (panel_image);
  if (lock_key_statusA) lock_key_statusA->Destroy(lock_key_statusA);
  if (lock_key_statusB) lock_key_statusB->Destroy(lock_key_statusB);
  if (lock_key_statusC) lock_key_statusC->Destroy(lock_key_statusC);
  if (lock_key_statusD) lock_key_statusD->Destroy(lock_key_statusD);
  if (username) username->Destroy(username);
  if (password) password->Destroy(password);
  if (session) session->Destroy(session);
  if (font_small) font_small->Release (font_small);
  if (font_normal) font_normal->Release (font_normal);
  if (font_large) font_large->Release (font_large);
  if (primary) primary->Release (primary);
  if (events) events->Release (events);
  if (layer) layer->Release (layer);
  while (devices)
    {
      DeviceInfo *next = devices->next;
      free (devices);
      devices = next;
    }
  if (dfb) dfb->Release (dfb);
  if (we_stopped_gpm) start_gpm();
}

void DirectFB_Error()
{
  fprintf(stderr, "Unrecoverable error: reverting to text mode!\n");
  close_framebuffer_mode();
}

/* mouse movement in buttons area */
void handle_buttons(int *mouse_x, int *mouse_y)
{
  Button_list *buttons       = Buttons;
  Button_list *other_buttons = Buttons;
  
  /* let's check wether mouse is over a button... */
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
  handle_buttons(&mouse_x, &mouse_y);
  handle_text_combo_boxes(&mouse_x, &mouse_y);
  handle_labels(&mouse_x, &mouse_y);
}

void show_lock_key_status(DFBInputEvent *evt)
{
  if (lock_is_pressed(evt) == 3)
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
  Draw_Background_Image(1);  
  primary->SetFont (primary, font_large);
  primary->SetColor (primary, OTHER_TEXT_COLOR.R, OTHER_TEXT_COLOR.G, OTHER_TEXT_COLOR.B, OTHER_TEXT_COLOR.A);
}

void begin_shutdown_sequence (int action)
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
      primary->DrawString (primary, "Shutting down this machine is not allowed!", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
      primary->Flip (primary, NULL, 0);
      sleep(2);
      events->GetEvent(events, DFB_EVENT (&evt));
      reset_screen(&evt);
      return;
    case ROOT: /* only root can shutdown the system */
      if (!check_password("root", password->text))
	{
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
	case POWEROFF:
	  strcat (message, "shutdown");
	  break;
	case REBOOT:
	  strcat (message, "restart");
	  break;
	default:
	  Draw_Background_Image (1);
	  return;
	}
      primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
      Draw_Background_Image(0);
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
  if (no_shutdown_screen)
    {
      close_framebuffer_mode ();
      if (black_screen_workaround != -1) tty_redraw();
    }
  else
    {
      primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
      Draw_Background_Image(0);
    }
  if (action == POWEROFF)
    {
      if (!no_shutdown_screen)
	{
	  primary->DrawString (primary, "shutting down system...", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
	  primary->Flip (primary, NULL, 0);
	}
      execl ("/sbin/shutdown", "/sbin/shutdown", "-h", "now", (char *) 0);
    }
  if (action == REBOOT)
    {
      if (!no_shutdown_screen)
	{
	  primary->DrawString (primary, "rebooting system...", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
	  primary->Flip (primary, NULL, 0);
	}
      execl ("/sbin/shutdown", "/sbin/shutdown", "-r", "now", (char *) 0);
    }
  
  /* we should never get here unless call to /sbin/shutdown fails */
  fprintf (stderr, "\nfatal error: unable to exec \"/sbin/shutdown\"!\n");
  if (!no_shutdown_screen) close_framebuffer_mode ();
  exit (EXIT_FAILURE);
}

void do_ctrl_alt_del(DFBInputEvent *evt)
{
  char *action = parse_inittab_file();
  
  if (!action) return;	
  if (!strcmp(action, "poweroff")) {free(action); begin_shutdown_sequence (POWEROFF); return;}
  if (!strcmp(action, "reboot"))   {free(action); begin_shutdown_sequence (REBOOT);   return;}
  
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
    handle_mouse_movement ();
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
	}
      else
	{	/* 
		 * left mouse button is up:
		 * if it was on a specific area when down we check if it is still there
		 */
	  if (button)
	    if (button->mouse)
	      switch (button->command)
		{
		case HALT:
		  begin_shutdown_sequence (POWEROFF);
		  break;
		case REBOOT:
		  begin_shutdown_sequence (REBOOT);
		  break;
		case SCREEN_SAVER:
		  screensaver_countdown = 0;
		  screensaver_active    = 1;
		  clear_screen();
		  primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
		  primary->Flip  (primary, NULL, DSFLIP_BLIT);
		  break;
		case SLEEP:
		  {
		    DFBInputEvent evt;
		    clear_screen();
		    primary->DrawString (primary, "I'm not tired, yet!", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
		    primary->Flip (primary, NULL, 0);
		    sleep(2);
		    events->GetEvent (events, DFB_EVENT (&evt));
		    reset_screen(&evt);
		    break;
		  }
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
	      session->SetFocus(session, 1);
	      if (session_label) session_label->SetFocus(session_label, 1);
	    }
	  status = 0;		/* we reset click status because button went up */
	  button = NULL;
	}
    }
}

void start_login_sequence(DFBInputEvent *evt)
{
  char *message;
  char *user_name;
  char *user_session;
  char *temp;
  int free_temp = 0;

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
      Draw_Background_Image(0);
      primary->DrawString (primary, "Login failed!", -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
      primary->Flip (primary, NULL, DSFLIP_BLIT);
      sleep(2);
      password->ClearText(password);
      reset_screen(evt);
      if (free_temp) free(temp);
      return;
    }
  primary->Clear (primary, 0x00, 0x00, 0x00, 0xFF);
  Draw_Background_Image(0);
  /* see if we know this guy... */
  {
    char *welcome_msg;
    char *user = NULL;
    char  line[128];
    FILE *users = fopen("/etc/qingy/welcomes", "r");
    if(!strcmp(temp, "root"))
      welcome_msg = strdup("Greetings, Master...");
    else
      welcome_msg = strdup("Starting selected session...");
    if (users)
      while (fgets(line, 127, users))
	{
	  user = strtok(line, " \t");
	  if(!strcmp(user, temp))
	    {
	      free(welcome_msg);
	      welcome_msg=strtok(NULL, "\n");
	      break;
	    }
	}

    primary->DrawString (primary, welcome_msg, -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
    primary->Flip (primary, NULL, DSFLIP_BLIT);
    sleep(1);
    user_name = strdup(temp);  
    user_session = strdup(session->selected->name);
    if (free_temp) free(temp);
    close_framebuffer_mode();
    start_session(user_name, user_session);
  }
  /* The above never returns, so... */
  free(user_name); free(user_session);
  fprintf(stderr, "Go tell my creator his brains went pop!\n");
  exit(EXIT_FAILURE);
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
  symbol_name = bsearch (&(evt->key_symbol), keynames, 
			 sizeof (keynames) / sizeof (keynames[0]) - 1,
			 sizeof (keynames[0]), compare_symbol);

  modifier = modifier_is_pressed(evt);
  if (modifier)
    {
      if (modifier == ALT)
	{	/* we check if user press ALT-p or ALT-r to start shutdown/reboot sequence */
	  if ((ascii_code == 'P')||(ascii_code == 'p')) 
	    begin_shutdown_sequence (POWEROFF);
	  if ((ascii_code == 'R')||(ascii_code == 'r')) 
	    begin_shutdown_sequence (REBOOT);
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
		  temp = atoi (symbol_name->name + 1);
		  if ((temp > 0) && (temp < 13))
		    if (get_active_tty () != temp)
		      return temp;
		}
	    }
	  return returnstatus;
	}
    }

  if (symbol_name)
    {
      /* Rock'n Roll! */
      if (!username->hasfocus && ascii_code == RETURN) start_login_sequence(evt);

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
	  else password->KeyEvent(password, ascii_code, 1);
	}

      /* session events */
      if (session->hasfocus && allow_tabbing)
	{
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
    }

  return returnstatus;
}

void load_sessions(ComboBox *session)
{
  char *temp;

  while ((temp = get_sessions()) != NULL)
    {
      session->AddItem(session, temp);
      free(temp);
    }
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
      window_desc.posx   = window->x      * screen_width  / THEME_WIDTH;
      window_desc.posy   = window->y      * screen_height / THEME_HEIGHT;
      window_desc.width  = window->width  * screen_width  / THEME_WIDTH;
      window_desc.height = window->height * screen_height / THEME_HEIGHT;	
      switch(window->text_size)
	{
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
	    buttons->button = Button_Create(image1, image2, window_desc.posx, window_desc.posy, layer, primary, dfb);
	    if (!buttons->button) return 0;			
	    buttons->next = NULL;
	    buttons->button->MouseOver(buttons->button, 0);
	    free(image1); free(image2);
	    if (!strcmp(window->command, "halt"       )) buttons->button->command = HALT;
	    if (!strcmp(window->command, "reboot"     )) buttons->button->command = REBOOT;
	    if (!strcmp(window->command, "sleep"      )) buttons->button->command = SLEEP;
	    if (!strcmp(window->command, "screensaver")) buttons->button->command = SCREEN_SAVER;
	    break;
	  }
	case COMBO:
	  if (window->type == COMBO && !strcmp(window->command, "sessions"))
	    {
	      session = ComboBox_Create(layer, font, window->text_color, &window_desc);
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

  return 1;
}

int directfb_mode (int argc, char *argv[])
{
  int returnstatus = -1;        /* return value of this function...         */
  DFBSurfaceDescription sdsc;   /* description for the primary surface      */
  DFBInputEvent evt;            /* generic input events will be stored here */
  DFBResult result;             /* we store eventual errors here            */
  char *lastuser=NULL;          /* latest user who logged in                */

  /* load settings from file */
  if (!load_settings()) return TEXT_MODE;
  if (!disable_last_user) lastuser = get_last_user();

#ifdef DEBUG
  show_windows_list();
#endif

  /* Stop GPM if necessary */
  we_stopped_gpm = stop_gpm();

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
      return TEXT_MODE;
    }

  /* more initialization */
  layer->SetCooperativeLevel (layer, DLSCL_ADMINISTRATIVE);
  layer->EnableCursor (layer, 0);
  sdsc.flags = DSDESC_CAPS;
  sdsc.caps  = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
  if (dfb->CreateSurface( dfb, &sdsc, &primary ) != DFB_OK)
    {
      DirectFB_Error();
      return TEXT_MODE;
    }
  primary->GetSize(primary, &screen_width, &screen_height);

  if (!set_font_sizes ())
    {
      DirectFB_Error();
      return TEXT_MODE;
    }
  Draw_Background_Image(1);

  if (!create_windows())
    {
      DirectFB_Error();
      return TEXT_MODE;
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

  /* initialize screen saver stuff */
  screen_saver_kind    = SCREENSAVER;
  screen_saver_surface = primary;
  screen_saver_dfb     = dfb;
  screen_saver_events  = events;

  /* it should be now safe to unlock vt switching again */
  unlock_tty_switching();

  /* we go on for ever... or until the user does something in particular */
  while (returnstatus == -1)
    {
      if (!screensaver_countdown)
	screensaver_countdown = screensaver_timeout * 120;		
      
      /* we wait for an input event... */
      if (!screensaver_active) events->WaitForEventWithTimeout(events, 0, 500);
      else
	{
	  primary->SetFont  (primary, font_large);
	  primary->SetColor (primary, OTHER_TEXT_COLOR.R, OTHER_TEXT_COLOR.G, OTHER_TEXT_COLOR.B, OTHER_TEXT_COLOR.A);
	  activate_screen_saver();
	}

      if (events->HasEvent(events) == DFB_OK)
	{ /* ...got that! */
	  events->GetEvent (events, DFB_EVENT (&evt));
	  screensaver_countdown = screensaver_timeout * 120;			
	  if (screensaver_active)
	    {
	      screensaver_active = 0;
	      reset_screen(&evt);
	    }
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
	  
	  if (!screensaver_active)
	    {
	      if (username->hasfocus) username->KeyEvent(username, REDRAW, flashing_cursor);
	      if (password->hasfocus) password->KeyEvent(password, REDRAW, flashing_cursor);
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
	}
    }

  close_framebuffer_mode ();
  return returnstatus;
}
