/***************************************************************************
                       load_settings.c  -  description
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
# include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
# include <sys/dir.h>
# endif
# if HAVE_NDIR_H
# include <ndir.h>
# endif
#endif

#include "load_settings.h"
#include "misc.h"
#include "chvt.h"

extern FILE* yyin;
extern int yyparse(void);

char *file_error = NULL;
int   GOT_THEME  = 0;
int   in_theme   = 0;

void initialize_variables(void)
{
	SCREENSAVER             = PIXEL_SCREENSAVER;  
	TEXT_SESSIONS_DIRECTORY = NULL;
  X_SESSIONS_DIRECTORY    = NULL;
	BACKGROUND              = NULL;
	THEME_DIR               = NULL;
	LAST_USER               = NULL;
	SETTINGS                = NULL;	
	DATADIR                 = NULL;
	XINIT                   = NULL;
  FONT                    = NULL;
	image_paths             = NULL;	
	windowsList             = NULL;
  black_screen_workaround = 0;
  screensaver_timeout     = 5;
  no_shutdown_screen      = 0;
	disable_last_user       = 0;
  use_screensaver         = 1;
  hide_last_user          = 0;
  hide_password           = 0;
  silent                  = 1;
	SHUTDOWN_POLICY         = EVERYONE;
}

void set_default_session_dirs(void)
{
	TEXT_SESSIONS_DIRECTORY = strdup("/etc/qingy/sessions");
  X_SESSIONS_DIRECTORY    = strdup("/etc/X11/Sessions/");  
}

void set_default_xinit(void)
{
	XINIT = strdup("/usr/X11R6/bin/xinit");
}

void set_default_font(void)
{
  FONT = StrApp((char**)NULL, THEME_DIR, "decker.ttf",(char *)NULL);
}

void set_default_colors(void)
{
  BUTTON_OPACITY = 0xFF;
  WINDOW_OPACITY = 0x80;
  SELECTED_WINDOW_OPACITY = 0xCF;

  MASK_TEXT_COLOR_R = 0xFF;
  MASK_TEXT_COLOR_G = 0x00;
  MASK_TEXT_COLOR_B = 0x00;
  MASK_TEXT_COLOR_A = 0xFF;

  TEXT_CURSOR_COLOR_R = 0x80;
  TEXT_CURSOR_COLOR_G = 0x00;
  TEXT_CURSOR_COLOR_B = 0x00;
  TEXT_CURSOR_COLOR_A = 0xDD;

  OTHER_TEXT_COLOR_R = 0x40;
  OTHER_TEXT_COLOR_G = 0x40;
  OTHER_TEXT_COLOR_B = 0x40;
  OTHER_TEXT_COLOR_A = 0xFF;
}

void add_to_paths(char *path)
{
	static struct _image_paths *temp = NULL;

	if (!path) return;
	if (!temp)
	{
		image_paths = (struct _image_paths *) calloc(1, sizeof(struct _image_paths));
		temp = image_paths;
	}
	else
	{
		temp->next = (struct _image_paths *) calloc(1, sizeof(struct _image_paths));
		temp = temp->next;
	}
	
	temp->path = strdup(path);
	temp->next = NULL;
	if (!silent) fprintf(stderr, "Added '%s' to image paths...\n", path);
}

char *get_random_theme()
{
  DIR *dir;
  char *themes_dir = StrApp((char**)0, DATADIR, "themes/", (char*)0);
	char *result;
  struct dirent *entry;
  int n_themes = 0;
  char *themes[128];
  time_t epoch;
  struct tm curr_time;
  int i;

  dir= opendir(themes_dir);
	free(themes_dir);
  if (!dir) return strdup("default");

  while ((entry= readdir(dir)))
  {
		char *temp;
    if (!strcmp(entry->d_name, "." )) continue;
    if (!strcmp(entry->d_name, "..")) continue;    

    temp = StrApp((char**)NULL, themes_dir, entry->d_name);
    if (is_a_directory(temp))
    {
      themes[n_themes] = strdup(entry->d_name);
      n_themes++;
    }
		free(temp);
  }
  closedir(dir);

  /* let's create a random number between 0 and n_themes-1 */
  epoch = time(NULL);
  localtime_r(&epoch, &curr_time);
  srand(curr_time.tm_sec);
  i = rand() % n_themes;

	result = strdup(themes[i]);
	for (i=0; i<n_themes; i++) free(themes[i]);

  return result;
}

