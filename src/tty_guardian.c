/***************************************************************************
                       tty_guardian.c  -  description
                            --------------------
    begin                : Feb 04 2004
    copyright            : (C) 2004 by Noberasco Michele
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
#include <sys/wait.h>
#include <time.h>

#include "misc.h"
#include "chvt.h"

#define RETURN                   \
{                                \
	(void)waitpid(child, NULL, 0); \
	return;                        \
}

/*
 * guard specified ttys from unauthorized access
 * fence1 is text tty, fence2, if present is X tty
 */
void ttyWatchDog(pid_t child, char *dog_master, int fence1, int fence2);
/* check wether user has auth to visit that tty */
void WatchDog_Sniff(char *dog_master, int where_was_intruder, int where_is_intruder);
/* block user until he authenticates */
int WatchDog_Bark(char *dog_master, char *intruder, int where_was_intruder, int where_is_intruder);




/* IMPLEMENTATION */



void ttyWatchDog(pid_t child, char *dog_master, int fence1, int fence2)
{
	struct timespec delay;
	int where_is_intruder  = 0;
	int where_was_intruder = 0;

	if (!child)    return;
	if (!dog_master) RETURN;
	if (!fence1 && !fence2) RETURN;

	/* We set up a delay of 0.5 seconds */
  delay.tv_sec  = 0;
  delay.tv_nsec = 500000000;	/* that's 500M */

	/* Main loop: we wait until the user switches to the tty we are guarding */
  while (waitpid(child, NULL, WNOHANG) != child)
	{
		if (!where_was_intruder) where_was_intruder = get_active_tty();
		else where_was_intruder = where_is_intruder;
		where_is_intruder = get_active_tty();
		if (where_is_intruder == -1)
		{
			fprintf(stderr, "\nfatal error: cannot get active tty number!\n");
			abort();
		}
		if (where_is_intruder == fence1)
		{ /* if an X session is active and user passed control, we send him there */
			WatchDog_Sniff(dog_master, where_was_intruder, fence1);
			if (fence2) if (!set_active_tty(fence2)) abort();
		}
		if (where_is_intruder == fence2) WatchDog_Sniff(dog_master, where_was_intruder, fence2);
		nanosleep(&delay, NULL); /* wait a little before checking again */
	}
}

void WatchDog_Sniff(char *dog_master, int where_was_intruder, int where_is_intruder)
{
	static int already_sniffed = 0;
	int retval;
	char *intruder;

	if (already_sniffed == where_was_intruder) return;

	intruder = get_tty_owner(where_was_intruder);
	if (!strcmp(intruder, dog_master) || !strcmp(intruder, "root"))
	{
		already_sniffed = where_was_intruder;
		free(intruder);
		return;
	}

	already_sniffed = 0;
	retval = WatchDog_Bark(dog_master, intruder, where_was_intruder, where_is_intruder);	
	free(intruder);

	if (!retval)
	{ /* user has no right to be here */
		if (!set_active_tty(where_was_intruder)) abort();
		return;
	}

	already_sniffed = where_was_intruder;
}

int WatchDog_Bark (char *dog_master, char *intruder, int where_was_intruder, int where_is_intruder)
{
	fprintf(stderr, "authenticate yourself, %s\n", intruder);
	sleep(2);
	return 0;
}
