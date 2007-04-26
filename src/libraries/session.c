/***************************************************************************
                         session.c  -  description
                            -------------------
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
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <utmp.h>
#include <sys/stat.h>

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


#ifdef WANT_CRYPTO
#include "crypto.h"
#endif

#include "session.h"
#include "memmgmt.h"
#include "vt.h"
#include "misc.h"
#include "load_settings.h"
#include "tty_guardian.h"
#include "logger.h"

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
  
#ifdef USE_X
  if (!dirname) dirname = x_sessions_directory;
#else
  if (!dirname) dirname = text_sessions_directory;
#endif /* USE_X */
  
  switch (status)
	{
    case 0:
#ifndef gentoo
#ifndef slackware
#ifdef USE_X
			{
				struct stat dirstat;
				int         createdir   = 0;
				int         populatedir = 1;

				if (stat(dirname, &dirstat) == -1)
					createdir = 1;

				if (!createdir)
					if (!S_ISDIR(dirstat.st_mode))
						createdir = 1;

				if (createdir)
					if (mkdir(dirname, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) == -1)
					{
						WRITELOG(ERROR, "Unable to create directory \"%s\"\n", dirname);
						populatedir = 0;
					}

				populatedir=1;

				if (populatedir)
				{
					DIR *dir = opendir("/usr/share/xsessions");
					if (dir)
					{
						struct dirent *direntry;
						while (1)
						{
							char *filename;
							FILE *fp;

							if (!(direntry = readdir(dir))) break;
							if (!strncmp(direntry->d_name, ".", 1)) continue;

							filename = StrApp((char**)NULL, "/usr/share/xsessions/", direntry->d_name, (char*)NULL);
							fp = fopen(filename, "r");
							free(filename);

							if (fp)
							{
								char   *buf = NULL;
								size_t  len = 0;
								while (-1 != getline(&buf, &len, fp))
								{
									int offset = 0;
									if (!strncmp(buf, "Exec=",    5)) offset = 5;
									if (!strncmp(buf, "TryExec=", 8)) offset = 8;

									if (offset)
									{
										char *content  = buf + offset;
										char *extension= strstr(direntry->d_name, ".desktop");
										char *filename = (extension) ? strndup(direntry->d_name, extension-direntry->d_name) : strdup(direntry->d_name);
										char *pathname = StrApp((char**)NULL, dirname, "/", filename, (char*)NULL);
										int   fd       = open(pathname, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
										FILE *fileout  = fdopen(fd, "w");

										free(filename);
										free(pathname);

										if (fileout)
										{
											fprintf(fileout, "%s\n", content);
											fclose(fileout);
										}
										close(fd);
										break;
									}
								}
								fclose(fp);
							}
						}
						closedir(dir);
					}
				}
			}
#endif /* USE_X */
#endif /* gentoo */
#endif /* slackware */
      status = 1;
      return strdup("Text: Console");
    case 1:
      status = 2;
#ifdef USE_X
      return strdup("Your .xsession");
#else
			return get_sessions();
#endif /* USE_X */
    case 2:
      dir= opendir(dirname);
      if (!dir)
			{				
				WRITELOG(ERROR, "Unable to open directory \"%s\"\n", dirname);
				if (dirname == x_sessions_directory)
				{
					dirname = text_sessions_directory;
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
				if (!strncmp(entry->d_name, ".", 1)) continue;
				if (dirname == x_sessions_directory)
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
      if (dirname == text_sessions_directory)
			{
				dirname = NULL;
				status = 0;
				return NULL;
			}
      dirname = text_sessions_directory;
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

int gui_check_password(char *username, char *password, char *session, int ppid)
{
	char   result[10];
	time_t start;

	strcpy(result, "\0");

	/* sent auth data to our parent... */
#ifdef WANT_CRYPTO
	encrypt_item(stdout, username);
	encrypt_item(stdout, password);
	encrypt_item(stdout, session);
#else
	fprintf(stdout, "%s\n%s\n%s\n", username, password, session);
#endif
	fflush(stdout);

	/* ...signal it to check auth data... */
	if (kill (ppid, SIGUSR1))
	  return 0;

	start = time(NULL);

	/* ... wait until it has done so... and set a handy timeout! */
	while (time(NULL) - start <= 10)
	{
		sleep(1);

		if (fscanf(stdin, "%9s", result) != EOF)
			break;
	}

	/* if a timeout occurred, return error */
	if (time(NULL) - start > 10)
		return -1;

	/* ...finally, fetch the results */
	if (!strcmp(result, "AUTH_OK"))
		return 1;

	return 0;
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
  if (!user_password) password = strdup("\0");
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

static char *shell_base_name(char *name)
{
  char *base = name;
  char *temp = name;
  
  if (!name) return NULL;
  while (*temp)
	{
		if (*temp == '/') base = temp + 1;
		temp++;
	}
  
  return base;
}

static char *add_escapes(char *filename)
{
	unsigned int i, j, len;
	char *escaped_filename;

	if (!filename) return NULL;

	len = strlen(filename);

	/* how many escapes should we add? */
	for (i=0; filename[i] != '\0'; i++)
		if (filename[i] == ' ' || filename[i] == '\t')
			len++;

	escaped_filename = (char*)calloc(len+1, sizeof(char));

	for (j=i=0; i<strlen(filename); i++)
	{
		if (filename[i] == ' ' || filename[i] == '\t')
			escaped_filename[j++] = '\\';
		escaped_filename[j++] = filename[i];
	}
	escaped_filename[j] = '\0';
	
	return escaped_filename;
}

void setEnvironment(struct passwd *pwd, int is_x_session)
{
#ifdef USE_PAM
	int    i;
	char **env      = pam_getenvlist(pamh);
#endif
  char  *mail     = StrApp((char**)NULL, _PATH_MAILDIR, "/", pwd->pw_name, (char*)NULL);
	char  *path     = strdup("/bin:/usr/bin:/usr/local/bin:/usr/X11R6/bin");
	char  *my_xinit = NULL;
  
  environ    = (char **) calloc(2, sizeof(char *));
  environ[0] = NULL;

	/*
	 * if XINIT is set, we add it's path to PATH,
	 * assuming that X will sit in the same directory...
	 */
	if (xinit)
	{
		int i = strlen(xinit);

		for (; i>=0; i--)
			if (xinit[i] == '/')
			{
				my_xinit = strndup(xinit, (i+1)*sizeof(char));
				break;
			}
	}
	if (my_xinit)
	{
		StrApp(&path, ":", my_xinit, (char*)NULL);
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
	int   gid          = get_group_id("tty");

	if (!gid)
	{
		writelog(ERROR, "Could not restore tty ownership to root:tty, as group tty does not exist\n");
		return;
	}

  /* Restore tty ownership to root:tty */
  chown(our_tty_name, 0, gid);
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
				time_t when = ll.ll_time;
				printf("Last login: %.*s ", 24-5, (char*)ctime(&when));
	      if (*ll.ll_host != '\0')
					printf("from %.*s\n", (int)sizeof(ll.ll_host), ll.ll_host);
	      else
					printf("on %.*s\n", (int)sizeof(ll.ll_line), ll.ll_line);
	    }
			lseek(fd, (off_t)pwd->pw_uid * sizeof(ll), SEEK_SET);
		}
		memset((char *)&ll, 0, sizeof(ll));
		ll.ll_time = time(NULL);
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
  char *args[5] = {NULL, NULL, NULL, NULL, NULL};
	int   count   = 0;
	int   i       = 0;
#ifdef USE_PAM
  int retval;
#endif
  
  
	args[count++] = StrApp((char**)NULL, "-", shell_base_name(pw->pw_shell), (char*)NULL); /* make it a login shell */

	if (session)
		if (strcmp(session+6, "Console"))
		{
			char *session_name = add_escapes(session+6);

			args[count++] = strdup("-c");
			args[count++] = StrApp((char **)NULL, text_sessions_directory, session_name, (char *)NULL);

			free(session_name);
		}

  /* done... as a final touch we suppress verbose output */
	if (DEBUG <= max_loglevel)
		for (; args[i]; i++)
			WRITELOG(DEBUG, "Starting text session with argument #%d: %s\n", i, args[i]);

  proc_id = fork();
  if (proc_id == -1)
	{
		writelog(ERROR, "Cannot issue fork() command!\n");
		free(args[0]); free(args[1]); free(args[2]); free(args[3]);
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
      
		/* remove last session file from user home dir, as previous qingy versions
			 created it with root ownership
		*/
		wipe_last_session_file(username);

    /* drop root privileges and set user enviroment */
		switchUser(pw, 0);

		/* save last session for this user */
		set_last_session_user(username, session);

		/* let's start the shell! */
		execve(pw->pw_shell, args, environ);
      
		/* execve should never return! */
		WRITELOG(ERROR, "Cannot start your session: %s!\n", strerror(errno));
		exit(0);
	}

	/* detach from teminal */
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);

  set_last_user(username);
  set_last_session_tty(session, current_vt);

  watch_over_session(proc_id, username, current_vt, current_vt, 0, 0);

  memset(username, '\0', sizeof(username));	
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
	free(args[3]);
  exit(EXIT_SUCCESS);
}

/*
 * if somebody else, somewhere else, sometime else
 * somehow started some other X servers ;-)
 */
int which_X_server(void)
{
  FILE *fp;
  int   num      = x_server_offset;
	char *temp     = int_to_str(num);
  char *filename = StrApp((char**)NULL, "/tmp/.X", temp, "-lock", (char*)NULL);
  
  /*
   * we start our search from server :x_server_offset (default is :1, instead of :0)
   * to allow a console user to start his own X server
   * using the default startx command
   */

	free(temp);

  while ((fp = fopen(filename, "r")))
  {
    fclose(fp);
		free(filename);
    temp = int_to_str(++num);
		filename = StrApp((char**)NULL, "/tmp/.X", temp, "-lock", (char*)NULL);
		free(temp);
	}

	free(filename);

  return num;
}

void Graph_Login(struct passwd *pw, char *session, char *username)
{
  pid_t proc_id;
	int   x_offset    = which_X_server();
  char *my_x_server = int_to_str(x_offset);
  char *vt   = NULL;
	int   x_vt = current_vt;
  char *args[5] = {NULL, NULL, NULL, NULL, NULL};
	int   count   = 0;
	int   i       = 0;
#ifdef USE_PAM
  int retval;
#endif

	if (x_serv_tty_mgmt == UNUSED_TTY)
		x_vt = get_available_tty();

	vt = int_to_str(x_vt);
  
	args[count++] = StrApp((char**)NULL, "-", shell_base_name(pw->pw_shell), (char*)NULL); /* make it a login shell */
  args[count++] = strdup("-c");

  /* now we compose the xinit launch command line */
	args[count] = StrApp((char**)NULL, "exec ", xinit, " ", (char*)NULL);

  /* add the chosen X session */
  if (!strcmp(session, "Your .xsession"))
    args[count] = StrApp(&(args[count]), "$HOME/.xsession -- ", (char*)NULL);
  else
	{
		char *session_name = add_escapes(session);

    args[count] = StrApp(&(args[count]), x_sessions_directory, session_name, " -- ", (char*)NULL);
		free(session_name);
	}

  /* add the chosen X server, if one has been chosen */
	if (x_server)
		args[count] = StrApp(&(args[count]), x_server, " :", my_x_server, " vt", vt, (char*)NULL);
	else
		args[count] = StrApp(&(args[count]), ":", my_x_server, " vt", vt, (char*)NULL);

  /* add the supplied X server arguments, if supplied */
	if (x_args)
		args[count] = StrApp(&(args[count]), " ", x_args, (char*)NULL);

  /* done... as a final touch we suppress verbose output */
	if (DEBUG > max_loglevel)
		args[count] = StrApp(&(args[count]), " >& /dev/null", (char*)NULL);
	else
		for (; args[i]; i++)
			WRITELOG(DEBUG, "Starting X session with argument #%d: %s\n", i, args[i]);

  free(my_x_server);
  free(vt);

  proc_id = fork();
  if (proc_id == -1)
	{
		writelog(ERROR, "Cannot issue fork() command!\n");
		free(args[0]); free(args[1]); free(args[2]); free(args[3]);
		exit(EXIT_FAILURE);
	}
  if (!proc_id)
	{
		char *ttyname = create_tty_name(current_vt);

		/* write to system logs */
		dolastlog(pw, 1);
		add_utmp_wtmp_entry(username);
#ifdef USE_PAM
		pam_setcred(pamh, PAM_ESTABLISH_CRED);
		pam_open_session(pamh, 0);
#else
		LogEvent(pw, OPEN_SESSION);
#endif

		/* remove last session file from user home dir, as previous qingy versions
			 created it with root ownership
		*/
		wipe_last_session_file(username);

		/* drop root privileges and set user enviroment */
		switchUser(pw, 1);

		/* clean up standard input, output, error */
    freopen("/dev/null", "r", stdin);
	  freopen(ttyname, "w", stdout);
		freopen(ttyname, "w", stderr);
		free(ttyname);

		/* save last session for this user */
		set_last_session_user(username, session);

		/* start X server and selected session! */
		execve(pw->pw_shell, args, environ);
      
		/* execve should never return! */
		WRITELOG(ERROR, "Cannot start your session: %s!\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* detach from teminal */
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);

  set_last_user(username);
  set_last_session_tty(session, current_vt);

	watch_over_session(proc_id, username, current_vt, x_vt, 1, x_offset);
	set_active_tty(current_vt);

  memset(username, '\0', sizeof(username));	
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
  
  /* disallocate tty X was running in */
	disallocate_tty(current_vt);
  
  free(args[0]);
  free(args[1]);
  free(args[2]);
	free(args[3]);
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
  WRITELOG(ERROR, "Couldn't login user '%s'!\n", username);
	sleep(3);
  exit(EXIT_FAILURE);
}

void sort_sessions(char **sessions, int n_items)
{
	int i          = 0;
	int x_sessions = 0;

	if (!sessions)    return;
	if (!(*sessions)) return;
	if (!n_items)     return;

	/* We divide X and text sessions ... */
	for (; i<(n_items-1); i++)
	{
		int j = i + 1;
		for (; j<n_items; j++)
			if (!strncmp(sessions[i], "Text: ", 6))
				if (strncmp(sessions[j], "Text: ", 6))
				{
					char *swap = sessions[i];
					sessions[i] = sessions[j];
					sessions[j] = swap;
					break;
				}

		if (strncmp(sessions[i], "Text: ", 6))
			x_sessions++;
	}

	/* ... we sort X sessions ... */
	for (i=0; i<(x_sessions-1); i++)
	{
		int j = i + 1;
		for (; j<x_sessions; j++)
			if (strcasecmp(sessions[i], sessions[j]) > 0)
			{
				char *swap = sessions[i];
				sessions[i] = sessions[j];
				sessions[j] = swap;
			}
	}

	/* ... and text ones */
	for (i=x_sessions; i<(n_items-1); i++)
	{
		int j = i + 1;
		for (; j<n_items; j++)
			if (strcasecmp(sessions[i], sessions[j]) > 0)
			{
				char *swap = sessions[i];
				sessions[i] = sessions[j];
				sessions[j] = swap;
			}
	}
}