int set_theme(char *theme)
{
	char *oldfile_error = file_error;
	FILE *oldfile       = yyin;
	char *file;
	
	if (!theme) return 0;

	THEME_DIR = StrApp((char**)NULL, DATADIR, "themes/", theme, "/", (char*)NULL);
	file = StrApp((char**)NULL, THEME_DIR, "theme", (char*)NULL);
	file_error = file;

  yyin = fopen(file, "r");
  if (!yyin)
  {
    if (!silent) fprintf(stderr, "load_settings: theme '%s' does not exist.\n", theme);		
		file_error = oldfile_error;
		yyin       = oldfile;
    return 0;
  }

  in_theme = 1;
  yyparse();
  fclose(yyin);

	file_error = oldfile_error;
  yyin       = oldfile;
	GOT_THEME  = 1;
  in_theme   = 0;

  return 1;
}

void yyerror(char *error)
{
  if (!silent)
	{
		fprintf(stderr, "qingy: error in configuration file %s:\n", file_error);
		fprintf(stderr, "       %s.\n", error);
	}
  free(X_SESSIONS_DIRECTORY);
  free(TEXT_SESSIONS_DIRECTORY);
  free(XINIT);
  free(FONT);
  free(THEME_DIR);
	THEME_DIR = StrApp((char**)NULL, DATADIR, "themes/default/", (char*)NULL);
  set_default_session_dirs();
  set_default_xinit();
  set_default_font();
  set_default_colors();
}

int load_settings(void)
{
  TEXT_SESSIONS_DIRECTORY = NULL;
	X_SESSIONS_DIRECTORY    = NULL;
	THEME_DIR               = NULL;
	XINIT                   = NULL;
	FONT                    = NULL;

  DATADIR   = strdup("/etc/qingy/");
	SETTINGS  = StrApp((char**)NULL, DATADIR, "settings", (char*)NULL);
  LAST_USER = StrApp((char**)NULL, DATADIR, "lastuser", (char*)NULL);  

  yyin = fopen(SETTINGS, "r");
  if (!yyin)
  {
    if (!silent) fprintf(stderr, "load_settings: settings file not found...\nusing internal defaults\n");
    set_default_session_dirs();
    set_default_xinit();
    set_default_font();
    set_default_colors();
    return 1;
  }
	file_error = SETTINGS;
  yyparse();
  fclose(yyin);
	file_error = NULL;

  if (!X_SESSIONS_DIRECTORY) set_default_session_dirs();
  if (!XINIT) set_default_xinit();
  if (!GOT_THEME)
	{
		char *theme = strdup("default");
		set_theme(theme);
		free(theme);
	}

  if (!FONT) set_default_font();

#ifdef DEBUG
	fprintf(stderr, "XINIT is '%s'\n", XINIT);
	fprintf(stderr, "X_SESSIONS_DIRECTORY is '%s'\n", X_SESSIONS_DIRECTORY);
	fprintf(stderr, "TEXT_SESSIONS_DIRECTORY is '%s'\n",  TEXT_SESSIONS_DIRECTORY);
	fprintf(stderr, "FONT is '%s'\n", FONT);
	fprintf(stderr, "BACKGROUND is '%s'\n", BACKGROUND);
	
	fprintf(stderr, "BUTTON_OPACITY is %d\n", BUTTON_OPACITY);
	fprintf(stderr, "WINDOW_OPACITY is %d\n", WINDOW_OPACITY);
	fprintf(stderr, "SELECTED_WINDOW_OPACITY is %d\n", SELECTED_WINDOW_OPACITY);
	
	fprintf(stderr, "MASK_TEXT_COLOR is %d, %d, %d, %d\n", MASK_TEXT_COLOR_R, MASK_TEXT_COLOR_G, MASK_TEXT_COLOR_B, MASK_TEXT_COLOR_A);
	fprintf(stderr, "TEXT_CURSOR_COLOR is %d, %d, %d, %d\n", TEXT_CURSOR_COLOR_R, TEXT_CURSOR_COLOR_G, TEXT_CURSOR_COLOR_B, TEXT_CURSOR_COLOR_A);
	fprintf(stderr, "OTHER_TEXT_COLOR is %d, %d, %d, %d\n", OTHER_TEXT_COLOR_R, OTHER_TEXT_COLOR_G, OTHER_TEXT_COLOR_B, OTHER_TEXT_COLOR_A);

	fprintf(stderr, "Allowed to shutdown: %s\n", (SHUTDOWN_POLICY==EVERYONE) ? "everyone" : (SHUTDOWN_POLICY==ROOT) ? "root only" : "no one");
#endif

  return 1;
}

