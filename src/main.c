/***************************************************************************
                           main.c  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003-2006 by Noberasco Michele
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
#include <sys/wait.h>
#include <signal.h>

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
char *username           = NULL;
char *password           = NULL;
char *session            = NULL;
FILE *fp_fromGUI         = NULL;
FILE *fp_toGUI           = NULL;
int   auth_ok            = 0;
int   gui_retval         = QINGY_FAILURE;


void authenticate_user(int signum)
{
#ifndef WANT_CRYPTO
	size_t len = 0;
#endif 

	/* temporarily disable signal catching */
	signal(signum, SIG_IGN);

#ifndef WANT_CRYPTO
	if (getline(&username, &len, fp_fromGUI) == -1) username = NULL; len = 0;
	if (getline(&password, &len, fp_fromGUI) == -1) password = NULL; len = 0;
	if (getline(&session,  &len, fp_fromGUI) == -1) session  = NULL; len = 0;

	/* remove trailing newlines from these values */
	if (username) username[strlen(username) - 1] = '\0';
	if (password) password[strlen(password) - 1] = '\0';
	if (session)  session [strlen(session)  - 1] = '\0';
#else
	username = decrypt_item(fp_fromGUI);
	password = decrypt_item(fp_fromGUI);
	session  = decrypt_item(fp_fromGUI);
#endif

	if (check_password(username, password))
	{
		fprintf(fp_toGUI, "\nAUTH_OK\n");
		auth_ok = 1;
	}
	else
		fprintf(fp_toGUI, "\nAUTH_FAIL\n");

	fflush(fp_toGUI);

	/* re-enable signal catching */
	signal(signum, authenticate_user);
}

void read_action(int signum)
{

	/* temporarily disable signal catching */
	signal(signum, SIG_IGN);

	fscanf(fp_fromGUI, "%d", &gui_retval);

	/* re-enable signal catching */
	signal(signum, read_action);
}

