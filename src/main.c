/***************************************************************************
                           main.c  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003,2004 by Noberasco Michele
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "memmgmt.h"
#include "vt.h"
#include "misc.h"
#include "directfb_mode.h"
#include "load_settings.h"
#include "session.h"


void start_up(int argc, char *argv[], int our_tty_number)
{
	FILE *fp;
  int i, returnstatus = QINGY_FAILURE;
	char *interface  = NULL;
	char *username   = NULL;
	char *password   = NULL;
	char *session    = NULL;
	size_t len = 0;

	/* parse settings file */
	load_settings();

  /* We clear the screen */
  ClearScreen();

	/* get resolution of console framebuffer */
	if (!resolution) resolution = get_fb_resolution( (fb_device) ? fb_device : "/dev/fb0" );
	if (!silent && resolution) fprintf(stderr, "framebuffer resolution is '%s'.\n", resolution);

	/* Set up command line for our interface */
	interface = StrApp((char**)NULL, DFB_INTERFACE, (char*)NULL);
	for (i=1; i<argc; i++)
		StrApp(&interface, " ", argv[i], (char*)NULL);
	StrApp(&interface, " --dfb:vt-switch,bg-none", (char*)NULL);
	if (silent)     StrApp(&interface, ",quiet", (char*)NULL);
	if (fb_device)  StrApp(&interface, ",fbdev=", fb_device,  (char*)NULL);
	if (resolution) StrApp(&interface, ",mode=",  resolution, (char*)NULL);
	if (resolution) free(resolution);
	if (fb_device)  free(fb_device);

	/* let's go! */
	for (i=0; i<=retries; i++)
	{
		fp = popen(interface, "r");
		if (getline(&username, &len, fp) == -1) username = NULL; len = 0;
		if (getline(&password, &len, fp) == -1) password = NULL; len = 0;
		if (getline(&session,  &len, fp) == -1) session  = NULL; len = 0;

		returnstatus = pclose(fp);
		if (WIFEXITED(returnstatus))
			returnstatus = WEXITSTATUS(returnstatus);
		else
			returnstatus = QINGY_FAILURE;

		if (returnstatus != QINGY_FAILURE) break;
		if (username && password && session) break;

		if (i == retries) returnstatus = TEXT_MODE;
	}
	free(interface);

	/* remove trailing newlines from these values */
	if (username) username[strlen(username) - 1] = '\0';
	if (password) password[strlen(password) - 1] = '\0';
	if (session)  session [strlen(session)  - 1] = '\0'; 

	/* re-allow vt switching if it is still disabled */
	unlock_tty_switching();

	/* return to our righteous tty */
	set_active_tty(our_tty_number);

	switch (returnstatus)
	{
		case EXIT_SUCCESS:
			if (check_password(username, password))
				start_session(username, session);
			break;
		case TEXT_MODE:
			text_mode();
			break;
		case SHUTDOWN_R:
			execl ("/sbin/shutdown", "/sbin/shutdown", "-r", "now", (char*)NULL);
			break;
		case SHUTDOWN_H:
			execl ("/sbin/shutdown", "/sbin/shutdown", "-h", "now", (char*)NULL);
			break;
		default: /* user wants to switch to another tty ... */
			if (!set_active_tty(returnstatus))
			{
				fprintf(stderr, "\nfatal error: unable to change active tty!\n");
				exit(EXIT_FAILURE);
			}
			exit(EXIT_SUCCESS); /* init will restart us in listen mode */
	}

  /* We should never get here */
  fprintf(stderr, "\nGo tell my creator that his brains went pop!\n");
  /* NOTE (paolino): I never got here, but still your brains are gone pop! */
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  int user_tty_number;
  int our_tty_number;
  struct timespec delay;
	char *ptr;

  /* We set up a delay of 0.5 seconds */
  delay.tv_sec  = 0;
  delay.tv_nsec = 500000000;	/* that's 500M */
  
  /*
	 * We enable vt switching in case some previous session
	 * crashed on start and left it disabled
	 */
  unlock_tty_switching();
  
  /* We set up some default values */
  initialize_variables();
	program_name   = argv[0];
	if ((ptr = strrchr(argv[0], '/')))
		program_name = ++ptr;
  our_tty_number = ParseCMDLine(argc, argv, 1);
	current_tty    = our_tty_number;
  
  /* We switch to tty <tty> */
	Switch_TTY;

  /* Main loop: we wait until the user switches to the tty we are running in */
  while (1)
	{
		user_tty_number = get_active_tty();
		if (user_tty_number == -1)
		{
			fprintf(stderr, "\nfatal error: cannot get active tty number!\n");
			return EXIT_FAILURE;
		}
		if (user_tty_number == our_tty_number) start_up(argc, argv, our_tty_number);
		nanosleep(&delay, NULL); /* wait a little before checking again */
	}
  
  /* We should never get here */
  fprintf(stderr, "\nGo tell my creator not to smoke that stuff, next time...\n");
  /* NOTE (paolino): indeed! */
  
  return EXIT_FAILURE;
}
