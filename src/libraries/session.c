/***************************************************************************
                         session.c  -  description
                            -------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003-2004 by Noberasco Michele
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

#include <fcntl.h>
#include <grp.h>
#include <lastlog.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <utmp.h>

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
# include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
# include <sys/dir.h>
# endif
# if HAVE_NDIR_H
# include <ndir.h>
# endif
#endif

#include <sys/types.h>
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#ifdef SHADOW_PASSWD
#include <shadow.h>
#endif

#include "session.h"
#include "memmgmt.h"
#include "vt.h"
#include "misc.h"
#include "load_settings.h"
#include "tty_guardian.h"

#define UNKNOWN_USER            0
#define WRONG_PASSWORD          1
#define OPEN_SESSION            2
#define CLOSE_SESSION           3
#define CANNOT_SWITCH_USER      4
#define CANNOT_CHANGE_TTY_OWNER 5

int current_vt;
extern char **environ;

#ifdef USE_PAM
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#define PAM_FAILURE             6
#define PAM_ERROR               7
#define PAM_UPDATE_TOKEN        8
#define PAM_CANNOT_UPDATE_TOKEN 9
char *PAM_password;
char *infostr=NULL, *errstr=NULL;
static pam_handle_t *pamh;
static int update_token = 0;

# ifdef sun
typedef struct pam_message pam_message_type;
# else
typedef const struct pam_message pam_message_type;
# endif

int PAM_conv (int num_msg, pam_message_type **msg, struct pam_response **resp, void *appdata_ptr)
{
  int count;
  struct pam_response *reply;
  
  if (appdata_ptr) {} /* just to avoid compiler warnings */
  reply = calloc(num_msg, sizeof(*reply));
  
  for (count = 0; count < num_msg; count++)
	{
		switch (msg[count]->msg_style)
		{
			case PAM_TEXT_INFO:
				StrApp(&infostr, msg[count]->msg, "\n", (char*)NULL);
				break;
			case PAM_ERROR_MSG:
				StrApp(&errstr, msg[count]->msg, "\n", (char*)NULL);
				break;
			case PAM_PROMPT_ECHO_OFF: /* wants password */
				reply[count].resp         = strdup(PAM_password);
				reply[count].resp_retcode = PAM_SUCCESS;
				break;
			case PAM_PROMPT_ECHO_ON: /* fall through */
			default: /* something wrong happened */
				for (; count >= 0; count--)
					free(reply[count].resp);
				free (reply);
				return PAM_CONV_ERR;				
		}
	}
  *resp = reply;
  return PAM_SUCCESS;  
}

struct pam_conv PAM_conversation =
{
	PAM_conv,
	NULL
};

#endif /* End of USE_PAM */


char *get_sessions(void)
{
  static DIR    *dir;
  struct dirent *entry;
  static char   *dirname = NULL;
  static int     status  = 0;
  
  if (!dirname) dirname = X_SESSIONS_DIRECTORY;
  
  switch (status)
	{
    case 0:
      status = 1;
      return strdup("Text: Console");
    case 1:
      status = 2;
      return strdup("Your .xsession");
    case 2:
      dir= opendir(dirname);
      if (!dir)
			{				
				fprintf(stderr, "session: unable to open directory \"%s\"\n", dirname);
				if (dirname == X_SESSIONS_DIRECTORY)
				{
					dirname = TEXT_SESSIONS_DIRECTORY;
					return get_sessions();
				}
				status = 0;
				return NULL;
			}
      status = 3;
    case 3:
      while (1)
			{
				if (!(entry= readdir(dir))) break;
				if (!strcmp(entry->d_name, "." )) continue;
				if (!strcmp(entry->d_name, "..")) continue;      
				if (dirname == X_SESSIONS_DIRECTORY)
				{
#ifdef slackware
					if (strncmp(entry->d_name, "xinitrc.", 8)) continue;
#else /* !slackware  */
#ifdef gentoo
					if (!strcmp(entry->d_name, "Xsession")) continue;
#endif /* gentoo     */
#endif /* !slackware */
					return strdup(entry->d_name);
				}
				else return StrApp((char**)NULL, "Text: ", entry->d_name, (char*)NULL);
			}
      closedir(dir);
      if (dirname == TEXT_SESSIONS_DIRECTORY)
			{
				dirname = NULL;
				status = 0;
				return NULL;
			}
      dirname = TEXT_SESSIONS_DIRECTORY;
      status = 2;
      return get_sessions();
	}
  
  return NULL;
}

