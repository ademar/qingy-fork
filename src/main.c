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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "chvt.h"
#include "misc.h"
#include "directfb_mode.h"
#include "load_settings.h"

#define Switch_TTY                                                                          \
	if (!switch_to_tty(our_tty_number))                                                       \
	{                                                                                         \
		fprintf(stderr, "\nUnable to switch to virtual terminal /dev/tty%d\n", our_tty_number); \
		exit(EXIT_FAILURE);                                                                     \
	}


char *fb_device = NULL;


void PrintUsage()
{
  printf("\nqingy version %s\n", VERSION);
  printf("\nusage: ginqy <ttyname> [options]\n");
  printf("Options:\n");
  printf("\t-f <device>, --fb-device <device>\n");
  printf("\tUse <device> as framebuffer device.\n\n");
  printf("\t-p, --hide-password\n");
  printf("\tDo not show password asterisks.\n\n");
  printf("\t-l, --hide-lastuser\n");
  printf("\tDo not display last user name.\n\n");
  printf("\t-d, --disable-lastuser\n");
  printf("\tDo not remember last user name.\n\n");
  printf("\t-v, --verbose\n");
  printf("\tDisplay some diagnostic messages on stderr.\n\n");
  printf("\t-n, --no-shutdown-screen\n");
  printf("\tClose DirectFB mode before shutting down.\n");
  printf("\tThis way you will see system shutdown messages.\n\n");
  printf("\t-s <timeout>, --screensaver <timeout>\n");
  printf("\tActivate screensaver after <timeout> minutes (default is 5).\n");
  printf("\tA value of 0 disables screensaver completely.\n\n");
  printf("\t-b, --black-screen-workaround\n");
  printf("\tTry this if you get a black screen instead of a text console.\n");
  printf("\tNote: switching to another vt and back also solves the problem.\n\n");
}

void text_mode()
{
  execl("/bin/login", "/bin/login", NULL);

  /* We should never get here... */
  fprintf(stderr, "\nCannot exec \"/bin/login\"...\n");
  exit(EXIT_FAILURE);
}

void Error(int fatal)
{
	int countdown = 16;

  /* We reenable VT switching if it is disabled */
  unlock_tty_switching();   
  
  PrintUsage();
  if (!fatal) text_mode();
  
  /*
   * we give the user some time to read the message
   * and change VT before dying
   */
	/* ED (paolino): was sleep(5), too fast to read errors */
	/* ED (michele): all right, but we should let them know what's happening! */
	while (--countdown)
	{
		fprintf(stderr, "%s will be restarted automatically in %d seconds\r", program_name, countdown);
		fflush(stderr);
		sleep(1);
	}
  exit(EXIT_FAILURE);
}

void start_up(void)
{
  int returnstatus;
  int argc = 2;
	char *resolution;
  char *argv[3];
  
  /* First of all, we lock vt switching */
  lock_tty_switching();
  
  /* We clear the screen */
  ClearScreen();

	/* get resolution of console framebuffer */
	resolution = get_fb_resolution( (fb_device) ? fb_device : "/dev/fb0" );
	if (!silent && resolution) fprintf(stderr, "framebuffer resolution is '%s'.\n", resolution);
  
  /* Set up some stuff */
  argv[0]= strdup(program_name);	/* NOTE: get it maybe dynamic (grep strings)... Done! ;-P */
  argv[1]= strdup("--dfb:no-vt-switch,bg-none");
  if (silent)     StrApp(&(argv[1]), ",quiet", (char*)NULL);
  if (fb_device)  StrApp(&(argv[1]), ",fbdev=", fb_device,  (char*)NULL);
	if (resolution) StrApp(&(argv[1]), ",mode=",  resolution, (char*)NULL);
  argv[2]= NULL;
	if (resolution) free(resolution);
	if (fb_device)  free(fb_device);
  
  /* Now we try to initialize the framebuffer */
  returnstatus = directfb_mode(argc, argv);		
  
  /* We get here only if directfb fails or user wants to change tty */  
  free(argv[1]); free(argv[0]);
  
  /* re-allow vt switching if it is still disabled */
  unlock_tty_switching();
  
  /* if user wants to switch to another tty ... */
  if (returnstatus != TEXT_MODE)
	{
		if (!set_active_tty(returnstatus))
		{
			fprintf(stderr, "\nfatal error: unable to change active tty!\n");
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS); /* init will restart us in listen mode */    
	}
  
  /*
   * framebuffer init failed or user pressed ESC twice...
   * ... we revert to text mode
   */
  
  /* This is to avoid a black display after framebuffer dies */
  if (black_screen_workaround) tty_redraw();
  
  text_mode(); /* This call does not return */
  
  /* We should never get here */
  fprintf(stderr, "\nGo tell my creator that his brains went pop!\n");
  /* NOTE (paolino): I never got here, but still your brains are gone pop! */
  exit(EXIT_FAILURE);
}

