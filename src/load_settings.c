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

char *DEFAULT_THEME;

void set_default_xsession_dir(void)
{
	XSESSIONS_DIRECTORY = (char *) calloc(19, sizeof(char));
	strcpy(XSESSIONS_DIRECTORY, "/etc/X11/Sessions/");
}

void set_default_xinit(void)
{
	XINIT = (char *) calloc(21, sizeof(char));
	strcpy(XINIT, "/usr/X11R6/bin/xinit");
}

void set_default_font(void)
{
	FONT = (char *) calloc(strlen(DEFAULT_THEME)+11, sizeof(char));
	strcat(FONT, DEFAULT_THEME);
	strcat(FONT, "decker.ttf");
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

void error(char *where)
{
	if (!silent) fprintf(stderr, "load_settings: parse error in %s file... using defaults\n", where);
	if (XSESSIONS_DIRECTORY) free(XSESSIONS_DIRECTORY);
	if (XINIT) free(XINIT);
	if (FONT) free(FONT);
	if (THEME_DIR) free(THEME_DIR);
	XSESSIONS_DIRECTORY = XINIT = FONT = THEME_DIR = NULL;
	set_default_xsession_dir();
	set_default_xinit();
	set_default_font();
	set_default_colors();
	THEME_DIR = (char *) calloc(strlen(DEFAULT_THEME)+1, sizeof(char));
	strcpy(THEME_DIR, DEFAULT_THEME);
}

char *get_random_theme()
{
	DIR *dir;
	struct dirent *entry;
	FILE *fp;
	char *temp = NULL;
	int n_themes = 0;
	char *themes[128];
	char *seed;
	int i;

	dir= opendir(DATADIR);
	if (!dir)
	{
		temp = (char *) calloc(8, sizeof(char));
		strcpy(temp, "default");
		return temp;
	}

	while ((entry= readdir(dir)))
	{
	 	if (!strcmp(entry->d_name, "." )) continue;
		if (!strcmp(entry->d_name, "..")) continue;

		temp = (char *) calloc(strlen(DATADIR)+strlen(entry->d_name)+1, sizeof(char));
		strcpy(temp, DATADIR);
		strcat(temp, entry->d_name);
		if (is_a_directory(temp))
		{
			themes[n_themes] = (char *) calloc(strlen(entry->d_name)+1, sizeof(char));
			strcpy(themes[n_themes], entry->d_name);
			n_themes++;
		}
	}
	closedir(dir);

	/* all this stuff just to get a random number between 0 and n_themes-1 */
	seed = (char *) calloc(strlen(DATADIR)+5, sizeof(char));
	strcpy(seed, DATADIR);
	strcat(seed, "seed");
	fp = fopen(seed, "r");
	if (fp)
	{
		fscanf(fp, "%d", &i);
		fclose(fp);
		srand(i);
	}
	else srand(n_themes);
	i = rand() % n_themes;
	fp = fopen(seed, "w");
	if (fp)
	{
		fprintf(fp, "%d", rand());
		fclose(fp);
	}

	return themes[i];
}

int load_settings(void)
{
	FILE *fp;
	char *theme = NULL;
	char tmp[MAX];
	int temp[4];

	XSESSIONS_DIRECTORY = XINIT = FONT = THEME_DIR = NULL;

	DATADIR = (char *) calloc(12, sizeof(char));
	strcpy(DATADIR, "/etc/qingy/");
	SETTINGS = (char *) calloc(9+strlen(DATADIR), sizeof(char));
	strcpy(SETTINGS, DATADIR);
	strcat(SETTINGS, "settings");
	LAST_USER = (char *) calloc(9+strlen(DATADIR), sizeof(char));
	strcpy(LAST_USER, DATADIR);
	strcat(LAST_USER, "lastuser");
	DEFAULT_THEME = (char *) calloc(9+strlen(DATADIR), sizeof(char));
	strcpy(DEFAULT_THEME, DATADIR);
	strcat(DEFAULT_THEME, "default/");

	fp = fopen(SETTINGS, "r");
	if (!fp)
	{
		if (!silent) fprintf(stderr, "load_Settings: settings file not found...\nusing internal defaults\n");
		set_default_xsession_dir();
		set_default_xinit();
		set_default_font();
		set_default_colors();
		free(DEFAULT_THEME); DEFAULT_THEME = NULL;
		return 0;
	}
	while (fscanf(fp, "%s", tmp) != EOF)
	{
		int err = 0;
		int found = 0;

		if (strcmp(tmp, "XSESSIONS_DIRECTORY") == 0)
		{
			if (fscanf(fp, "%s", tmp) == EOF)
			{
				err = 1;
				break;
			}
			XSESSIONS_DIRECTORY = (char *) calloc(strlen(tmp)+1, sizeof(char));
			strcpy(XSESSIONS_DIRECTORY, tmp);
			found = 1;
		}
		if (strcmp(tmp, "XINIT") == 0)
		{
			if (fscanf(fp, "%s", tmp) == EOF)
			{
				err = 1;
				break;
			}
			XINIT = (char *) calloc(strlen(tmp)+1, sizeof(char));
			strcpy(XINIT, tmp);
			found = 1;
		}
		if (strcmp(tmp, "THEME") == 0)
		{
			if (fscanf(fp, "%s", tmp) == EOF)
			{
				err = 1;
				break;
			}
			if (!strcmp(tmp, "random") || !strcmp(tmp, "RANDOM") || !strcmp(tmp, "Random"))
			{
				char *temp = get_random_theme();
				strcpy(tmp, temp);
				free(temp);
			}
			THEME_DIR = (char *) calloc(strlen(DATADIR)+strlen(tmp)+2, sizeof(char));
			strcpy(THEME_DIR, DATADIR);
			strcat(THEME_DIR, tmp);
			if (THEME_DIR[strlen(THEME_DIR)-1] != '/')
				THEME_DIR[strlen(THEME_DIR)] = '/';
			theme = (char *) calloc(strlen(THEME_DIR)+6, sizeof(char));
			strcpy(theme, THEME_DIR);
			strcat(theme, "theme");
			found = 1;
		}
		if (!found || err)
		{
			error("settings");
			fclose(fp);
			free(DEFAULT_THEME); DEFAULT_THEME = NULL;
			return 0;
		}
	}
	fclose(fp);

	if (!XSESSIONS_DIRECTORY) set_default_xsession_dir();
	if (!XINIT) set_default_xinit();
	if (!theme)
	{
		THEME_DIR = (char *) calloc(strlen(DEFAULT_THEME)+1, sizeof(char));
		strcpy(THEME_DIR, DEFAULT_THEME);
		theme = (char *) calloc(strlen(THEME_DIR)+6, sizeof(char));
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

	BACKGROUND = (char *) calloc(strlen(THEME_DIR)+15, sizeof(char));
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
			FONT = (char *) calloc(strlen(THEME_DIR)+strlen(tmp)+1, sizeof(char));
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
	user = (char *) calloc(strlen(tmp)+1, sizeof(char));
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
	filename = (char *) calloc(strlen(homedir)+8, sizeof(char));
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
	session = (char *) calloc(strlen(tmp)+1, sizeof(char));
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
	filename = (char *) calloc(strlen(homedir)+8, sizeof(char));
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
	free_stuff(8, DATADIR, SETTINGS, LAST_USER, XSESSIONS_DIRECTORY, XINIT, FONT, BACKGROUND, THEME_DIR);
	exit(n);
}
