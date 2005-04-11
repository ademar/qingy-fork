/***************************************************************************
                       tty_guardian.c  -  description
                            --------------------
    begin                : Feb 04 2004
    copyright            : (C) 2004-2005 by Noberasco Michele
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

#include <dirent.h>
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

#include "memmgmt.h"
#include "misc.h"
#include "vt.h"
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



/* checks wether given string is a number */
int is_number(char *string)
{
	char *endptr = NULL;

	if (!string)         return 0;
	if (*string == '\0') return 0;
	(void)strtol(string, &endptr, 10);

	return ((*endptr == '\0'));
}

/* we check wether <process> is qingy or getty */
int is_getty(pid_t process)
{
	char *temp = int_to_str(process);
	char *link = StrApp((char**)NULL, "/proc/", temp, "/exe", (char*)NULL);	
	char buf[256];
	int size;
	int result = 0;

	free(temp);
	size = readlink(link, buf, 255);
	free(link);
	
	if (size == -1) abort();
	buf[size] = '\0';

	if (strstr(buf, "getty")) result = 1;
	if (strstr(buf, "qingy")) result = 1;

	return result;
}

/*
 * hairy stuff here: we need to parse all /proc/<number>
 * entries to find a process that owns /dev/tty<tty> (or /dev/vc/<tty>)
 * of which we return the pid
 */
pid_t has_controlling_process(int tty)
{
	struct dirent *entry;
	char  *device  = create_tty_name(tty);
	char  *device2 = create_vc_name (tty);
	DIR   *dir     = opendir("/proc");
	pid_t  result  = 0;

	if (!dir)
	{
		fprintf(stderr, "program_name: tty guardian feature needs /proc filesystem support!\n");
		sleep(5);
		abort();
	}

	/* scan /proc for process subdirs */
	while ((entry = readdir(dir)))
	{
		int counter = 0;
		char *dirname; 
		DIR  *dir2;

    if (!strcmp(entry->d_name, "." )) continue;
    if (!strcmp(entry->d_name, "..")) continue;
		if (!is_number(entry->d_name))    continue;

		dirname = StrApp((char**)NULL, "/proc/", entry->d_name, (char*)NULL);
		if ((dir2=opendir(dirname)))
		{
			int i;
			char *where = StrApp((char**)NULL, dirname, "/fd/", (char*)NULL);			

			/* check wether this process controls /dev/tty<tty> */
			for (i=0; i<3; i++)
			{
				int size;
				char buf[16];
				char *temp = int_to_str(i);
				char *link = StrApp((char**)NULL, where, temp, (char*)NULL);

				free(temp);
				size = readlink(link, buf, 15);
				free(link);
				if (size != -1)
				{
					buf[size] = '\0';
					if (!strcmp(buf, device) || !strcmp(buf, device2)) counter++;
				}
			}
			free(where);
			closedir(dir2);
		}		
		free(dirname);
		if (counter == 3)
		{
			result = atoi(entry->d_name);
			break;
		}
	}
	closedir(dir);
	free(device);

	return result;
}

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
	ClearScreen();
	unlock_tty_switching();
	switch_to_tty(our_land);
	disallocate_tty(dest);
	
	return retval;
}

/* check wether user has auth to visit our tty */
void WatchDog_Sniff(char *dog_master, int where_was_intruder, int where_is_intruder, int send_him_here)
{
	static char *previous_intruder = NULL;
	char        *intruder;
	char        *file;
	int          retval;

	file = create_tty_name(where_was_intruder);
	intruder = get_file_owner(file);
	free(file);

	if (!strcmp(intruder, dog_master) && strcmp(intruder, "root"))
	{ /* this is our master, not an intruder */
		free(previous_intruder);
		previous_intruder = intruder;
		return;
	}

	if (!strcmp(intruder, "root"))
	{ /* there are 2 possibilities here:
		 * - tty is unused (thus we cannot know the user)
		 * - tty is in use, thus either:
		 *   - check wether qingy (or another getty) is running in this terminal, then
		 *   - check owner of /dev/vc/<where_was_intruder> (probably an X session)
		 */
		if (!is_tty_available(where_was_intruder))
		{
			pid_t process = has_controlling_process(where_was_intruder);
			if (!process)
			{ /* This tty in not controlled by any process:
				 * we assume an X server is running here, thus
				 * we check owner of /dev/vc/<where_was_intruder>
				 * to get owner of this tty
				 */
				char *temp = int_to_str(where_was_intruder);
				
				free(intruder);
				file = StrApp((char**)NULL, "/dev/vc/", temp, (char*)NULL);
				free(temp);
				intruder = get_file_owner(file);
				free(file);

				if (!strcmp(intruder, "root"))
				{ /* we cannot be sure of user identity */
					free(intruder);
					intruder = strdup("unknown");
					free(previous_intruder);
				}
				
				if (!strcmp(intruder, dog_master))
				{ /* now we are sure about user identity */
					free(previous_intruder);
					previous_intruder = intruder;
					return;
				}
			}
			else
			{ /* some process is controlling this tty:
				 * we check wether it is some sort of getty
				 */
				if (is_getty(process))
				{	/* getty is running here: we cannot know who intruder is */
					free(intruder);
					intruder = strdup("unknown");
					free(previous_intruder);
				}
				else
				{ /* this tty is controlled by root: we grant access */
					free(previous_intruder);
					previous_intruder = intruder;
					return;					
				}
			}
		}
		else
		{ /* vt is not in use: we cannot know who intruder is */
			free(intruder);
			intruder = strdup("unknown");
			free(previous_intruder);
		}
	}

	if (previous_intruder)
		if (strcmp(previous_intruder, "root"))
				if (!strcmp(previous_intruder,intruder))
					return; /* it's a hard life being sure of someone... */

	/* tell user to authenticate himself */
	retval = WatchDog_Bark(dog_master, intruder, where_is_intruder);	

	if (!retval)
	{ /* user has no right to be here */
		set_active_tty(where_was_intruder);
		free(previous_intruder);
		return;
	}

	/* user has authenticated correctly */
	set_active_tty(send_him_here);
	free(previous_intruder);
	previous_intruder = intruder;
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

	if (!child)             return;
	if (!dog_master)        WAIT_N_RETURN;
	if (!fence1 && !fence2) WAIT_N_RETURN;

	/* We set up a delay of 0.1 seconds */
  delay.tv_sec  = 0;
  delay.tv_nsec = 100000000;	/* that's 100M */

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