/* write events to system logs */
void LogEvent(struct passwd *pw, int status)
{
  openlog("qingy", LOG_PID, LOG_AUTHPRIV);
  
  switch (status)
	{
    case UNKNOWN_USER:
      syslog(LOG_AUTHPRIV|LOG_WARNING, "Authentication failure: user '%s' is unknown\n", pw->pw_name);
      break;
    case WRONG_PASSWORD:
      syslog(LOG_AUTHPRIV|LOG_WARNING, "Authentication failure: wrong password for user '%s' (UID %d)\n", pw->pw_name, pw->pw_uid);
      break;
    case OPEN_SESSION:
      syslog(LOG_AUTHPRIV|LOG_INFO, "Session opened for user '%s' (UID %d)\n", pw->pw_name, pw->pw_uid);
      break;
    case CLOSE_SESSION:
      syslog(LOG_AUTHPRIV|LOG_INFO, "Session closed for user '%s' (UID %d)\n", pw->pw_name, pw->pw_uid);
      break;
    case CANNOT_SWITCH_USER:
      syslog(LOG_AUTHPRIV|LOG_WARNING, "Fatal Error: cannot switch user id!\n");
      break;
    case CANNOT_CHANGE_TTY_OWNER:
      syslog(LOG_AUTHPRIV|LOG_WARNING, "Fatal Error: cannot change tty owner!\n");
      break;
#ifdef USE_PAM
    case PAM_FAILURE:
      syslog(LOG_AUTHPRIV|LOG_WARNING, "Unrecoverable error: PAM failure!\n");
      break;
    case PAM_ERROR:
      syslog(LOG_AUTHPRIV|LOG_WARNING, "PAM was unable to authenticate user '%s' (UID %d)\n", pw->pw_name, pw->pw_uid);
      break;
    case PAM_UPDATE_TOKEN:
      syslog(LOG_AUTHPRIV|LOG_WARNING, "user '%s' (UID %d) authentication token has expired!\n", pw->pw_name, pw->pw_uid);
      break;
    case PAM_CANNOT_UPDATE_TOKEN:
      syslog(LOG_AUTHPRIV|LOG_WARNING, "Cannot update authentication token for user '%s' (UID %d)!\n", pw->pw_name, pw->pw_uid);
      break;
#endif	
    default:
      syslog(LOG_AUTHPRIV|LOG_WARNING, "Error #666, coder brains are severely damaged!\n");
	}
  
  closelog();
}

