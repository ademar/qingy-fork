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
  SCREENSAVER             = NULL;  
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
  THEME_WIDTH             = 800;
  THEME_HEIGHT            = 600;
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

  DEFAULT_TEXT_COLOR.R = 0xFF;
  DEFAULT_TEXT_COLOR.G = 0x00;
  DEFAULT_TEXT_COLOR.B = 0x00;
  DEFAULT_TEXT_COLOR.A = 0xFF;

  DEFAULT_CURSOR_COLOR.R = 0x80;
  DEFAULT_CURSOR_COLOR.G = 0x00;
  DEFAULT_CURSOR_COLOR.B = 0x00;
  DEFAULT_CURSOR_COLOR.A = 0xDD;
  
  OTHER_TEXT_COLOR.R = 0x40;
  OTHER_TEXT_COLOR.G = 0x40;
  OTHER_TEXT_COLOR.B = 0x40;
  OTHER_TEXT_COLOR.A = 0xFF;
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
  char *themes_dir = StrApp((char**)NULL, DATADIR, "themes/", (char*)NULL);
  char *result;
  struct dirent *entry;
  int n_themes = 0;
  char *themes[128];
  time_t epoch;
  struct tm curr_time;
  int i;

  dir= opendir(themes_dir);	
  if (!dir) return strdup("default");

  while ((entry= readdir(dir)))
  {
    char *temp;
    if (!strcmp(entry->d_name, "." )) continue;
    if (!strcmp(entry->d_name, "..")) continue;    
    
    temp = StrApp((char**)NULL, themes_dir, entry->d_name, (char*)NULL);
    if (is_a_directory(temp))
      {
	themes[n_themes] = strdup(entry->d_name);
	n_themes++;
      }
    free(temp);
  }
  closedir(dir);
  free(themes_dir);
  
  if (!n_themes) return strdup("default");
  
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

/* see if we know this guy... */
char *get_welcome_msg(char *username)
{
	char line[256];
	char *welcome_msg = NULL;
	char *user        = NULL;
	char *path;
	FILE *fp;
	
	if (!username) return NULL;

	path = StrApp((char**)NULL, DATADIR, "welcomes", (char*)NULL);
	fp   = fopen("/etc/qingy/welcomes", "r");
	free(path);

	if (fp)
	{
		while (fgets(line, 255, fp))
		{
			user = strtok(line, " \t");
			if(!strcmp(user, username))
			{
				welcome_msg = strdup(strtok(NULL, "\n"));
				break;
			}
		}
		fclose(fp);
	}
	if (!welcome_msg) welcome_msg = strdup("Starting selected session...");

	return welcome_msg;
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

void restore_default_contents(window_t *window)
{
  window->x                 = 0;
  window->y                 = 0;
  window->width             = 0;
  window->height            = 0;
  window->polltime          = 0;
  window->text_size         = LARGE;
  window->text_orientation  = LEFT;
  window->text_color        = &DEFAULT_TEXT_COLOR;
  window->cursor_color      = &DEFAULT_CURSOR_COLOR;
  window->type              = UNKNOWN;
  window->next              = NULL;
  window->content           = NULL;
  window->command           = NULL;
  window->linkto            = NULL;
}

int add_window_to_list(window_t *w)
{
  static window_t *aux = NULL;
  
  if (!w) return 0;
  
  /* there can be only one login, one password and one session window... */
  if (windowsList && (w->type == LOGIN || w->type == PASSWORD || (w->type == COMBO && !strcmp(w->command, "sessions"))))
    { /* we search for an already-defined one */
      window_t *temp = windowsList;
      
      while (temp)
	{
	  if (temp->type == w->type)
	    { /* we overwrite old settings with new ones */
	      temp->x            = w->x;
	      temp->y            = w->y;
	      temp->width        = w->width;
	      temp->height       = w->height;
	      temp->text_size    = w->text_size;
	      temp->text_color   = w->text_color;
	      temp->cursor_color = w->cursor_color;
	      /*
	       * other settings are not used in this kind of window
	       * so we don't bother copying them...
	       */
	      restore_default_contents(w);
	      return 1;
	    }
	  temp = temp->next;
	}
    }
  
  /* Now we are sure that there is only one login and password window */
  if (!aux)
    {
      aux = (window_t *) calloc(1, sizeof(window_t));
      windowsList = aux;
    }
  else
    {
      aux->next = (window_t *) calloc(1, sizeof(window_t));
      aux = aux->next;
    }
  
  aux->type             = w->type;
  aux->x                = w->x;
  aux->y                = w->y;
  aux->width            = w->width;
  aux->height           = w->height;
  aux->polltime         = w->polltime;
  aux->text_size        = w->text_size;
  aux->text_orientation = w->text_orientation;
  aux->command          = strdup(w->command);
  aux->content          = strdup(w->content);
  aux->linkto           = strdup(w->linkto);
  aux->next             = NULL;
  aux->text_color       = w->text_color;	
  aux->cursor_color     = w->cursor_color;
  
  restore_default_contents(w);
  
  return 1;
}

void destroy_windows_list(window_t *w)
{	
  while (w)
    {
      window_t *temp = w;
      w = w->next;
      
      free(temp->command);
      free(temp->content);
      free(temp->linkto);
      if (temp->text_color   != &DEFAULT_TEXT_COLOR)   free(temp->text_color);
      if (temp->cursor_color != &DEFAULT_CURSOR_COLOR) free(temp->cursor_color);
      free(temp);
    }
}

int get_win_type(const char* name)
{
  register int i;
  static char* types[]=
    {
      "(none)", "label", "button", "login", "password", "combo", (char*) NULL
    };
  
  for(i=0; types[i]; ++i)
    if(!strcmp(name, types[i])) return i;
  
  return 0;
}

int check_windows_sanity()
{
  window_t* temp  = windowsList;
  int got_login   = 0;
  int got_passwd  = 0;
  int got_session = 0;
  
  while(temp)
    {
      switch (temp->type)
	{
	case LOGIN:
	  got_login  = 1;
	  break;
	case PASSWORD:
	  got_passwd = 1;
	  break;
	case COMBO:
	  if (temp->command) if (!strcmp(temp->command, "sessions"))
	    {
	      got_session = 1;
	      break;
	    }
	  fprintf(stderr, "Invalid combo window: forbidden command '%s'.\n", temp->command);
	  return 0;
	case BUTTON:
	  if (temp->content && temp->command)
	    {
	      if (!strcmp(temp->command, "halt"       )) break;
	      if (!strcmp(temp->command, "reboot"     )) break;
	      if (!strcmp(temp->command, "sleep"      )) break;
	      if (!strcmp(temp->command, "screensaver")) break;
	    }
	  fprintf(stderr, "Invalid button: command must be one of the following:\n");
	  fprintf(stderr, "halt, reboot, sleep, screensaver\n");
	  fprintf(stderr, "And content must point to button images\n");
	  return 0;
	case LABEL:
	  break;
	default:
	  return 0;
	}
      temp = temp->next;
    }
  if (!got_login || !got_passwd || !got_session) return 0;
  
  return 1;
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
  
  if (!check_windows_sanity())
    {
      fprintf(stderr, "Error in windows configuration:\n");
      fprintf(stderr, "make sure you set up at least login password and session windows!\n");
      return 0;
    }
  
  return 1;
}
