/***************************************************************************
                       load_settings.h  -  description
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


int black_screen_workaround;
int silent;
int hide_password;
int hide_last_user;
int disable_last_user;
int no_shutdown_screen;
int use_screensaver;
int screensaver_timeout;

char *DATADIR;
char *SETTINGS;
char *LAST_USER;
char *X_SESSIONS_DIRECTORY;
char *TEXT_SESSIONS_DIRECTORY;
char *XINIT;
char *FONT;
char *BACKGROUND;
char *THEME_DIR;

int BUTTON_OPACITY;
int WINDOW_OPACITY;
int SELECTED_WINDOW_OPACITY;

/* colors */
typedef struct
{
	unsigned int R;
	unsigned int G;
	unsigned int B;
	unsigned int A;
}
color_t;
color_t MASK_TEXT_COLOR;
color_t TEXT_CURSOR_COLOR;
color_t OTHER_TEXT_COLOR;

/* Shutdown permissions policy... */
typedef enum 
{
	EVERYONE=0,
	ROOT,
	NOONE
} shutdown_policies;
shutdown_policies SHUTDOWN_POLICY;

/* screen saver stuff */
struct _image_paths
{
	char *path;
	struct _image_paths *next;
};
struct _image_paths *image_paths;
typedef enum _kinds
{
  PIXEL_SCREENSAVER,
  PHOTO_SCREENSAVER
} screensaver_kinds;
screensaver_kinds SCREENSAVER;

/* Custom windows can be of the following- they mean:
   - UNKNOWN: default value, treat as error.
   - LABEL: display static text. if command is empty, not executable
   or error returning, display content. Otherwise, command output.
   - BUTTON: execute command when mouse pressed (key 1). content has
   path to button img file stem. That is, /path/to/button where in path/to
   files button-mouseover.png and button-normal.png are found.
   button commands may be "halt" "reboot" "sleep" "screensave"
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
	SMALL,
	MEDIUM,
	LARGE
} text_size_t;

/* a window structure */
typedef struct _window
{  
  int x;
  int y;
  int width;
  int height;
  int polltime;
	text_size_t text_size;
	color_t text_color;
  window_types_t type;
  char *command;
  char *content;
  struct _window *next;
} window_t;

window_t* windowsList;

int add_window_to_list(window_t w);
int get_win_type(const char* name);


/* initialize some stuff */
void initialize_variables(void);

/* get program settings from config file */
int load_settings(void);

/* parse /etc/inittab to detect behaviour when user presses ctrl-alt-del */
char *parse_inittab_file(void);

/* get/set name of last user that logged in */
char *get_last_user(void);
int set_last_user(char *user);

/* get/save last session used by <user> */
char *get_last_session(char *user);
int set_last_session(char *user, char *session);

/* theme stuff */
int set_theme(char *theme);
char *get_random_theme();

/* stuff you don't want to know about ;-P */
void yyerror(char *where);
void add_to_paths(char *path);

#ifdef DEBUG
void show_windows_list(void);
#endif
