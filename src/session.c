/***************************************************************************
                         session.c  -  description
                            -------------------
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
#include <dirent.h>

#include "session.h"


int how_many_sessions(void)
{
	DIR *dir;
	int i=-2;

	dir= opendir(XSESSIONS_DIRECTORY);
	if (dir == NULL)
	{
	  fprintf(stderr, "session: unable to open directory \"%s\"\n", XSESSIONS_DIRECTORY);
		return 0;
	}
	while ( readdir(dir) != NULL ) i++;
	closedir(dir);
	if (!i) fprintf(stderr, "session: directory \"%s\" is empty\n", XSESSIONS_DIRECTORY);

	return i;
}

char **get_sessions(int max)
{
	char **sessions;
	struct dirent *entry;
	DIR *dir;
	int i=0;

	sessions = (char **) calloc(max, sizeof(char *));
	dir= opendir(XSESSIONS_DIRECTORY);
	if (dir == NULL)
	{
	  fprintf(stderr, "session: unable to open directory \"%s\"\n", XSESSIONS_DIRECTORY);
		sessions[0] = NULL;
		return sessions;
	}
	while ((entry= readdir(dir)) != NULL)
	{
	  if ( (strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) )
		{
			if (i > max) break;
			sessions[i] = (char *) calloc(strlen(entry->d_name)+1, sizeof(char));
			strcpy(sessions[i], entry->d_name);
			i++;
		}
	}
	closedir(dir);

	return sessions;
}

/* Start the session of your choice.
   Returns 0 if starting failed      */
int start_session(int session)
{
  if (session) {}

	return 0;
}
