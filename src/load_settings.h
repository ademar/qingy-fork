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

unsigned int MASK_TEXT_COLOR_R;
unsigned int MASK_TEXT_COLOR_G;
unsigned int MASK_TEXT_COLOR_B;
unsigned int MASK_TEXT_COLOR_A;

unsigned int TEXT_CURSOR_COLOR_R;
unsigned int TEXT_CURSOR_COLOR_G;
unsigned int TEXT_CURSOR_COLOR_B;
unsigned int TEXT_CURSOR_COLOR_A;

unsigned int OTHER_TEXT_COLOR_R;
unsigned int OTHER_TEXT_COLOR_G;
unsigned int OTHER_TEXT_COLOR_B;
unsigned int OTHER_TEXT_COLOR_A;

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
   - TEXT: text input. command will be executed as `command
   <text>`. Content is default text.
   - LABEL: display static text. if command is empty, not executable
   or error returning, display content. Otherwise, command output.
   - BUTTON: execute command when mouse pressed (key 1). content has
   path to button root. That is, /path/to/button where in path/to
   files button-mouseover.png and button-normal.png are found.
   - LOGIN: standard login. command and content aren't used.
   - PASSWORD: standard password. command and content aren't used.
   - SESSIONS: standard sessions combo. content has path/to/sessions,
   command holds prefix (e.g. now "Text:").
   - COMBO: a combo as in sessions, holding different values. command
   holds command, content holds path/to/file/of/values. command is
   executed as `command selected-option` 
 */
typedef enum {UNKOWN=0, TEXT, LABEL, BUTTON, LOGIN, PASSWORD, 
	      COMBO} window_types;

/* a window structure */
typedef struct _window {
  struct _window *next;
  int x;
  int y;
  int type;
  char* command;
  char* content;
} window_t;

window_t* windowsList;

/* init to NULL some stuff */
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
