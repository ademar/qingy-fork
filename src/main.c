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
#include "directfb_mode.h"
#include "load_settings.h"

char *fb_device = NULL;

void PrintUsage()
{
  printf("\nqingy version %s\n", VERSION);
  printf("\nusage: ginqy <ttyname> [options]\n");
  printf("Options:\n");
  printf("\t--fb-device <device>\n");
  printf("\tUse <device> as framebuffer device.\n\n");
  printf("\t--hide-password\n");
  printf("\tDo not show password asterisks.\n\n");
  printf("\t--hide-lastuser\n");
  printf("\tDo not display last user name.\n\n");
  printf("\t--verbose\n");
  printf("\tDisplay some diagnostic messages on stderr.\n\n");
  printf("\t--no-shutdown-screen\n");
  printf("\tClose DirectFB mode before shutting down.\n");
  printf("\tThis way you will see system shutdown messages.\n\n");
  printf("\t--screensaver <timeout>\n");
  printf("\tActivate screensaver after <timeout> minutes (default is 5).\n");
  printf("\tA value of 0 disables screensaver completely.\n\n");
  printf("\t--black-screen-workaround\n");
  printf("\tTry this if you get a black screen instead of a text console.\n");
  printf("\tNote: switching to another vt and back also solves the problem.\n\n");
}

void text_mode()
{
  execl("/bin/login", "/bin/login", (char *) 0);

  /* We should never get here... */
  fprintf(stderr, "\nCannot exec \"/bin/login\"...\n");
  exit(EXIT_FAILURE);
}

void Error(int fatal)
{
	/* We reenable VT switching if it is disabled */
	unlock_tty_switching();   

	PrintUsage();
	if (!fatal) text_mode();
	exit(EXIT_FAILURE);
}

void start_up(void)
{
  int returnstatus;
  int argc = 2;
	char *argv[3];
  
  /* First of all, we lock vt switching */
  lock_tty_switching();
  
  /* We clear the screen */
  ClearScreen();

  /* Set up some stuff */
  argv[0]= (char *) my_calloc(6,  sizeof(char));
  argv[1]= (char *) my_calloc(50, sizeof(char));
  strcpy(argv[0], "qingy");
  strcpy(argv[1], "--dfb:no-vt-switch,bg-none");
  if (silent) strcat(argv[1], ",quiet");
  if (fb_device)
  {
    strcat(argv[1], ",fbdev=");
    strcat(argv[1], fb_device);
  }
  argv[2]= NULL;

	/* Now we try to initialize the framebuffer */
	returnstatus = directfb_mode(argc, argv);		

  /* We get here only if directfb fails or user wants to change tty */  
	free(argv[1]); free(argv[0]); argv[1]= argv[0]= NULL;	

  /* re-allow vt switching if it is still disabled */
  unlock_tty_switching();

  /* if user wants to switch to another tty ... */
  if (returnstatus != TEXT_MODE)
  {
    if (set_active_tty(returnstatus) == 0)
    {
      fprintf(stderr, "\nfatal error: unable to change active tty!\n");
      exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS); /* init will restart us in listen mode */    
  }

  /* framebuffer init failed or user pressed ESC twice...
		 ... we revert to text mode                            */

  /* This is to avoid a black display after framebuffer dies */
  if (black_screen_workaround) tty_redraw();

  text_mode(); /* This call does not return */

  /* We should never get here */
  fprintf(stderr, "\nGo tell my creator that his brains went pop!\n");
  exit(EXIT_FAILURE);
}

int ParseCMDLine(int argc, char *argv[])
{
  int i;
  char *tty;
  int our_tty_number;
     
  if (argc < 2) Error(1);
  tty= argv[1];
  if (strncmp(tty, "tty", 3) != 0) Error(1);
  our_tty_number= atoi(tty+3);
  if (our_tty_number < 1) Error(1);
     
  for (i=2; i<argc; i++)
  {
    if (strcmp(argv[i], "--fb-device") == 0)
    {
      if (i == argc) Error(0);
      i++;
      fb_device = argv[i];
      continue;
    }
	  
    if (strcmp(argv[i], "--screensaver") == 0)
    {
      int temp;
	       
      if (i == argc) Error(0);
      i++;
      temp = atoi(argv[i]);
      if (temp < 0) Error(0);
      if (!temp)
      {
				use_screensaver = 0;
				continue;
      }
      use_screensaver = 1;
      screensaver_timeout = temp;
      continue;
    }
	  
    if (strcmp(argv[i], "--verbose") == 0)
    {
      silent = 0;
      continue;
    }
    if (strcmp(argv[i], "--black-screen-workaround") == 0)
    {
      black_screen_workaround = 1;
      continue;
    }
    if (strcmp(argv[i], "--hide-password") == 0)
    {
      hide_password = 1;
      continue;
    }
    if (strcmp(argv[i], "--hide-lastuser") == 0)
    {
      hide_last_user = 1;
      continue;
    }
    if (strcmp(argv[i], "--no-shutdown-screen") == 0)
    {
      no_shutdown_screen = 1;
      continue;
    }
	  
    Error(0);
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
  
  /* We enable vt switching in case some previous session
     crashed on start and left it disabled                */
  unlock_tty_switching();

  /* We set up some default values */
	image_paths = NULL;
  black_screen_workaround = 0;
  silent = 1;
  hide_password = 0;
  hide_last_user = 0;
  no_shutdown_screen = 0;
  use_screensaver = 1;
  screensaver_timeout = 5;
	SCREENSAVER = PIXEL_SCREENSAVER;  
  
  our_tty_number = ParseCMDLine(argc, argv);

  /* We switch to tty <tty> */
  if (!switch_to_tty(our_tty_number))
  {
    fprintf(stderr, "\nUnable to switch to virtual terminal %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  /* we sleep a cycle to allow user to chance vt in case
     settings file or config options are badly broken    */
  nanosleep(&delay, NULL);

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
  
  return EXIT_FAILURE;
}