int check_password(char *username, char *user_password)
{
  struct passwd *pw;
  char *password;
#ifdef USE_PAM
  int retcode;
  char *ttyname;
  char *short_ttyname;
#else
  char*  correct;
#ifdef HAVE_LIBCRYPT
  char*  encrypted;
#endif
#ifdef SHADOW_PASSWD
  struct spwd* sp;
#endif
#endif /* End of USE_PAM */
  
  if (!username) return 0;
  if (!user_password) password = "\0";
  else password = user_password;
  
  pw = getpwnam(username);
  endpwent();
  if (!pw)
	{
		struct passwd pwd;
      
		pwd.pw_name = username;
		LogEvent(&pwd, UNKNOWN_USER);
		return 0;
	}
  
#ifdef USE_PAM
  PAM_password = (char *)password;
  ttyname = create_tty_name(get_active_tty());
  if ((short_ttyname = strrchr(ttyname, '/')) != NULL)
	if (*(++short_ttyname) == '\0') short_ttyname = NULL;
  if (pam_start("qingy", username, &PAM_conversation, &pamh) != PAM_SUCCESS)
	{
		LogEvent(pw, PAM_FAILURE);
		return 0;
	}
	if (!short_ttyname)
		retcode = pam_set_item(pamh, PAM_TTY, ttyname);
	else
	{
		retcode = pam_set_item(pamh, PAM_TTY, short_ttyname);
		if (retcode != PAM_SUCCESS)
			retcode = pam_set_item(pamh, PAM_TTY, ttyname);
	}
  if (retcode != PAM_SUCCESS)
	{
		pam_end(pamh, retcode);
		pamh = NULL;
		LogEvent(pw, PAM_FAILURE);
		return 0;
	}
  if ((retcode = pam_set_item(pamh, PAM_RHOST, "")) != PAM_SUCCESS)
	{
		pam_end(pamh, retcode);
		pamh = NULL;
		LogEvent(pw, PAM_FAILURE);
		return 0;
	}
  free (infostr); free (errstr);
  
  if ((retcode = pam_authenticate(pamh, PAM_DISALLOW_NULL_AUTHTOK)) != PAM_SUCCESS)
	{
		pam_end(pamh, retcode);
		pamh = NULL;
		switch (retcode)
		{
			case PAM_USER_UNKNOWN:
				LogEvent(pw, UNKNOWN_USER);
				break;
			case PAM_AUTH_ERR:
				LogEvent(pw, WRONG_PASSWORD);
				break;
			default:
				LogEvent(pw, PAM_ERROR);
		}
		return 0;
	}
  if ((retcode = pam_acct_mgmt(pamh, PAM_DISALLOW_NULL_AUTHTOK)) != PAM_SUCCESS)
	{
		pam_end(pamh, retcode);
		pamh = NULL;
		switch (retcode)
		{
			case PAM_NEW_AUTHTOK_REQD:
				LogEvent(pw, PAM_UPDATE_TOKEN);
				update_token = 1;
				return 1;
				break;
			case PAM_USER_UNKNOWN:
				LogEvent(pw, UNKNOWN_USER);
				break;
			default:
				LogEvent(pw, PAM_ERROR);
		}
		return 0;
	}
  return 1;
#else /* thus we don't USE_PAM */
#ifdef SHADOW_PASSWD
  sp = getspnam(pw->pw_name);
  endspent();
  if (sp) correct = sp->sp_pwdp;
  else
#endif
    correct = pw->pw_passwd;
  if (correct == 0 || correct[0] == '\0') return 1;
#ifdef HAVE_LIBCRYPT
  encrypted = crypt(password, correct);
  if (!strcmp(encrypted, correct)) return 1;
#endif
  LogEvent(pw, WRONG_PASSWORD);
  return 0;
#endif /* End of USE_PAM */
}

char *shell_base_name(char *name)
{
  char *base = name;
  char *temp = name;
  
  if (!name) return NULL;
  while (*temp)
	{
		if (*temp == '/') base = temp + 1;
		temp++;
	}
  
  return StrApp((char**)NULL, "-", base, (char*)NULL);
}

void setEnvironment(struct passwd *pwd, int is_x_session)
{
#ifdef USE_PAM
	int    i;
	char **env   = pam_getenvlist(pamh);
#endif
  char  *mail  = StrApp((char**)NULL, _PATH_MAILDIR, "/", pwd->pw_name, (char*)NULL);
	char  *path  = strdup("/bin:/usr/bin:/usr/local/bin:/usr/X11R6/bin");
	char  *xinit = NULL;
  
  environ    = (char **) calloc(2, sizeof(char *));
  environ[0] = NULL;

	/*
	 * if XINIT is set, we add it's path to PATH,
	 * assuming that X will sit in the same directory...
	 */
	if (XINIT)
	{
		int i = strlen(XINIT);

		for (; i>=0; i--)
			if (XINIT[i] == '/')
			{
				xinit = strndup(XINIT, (i+1)*sizeof(char));
				break;
			}
	}
	if (xinit)
	{
		StrApp(&path, ":", xinit, (char*)NULL);
		free(xinit);
	}

	setenv("PATH",    path,          1);
  setenv("TERM",    "linux",       1);
  setenv("HOME",    pwd->pw_dir,   1);
  setenv("SHELL",   pwd->pw_shell, 1);
  setenv("USER",    pwd->pw_name,  1);
  setenv("LOGNAME", pwd->pw_name,  1);
  setenv("MAIL",    mail,          1);
  chdir(pwd->pw_dir);
  free(mail);
	free(path);

#ifdef USE_PAM
	if (env)
		for (i = 0; env[i++];)
			putenv (env[i - 1]);
#endif

	/* We unset DISPLAY if this is not an X session... */
	if (!is_x_session)
		unsetenv("DISPLAY");
}

void restore_tty_ownership(void)
{
  char *our_tty_name = create_tty_name(current_vt);
  
  /* Restore tty ownership to root:tty */
  chown(our_tty_name, 0, 5);
  free(our_tty_name);
}

