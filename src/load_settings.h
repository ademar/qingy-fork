/***************************************************************************
                       load_settings.h  -  description
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


int black_screen_workaround;
int silent;
int hide_password;
int hide_last_user;
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

int MASK_TEXT_COLOR_R;
int MASK_TEXT_COLOR_G;
int MASK_TEXT_COLOR_B;
int MASK_TEXT_COLOR_A;

int TEXT_CURSOR_COLOR_R;
int TEXT_CURSOR_COLOR_G;
int TEXT_CURSOR_COLOR_B;
int TEXT_CURSOR_COLOR_A;

int OTHER_TEXT_COLOR_R;
int OTHER_TEXT_COLOR_G;
int OTHER_TEXT_COLOR_B;
int OTHER_TEXT_COLOR_A;

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

/* load a theme */
int set_theme(char *theme);