char *get_last_user(void)
{
  FILE *fp = fopen(LAST_USER, "r");
  char tmp[MAX];  

  if (!fp) return NULL;
  if (fscanf(fp, "%s", tmp) != 1)
  {
    fclose(fp);
    return NULL;
  }
  fclose(fp);

  return strdup(tmp);
}

int set_last_user(char *user)
{
  FILE *fp;

  if (!user) return 0;
  fp = fopen(LAST_USER, "w");
  fprintf(fp, "%s", user);
  fclose(fp);

  return 1;
}

char *get_last_session(char *user)
{
  char *homedir;
  char filename[MAX];
  char tmp[MAX];
  FILE *fp;

  if (!user) return NULL;  
	homedir = get_home_dir(user);
  if (!homedir) return NULL;

  strcpy(filename, homedir);
	free(homedir);
  if (filename[strlen(filename)-1] != '/') strcat(filename, "/");
  strcat(filename, ".qingy");
  fp = fopen(filename, "r");
  if (!fp) return NULL;
  if (!get_line(tmp, fp, MAX))
  {
    fclose(fp);
    return NULL;
  }
  fclose(fp);

  return strdup(tmp);
}

int set_last_session(char *user, char *session)
{
  char *homedir = get_home_dir(user);
  char *filename;
  FILE *fp;

  if (!user)    return 0;
	if (!session) return 0;
  if (!homedir) return 0;

  filename = (char *) calloc(strlen(homedir)+8, sizeof(char));
  strcpy(filename, homedir);
	free(homedir);
  if (filename[strlen(filename)-1] != '/') strcat(filename, "/");
  strcat(filename, ".qingy");
  fp = fopen(filename, "w");
	free(filename);

  if (!fp) return 0;
  fprintf(fp, "%s", session);
  fclose(fp);

  return 1;
}

char *get_action(char *action)
{
	char *temp;

	if (!action) return NULL;

	/* should we shutdown? */
	temp = strstr(action, "shutdown");
	if (temp)
	{
		if (strstr(temp + 8, "-h")) return strdup("poweroff");
		if (strstr(temp + 8, "-r")) return strdup("reboot");
		return NULL;
	}
	if (strstr(action, "poweroff")) return strdup("poweroff");
	if (strstr(action, "halt"))     return strdup("poweroff");
	if (strstr(action, "reboot"))   return strdup("reboot");

	/* should we print something? */
	temp = strstr(action, "echo");
	if (temp)
	{
		size_t  length;
		char   *begin;
		temp = strchr(temp+4, '"');
		if (!temp) return NULL;
		begin = temp + 1;
		temp = strchr(begin, '"');
		if (!temp) return NULL;
		length = temp - begin;
		return strndup(begin, length);		
	}

	return NULL;
}

char *parse_inittab_file(void)
{
	FILE   *fp     = fopen("etc/inittab", "r");
	size_t  length = 0;
	char   *line   = NULL;
	char   *result = NULL;

	if (!fp) return NULL;

	while (getline(&line, &length, fp) != -1)
	{
		char *test = strstr(line, ":ctrlaltdel:");
		if (!test) continue;
		if (*line == '#') continue;
		result = get_action(test + 12);
		break; 
	}
	fclose(fp);

	if (length) free(line);
	return result;
}

int add_window_to_list(window_t w)
{
  window_t *aux=malloc(sizeof(window_t));
  aux->type=w.type;
  aux->x=w.x;
  aux->y=w.y;
  aux->command=strdup(w.command);
  free(w.command);
  aux->content=strdup(w.content);
  free(w.content);
  aux->next=windowList;
  windowList=aux;
  return 1;
}