void start_up(int argc, char *argv[], int our_tty_number, int do_autologin)
{
  int    i,j;
	int    returnstatus = QINGY_FAILURE;
	char **gui_argv     = NULL;
	char  *toGUI        = StrApp((char**)NULL, tmp_files_dir, "/qingyXXXXXX", (char*)NULL);
	char  *fromGUI      = strdup(toGUI);
	int    fd;

  /* We clear the screen */
  if (silent) ClearScreen();

	if (!do_autologin)
	{
		/* parse settings file again, it may have been changed in the mean while */
		load_settings();

		/* should we perform a text-mode login? */
		if (text_mode_login) text_mode();

		/* display native theme resolution */
		if (!silent)
			fprintf(stderr, "Native theme resolution is '%dx%d'\n", theme_xres, theme_yres);

		/* get resolution of console framebuffer */
		if (!resolution) resolution = get_fb_resolution( (fb_device) ? fb_device : "/dev/fb0" );
		if (!silent && resolution) fprintf(stderr, "framebuffer resolution is '%s'.\n", resolution);

		/* Set up command line for our interface */
		gui_argv = (char **) calloc(argc+3, sizeof(char *));
		gui_argv[0] = dfb_interface;
		for (j=1; j<argc; j++)
			gui_argv[j] = argv[j];

		gui_argv[j] = StrApp((char**)NULL, "--dfb:vt-switch,no-vt-switching,bg-none,no-hardware", (char*)NULL);
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
		gui_argv[++j] = int_to_str(getpid());
		gui_argv[++j] = NULL;

		/* should we execute some script before switching to graphic mode? */
		if (pre_gui_script)
			execute_script(pre_gui_script);

		/* let's create a couple of file names to communicate with our GUI:
		 * First the one we will receive auth data from...
		 */
		fd = mkstemp(fromGUI);
		if (fd == -1)
		{
			fprintf(stderr, "%s: fatal error: could not create temporary file!\n", argv[0]);
			free(fromGUI);
			text_mode();
		}

		/* sad thing having to call mkstemp just to have a reasonably secure unique file name,
		 * yet I think it is better than callind tempnam...
		 */
		close(fd);
		unlink(fromGUI);

		/* let's create a nice FIFO to receive user auth data */
		if (mkfifo(fromGUI, S_IRUSR|S_IWUSR))
		{
			fprintf(stderr, "%s: fatal error: could not create temporary file!\n", argv[0]);
			free(fromGUI);
			text_mode();
		}

		/* ...then the one we will use to send data to our GUI */
		fd = mkstemp(toGUI);
		if (fd == -1)
		{
			fprintf(stderr, "%s: fatal error: could not create temporary file!\n", argv[0]);
			exit(EXIT_FAILURE);
		}
		if (chmod(toGUI, S_IRUSR|S_IWUSR))
		{
			fprintf(stderr, "%s: fatal error: cannot chmod() file '%s'!\n", argv[0], toGUI);
			exit(EXIT_FAILURE);
		}
		fp_toGUI = fdopen(fd, "w");
		if (!fp_toGUI)
		{
			fprintf(stderr, "%s: fatal error: unable to open temporary file '%s'!\n", argv[0], toGUI);
			free(fromGUI);
			free(toGUI);
			text_mode();
		}

#ifdef WANT_CRYPTO
		/* we write the public key to our GUI... */
		save_public_key(fp_toGUI);
		/* ...and make sure it gets through */
		fflush(fp_toGUI);
#endif

		/* let's go! */
		for (i=0; i<=retries; i++)
		{
			pid_t pid = fork(); /* let's spawn a child, that will fire up our GUI and make it write to our temp file */

			switch ((int)pid)
			{
				case -1:
					fprintf(stderr, "%s: fatal error: fork() failed!\n", argv[0]);
					fclose(fp_toGUI);
					unlink(fromGUI);
					unlink(toGUI);
					free(toGUI);
					free(fromGUI);
					text_mode();

				case 0: /* child */
					/* we set up the standard input for our GUI... */
					if (!freopen(toGUI, "r", stdin))
					{
						fprintf(stderr, "%s: fatal error: unable to redirect standard input!\n", argv[0]);
						exit(EXIT_FAILURE);
					}

					/* once set up, we unlink() it so that noone else will tamper */
					unlink(toGUI);

					/* ... then the standard output */
					if (!freopen(fromGUI, "w", stdout))
					{
						fprintf(stderr, "%s: fatal error: unable to redirect standard output!\n", argv[0]);
						exit(EXIT_FAILURE);
					}
					/* remove comm file so that noone will tamper */
					unlink(fromGUI);

					/* OK, let's fire up the GUI */
					execve(gui_argv[0], gui_argv, NULL);

					/* we should never get here unless the execve failed */
					fprintf(stderr, "%s: fatal error: unable to execute %s!\n", argv[0], gui_argv[0]);
					exit(EXIT_FAILURE);

				default: /* parent */
					/* install handler so that our GUI can signal us to
					 * (try to) authenticate a user...
					 */
					signal(SIGUSR1, authenticate_user);

					/* ...and another to get GUI exit status */
					signal(SIGUSR2, read_action);

					/* it is now safe to open our fifo to read auth data */
					fp_fromGUI = fopen(fromGUI, "r");

					/* we wait for our child to exit */
					while (1)
					{
						waitpid(pid, &returnstatus, 0);
						if (WIFEXITED(returnstatus) || WIFSIGNALED(returnstatus))
							break;
					}

					/* we no longer need the signal handlers */
					signal(SIGUSR1, SIG_DFL);
					signal(SIGUSR2, SIG_DFL);

					break;
			}

			if (WIFEXITED(returnstatus) || WIFSIGNALED(returnstatus))
				returnstatus = gui_retval;
			else
				returnstatus = QINGY_FAILURE;

			fclose(fp_fromGUI);
			fclose(fp_toGUI);

			/* break the cycle if we are sure we don't have to read authentication data... */
			if (returnstatus != QINGY_FAILURE )
				break;

			/* if we have authentication data we can break the cycle */
			if (username && password && session)
			{
				returnstatus = EXIT_SUCCESS;
				break;
			}

			if (i == retries) returnstatus = EXIT_TEXT_MODE;
			else sleep(1);
		}

		free(toGUI);
		free(fromGUI);
		free(gui_argv[--j]);
		free(gui_argv[--j]);
		free(gui_argv);

		/* should we execute some script after returning from graphic mode? */
		if (post_gui_script)
			execute_script(post_gui_script);
	}
	else
	{
		/* create our already autologged temp file */
		int fd = creat(autologin_filename, S_IRUSR|S_IWUSR);
		close(fd);

		username     = autologin_username;
		password     = autologin_password;
		session      = autologin_session;
		returnstatus = EXIT_SUCCESS;
	}

	/* re-allow vt switching if it is still disabled */
	unlock_tty_switching();

	/* return to our righteous tty */
	set_active_tty(our_tty_number);

	switch (returnstatus)
	{
		case EXIT_SUCCESS:
			if (!auth_ok)
				auth_ok = check_password(username, password);

			if (auth_ok)				
			{
				if (password) memset(password, '\0', sizeof(password));
				start_session(username, session);
			}
			/* We won't get here unless there was a failure starting user session */
			fprintf(stderr, "\nLogin failed, reverting to text mode!\n");			
			/* Fall trough */
		case EXIT_RESPAWN:
			if (username) memset(username, '\0', sizeof(username));
      if (password) memset(password, '\0', sizeof(password));
			exit(EXIT_SUCCESS);
			break;
		case EXIT_TEXT_MODE:
			if (username) memset(username, '\0', sizeof(username));
      if (password) memset(password, '\0', sizeof(password));
			text_mode();
			break;
		case EXIT_SHUTDOWN_R:
			if (username) memset(username, '\0', sizeof(username));
      if (password) memset(password, '\0', sizeof(password));
			execl ("/sbin/shutdown", "/sbin/shutdown", "-r", "now", (char*)NULL);
			break;
		case EXIT_SHUTDOWN_H:
			if (username) memset(username, '\0', sizeof(username));
      if (password) memset(password, '\0', sizeof(password));
			execl ("/sbin/shutdown", "/sbin/shutdown", "-h", "now", (char*)NULL);
			break;
		case EXIT_SLEEP:
			if (username) memset(username, '\0', sizeof(username));
      if (password) memset(password, '\0', sizeof(password));
			if (sleep_cmd) execl (sleep_cmd, sleep_cmd, (char*)NULL);
			fprintf(stderr, "\nfatal error: could not execute sleep command!\n");
			exit(EXIT_FAILURE);
			break;
		default: /* user wants to switch to another tty ... */
			if (username) memset(username, '\0', sizeof(username));
      if (password) memset(password, '\0', sizeof(password));
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

	if (!do_autologin) return 0;

	/* Sanity checks */
	if (!autologin_username || !autologin_password || !autologin_session)
	{
		fprintf(stderr, "\nAutologin disabled: insuffucient user data!\n");
		return 0;
	}
	if (!strcmp(autologin_session, "LAST"))
	{
		free(autologin_session);
		autologin_session = get_last_session(autologin_username);
		if (!autologin_session)
		{
			fprintf(stderr, "\nAutologin disabled: could not get last session of user %s!\n", autologin_username);
			return 0;			
		}
	}

	if (auto_relogin) return 1;

	/* set autologin temp file name */
	temp = int_to_str(our_tty_number);
	autologin_filename =
		StrApp((char**)NULL, tmp_files_dir, "/", autologin_file_basename, "tty", temp, (char*)NULL);
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

	/* parse settings file */
	if (!load_settings())
		text_mode();

#ifdef WANT_CRYPTO
	/* generate (openssl) or restore (all others) public/private keys */
	if (!generate_keys())
	{
		fprintf(stderr, "\n\nqingy: key pair does not exist!\n");
		fprintf(stderr, "Make sure you run qingy-keygen to generate it\n\n");
		text_mode();
	}
#endif

	/* Should we log in user directly? Totally insecure, but handy */
	if (check_autologin(our_tty_number)) start_up(argc, argv, our_tty_number, 1);

	/* Should we perform a text mode login? */
	if (text_mode_login) text_mode();

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
