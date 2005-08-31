
/***************************************************************************
                       load_settings.c  -  description
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

#include <getopt.h>
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
#include "keybindings.h"


extern FILE* yyin;
extern int yyparse(void);

char *file_error = NULL;
int   in_theme   = 0;

void initialize_variables(void)
{
  SCREENSAVER_NAME        = NULL;
	AUTOLOGIN_FILE_BASENAME = strdup("qingy-autologin-");
  TEXT_SESSIONS_DIRECTORY = NULL;
  X_SESSIONS_DIRECTORY    = NULL;
	AUTOLOGIN_USERNAME      = NULL;
	AUTOLOGIN_PASSWORD      = NULL;
	AUTOLOGIN_SESSION       = NULL;
	SCREENSAVERS_DIR        = NULL;
	DFB_INTERFACE           = StrApp((char**)NULL, SBINDIR, "qingy-DirectFB", (char*)NULL);
	TMP_FILE_DIR            = strdup("/var/lib/misc");
  BACKGROUND              = NULL;
	THEMES_DIR              = NULL;
  THEME_DIR               = NULL;
  LAST_USER               = NULL;
	SLEEP_CMD               = NULL;
  SETTINGS                = NULL;	
	X_SERVER                = NULL;
  X_ARGS                  = NULL;
  DATADIR                 = NULL;	
  XINIT                   = NULL;
  FONT                    = NULL;
  screensaver_options     = NULL;
  windowsList             = NULL;
	fb_device               = NULL;
	resolution              = NULL;
	DO_AUTOLOGIN            = 0;
	AUTO_RELOGIN            = 0;
  no_shutdown_screen      = 0;
  disable_last_user       = 0;
  hide_last_user          = 0;
  hide_password           = 0;
  silent                  = 1;
	clear_background        = 0;
  SHUTDOWN_POLICY         = EVERYONE;
	LAST_USER_POLICY        = LU_GLOBAL;
	LAST_SESSION_POLICY     = LS_USER;
	GOT_THEME               = 0;
	lock_sessions           = 0;
	retries                 = 0;
	THEME_XRES              = 800;
	THEME_YRES              = 600;
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
	FILE   *fp     = fopen(LAST_USER, "r");
	char   *line   = NULL;
	char   *result = NULL;
	char   *ttystr = NULL;
	size_t  len    = 0;

	if (!fp) return NULL;

	if (getline(&line, &len, fp) == -1)
	{
		fclose(fp);
		return NULL;
	}

	if (LAST_USER_POLICY == LU_GLOBAL)
	{
		char temp[strlen(line) + 1];
		int items = sscanf(line, "%s", temp);

		fclose(fp);
		free(line);

		if (items != 1)
			return NULL;

		return strdup(temp);
	}

	ttystr = int_to_str(current_tty);

	while (1)
	{
		int  size = strlen(line) + 1;
		char user[size];
		char tty [size];
		int  items = sscanf(line, "%s%s", user, tty);

		if (items == 0)
			break;

		if (items == 2)
			if (!strcmp(tty, ttystr))
			{
				result = strdup(user);
				break;
			}

		if (getline(&line, &len, fp) == -1)
			break;
	}

	fclose(fp);
	free(line);
	free(ttystr);

	return result;
}

int set_last_user(char *user)
{
	char   *fileOUT = StrApp((char**)NULL, LAST_USER, "-new", (char*)NULL);
	char   *line    = NULL;
	size_t  len     = 0;
  FILE   *fpIN;
	FILE   *fpOUT;
  
  if (!user)
	{
		free(fileOUT);
		return 0;
	}

	fpIN  = fopen(LAST_USER, "r");
	fpOUT = fopen(fileOUT,   "w");

	if (!fpOUT)
	{
		if (fpIN) fclose(fpIN);
		free(fileOUT);

		return 0;
	}

	fprintf(fpOUT, "%s %d\n", user, current_tty);

	if (fpIN)
	{
		while (getline(&line, &len, fpIN) != -1)
		{
			char name[strlen(line) + 1];
			int tty;

			if (sscanf(line, "%s%d", name, &tty) == 2)
				if (current_tty != tty)
					fprintf(fpOUT, "%s", line);
		}

		fclose(fpIN);
	}

	fclose(fpOUT);
	remove(LAST_USER);
	rename(fileOUT, LAST_USER);
	free(fileOUT);
  
  return 1;
}

char *get_last_session(char *user)
{
  FILE   *fp;
	char   *result   = NULL;
  char   *filename = NULL;
	char   *line     = NULL;
	size_t  len      = 0;


	if (LAST_SESSION_POLICY == LS_TTY)
	{
		filename = (char *) calloc(strlen(TMP_FILE_DIR)+20, sizeof(char));
  
		strcpy(filename, TMP_FILE_DIR);
		if (filename[strlen(filename)-1] != '/') strcat(filename, "/");
		strcat(filename, "qingy-lastsessions");
	}

	if (LAST_SESSION_POLICY == LS_USER)
	{
		char *homedir;

		if (!user) return NULL;

		homedir  = get_home_dir(user);
		if (!homedir) return NULL;

		filename = (char *) calloc(strlen(homedir)+8, sizeof(char));
  
		strcpy(filename, homedir);
		free(homedir);
		if (filename[strlen(filename)-1] != '/') strcat(filename, "/");
		strcat(filename, ".qingy");
	}

	fp = fopen(filename, "r");
	free(filename);
	if (!fp) return NULL;

	if (LAST_SESSION_POLICY == LS_USER)
		if (getline(&line, &len, fp) != -1)
			result = line;

	if (LAST_SESSION_POLICY == LS_TTY)
	{
		char *ttystr    = int_to_str(current_tty);
		int   lenttystr = strlen(ttystr);
		int   lenline;

		while ((lenline=getline(&line, &len, fp)) != -1)
			if (!strncmp(line, ttystr, lenttystr))
			{
				result = strndup(line + lenttystr + 1, lenline - lenttystr - 2);
				break;
			}
		free(line);
		free(ttystr);
	}

	fclose(fp);

	return result;
}

void set_last_session(char *user, char *session, int tty)
{
  if (!session) return;

	/* we write last session in user home dir */
	if (user)
	{
		char *filename;
		FILE *fp;
		char *homedir = get_home_dir(user);

		if (homedir)
		{
			filename = (char *) calloc(strlen(homedir)+8, sizeof(char));
			strcpy(filename, homedir);
			free(homedir);
			if (filename[strlen(filename)-1] != '/') strcat(filename, "/");
			strcat(filename, ".qingy");
			fp = fopen(filename, "w");
			free(filename);

			if (fp) 
			{		
				fprintf(fp, "%s", session);
				fclose(fp);
			}
		}
	}

	/* we write last session in tty_last_session file */
	if (tty)
	{
		char   *ttystr      = int_to_str(tty);
		int     lenttystr   = strlen(ttystr);
		char   *filenamein  = (char *) calloc(strlen(TMP_FILE_DIR)+20, sizeof(char));
		char   *filenameout = (char *) calloc(strlen(TMP_FILE_DIR)+24, sizeof(char));
		char   *line        = NULL;
		size_t  len         = 0;
		FILE   *filein;
		FILE   *fileout;

		strcpy(filenamein, TMP_FILE_DIR);
		if (filenamein[strlen(filenamein)-1] != '/') strcat(filenamein, "/");
		strcpy(filenameout, filenamein);
		strcat(filenamein,  "qingy-lastsessions");
		strcat(filenameout, "qingy-lastsessions-new");
		filein  = fopen(filenamein,  "r");
		fileout = fopen(filenameout, "w");

		if (!fileout) 
		{
			if (filein) fclose(filein);
			remove(filenameout);
			free(filenamein);
			free(filenameout);
			free(ttystr);
			return;
		}

		if (filein)
		{
			while (getline(&line, &len, filein) != -1)
				if (strncmp(line, ttystr, lenttystr))
					fputs(line, fileout);

			fclose(filein);
		}

		fprintf(fileout, "%s %s\n", ttystr, session);

		fclose(fileout);
		remove(filenamein);
		rename(filenameout, filenamein);

		free(filenamein); free(filenameout);
		free(ttystr);
		if (line) free(line);
	}
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
	static int first_time = 1;

	if (!first_time)
		destroy_keybindings_list();

	first_time = 0;

  DATADIR   = strdup(SETTINGS_DIR "/");
  SETTINGS  = StrApp((char**)NULL, DATADIR, "settings", (char*)NULL);
  LAST_USER = StrApp((char**)NULL, TMP_FILE_DIR, "/qingy-lastuser", (char*)NULL);  
  
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

