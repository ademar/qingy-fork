/***************************************************************************
                           main.c  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003-2005 by Noberasco Michele
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
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef WANT_CRYPTO
#include "crypto.h"
#endif

#include "memmgmt.h"
#include "vt.h"
#include "misc.h"
#include "directfb_mode.h"
#include "load_settings.h"
#include "session.h"


char *autologin_filename = NULL;


void start_up(int argc, char *argv[], int our_tty_number, int do_autologin)
{
  int      i,j;
	int      returnstatus = QINGY_FAILURE;
	char   **gui_argv     = NULL;
	char    *username     = NULL;
	char    *password     = NULL;
	char    *session      = NULL;
#ifndef WANT_CRYPTO
	size_t   len          = 0;
#endif

  /* We clear the screen */
  ClearScreen();

	if (!do_autologin)
	{
		/* parse settings file again, it may have been changed in the mean while */
		load_settings();

		/* display native theme resolution */
		if (!silent)
			fprintf(stderr, "Native theme resolution is '%dx%d'\n", THEME_XRES, THEME_YRES);

		/* get resolution of console framebuffer */
		if (!resolution) resolution = get_fb_resolution( (fb_device) ? fb_device : "/dev/fb0" );
		if (!silent && resolution) fprintf(stderr, "framebuffer resolution is '%s'.\n", resolution);

		/* Set up command line for our interface */
		gui_argv = (char **) calloc(argc+2, sizeof(char *));
		gui_argv[0] = DFB_INTERFACE;
		for (j=1; j<argc; j++)
			gui_argv[j] = argv[j];

		gui_argv[j] = StrApp((char**)NULL, "--dfb:vt-switch,no-vt-switching,bg-none", (char*)NULL);
		if (silent) StrApp(&(gui_argv[j]), ",quiet", (char*)NULL);
		if (fb_device)
		{
			StrApp(&(gui_argv[j]), ",fbdev=", fb_device,  (char*)NULL);
			free(fb_device);
		}
		if (resolution)
		{
			StrApp(&(gui_argv[j]), ",mode=",  resolution, (char*)NULL);
			free(resolution);
		}
		gui_argv[++j] = NULL;

		/* let's go! */
		for (i=0; i<=retries; i++)
		{ /* let's create a unique file name to communicate with our GUI */
			char  *comm_file_name = StrApp((char**)NULL, TMP_FILE_DIR, "/", "qingyXXXXXX", (char*)NULL);
			int    fd;
			FILE  *fp;
			pid_t  pid;
			
			fd = mkstemp(comm_file_name);
			if (fd == -1)
			{
				fprintf(stderr, "%s: fatal error: could not create temporary file!\n", argv[0]);
				abort();
			}

			/* set file mode to 600 */
			if (chmod(comm_file_name, S_IRUSR|S_IWUSR))
			{
				fprintf(stderr, "%s: fatal error: cannot chmod() file '%s'!\n", argv[0], comm_file_name);
				abort();
			}

			/* open our file for reading */
			fp = fdopen(fd, "r");
			if (!fp)
			{
				fprintf(stderr, "%s: fatal error: unable to open temporary file '%s'!\n", argv[0], comm_file_name);
				abort();
			}

			/* let's spawn a child, that will fire up our GUI and make it write to our temp file */
			pid = fork();

			switch ((int)pid)
			{
				case -1:
					fprintf(stderr, "%s: fatal error: fork() failed!\n", argv[0]);
					abort();
				case 0:  /* child */
					/* let's redirect stdout to our comm file */
					if (!freopen(comm_file_name, "w", stdout))
					{
						fprintf(stderr, "%s: fatal error: unable to redirect standard output!\n", argv[0]);
						exit(EXIT_FAILURE);
					}
					/* remove comm file so that noone will tamper */
					unlink(comm_file_name);

#ifdef WANT_CRYPTO
					/* we will write the public key to a (securely created) temp file... */
					strcpy(comm_file_name + strlen(comm_file_name) - 6, "XXXXXX");
					fd = mkstemp(comm_file_name);
					if (fd == -1)
					{
						fprintf(stderr, "%s: fatal error: could not create temporary file!\n", argv[0]);
						abort();
					}
					if (chmod(comm_file_name, S_IRUSR|S_IWUSR))
					{
						fprintf(stderr, "%s: fatal error: cannot chmod() file '%s'!\n", argv[0], comm_file_name);
						abort();
					}
					fp = fdopen(fd, "w");
					if (!fp)
					{
						fprintf(stderr, "%s: fatal error: unable to open temporary file '%s'!\n", argv[0], comm_file_name);
						abort();
					}
					save_public_key(fp);

					/* ...which we set up as standard input for our GUI */
					if (!freopen(comm_file_name, "r", stdin))
					{
						fprintf(stderr, "%s: fatal error: unable to redirect standard input!\n", argv[0]);
						exit(EXIT_FAILURE);
					}
					fclose(fp);

					/* once set up, we unlink() it so that noone else will tamper */
					unlink(comm_file_name);
#endif

					/* OK, let's fire up the GUI */
					execve(gui_argv[0], gui_argv, NULL);

					/* we should never get here unless the execve failed */
					fprintf(stderr, "%s: fatal error: unable to execute %s!\n", argv[0], gui_argv[0]);
					exit(EXIT_FAILURE);
				default: /* parent */
					/* we wait for our child to exit */
					waitpid(pid, &returnstatus, 0);
					break;
			}

#ifdef WANT_CRYPTO
			username = decrypt_item(fp);
			password = decrypt_item(fp);
			session  = decrypt_item(fp);
#else
			if (getline(&username, &len, fp) == -1) username = NULL; len = 0;
			if (getline(&password, &len, fp) == -1) password = NULL; len = 0;
			if (getline(&session,  &len, fp) == -1) session  = NULL; len = 0;
#endif

			fclose(fp);
			free(comm_file_name);

			if (WIFEXITED(returnstatus))
				returnstatus = WEXITSTATUS(returnstatus);
			else
				returnstatus = QINGY_FAILURE;

			if (returnstatus != QINGY_FAILURE) break;

			if (username && password && session)
			{ /* we failed, yet we suceeded, weird eh? */
				returnstatus = EXIT_SUCCESS;
				break;
			}

			if (i == retries) returnstatus = EXIT_TEXT_MODE;
			else sleep(1);
		}

		free(gui_argv[--j]);
		free(gui_argv);

#ifndef WANT_CRYPTO
		/* remove trailing newlines from these values */
		if (username) username[strlen(username) - 1] = '\0';
		if (password) password[strlen(password) - 1] = '\0';
		if (session)  session [strlen(session)  - 1] = '\0';
#endif
	}
	else
	{
		/* create our already autologged temp file */
		int fd = creat(autologin_filename, S_IRUSR|S_IWUSR);
		close(fd);

		username     = AUTOLOGIN_USERNAME;
		password     = AUTOLOGIN_PASSWORD;
		session      = AUTOLOGIN_SESSION;
		returnstatus = EXIT_SUCCESS;
	}

	/* re-allow vt switching if it is still disabled */
	unlock_tty_switching();

	/* return to our righteous tty */
	set_active_tty(our_tty_number);

	switch (returnstatus)
	{
		case EXIT_SUCCESS:
			if (check_password(username, password))
				start_session(username, session);
			/* We won't get here unless there was a failure starting user session */
			fprintf(stderr, "\nLogin failed, reverting to text mode!\n");			
			/* Fall trough */
		case EXIT_TEXT_MODE:
			text_mode();
			break;
		case EXIT_SHUTDOWN_R:
			execl ("/sbin/shutdown", "/sbin/shutdown", "-r", "now", (char*)NULL);
			break;
		case EXIT_SHUTDOWN_H:
			execl ("/sbin/shutdown", "/sbin/shutdown", "-h", "now", (char*)NULL);
			break;
		case EXIT_SLEEP:
			if (SLEEP_CMD) execl (SLEEP_CMD, SLEEP_CMD, (char*)NULL);
			fprintf(stderr, "\nfatal error: could not execute sleep command!\n");
			exit(EXIT_FAILURE);
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

/*
 * if already_logged_in file exists and its modification time
 * is not antecedent than system uptime, we disable autologin
 * (unless of course user chooses to always re-autologin)
 */
int check_autologin(int our_tty_number)
{
	char        *temp;
	struct stat  filestat;
	time_t       uptime;

	if (!DO_AUTOLOGIN) return 0;

	/* Sanity checks */
	if (!AUTOLOGIN_USERNAME || !AUTOLOGIN_PASSWORD || !AUTOLOGIN_SESSION)
	{
		fprintf(stderr, "\nAutologin disabled: insuffucient user data!\n");
		return 0;
	}
	if (!strcmp(AUTOLOGIN_SESSION, "LAST"))
	{
		free(AUTOLOGIN_SESSION);
		AUTOLOGIN_SESSION = get_last_session(AUTOLOGIN_USERNAME);
		if (!AUTOLOGIN_SESSION)
		{
			fprintf(stderr, "\nAutologin disabled: could not get last session of user %s!\n", AUTOLOGIN_USERNAME);
			return 0;			
		}
	}

	if (AUTO_RELOGIN) return 1;

	/* set autologin temp file name */
	temp = int_to_str(our_tty_number);
	autologin_filename =
		StrApp((char**)NULL, TMP_FILE_DIR, "/", AUTOLOGIN_FILE_BASENAME, "tty", temp, (char*)NULL);
	free(temp);	

	if (access(autologin_filename, F_OK     )) return 1; /* file does not exist */
	if (stat  (autologin_filename, &filestat)) return 0; /* Could not get file stats */

	uptime = get_system_uptime();
	if (!uptime) return 0; /* could not get system uptime */

	if (filestat.st_mtime < time(NULL)-uptime)
	{
		unlink (autologin_filename);
		return 1;
	}

	return 0;
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

	/* parse settings file again */
	load_settings();

#ifdef WANT_CRYPTO
	generate_keys(); /* generate public/private keys */
#endif

	/* Should we log in user directly? Totally insecure, but handy */
	if (check_autologin(our_tty_number)) start_up(argc, argv, our_tty_number, 1);

  /* Main loop: we wait until the user switches to the tty we are running in */
  while (1)
	{
		user_tty_number = get_active_tty();
		if (user_tty_number == -1)
		{
			fprintf(stderr, "\nfatal error: cannot get active tty number!\n");
			return EXIT_FAILURE;
		}
		if (user_tty_number == our_tty_number) start_up(argc, argv, our_tty_number, 0);
		nanosleep(&delay, NULL); /* wait a little before checking again */
	}
  
  /* We should never get here */
  fprintf(stderr, "\nGo tell my creator not to smoke that stuff, next time...\n");
  /* NOTE (paolino): indeed! */
  
  return EXIT_FAILURE;
}
