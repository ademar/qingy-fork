/***************************************************************************
                       load_settings.c  -  description
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

char *DEFAULT_THEME;

void set_default_session_dirs(void)
{
  X_SESSIONS_DIRECTORY = strdup("/etc/X11/Sessions/");
  TEXT_SESSIONS_DIRECTORY = strdup("/etc/qingy/sessions");
}

void set_default_xinit(void)
{
  XINIT = strdup("/usr/X11R6/bin/xinit");
}

void set_default_font(void)
{
  FONT = StrApp((char**)NULL, DEFAULT_THEME, "decker.ttf",(char *)NULL);
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

void yyerror(char *where)
{
  if (!silent) fprintf(stderr, "qingy: parse error in %s file... reverting to text mode.\n", where);
  if (X_SESSIONS_DIRECTORY) free(X_SESSIONS_DIRECTORY);
  if (TEXT_SESSIONS_DIRECTORY) free(TEXT_SESSIONS_DIRECTORY);
  if (XINIT) free(XINIT);
  if (FONT) free(FONT);
  if (THEME_DIR) free(THEME_DIR);
  TEXT_SESSIONS_DIRECTORY = X_SESSIONS_DIRECTORY = XINIT = FONT = THEME_DIR = NULL;
  set_default_session_dirs();
  set_default_xinit();
  set_default_font();
  set_default_colors();
  THEME_DIR = (char *) my_calloc(strlen(DEFAULT_THEME)+1, sizeof(char));
  strcpy(THEME_DIR, DEFAULT_THEME);
}

char *get_random_theme()
{
  DIR *dir;
  char *themes_dir = StrApp((char**)0, DATADIR, "themes/", (char*)0);
  struct dirent *entry;
  char *temp = NULL;
  int n_themes = 0;
  char *themes[128];
  time_t epoch;
  struct tm curr_time;
  int i;

  dir= opendir(themes_dir);
  if (!dir)
  {
    temp = (char *) my_calloc(8, sizeof(char));
    strcpy(temp, "default");
    return temp;
  }

  while ((entry= readdir(dir)))
  {
    if (!strcmp(entry->d_name, "." )) continue;
    if (!strcmp(entry->d_name, "..")) continue;    

    temp = (char *) my_calloc(strlen(themes_dir)+strlen(entry->d_name)+1, sizeof(char));
    strcpy(temp, themes_dir);
    strcat(temp, entry->d_name);
    if (is_a_directory(temp))
    {
      themes[n_themes] = (char *) my_calloc(strlen(entry->d_name)+1, sizeof(char));
      strcpy(themes[n_themes], entry->d_name);
      n_themes++;
    }
  }
  closedir(dir);

  /* let's create a random number between 0 and n_themes-1 */
  epoch = time(NULL);
  localtime_r(&epoch, &curr_time);
  srand(curr_time.tm_sec);
  i = rand() % n_themes;

  return themes[i];
}

