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

void set_default_settings()
{
	XSESSIONS_DIRECTORY = (char *) calloc(19, sizeof(char));
	strcpy(XSESSIONS_DIRECTORY, "/etc/X11/Sessions/");
	XINIT = (char *) calloc(21, sizeof(char));
	strcpy(XINIT, "/usr/X11R6/bin/xinit");
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
	set_default_settings();
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
		set_default_settings();
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
	return 1;
}
