/***************************************************************************
                       load_settings.h  -  description
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

#ifndef LOAD_SETTINGS_H
#define LOAD_SETTINGS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

int silent;
int text_mode_login;
int hide_password;
int hide_last_user;
int disable_last_user;
int no_shutdown_screen;
int use_screensaver;
int screensaver_timeout;
int clear_background;
int current_tty;
int lock_sessions;
int retries;
int x_server_offset;

/* NOTE: some of these should become #defines through autoconf... */
char *program_name;
char *datadir;
char *screensavers_dir;
char *settings;
char *last_user;
char *x_sessions_directory;
char *text_sessions_directory;
char *xinit;
char *x_server;
char *x_args;
char *font;
char *background;
char *theme_dir;
char *themes_dir;
char *dfb_interface;
char *tmp_files_dir;
char *sleep_cmd;
char *pre_gui_script;
char *post_gui_script;

/* autologin stuff */
char *autologin_file_basename;
char *autologin_username;
char *autologin_password;
char *autologin_session;
int   do_autologin;
int   auto_relogin;

/* other stuff */
char *fb_device;
char *resolution;

int button_opacity;
int window_opacity;
int selected_window_opacity;

/*
 * this is the native resolution of the theme, i.e. the resolution
 * the theme was designed for, and we should scale only if our
 * current resolution is different from this one
 */
int theme_xres;
int theme_yres;

/* colors */
typedef struct
{
  unsigned int R;
  unsigned int G;
  unsigned int B;
  unsigned int A;
}
color_t;

color_t default_text_color;
color_t default_cursor_color;
color_t other_text_color;

/* Shutdown permissions policy... */
typedef enum 
{
  EVERYONE=0,
  ROOT,
  NOONE
} shutdown_policies;
shutdown_policies shutdown_policy;

/* Last user policy... */
typedef enum 
{
  LU_GLOBAL=0,
  LU_TTY
} last_user_policies;
last_user_policies last_user_policy;

/* Last session policy... */
typedef enum 
{
  LS_USER=0,
  LS_TTY
} last_session_policies;
last_session_policies last_session_policy;

/* screen saver stuff */
struct _screensaver_options
{
  char *option;
  struct _screensaver_options *next;
};
struct _screensaver_options *screensaver_options;

/* screensaver name */
char* screensaver_name;

/* Custom windows can be of the following- they mean:
   - UNKNOWN: default value, treat as error.
   - LABEL: display static text. if command is empty, not executable
   	    or error returning, display content. Otherwise, command output.
	    If polltime is 0, do not update window content. You can also specify
	    "%s" somewhere inside content: that "%s" will be converted into
	    the output of command...
   - BUTTON: execute command when mouse pressed (key 1). content has
	     button file name prefix, i.e. "reset" if button image names are
	     "reset_normal.png" and "reset_mouseover.png".   
	     button commands may be "halt" "reboot" "sleep" "screensaver"
   - LOGIN: standard login. command and content aren't used.
   - PASSWORD: standard password. command and content aren't used.
   - COMBO: a combo as in sessions, holding different values. command holds
   	    command, content is not used. possible commands here are
	    "sessions".
*/

typedef enum 
{
  UNKNOWN,
  LABEL,
  BUTTON,
  LOGIN,
  PASSWORD, 
  COMBO
} window_types_t;

typedef enum
{
	TINY,
	SMALLER,
  SMALL,
  MEDIUM,
  LARGE
} text_size_t;

typedef enum
{
  LEFT=0,
  CENTER,
  RIGHT
}
text_orient_t;

/* a window structure */
typedef struct _window
{  
  int x;
  int y;
  int width;
  int height;
  int polltime;
  text_size_t text_size;
  text_orient_t text_orientation;
  color_t *text_color;
  color_t *cursor_color;
  window_types_t type;
  char *command;
  char *content;
  char *linkto;
  struct _window *next;
} window_t;

window_t* windowsList;

int add_window_to_list(window_t *w);
int get_win_type(const char* name);
void destroy_windows_list(window_t *w);


/* initialize some stuff */
void initialize_variables(void);

/* get program settings from config file and command line */
int load_settings(void);
int ParseCMDLine(int argc, char *argv[], int paranoia);

/* parse /etc/inittab to detect behaviour when user presses ctrl-alt-del */
char *parse_inittab_file(void);

/* get/set name of last user that logged in */
char *get_last_user(void);
int set_last_user(char *user);

/* get/save last session used by <user> */
char *get_last_session(char *user);
void set_last_session(char *user, char *session, int tty);

/* see if we know this guy... */
char *get_welcome_msg(char *user);

/* theme stuff */
int got_theme;
extern int set_theme(char *theme);
char *get_random_theme();

/* stuff you don't want to know about ;-P */
void yyerror(char *where);
void add_to_options(char *option);
void erase_options(void);

#endif /* !LOAD_SETTINGS_H */