/* okeeey... Whatabout using getopt?  */
/* okeeey... But you do be tiresome!  */
int ParseCMDLine(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind, opterr, optopt;
	const char optstring[] = "f:pldvns:b";
	const struct option longopts[] =
	{
		{"fb-device",               required_argument, NULL, 'f'},
		{"hide-password",           no_argument,       NULL, 'p'},
		{"hide-lastuser",           no_argument,       NULL, 'l'},
		{"disable-lastuser",        no_argument,       NULL, 'd'},
		{"verbose",                 no_argument,       NULL, 'v'},
		{"no-shutdown-screen",      no_argument,       NULL, 'n'},
		{"screensaver",             required_argument, NULL, 's'},
		{"black-screen-workaround", no_argument,       NULL, 'b'},
		{0, 0, 0, 0}
	};
  char *tty;
  int our_tty_number;
	
  if (argc < 2) Error(1);
  tty= argv[1];
  if (strncmp(tty, "tty", 3) != 0) Error(1);
  our_tty_number= atoi(tty+3);
  if (our_tty_number < 1) Error(1);
  
	while (1)
	{
		int retval = getopt_long(argc, argv, optstring, longopts, NULL);

		if (retval == -1) break;
		switch (retval)
		{
			case 'f': /* use this framebuffer device */
				fb_device = strdup(optarg);
				break;
			case 'p': /* hide password */
				hide_password = 1;
				break;
			case 'l': /* hide lastuser */
				hide_last_user = 1;
				break;
			case 'd': /* disable lastuder */
				disable_last_user = 1;
				break;
			case 'v': /* verbose */
				silent = 0;
				break;
			case 'n': /* no shutdown screen */
				no_shutdown_screen = 1;
				break;
			case 's': /* screen_saver */
			{
				int temp = atoi(optarg);
				if (temp < 0)
				{
					Switch_TTY;
					ClearScreen();
					fprintf(stderr, "%s: invalid screen saver timeout: fall back to text mode.\n", program_name);
					Error(0);
				}
				if (!temp)
				{
					use_screensaver = 0;
					break;
				}
				use_screensaver = 1;
				screensaver_timeout = temp;
				break;
			}
			case 'b': /* black-screen-workaround */
				black_screen_workaround = 1;
				break;
			default:
				Switch_TTY;
				ClearScreen();
				fprintf(stderr, "%s: error in command line options: fall back to text mode.\n", program_name);
				Error(0);
		}
	}
  
  return our_tty_number;
}

int main(int argc, char *argv[])
{
  int user_tty_number;
  int our_tty_number;
  struct timespec delay;
  
  /* We set up a delay of 0.5 seconds */
  delay.tv_sec  = 0;
  delay.tv_nsec = 500000000;	/* that's 500M */
  
  /*
	 * We enable vt switching in case some previous session
	 * crashed on start and left it disabled
	 */
  unlock_tty_switching();
  
  /* We set up some default values */
  initialize_variables(); /* NOTE: Check alot more... Macros? Structs? etc.? */
	program_name = argv[0];
  
  our_tty_number = ParseCMDLine(argc, argv); /* wie gesagt: getopt? Done, done ;-P */
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
		if (user_tty_number == our_tty_number) start_up();
		nanosleep(&delay, NULL); /* wait a little before checking again */
	}
  
  /* We should never get here */
  fprintf(stderr, "\nGo tell my creator not to smoke that stuff, next time...\n");
  /* NOTE (paolino): indeed! */
  
  return EXIT_FAILURE;
}
