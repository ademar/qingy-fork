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
#include <directfb.h>

#include "load_settings.h"
#include "misc.h"

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
	set_default_colors();
}

int load_settings(void)
{
	FILE *fp = fopen(SETTINGS, "r");
	char tmp[MAX];
	int temp[4];
	int err = 0;

	XSESSIONS_DIRECTORY = XINIT = FONT = NULL;

	if (!fp)
	{
		fprintf(stderr, "load_Settings: settings file not found... using defaults\n");
		set_default_xsession_dir();
		set_default_xinit();
		set_default_font();
		set_default_colors();
		return 0;
	}
	while (fscanf(fp, "%s", tmp) != EOF)
	{
		int found = 0;

		if (strcmp(tmp, "FONT") == 0)
		{
			if (fscanf(fp, "%s", tmp) == EOF)
			{
				err = 1;
				break;
			}
			FONT = (char *) calloc(strlen(DATADIR)+strlen(tmp)+1, sizeof(char));
			strcpy(FONT, DATADIR);
			strcat(FONT, tmp);
			found = 1;
		}
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
			WINDOW_OPACITY = (__u8) temp[0];
			found = 1;
		}
		if (strcmp(tmp, "SELECTED_WINDOW_OPACITY") == 0)
		{
			if (fscanf(fp, "%d", &(temp[0])) == EOF)
			{
				err = 1;
				break;
			}
			SELECTED_WINDOW_OPACITY = (__u8) temp[0];
			found = 1;
		}
		if (strcmp(tmp, "MASK_TEXT_COLOR") == 0)
		{
			if (fscanf(fp, "%d%d%d%d", &(temp[0]), &(temp[1]), &(temp[2]), &(temp[3])) == EOF)
			{
				err = 1;
				break;
			}
			MASK_TEXT_COLOR_R = (__u8) temp[0];
			MASK_TEXT_COLOR_G = (__u8) temp[1];
			MASK_TEXT_COLOR_B = (__u8) temp[2];
			MASK_TEXT_COLOR_A = (__u8) temp[3];
			found = 1;
		}
		if (strcmp(tmp, "TEXT_CURSOR_COLOR") == 0)
		{
			if (fscanf(fp, "%d%d%d%d", &(temp[0]), &(temp[1]), &(temp[2]), &(temp[3])) == EOF)
			{
				err = 1;
				break;
			}
			TEXT_CURSOR_COLOR_R = (__u8) temp[0];
			TEXT_CURSOR_COLOR_G = (__u8) temp[1];
			TEXT_CURSOR_COLOR_B = (__u8) temp[2];
			TEXT_CURSOR_COLOR_A = (__u8) temp[3];
			found = 1;
		}
		if (strcmp(tmp, "OTHER_TEXT_COLOR") == 0)
		{
			if (fscanf(fp, "%d%d%d%d", &(temp[0]), &(temp[1]), &(temp[2]), &(temp[3])) == EOF)
			{
				err = 1;
				break;
			}
			OTHER_TEXT_COLOR_R = (__u8) temp[0];
			OTHER_TEXT_COLOR_G = (__u8) temp[1];
			OTHER_TEXT_COLOR_B = (__u8) temp[2];
			OTHER_TEXT_COLOR_A = (__u8) temp[3];
			found = 1;
		}
		if (!found)
		{
			error();
			fclose(fp);
			return 0;
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
	//set_default_colors();

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
