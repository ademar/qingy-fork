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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "misc.h"
#include "chvt.h"
#include "session.h"
#include "load_settings.h"

#define WAIT_N_RETURN                   \
{                                       \
	(void)waitpid(child, NULL, 0);        \
	return;                               \
}

#define ABORT_N_RETURN                                                                 \
{                                                                                      \
	fprintf(stderr, "%s: fatal error: cannot set terminal attributes!\n", program_name); \
	return NULL;                                                                         \
}

#ifndef TCSASOFT
#define TCSASOFT 0
#endif

#define PASSWD_MAX 128



/* read user password without echoing it */
char *read_password(int tty)
{
	char *device = create_tty_name(tty);
	int fd = open(device, O_RDONLY);
	struct termios attr;
	char buf[PASSWD_MAX];
	char *retval;
	int pos = 0;
	char c;	

	free(device);	
	if (fd == -1) return NULL;

	/* we disable input echoing */
	if (tcgetattr(fd, &attr) == -1) ABORT_N_RETURN;
	attr.c_lflag &= ~(ECHO|ISIG);
	if (tcsetattr(fd, TCSAFLUSH|TCSASOFT, &attr) == -1) ABORT_N_RETURN;

	/* we read the password */
	while (read(fd, &c, sizeof(char)) > 0)
	{
		if (c == '\n') break;
		if (c == '\0') break;

		buf[pos++] = c;
		if (pos == PASSWD_MAX-1) break;
	}
	buf[pos] = '\0';
	close(fd);
	
	/* we clear the buffer before exiting */
	retval = strdup(buf);
	memset(&buf, 0, PASSWD_MAX*sizeof(char));

	return retval;
}

/* block user until he authenticates */
int WatchDog_Bark (char *dog_master, char *intruder, int our_land)
{
	int dest = get_available_tty();
	char *password;
	int retval;

	if (dest == -1)            return 0;
	if (!dog_master)           return 0;
	if (!intruder)             return 0;
	if (!switch_to_tty(dest))  return 0;
	if (!set_active_tty(dest)) return 0;

	/*
	 * we don't want our intruder to switch to the tty
	 * we are guarding while we ask him the password,
	 * don't we? I'm afraid that our little guard dog
	 * does NOT get distracted by steaks ;-P
	 */
	lock_tty_switching();

	ClearScreen();
	printf("%s, terminal \"/dev/tty%d\" is in use by another user.\n", intruder, our_land);
	printf("Please supply root or tty owner password to continue.\n\nPassword: ");
	fflush(stdout);

	password = read_password(dest);
	printf("\n\nChecking password... ");
	fflush(stdout);

	retval = check_password(dog_master, password);
	if (!retval) retval = check_password("root", password);

	/* a password is sensitive stuff: let's destroy it! */
	memset(password, 0, strlen(password)*sizeof(char));
	free(password);

	if (!retval) printf("Access denied!\n");
	else         printf("Access allowed!\n");
	fflush(stdout);
	sleep(2);
	unlock_tty_switching();
	switch_to_tty(our_land);
	disallocate_tty(dest);
	
	return retval;
}

/* check wether user has auth to visit our tty */
void WatchDog_Sniff(char *dog_master, int where_was_intruder, int where_is_intruder, int send_him_here)
{
	static int already_sniffed = 0;
	int retval;
	char *intruder;
	char *file;

	if (already_sniffed == where_was_intruder) return;

	file = create_tty_name(where_was_intruder);
	intruder = get_file_owner(file);
	free(file);
	if (!strcmp(intruder, dog_master))
	{
		already_sniffed = where_was_intruder;
		free(intruder);
		return;
	}

	if (!strcmp(intruder, "root"))
	{ /*
		 * there are 2 possibilities here:
		 * - tty is unused (thus we cannot know the user)
		 * - tty is in use (probably an X session), thus either:
		 *   - check wether qingy is running in this terminal, then
		 *   - check owner of /dev/vc/<where_was_intruder>
		 */
		if (!is_tty_available(where_was_intruder))
		{ /* tty is in use: we check owner of /dev/vc/<where_was_intruder> */
			char *temp = int_to_str(where_was_intruder);

			free(intruder);
			file = StrApp((char**)NULL, "/dev/vc/", temp, (char*)NULL);
			free(temp);
			intruder = get_file_owner(file);
			free(file);

			if (!strcmp(intruder, dog_master) || !strcmp(intruder, "root"))
			{ /* now we are sure about user identity */
				already_sniffed = where_was_intruder;
				free(intruder);
				return;
			}
		}
		else
		{ /* vt is not in use: we cannot know who intruder is */
			free(intruder);
			intruder = strdup("unknown");
		}
	}

	already_sniffed = 0;
	retval = WatchDog_Bark(dog_master, intruder, where_is_intruder);	
	free(intruder);

	if (!retval)
	{ /* user has no right to be here */
		set_active_tty(where_was_intruder);
		return;
	}

	/* user has authenticated correctly */
	set_active_tty(send_him_here);
	already_sniffed = where_was_intruder;
}

/*
 * guard specified ttys against unauthorized access
 * fence1 is text tty, fence2, if present, is X tty
 */
void ttyWatchDog(pid_t child, char *dog_master, int fence1, int fence2)
{
	struct timespec delay;
	int where_is_intruder  = 0;
	int where_was_intruder = 0;

	if (!child)    return;
	if (!dog_master) WAIT_N_RETURN;
	if (!fence1 && !fence2) WAIT_N_RETURN;

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
		if (where_is_intruder != where_was_intruder)
			if (where_is_intruder == fence1 || where_is_intruder == fence2)
			{ /* if an X session is active user must be sent to X tty, after passing auth */
				if (fence2)
					WatchDog_Sniff(dog_master, where_was_intruder, where_is_intruder, fence2);
				else
					WatchDog_Sniff(dog_master, where_was_intruder, where_is_intruder, fence1);
			}		
		nanosleep(&delay, NULL); /* wait a little before checking again */
	}
}
