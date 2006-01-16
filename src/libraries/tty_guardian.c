/***************************************************************************
                       tty_guardian.c  -  description
                            --------------------
    begin                : Feb 04 2004
    copyright            : (C) 2004-2005 by Noberasco Michele
    e-mail               : michele.noberasco@tiscali.it
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
int is_getty(char *process)
{
	char *link = StrApp((char**)NULL, "/proc/", process, "/exe", (char*)NULL);	
	char buf[256];
	int size;

	size = readlink(link, buf, 255);
	free(link);
	
	if (size == -1) return -1;

	buf[size] = '\0';

	if (strstr(buf, "getty")) return 1;
	if (strstr(buf, "qingy")) return 1;

	return 0;
}

/*
 * hairy stuff here: we need to parse all /proc/<number>
 * entries to find a process that owns /dev/tty<tty> (or /dev/vc/<tty>)
 * of which we return the pid
 */
char *has_controlling_processes(int tty)
{
	struct dirent *entry;
	char  *device  = create_tty_name(tty);
	char  *device2 = create_vc_name (tty);
	DIR   *dir     = opendir("/proc");
	char  *result  = NULL;

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
			if (!result)
				result=strdup(entry->d_name);
			else
				StrApp(&result, ",", entry->d_name, (char*)NULL);
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
	struct termios orig_attr;
	struct termios attr;
	char buf[PASSWD_MAX];
	char *retval;
	int pos = 0;
	char c;	

	free(device);	
	if (fd == -1) return NULL;

	/* we disable input echoing */
	if (tcgetattr(fd, &attr) == -1) ABORT_N_RETURN;
	memcpy(&orig_attr, &attr, sizeof(struct termios));

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

	/* restore terminal attributes */
	if (tcsetattr(fd, TCSAFLUSH|TCSASOFT, &orig_attr) == -1) ABORT_N_RETURN;

	close(fd);
	
	/* we clear the buffer before exiting */
	retval = strdup(buf);
	memset(&buf, 0, PASSWD_MAX*sizeof(char));

	return retval;
}

/* block user until he authenticates */
int WatchDog_Bark (char *dog_master, char *intruder, int our_land)
{
	int   dest = get_available_tty();
	char *password;
	int   retval;

	if (dest == -1)            return 0;
	if (!dog_master)           return 0;
	if (!intruder)             return 0;
	if (!switch_to_tty(dest))  return 0;

	unlock_tty_switching();
	if (!set_active_tty(dest))
	{
		lock_tty_switching();
		return 0;
	}

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
	switch_to_tty(our_land);
	disallocate_tty(dest);

	if (retval)
	{
		unlock_tty_switching();
	  set_active_tty(our_land);
		lock_tty_switching();
	}

	return retval;
}

/* check wether user has auth to visit our tty */
void WatchDog_Sniff(char *dog_master, int fence, int where_was_intruder)
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
	{ /*
		 * if qingy or some getty is controlling this tty
		 * we cannot be sure of user identity
		 */
		char *procs = has_controlling_processes(where_was_intruder);

		if (procs)
		{
			char *proc = strtok(procs,",");

			while (proc)
			{
				if (!is_getty(proc))
				{ /* now we are sure about user identity */
					free(procs);
					free(previous_intruder);
					previous_intruder = intruder;
					return;					
				}
				proc = strtok(NULL,",");
			}
			free(procs);
		}

		free(intruder);
		intruder = strdup("unknown");
		free(previous_intruder);
	}

	if (previous_intruder)
		if (!strcmp(previous_intruder,intruder))
			return; /* it's a hard life being sure of someone... */

	/* tell user to authenticate himself */
	retval = WatchDog_Bark(dog_master, intruder, fence);

	if (!retval)
	{ /* user has no right to be here */
		unlock_tty_switching();
		set_active_tty(where_was_intruder);
		free(previous_intruder);
		return;
	}

	/* user has authenticated correctly */
	free(previous_intruder);
	previous_intruder = intruder;
}

/* guard specified ttys against unauthorized access */
void ttyWatchDog(pid_t child, char *dog_master, int fence)
{
	struct timespec delay;
	int where_is_intruder  = 0;
	int where_was_intruder = 0;

	if (!child)      return;
	if (!dog_master) WAIT_N_RETURN;
	if (!fence)      WAIT_N_RETURN;

	/* We set up a delay of 0.1 seconds */
  delay.tv_sec  = 0;
  delay.tv_nsec = 100000000;	/* that's 100M */

	/* Main loop: we wait until the user switches to the tty we are guarding */
  while (waitpid(child, NULL, WNOHANG) != child)
	{
		lock_tty_switching();

		if (!where_was_intruder) where_was_intruder = get_active_tty();
		else where_was_intruder = where_is_intruder;
		where_is_intruder = get_active_tty();
		if (where_is_intruder == -1)
		{
			fprintf(stderr, "\ntty guardian: serious issue: cannot get active tty number!\n");
			unlock_tty_switching();
			nanosleep(&delay, NULL);
			continue;
		}
		if (where_is_intruder != where_was_intruder)
			if (where_is_intruder == fence)
				WatchDog_Sniff(dog_master, fence, where_was_intruder);

		unlock_tty_switching();

		nanosleep(&delay, NULL); /* wait a little before checking again */
	}
}
