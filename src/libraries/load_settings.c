
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>

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

#include "memmgmt.h"
#include "load_settings.h"
#include "misc.h"
#include "vt.h"

extern FILE* yyin;
extern int yyparse(void);

char *file_error = NULL;
int   in_theme   = 0;

void initialize_variables(void)
{
  SCREENSAVER             = NULL;  
  TEXT_SESSIONS_DIRECTORY = NULL;
  X_SESSIONS_DIRECTORY    = NULL;
	SCREENSAVERS_DIR        = NULL;
	DFB_INTERFACE           = StrApp((char**)NULL, SBINDIR, "qingy-DirectFB", (char*)NULL);
  BACKGROUND              = NULL;
	THEMES_DIR              = NULL;
  THEME_DIR               = NULL;
  LAST_USER               = NULL;
  SETTINGS                = NULL;	
	X_SERVER                = NULL;
  DATADIR                 = NULL;	
  XINIT                   = NULL;
  FONT                    = NULL;
  screensaver_options     = NULL;
  windowsList             = NULL;
  no_shutdown_screen      = 0;
  disable_last_user       = 0;
  hide_last_user          = 0;
  hide_password           = 0;
  silent                  = 1;
	clear_background        = 0;
  SHUTDOWN_POLICY         = EVERYONE;
  THEME_WIDTH             = 800;
  THEME_HEIGHT            = 600;
	GOT_THEME               = 0;
	lock_sessions           = 0;
#ifdef USE_SCREEN_SAVERS
	screensaver_timeout     = 5;
	use_screensaver         = 1;
#else
	screensaver_timeout     = 0;
	use_screensaver         = 0;
#endif
}

void set_default_paths(void)
{
  TEXT_SESSIONS_DIRECTORY = strdup("/etc/qingy/sessions");
	X_SESSIONS_DIRECTORY    = strdup("/etc/X11/Sessions/"); 
  XINIT                   = strdup("/usr/X11R6/bin/xinit");
	SCREENSAVERS_DIR        = strdup("/usr/lib/qingy/screensavers");
	THEMES_DIR              = strdup("/usr/share/qingy/themes");
	/*
	 * (michele): no need to check strdup return values,
	 *            it is done automatically in memmgmt.c!
	 */  
}

void erase_options(void)
{
	while (screensaver_options)
	{
		struct _screensaver_options *temp = screensaver_options;
		screensaver_options = screensaver_options->next;
		free(temp->option);
		free(temp);
	}
}

void add_to_options(char *option)
{
#ifdef USE_SCREEN_SAVERS
  static struct _screensaver_options *temp = NULL;
  
  if (!option) return;

	/* the following is necessary to detect that options list got cleared */
	if (!screensaver_options) temp = NULL;	

  if (!temp)
	{
		screensaver_options = (struct _screensaver_options *) calloc(1, sizeof(struct _screensaver_options));
		temp = screensaver_options;
	}
  else
	{
		temp->next = (struct _screensaver_options *) calloc(1, sizeof(struct _screensaver_options));
		temp = temp->next;
	}
  
  temp->option = strdup(option);
  temp->next = NULL;
  if (!silent) fprintf(stderr, "Added '%s' to screen saver options...\n", option);
#else  /* no screensaver support */
	if (option) {} /* only to avoid compiler warnings */
#endif
}