int load_settings(void)
{
 
  TEXT_SESSIONS_DIRECTORY = X_SESSIONS_DIRECTORY = XINIT = FONT = THEME_DIR = NULL;

  DATADIR = (char *) my_calloc(12, sizeof(char));
  strcpy(DATADIR, "/etc/qingy/");
  SETTINGS = (char *) my_calloc(9+strlen(DATADIR), sizeof(char));
  strcpy(SETTINGS, DATADIR);
  strcat(SETTINGS, "settings");
  LAST_USER = (char *) my_calloc(9+strlen(DATADIR), sizeof(char));
  strcpy(LAST_USER, DATADIR);
  strcat(LAST_USER, "lastuser");
  DEFAULT_THEME = (char *) my_calloc(16+strlen(DATADIR), sizeof(char));
  strcpy(DEFAULT_THEME, DATADIR);
  strcat(DEFAULT_THEME, "themes/default/");

  yyin = fopen(SETTINGS, "r");
  if (!yyin)
  {
    if (!silent) fprintf(stderr, "load_Settings: settings file not found...\nusing internal defaults\n");
    set_default_session_dirs();
    set_default_xinit();
    set_default_font();
    set_default_colors();
    free(DEFAULT_THEME); DEFAULT_THEME = NULL;
    return 0;
  }
  yyparse();
  fclose(yyin);

  if (!X_SESSIONS_DIRECTORY) set_default_session_dirs();
  if (!XINIT) set_default_xinit();
  if (!theme)
  {
    THEME_DIR = (char *) my_calloc(strlen(DEFAULT_THEME)+1, sizeof(char));
    strcpy(THEME_DIR, DEFAULT_THEME);
    theme = (char *) my_calloc(strlen(THEME_DIR)+6, sizeof(char));
    strcpy(theme, THEME_DIR);
    strcat(theme, "theme");
  }

  fp = fopen(theme, "r");
  if (!fp)
  {
    if (!silent) fprintf(stderr, "load_settings: selected theme does not exist. Using internal defaults\n");
    error("");
    free(DEFAULT_THEME); DEFAULT_THEME = NULL;
    return 0;
  }

  BACKGROUND = (char *) my_calloc(strlen(THEME_DIR)+15, sizeof(char));
  strcpy(BACKGROUND, THEME_DIR);
  strcat(BACKGROUND, "background.png");
  while (fscanf(fp, "%s", tmp) != EOF)
  {
    int err = 0;
    int found = 0;

    if (strcmp(tmp, "FONT") == 0)
    {
      if (fscanf(fp, "%s", tmp) == EOF)
      {
				err = 1;
				break;
      }
      FONT = (char *) my_calloc(strlen(THEME_DIR)+strlen(tmp)+1, sizeof(char));
      strcpy(FONT, THEME_DIR);
      strcat(FONT, tmp);
      found = 1;
    }
    if (strcmp(tmp, "BUTTON_OPACITY") == 0)
    {
      if (fscanf(fp, "%d", &(temp[0])) == EOF)
      {
				err = 1;
				break;
      }
      BUTTON_OPACITY = temp[0];
      found = 1;
    }
    if (strcmp(tmp, "WINDOW_OPACITY") == 0)
    {
      if (fscanf(fp, "%d", &(temp[0])) == EOF)
      {
				err = 1;
				break;
      }
      WINDOW_OPACITY = temp[0];
      found = 1;
    }
    if (strcmp(tmp, "SELECTED_WINDOW_OPACITY") == 0)
    {
      if (fscanf(fp, "%d", &(temp[0])) == EOF)
      {
				err = 1;
				break;
      }			
      SELECTED_WINDOW_OPACITY = temp[0];
      found = 1;
    }
    if (strcmp(tmp, "MASK_TEXT_COLOR") == 0)
    {
      if (fscanf(fp, "%d%d%d%d", &(temp[0]), &(temp[1]), &(temp[2]), &(temp[3])) == EOF)
      {
				err = 1;
				break;
      }
      MASK_TEXT_COLOR_R = temp[0];
      MASK_TEXT_COLOR_G = temp[1];
      MASK_TEXT_COLOR_B = temp[2];
      MASK_TEXT_COLOR_A = temp[3];
      found = 1;
    }
    if (strcmp(tmp, "TEXT_CURSOR_COLOR") == 0)
    {
      if (fscanf(fp, "%d%d%d%d", &(temp[0]), &(temp[1]), &(temp[2]), &(temp[3])) == EOF)
      {
				err = 1;
				break;
      }
      TEXT_CURSOR_COLOR_R = temp[0];
      TEXT_CURSOR_COLOR_G = temp[1];
      TEXT_CURSOR_COLOR_B = temp[2];
      TEXT_CURSOR_COLOR_A = temp[3];
      found = 1;
    }
    if (strcmp(tmp, "OTHER_TEXT_COLOR") == 0)
    {
      if (fscanf(fp, "%d%d%d%d", &(temp[0]), &(temp[1]), &(temp[2]), &(temp[3])) == EOF)
      {
				err = 1;
				break;
      }
      OTHER_TEXT_COLOR_R = temp[0];
      OTHER_TEXT_COLOR_G = temp[1];
      OTHER_TEXT_COLOR_B = temp[2];
      OTHER_TEXT_COLOR_A = temp[3];
      found = 1;
    }
    if (!found || err)
    {
      error("theme");
      fclose(fp);
      free(DEFAULT_THEME); DEFAULT_THEME = NULL;
      return 0;
    }
  }
  fclose(fp);

  if (FONT == NULL) set_default_font();
  free(DEFAULT_THEME); DEFAULT_THEME = NULL;

  return 1;
}

char *get_last_user(void)
{
  FILE *fp = fopen(LAST_USER, "r");
  char tmp[MAX];
  char *user;

  if (!fp) return NULL;
  if (fscanf(fp, "%s", tmp) != 1)
  {
    fclose(fp);
    return NULL;
  }
  fclose(fp);
  user = (char *) my_calloc(strlen(tmp)+1, sizeof(char));
  strcpy(user, tmp);

  return user;
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
  char *filename;
  char *session;
  char tmp[MAX];
  FILE *fp;

  if (!user) return NULL;
  homedir = get_home_dir(user);
  if (!homedir) return NULL;
  filename = (char *) my_calloc(strlen(homedir)+8, sizeof(char));
  strcpy(filename, homedir);
  if (filename[strlen(filename)-1] != '/') strcat(filename, "/");
  strcat(filename, ".qingy");
  fp = fopen(filename, "r");
  if (!fp) return NULL;
  if (get_line(tmp, fp, MAX) == 0)
  {
    fclose(fp);
    return NULL;
  }
  fclose(fp);
  session = (char *) my_calloc(strlen(tmp)+1, sizeof(char));
  strcpy(session, tmp);

  return session;
}

int set_last_session(char *user, char *session)
{
  char *homedir;
  char *filename;
  FILE *fp;

  if (!user || !session) return 0;
  homedir = get_home_dir(user);
  if (!homedir) return 0;
  filename = (char *) my_calloc(strlen(homedir)+8, sizeof(char));
  strcpy(filename, homedir);
  if (filename[strlen(filename)-1] != '/') strcat(filename, "/");
  strcat(filename, ".qingy");
  fp = fopen(filename, "w");
  if (!fp) return 0;
  fprintf(fp, "%s", session);
  fclose(fp);

  return 1;
}

void my_exit(int n)
{
	/* We reenable VT switching if it is disabled */
	unlock_tty_switching();

  free_stuff(8, DATADIR, SETTINGS, LAST_USER, TEXT_SESSIONS_DIRECTORY, X_SESSIONS_DIRECTORY, XINIT, FONT, BACKGROUND, THEME_DIR);
  exit(n);
}