int ParseCMDLine(int argc, char *argv[], int paranoia)
{
	extern char *optarg;
	extern int optind, opterr, optopt;
	const char optstring[] = "-f:pldvns:rh";
	const struct option longopts[] =
	{
		{"fb-device",               required_argument, NULL, 'f'},
		{"hide-password",           no_argument,       NULL, 'p'},
		{"hide-lastuser",           no_argument,       NULL, 'l'},
		{"disable-lastuser",        no_argument,       NULL, 'd'},
		{"verbose",                 no_argument,       NULL, 'v'},
		{"no-shutdown-screen",      no_argument,       NULL, 'n'},
		{"screensaver",             required_argument, NULL, 's'},
		{"help",                    no_argument,       NULL, 'h'},
		{0, 0, 0, 0}
	};
  char *tty;
  int our_tty_number;

	if (!paranoia)
		opterr = 0;
	else
		if (argc < 2)	Error(1);

  tty= argv[1];
	if (paranoia)
	{
		if (!strcmp(tty, "-h") || !strcmp(tty, "--help"))
		{
			/*
			 * Print usage info...
			 * I put this here also as it would never have a chance
			 * of being parsed by getopt_long() because of the
			 * checks below (if passed as first argument)
			 */
			PrintUsage();
			exit(EXIT_SUCCESS);
		}
		if (strncmp(tty, "tty", 3)) Error(1);
	}
  our_tty_number= atoi(tty+3);
	if (paranoia)
		if ( (our_tty_number < 1) || (our_tty_number > 63) )
		{
			fprintf(stderr, "tty number must be > 0 and < 64\n");
			Error(1);
		}

	while (1)
	{
		int retval = getopt_long(argc, argv, optstring, longopts, NULL);

		if (retval == -1) break;
		switch (retval)
		{
			case 'f': /* use this framebuffer device */
				if (paranoia) fb_device = strdup(optarg);
				break;
			case 'p': /* hide password */
				hide_password = 1;
				break;
			case 'l': /* hide lastuser */
				hide_last_user = 1;
				break;
			case 'd': /* disable lastuder */
				disable_last_user = 1;
				break;
			case 'v': /* verbose */
				silent = 0;
				break;
			case 'n': /* no shutdown screen */
				no_shutdown_screen = 1;
				break;
			case 's': /* screen_saver */
			{
				int temp = atoi(optarg);
				if (paranoia && temp < 0)
				{
					Switch_TTY;
					ClearScreen();
					fprintf(stderr, "%s: invalid screen saver timeout: fall back to text mode.\n", program_name);
					Error(0);
				}
				if (!temp)
				{
					use_screensaver = 0;
					break;
				}
				use_screensaver = 1;
				screensaver_timeout = temp;
				break;
			}
			case 'r': /* use this framebuffer resolution */
				if (paranoia) resolution = get_resolution(optarg);
				break;
			case 'h': /* Print usage info... */
				PrintUsage();
				exit(EXIT_SUCCESS);
				break;
			case 1: /* not an option-like arg... we ignore it */
				break;
			default:
				if (paranoia)
				{
					Switch_TTY;
					ClearScreen();
					fprintf(stderr, "%s: error in command line options: fall back to text mode.\n", program_name);
					Error(0);
				}
		}
	}
  
  return our_tty_number;
}