void switchUser(struct passwd *pwd, int is_x_session)
{
  char *our_tty_name = create_tty_name(current_vt);
  
  /* Change tty ownership to uid:tty */
  if (chown(our_tty_name, pwd->pw_uid, 5))
	{
		LogEvent(pwd, CANNOT_CHANGE_TTY_OWNER);
		free(our_tty_name);
		exit(EXIT_FAILURE);
	}
  free(our_tty_name);
  
  /* Set user id */
  if (initgroups(pwd->pw_name, pwd->pw_gid) || setgid(pwd->pw_gid) || setuid(pwd->pw_uid))
	{
		LogEvent(pwd, CANNOT_SWITCH_USER);
		exit(EXIT_FAILURE);
	}
  
  /* Set enviroment variables */
  setEnvironment(pwd, is_x_session);
}

/* write login entry to /var/log/lastlog */
void dolastlog(struct passwd *pwd, int quiet)
{
  struct lastlog ll;
  int fd;
  char *hostname = (char *) calloc(UT_HOSTSIZE, sizeof(char));
  char *tty_name = (char *) calloc(UT_LINESIZE, sizeof(char));
  char *temp = int_to_str(current_vt);
  
  gethostname(hostname, UT_HOSTSIZE);
  strncpy(tty_name, "tty", UT_LINESIZE);
  strncat(tty_name, temp, UT_LINESIZE);
  free(temp);
  
  if ((fd = open(_PATH_LASTLOG, O_RDWR, 0)) >= 0)
	{
		lseek(fd, (off_t)pwd->pw_uid * sizeof(ll), SEEK_SET);
		if (!quiet)
		{
			if (read(fd, (char *)&ll, sizeof(ll)) == sizeof(ll) && ll.ll_time != 0)
	    {
	      printf("Last login: %.*s ", 24-5, (char *)ctime(&ll.ll_time));
	      if (*ll.ll_host != '\0')
					printf("from %.*s\n", (int)sizeof(ll.ll_host), ll.ll_host);
	      else
					printf("on %.*s\n", (int)sizeof(ll.ll_line), ll.ll_line);
	    }
			lseek(fd, (off_t)pwd->pw_uid * sizeof(ll), SEEK_SET);
		}
		memset((char *)&ll, 0, sizeof(ll));
		time(&ll.ll_time);
		xstrncpy(ll.ll_line, tty_name, sizeof(ll.ll_line));
		if (hostname) xstrncpy(ll.ll_host, hostname, sizeof(ll.ll_host));
		write(fd, (char *)&ll, sizeof(ll));
		close(fd);
	}
  free(hostname);
  free(tty_name);
}

/*
 * Write entries in utmp and wtmp:
 * this is necesssary for commands like 'w' and 'who'
 * to detect users that logged in with qingy...
 */
void add_utmp_wtmp_entry(char *username)
{
	struct utmp ut;
	pid_t mypid = getpid ();
	char *temp = int_to_str(current_vt);
	char *ttyn = StrApp((char**)NULL, "/dev/tty", temp, (char*)NULL);

	free(temp);
	utmpname (_PATH_UTMP);
	setutent ();
	memset (&ut, 0, sizeof (ut));
	strncpy (ut.ut_id, ttyn + strlen (_PATH_TTY), sizeof (ut.ut_id));
	strncpy (ut.ut_user, username, sizeof (ut.ut_user));
	strncpy (ut.ut_line, ttyn + strlen (_PATH_DEV), sizeof (ut.ut_line));
	ut.ut_line[sizeof (ut.ut_line) - 1] = 0;
	time(&ut.ut_time);
	ut.ut_type = USER_PROCESS;
	ut.ut_pid = mypid;
	pututline (&ut);
	endutent ();
	updwtmp (_PATH_WTMP, &ut);
	free(ttyn);
}

/* Remove entries from utmp */
void remove_utmp_entry(void)
{
	struct utmp ut;
	pid_t mypid = getpid ();
	char *temp = int_to_str(current_vt);
	char *ttyn = StrApp((char**)NULL, "/dev/tty", temp, (char*)NULL);

	free(temp);
	utmpname (_PATH_UTMP);
	setutent ();
	memset (&ut, 0, sizeof (ut));
	strncpy (ut.ut_id, ttyn + strlen (_PATH_TTY), sizeof (ut.ut_id));
	ut.ut_pid = mypid;
	free(ttyn);
	ut.ut_type=DEAD_PROCESS;
	memset(ut.ut_line,0,UT_LINESIZE);
	ut.ut_time=0;
	memset(ut.ut_user,0,UT_NAMESIZE);
	setutent();
	pututline(&ut);
	endutent ();
}