char *get_random_theme()
{
  DIR *dir;
  char *themes_dir = StrApp((char**)NULL, THEMES_DIR, "/", (char*)NULL);
  char *result;
  struct dirent *entry;
  int n_themes = 0;
  char *themes[128];
  int i;

  dir= opendir(themes_dir);	
  if (!dir)
	{		
    /* perror("Qingy error"); */
		/* This is not a qingy error ;-P */
		fprintf(stderr, "qingy: get_random_theme(): cannot open directory \"%s\"!\n", themes_dir);
		free(themes_dir);
    return strdup("default");
  }

  while ((entry= readdir(dir)))
  {
		char *temp;
		/*
			To the genius who wrote this snippet:
			if(!entry){
			  perror("Qingy error");
			  break;
			}
			It will never be executed!
			(See above while() condition ;-P)
		*/
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
	/* If the opendir() some lines above didn't fail,
	 * there is no reason to think closedir() will:
	 * if(closedir(dir)== -1)
	 *   perror("Qingy error");
	 */
  free(themes_dir);
  
  if (!n_themes) return strdup("default");
  
  /* let's create a random number between 0 and n_themes-1 */
  srand((unsigned int)time(NULL));
  i = rand() % n_themes;
  
  result = strdup(themes[i]);
  for (i=0; i<n_themes; i++) free(themes[i]);
  
  return result;
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
	free(SCREENSAVERS_DIR);
	free(THEMES_DIR);
	set_default_paths();
  THEME_DIR = StrApp((char**)NULL, THEMES_DIR, "/default/", (char*)NULL);
}

char *get_last_user(void)
{
  FILE *fp = fopen(LAST_USER, "r");
  char tmp[MAX];  
  
	/*
	 * no point in printing out an error:
	 * it just means that no user has
	 * logged in with qingy, yet
	 */
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
	/*
	 * no point in printing out an error:
	 * it just means that on next launch
	 * we will ask for user name, too
	 */
  if (!fp) return 0;
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
  if (!fp)
    /* perror("Qingy error"); */
		/* (MICHELE): Bloody hell, this is NOT an error!
		 * it just means that there is no previously recorded
		 * session for this user!
		 */
    return NULL;
  if (!get_line(tmp, fp, MAX))
	{
		/* (MICHELE) if it did open it,
		 * it will also close it without problems!
		 * if(fclose(fp)==EOF)
		 * perror("Qingy error");
		 */
		fclose(fp);
		return NULL;
	}
  fclose(fp);
  return strdup(tmp);
}

int set_last_session(char *user, char *session)
{
  char *homedir;
  char *filename;
  FILE *fp;
  
  if (!user || !session) return 0;
	homedir = get_home_dir(user);
  if (!homedir) return 0;
  
  filename = (char *) calloc(strlen(homedir)+8, sizeof(char));
  strcpy(filename, homedir);
  free(homedir);
  if (filename[strlen(filename)-1] != '/') strcat(filename, "/");
  strcat(filename, ".qingy");
  fp = fopen(filename, "w");
  free(filename);
  
  if (!fp) 
	{		
		/* (MICHELE): should not happen, but hell!
		 * who cares if it does?
		 * perror("Qingy error");
		 */
		return 0;
	}

  fprintf(fp, "%s", session);
  fclose(fp);

  return 1;
}

/* see if we know this guy... */
char *get_welcome_msg(char *username)
{
  char line[256];
	struct passwd *pw;
  char *welcome_msg = NULL;
  char *user        = NULL;
  char *path;
  FILE *fp;	
  
  if (!username) return NULL;

  /* see if this guy has a .qingy_welcome in the home */
  pw   = getpwnam(username);
  path = StrApp((char **)NULL, pw->pw_dir, "/.qingy_welcome", (char*)NULL);
  if (!access(path, F_OK))
	{
    fp = fopen(path, "r");
    free(path);
		if (fp)
		{
			fgets(line, 255, fp);
			welcome_msg = strdup(strtok(line, "\n"));
			fclose(fp);
			if (welcome_msg) return welcome_msg;
		}
  }
  path = StrApp((char**)NULL, DATADIR, "welcomes", (char*)NULL);
  fp   = fopen(path, "r");
  free(path);
  
  if (fp)
	{
		while (fgets(line, 255, fp))
		{
			user = strtok(line, " \t");
			if (!strcmp(user, username))
	    {
	      welcome_msg = strdup(strtok(NULL, "\n"));
	      break;
	    }
		}
		fclose(fp);
	}
  if (!welcome_msg) return strdup("Starting selected session...");

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
		length = (size_t)(temp - begin);
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

	/*
	 * this seems redundant, but it is not: if we reset theme (i.e. global theme is "bleargh"
	 * but for tty3 theme "urgh" is selected) we need to clear this also...
	 */
	if (!windowsList) aux = NULL;  
  
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
				if (temp->content)
				{
					if (!temp->command) break;
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
  DATADIR   = strdup(SETTINGS_DIR "/");
  SETTINGS  = StrApp((char**)NULL, DATADIR, "settings", (char*)NULL);
  LAST_USER = StrApp((char**)NULL, "/var/lib/misc/qingy-lastuser", (char*)NULL);  
  
  yyin = fopen(SETTINGS, "r");
  if (!yyin)
	{
		fprintf(stderr, "load_settings: settings file not found: reverting to text mode\n");
		return 0;
	}

  file_error = SETTINGS;
  yyparse();
  fclose(yyin);
  file_error = NULL;
  
  if (!TEXT_SESSIONS_DIRECTORY ||
			!X_SESSIONS_DIRECTORY    ||
			!XINIT                   ||
			!SCREENSAVERS_DIR        ||
			!THEMES_DIR)
	{
		fprintf(stderr, "load_settings: warning: you left some variables undefined\n");
		fprintf(stderr, "in settings file, anomalies may occur!\n");
	}

  if (!GOT_THEME)
	{
		fprintf(stderr, "load_settings: cannot proceed without a theme!\n");
		return 0;
	}

  if (!check_windows_sanity())
	{
		fprintf(stderr, "Error in windows configuration:\n");
		fprintf(stderr, "make sure you set up at least login password and session windows!\n");
		return 0;
	}

	if (!silent)
		fprintf(stderr, "Session locking is%s enabled.\n", (lock_sessions) ? "" : " NOT");
  
  return 1;
}
