/***************************************************************************
                           main.c  -  description
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

/***************************************************************************
	What we want to do here is:
	 - start the app in the vt of your choice
	   (but do not change the active vt)
   - do nothing but wait until the user has switched
	   to the vt in which we are running (we check every second)
	 - when we have focus, if possible start a directfb session that
	   - asks username and password and starts the session of your choice
		 - grabs the ALT-Fn combo, deactivating directfb, changing the active tty
		   to the one the user wants to go, and exiting (init will restart us)
	 - If we cannot initialize directfb, act as a normal getty, asking
	   user name and passing control to passwd
****************************************************************************/

/****** Yet to do:
- drop down menu for sessions
- allow session locking
- option for starting wms in an unique X server?!?
- add a clock and/or uptime thingie!
- start some apps before/after X session
- multilanguage support
******* Maybe these are done
- check PAM user permissions
*******/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <directfb.h>

#include "chvt.h"
#include "misc.h"
#include "framebuffer_mode.h"
#include "load_settings.h"

void Error()
{
	printf("\nqingy version " VERSION "\n");
	printf("\nusage: ginqy <ttyname> [options]\n");
	printf("Options:\n");
	printf("\t--resolution:<resolution>\n");
	printf("\tSpecify current screen resolution.\n");
	printf("\tUse XRESxYRES form.\n\n");
	printf("\t--hide-password\n");
	printf("\tDo not show password asterisks.\n\n");
	printf("\t--hide-lastuser\n");
	printf("\tDo not display last user name.\n\n");
	printf("\t--silent\n");
	printf("\tDo not display diagnostic messages on stderr.\n\n");
	printf("\t--no-shutdown-screen\n");
	printf("\tClose DirectFB mode before shutting down.\n");
	printf("\tThis way you will see system shutdown messages.\n\n");
	printf("\t--black-screen-workaround\n");
	printf("\tTry this if you get a black screen instead of a text console.\n");
	printf("\tNote: switching to another vt and back also solves the problem.\n\n");

	exit(EXIT_FAILURE);
}

void text_mode()
{
	char *username;

	/* We fall back here if framebuffer init fails */
	username = (char *) calloc(MAX, sizeof(char));
	while (strlen(username)==0)
	{
		printf("%s", print_welcome_message("\n", " login: "));
		scanf("%s", username); /* quick and dirty */
	}
	execl("/bin/login", "/bin/login", "--", username, (char *) 0);

	/* We should never get here... */
	fprintf(stderr, "\nCannot exec \"/bin/login\"...\n");
	exit(EXIT_FAILURE);
}

void start_up(void)
{
	int returnstatus;
	int argc = 2;
	char *argv[3];

	/* We clear the screen */
	ClearScreen();

	argv[0]= (char *) calloc(6, sizeof(char));
	strcpy(argv[0], "qingy");
	argv[1]= (char *) calloc(33, sizeof(char));
	strcpy(argv[1], "--dfb:no-vt-switch,bg-none");
	if (silent) strcat(argv[1], ",quiet");
	argv[2]= NULL;

	/* Now we try to initialize the framebuffer */
	returnstatus = framebuffer_mode(argc, argv);

	/* We get here only if directfb fails or user wants to change tty */
	free(argv[1]); free(argv[0]); argv[1]= argv[0]= NULL;

	/* this if user wants to switch to another tty */
	if (returnstatus != TEXT_MODE)
	{
		if (set_active_tty(returnstatus) == 0)
		{
			fprintf(stderr, "\nfatal error: unable to change active tty!\n");
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS); /* init will restart us in listen mode */
	}

	/* we get here if framebuffer init failed: we revert to text mode   */
	if (black_screen_workaround != -1)
	{
		/* This is to avoid a black display after framebuffer dies */
		set_active_tty(13);
		set_active_tty(black_screen_workaround);
	}
	text_mode(); /* This call does not return */

	/* We should never get here */
	fprintf(stderr, "\nGo tell my creator that his brains went pop!\n");
	exit(EXIT_FAILURE);
}

int get_resolution(char *resolution)
{
	char *temp;
	int *value = &width;

	if (!resolution) return 0;
	for(temp = resolution; (int)*temp; temp++)
		switch (*temp)
		{
			case 'x': case 'X':
				if (!width) return 0;
				if (value == &height) return 0;
				value = &height;
				break;
			default:
				if ((*temp) > '9' || (*temp) < '0') return 0;
				(*value) *= 10;
				(*value) += (*temp) - '0';
		}
	if (!width)  return 0;
	if (!height) return 0;

	return 1;
}

int ParseCMDLine(int argc, char *argv[])
{
	int i;
	char *tty;
	int our_tty_number;

	if (argc < 2) Error();
	tty= argv[1];
	if (strncmp(tty, "tty", 3) != 0) Error();
	our_tty_number= atoi(tty+3);
	if (our_tty_number < 1) Error();
	if (our_tty_number >12) Error();

	for (i=2; i<argc; i++)
	{
		int error = 1;

		if (strncmp(argv[i], "--resolution:", 13) == 0)
		{
			if (!get_resolution((argv[i]+13))) Error();
			error = 0;
		}
		if (strcmp(argv[i], "--silent") == 0)
		{
			silent = 1;
			error = 0;
		}
		if (strcmp(argv[i], "--black-screen-workaround") == 0)
		{
			black_screen_workaround = our_tty_number;
			error = 0;
		}
		if (strcmp(argv[i], "--hide-password") == 0)
		{
			hide_password = 1;
			error = 0;
		}
		if (strcmp(argv[i], "--hide-lastuser") == 0)
		{
			hide_last_user = 1;
			error = 0;
		}
		if (strcmp(argv[i], "--no-shutdown-screen") == 0)
		{
			no_shutdown_screen = 1;
			error = 0;
		}

		if (error) Error();
	}

	return our_tty_number;
}

int main(int argc, char *argv[])
{
	int user_tty_number;
	int our_tty_number;
	struct timespec delay;

	/* We set up a delay of 0.5 seconds */
	delay.tv_sec= 0;
	delay.tv_nsec= 500000000;

	/* We set up some default values */
	width=0;
	height=0;
	black_screen_workaround = -1;
	silent = 0;
	hide_password = 0;
	hide_last_user = 0;
	no_shutdown_screen = 0;

	our_tty_number = ParseCMDLine(argc, argv);

	/* We switch to tty <tty> */
	if (!switch_to_tty(our_tty_number))
	{
		fprintf(stderr, "\nUnable to switch to virtual terminal %s\n", argv[1]);
		return EXIT_FAILURE;
	}

	/* main loop: we wait until the user switches to the tty we are running in */
	while (1)
	{
		user_tty_number = get_active_tty();
		if (user_tty_number == -1)
		{
			fprintf(stderr, "\nfatal error: cannot get active tty number!\n");
			return EXIT_FAILURE;
		}
		if (user_tty_number == our_tty_number) start_up();
		nanosleep(&delay, NULL); /* wait a little before checking again */
	}

	/* We should never get here */
	fprintf(stderr, "\nGo tell my creator not to smoke that stuff, next time...\n");

	return EXIT_FAILURE;
}
