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

#include "load_settings.h"
#include "misc.h"

#define CHECK(x...) \
{                   \
	err = x;          \
  if(err == EOF)    \
	{                 \
		err = 1;        \
		break;          \
	}                 \
}

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
	FONT = (char *) calloc(strlen(DATADIR)+11, sizeof(char));
	strcpy(FONT, DATADIR);
	strcat(FONT, "decker.ttf");
}

void error(void)
{
	fprintf(stderr, "load_settings: parse error in settings file... using defaults\n");
	if (!!XSESSIONS_DIRECTORY) free(XSESSIONS_DIRECTORY);
	if (!!XINIT) free(XINIT);
	if (!!FONT) free(FONT);
	XSESSIONS_DIRECTORY = XINIT = FONT = NULL;
	set_default_xsession_dir();
	set_default_xinit();
	set_default_font();
}

int load_settings(void)
{
	FILE *fp = fopen(SETTINGS, "r");
	char tmp[MAX];
	int err = 0;

	XSESSIONS_DIRECTORY = XINIT = FONT = NULL;

	if (!fp)
	{
		fprintf(stderr, "load_Settings: settings file not found... using defaults\n");
		set_default_xsession_dir();
		set_default_xinit();
		set_default_font();
		return 0;
	}
	while (fscanf(fp, "%s", tmp) != EOF)
	{
		if (strcmp(tmp, "FONT") == 0)
		{
			CHECK (fscanf(fp, "%s", tmp));
			FONT = (char *) calloc(strlen(DATADIR)+strlen(tmp)+1, sizeof(char));
			strcpy(FONT, DATADIR);
			strcat(FONT, tmp);
		}
		if (strcmp(tmp, "XSESSIONS_DIRECTORY") == 0)
		{
			CHECK (fscanf(fp, "%s", tmp) == EOF);
			XSESSIONS_DIRECTORY = (char *) calloc(strlen(tmp)+1, sizeof(char));
			strcpy(XSESSIONS_DIRECTORY, tmp);
		}
		if (strcmp(tmp, "XINIT") == 0)
		{
			CHECK (fscanf(fp, "%s", tmp) == EOF);
			XINIT = (char *) calloc(strlen(tmp)+1, sizeof(char));
			strcpy(XINIT, tmp);
		}
	}
	fclose(fp);

	if (err)
	{
		error();
		return 0;
	}
	if (XSESSIONS_DIRECTORY == NULL) set_default_xsession_dir();
	if (XINIT == NULL) set_default_xinit();
	if (FONT == NULL) set_default_font();

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
	if (fscanf(fp, "%s", tmp) != 1)
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
