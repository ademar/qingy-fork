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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "chvt.h"
#include "misc.h"
#include "framebuffer_mode.h"

void Error()
{
  printf("\nqingy version " VERSION "\n");
	printf("\nusage: ginqy <ttyname> [options]\n");
	printf("Options:\n");
	printf("\t--black-screen-workaround\n");
	printf("\tTry this if you get a black screen instead of a text console.\n");
  printf("\tNote: switching to another vt and back also solves the problem.\n\n");
	//printf("\t--broken-ati128-workaround\n");
	//printf("\tTry this if you have got an ati card and you only get a black screen.\n\n");

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

void start_up(int workaround/*, int ati_workaround*/)
{
	int returnstatus;
  int argc = 2;
	char *argv[3];

  argv[0]= (char *) calloc(6, sizeof(char));
	strcpy(argv[0], "qingy");
  argv[1]= (char *) calloc(33, sizeof(char));
	strcpy(argv[1], "--dfb:no-vt-switch,quiet,bg-none");
	//if (ati_workaround) strcat(argv[1], ",pixelformat=RGB32");
  argv[2]= NULL;

	/* Now we try to initialize the framebuffer */
  returnstatus = framebuffer_mode(argc, argv, workaround);
	free(argv[1]); free(argv[0]); argv[1]= argv[0]= NULL;
	/* We should never get here unless directfb init failed
	   or user wants to change tty                          */

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
	if (workaround != -1)
  {
	  /* This is to avoid a black display after framebuffer dies */
    set_active_tty(13);
	  set_active_tty(workaround);
  }
  text_mode(); /* This call does not return */

	/* We should never get here */
	fprintf(stderr, "\nGo tell my creator that his brains went pop!\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  char *tty;
	int our_tty_number;
	int user_tty_number;
	int workaround = -1;
	//int ati_workaround = 0;
	int i = 2;
  struct timespec delay;

  /* We set up a delay of 0.5 seconds */
  delay.tv_sec= 0;
  delay.tv_nsec= 500000000;

	/* Some consistency checks... */
	if (argc > 4) Error();
	tty= argv[1];
  if (strncmp(tty, "tty", 3) != 0) Error();
	our_tty_number= atoi(tty+3);
	if (our_tty_number < 1) Error();
  if (our_tty_number >12) Error();
	while (i < argc)
	{
	  if (strcmp(argv[i], "--black-screen-workaround") == 0)
			workaround = our_tty_number;
		//else if (strcmp(argv[i], "--broken-ati128-workaround") == 0)
		//  ati_workaround=1;
		else Error();
		i++;
	}

	/* We switch to tty <tty> */
  if (!switch_to_tty(our_tty_number))
	{
	  fprintf(stderr, "\nUnable to switch to virtual terminal %s\n", tty);
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
		if (user_tty_number == our_tty_number) start_up(workaround/*, ati_workaround*/);
		nanosleep(&delay, NULL); /* wait a little before checking again */
	}

	/* We should never get here */
  fprintf(stderr, "\nGo tell my creator not to smoke that stuff, next time...\n");

	return EXIT_FAILURE;
}