void Text_Login(struct passwd *pw, char *session, char *username)
{
  pid_t proc_id;
  char *args[4] = {NULL, NULL, NULL, NULL};
#ifdef USE_PAM
  int retval;
#endif
  
  args[0] = shell_base_name(pw->pw_shell);
  if (!session || strcmp(session+6, "Console"))
	{
		args[1] = strdup("-c");
		args[2] = StrApp((char **)NULL, TEXT_SESSIONS_DIRECTORY, "\"", session+6, "\"", (char *)NULL);
	}
  
  proc_id = fork();
  if (proc_id == -1)
	{
		fprintf(stderr, "session: fatal error: cannot issue fork() command!\n");
		free(args[0]); free(args[1]); free(args[2]); 
		exit(EXIT_FAILURE);
	}
  if (!proc_id)
	{
		/* write to system logs */
		dolastlog(pw, 0);
		add_utmp_wtmp_entry(username);
#ifdef USE_PAM
		pam_setcred(pamh, PAM_ESTABLISH_CRED);
		pam_open_session(pamh, 0);
#else
		LogEvent(pw, OPEN_SESSION);
#endif
      
    /* drop root privileges and set user enviroment */
		switchUser(pw, 0);
      
		/* let's start the shell! */
		execve(pw->pw_shell, args, environ);
      
		/* execve should never return! */
		fprintf(stderr, "session: fatal error: cannot start your session!\n");
		exit(0);
	}
  set_last_user(username);
  set_last_session(username, session);	  	
  
  if (!lock_sessions) wait(NULL);
	else
		ttyWatchDog(proc_id, username, current_vt, 0);
	
	free(username); free(session);
	
#ifdef USE_PAM
  pam_setcred(pamh, PAM_DELETE_CRED);
  retval = pam_close_session(pamh, 0);
  pam_end(pamh, retval);
  pamh = NULL;
#else
  LogEvent(pw, CLOSE_SESSION);
#endif
  
	remove_utmp_entry();	
	
  /* Restore tty ownership to root:tty */
  restore_tty_ownership();
  
  /* free allocated stuff */
  free(args[0]);
  free(args[1]);
  free(args[2]);
  exit(EXIT_SUCCESS);
}

/*
 * if somebody else, somewhere else, sometime else
 * somehow started some other X servers ;-)
 */
int which_X_server(void)
{
  FILE *fp;
  char filename[20] = "/tmp/.X1-lock";
  int num = 1;
  
  /*
   * we start our search from server :1 (instead of :0)
   * to allow a console user to start his own X server
   * using the default startx command
   */
  
  while ((fp = fopen(filename, "r")))
  {
    char *temp = int_to_str(++num);
    fclose(fp);
    strcpy(filename, "/tmp/.X");
    strcat(filename, temp);
    free(temp);
    strcat(filename, "-lock");
  }
  
  return num;
}

void Graph_Login(struct passwd *pw, char *session, char *username)
{
  pid_t proc_id;
  int dest_vt = get_available_tty();
  char *x_server = int_to_str(which_X_server());
  char *vt = NULL;
  char *args[4];
#ifdef USE_PAM
  int retval;
#endif

  if (dest_vt != -1) vt = int_to_str(dest_vt);
  else
	{
		fprintf(stderr, "%s: fatal error: cannot find an unused vt!\n", program_name);
		sleep(5); /* be nice to init and allow user to read message */
		exit(EXIT_FAILURE);
	}
  
  args[0] = shell_base_name(pw->pw_shell);
  args[1] = strdup("-c");

  // now we compose the xinit launch command line
	args[2] = StrApp((char**)NULL, XINIT, " ", (char*)NULL);

  // add the chosen X session
  if (!strcmp(session, "Your .xsession"))
    args[2] = StrApp(&(args[2]), "$HOME/.xsession -- ", (char*)NULL);
  else
    args[2] = StrApp(&(args[2]), X_SESSIONS_DIRECTORY, "\"", session, "\" -- ", (char*)NULL);

  // add the chosed X server, if one was chosen
	if (X_SERVER)
		args[2] = StrApp(&(args[2]), X_SERVER, " :", x_server, " vt", vt, (char*)NULL);
	else
		args[2] = StrApp(&(args[2]), ":", x_server, " vt", vt, (char*)NULL);

  // add the supplied X server arguments, if supplied
	if (X_ARGS)
		args[2] = StrApp(&(args[2]), " ", X_ARGS, (char*)NULL);

  // done... as a final touch we suppress verbose output
	args[2] = StrApp(&(args[2]), " >& /dev/null", (char*)NULL);

  args[3] = NULL;
  free(x_server);
  free(vt);
  
  proc_id = fork();
  if (proc_id == -1)
	{
		fprintf(stderr, "session: fatal error: cannot issue fork() command!\n");
		free(args[0]); free(args[1]); free(args[2]);
		exit(EXIT_FAILURE);
	}
  if (!proc_id)
	{
		/* write to system logs */
		dolastlog(pw, 1);
		add_utmp_wtmp_entry(username);
#ifdef USE_PAM
		pam_setcred(pamh, PAM_ESTABLISH_CRED);
		pam_open_session(pamh, 0);
#else
		LogEvent(pw, OPEN_SESSION);
#endif
      
		/* drop root privileges and set user enviroment */
		switchUser(pw, 1);
      
		/* start X server and selected session! */
		execve(pw->pw_shell, args, environ);
      
		/* execve should never return! */
		fprintf(stderr, "session: fatal error: cannot start your session!\n");
		exit(EXIT_FAILURE);
	}
  set_last_user(username);
  set_last_session(username, session);  
  
  /* wait a bit, then clear console from X starting messages */
  sleep(3);
  ClearScreen();
  fprintf(stderr, "Switching to X Server...\n");
  
  /* while X server is active, we wait for user
     to switch to our tty and redirect him there */
  if (!lock_sessions)
		while(1)
		{
			pid_t result;
      
			result = waitpid(-1, NULL, WNOHANG);
			if (!result || result == -1)
			{
				if (get_active_tty() == current_vt) set_active_tty(dest_vt);
				sleep(1);
			}
			else break;
		}
	else
		ttyWatchDog(proc_id, username, current_vt, dest_vt);

	free(username); free(session);
		
#ifdef USE_PAM
  pam_setcred(pamh, PAM_DELETE_CRED);
  retval = pam_close_session(pamh, 0);
  pam_end(pamh, retval);
  pamh = NULL;
#else
  LogEvent(pw, CLOSE_SESSION);
#endif
  if (get_active_tty() == dest_vt) set_active_tty(current_vt);  

	remove_utmp_entry();
  
  /* Restore tty ownership to root:tty */
  restore_tty_ownership();
  
  /* disallocate tty X was running in */
  disallocate_tty(dest_vt);
  
  free(args[0]);
  free(args[1]);
  free(args[2]);
  exit(EXIT_SUCCESS);
}

/* Start the session of your choice */
void start_session(char *username, char *session)
{
  struct passwd *pwd = getpwnam(username);
  
  endpwent();
  
  current_vt = get_active_tty();
  
  if (!pwd)
	{
		struct passwd pw;

		pw.pw_name = username;
		LogEvent(&pw, UNKNOWN_USER);
		free(username);
		free(session);
		exit(EXIT_FAILURE);
	}
  
  ClearScreen();
  
#ifdef USE_PAM
  if (update_token)
	{ /*
		 * This is a quick hack... for some reason, if I try to
		 * do things properly, PAM output on screen when updating
		 * authorization token is all garbled, so for now I'll
		 * leave this to /bin/login
		 */
		printf("You need to update your authorization token...\n");
		printf("After that, log out and in again.\n\n");
		execl("/bin/login", "/bin/login", "--", username, (char*)NULL);
		exit(EXIT_SUCCESS);
	}
#endif
  
  if (!strncmp(session, "Text: ", 6)) Text_Login(pwd, session, username);
  else Graph_Login(pwd, session, username);
  
  /* we don't get here unless we couldn't start user session */
  fprintf(stderr, "Couldn't login user '%s'!\n", username);
  exit(EXIT_FAILURE);
}
