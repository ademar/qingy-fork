/***************************************************************************
                         session.c  -  description
                            -------------------
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
#include <directfb.h>

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
#include "chvt.h"
#include "misc.h"
#include "load_settings.h"
#include "directfb_combobox.h"

#define UNKNOWN_USER       0
#define WRONG_PASSWORD     1
#define OPEN_SESSION       2
#define CLOSE_SESSION      3
#define CANNOT_SWITCH_USER 4

int current_vt;
extern char **environ;

#ifdef USE_PAM
	#include <security/pam_appl.h>
	#include <security/pam_misc.h>
	#define PAM_FAILURE             5
	#define PAM_ERROR               6
	#define PAM_UPDATE_TOKEN        7
	#define PAM_CANNOT_UPDATE_TOKEN 8
	char *PAM_password;
	char *infostr, *errstr;
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

		if (appdata_ptr) {}
		if (!(reply = calloc(num_msg, sizeof(*reply)))) return PAM_CONV_ERR;

		for (count = 0; count < num_msg; count++)
		{
			switch (msg[count]->msg_style)
			{
				case PAM_TEXT_INFO:
					if (!StrApp(&infostr, msg[count]->msg, "\n", (char *)0))
					goto conv_err;
					break;
				case PAM_ERROR_MSG:
					if (!StrApp(&errstr, msg[count]->msg, "\n", (char *)0))
					goto conv_err;
					break;
				case PAM_PROMPT_ECHO_OFF:
	  	  	/* wants password */
					if (!StrDup (&reply[count].resp, PAM_password)) goto conv_err;
					reply[count].resp_retcode = PAM_SUCCESS;
					break;
				case PAM_PROMPT_ECHO_ON:
					/* user name given to PAM already */
					/* fall through */
				default:
					/* unknown */
					goto conv_err;
			}
		}
		*resp = reply;
		return PAM_SUCCESS;

		conv_err:
		for (; count >= 0; count--)
			if (reply[count].resp)
			{
				switch (msg[count]->msg_style)
				{
					case PAM_ERROR_MSG:
					case PAM_TEXT_INFO:
					case PAM_PROMPT_ECHO_ON:
						free(reply[count].resp);
						break;
					case PAM_PROMPT_ECHO_OFF:
						WipeStr(reply[count].resp);
						break;
				}
				reply[count].resp = 0;
			}
		/* forget reply too */
		free (reply);
		return PAM_CONV_ERR;
	}

	struct pam_conv PAM_conversation =
		{
			PAM_conv,
			NULL
		};

#endif /* End of USE_PAM */

void get_sessions(void *ext_sessions)
{
	DIR *dir;
	struct dirent *entry;
	ComboBox *sessions = (ComboBox *) ext_sessions;

	if (!sessions) return;
	sessions->AddItem(sessions, "Text Console");

	dir= opendir(XSESSIONS_DIRECTORY);
	if (dir == NULL)
	{
	  fprintf(stderr, "session: unable to open directory \"%s\"\n", XSESSIONS_DIRECTORY);
		return;
	}
	while ((entry= readdir(dir)) != NULL)
	{
	  if (strcmp(entry->d_name, "." ) != 0)
		if (strcmp(entry->d_name, "..") != 0)
		if (strcmp(entry->d_name, "Xsession") != 0)
			sessions->AddItem(sessions, entry->d_name);
	}
	closedir(dir);

	return;
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
	char ttyname[11];
#else
	char*  correct;
#ifdef HAVE_LIBCRYPT
	char*  encrypted;
#endif
#ifdef SHADOW_PASSWD
	struct spwd* sp;
#endif
#endif /* End of USE_PAM */

	if (user_password != NULL) password = user_password;
	else
	{
		password = (char *) calloc(1, sizeof(char));
		password[0] = '\0';
	}

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
	strcpy(ttyname, "/dev/tty");
	strcat(ttyname, int_to_str(get_active_tty()));
	if (pam_start("qingy", username, &PAM_conversation, &pamh) != PAM_SUCCESS)
	{
		LogEvent(pw, PAM_FAILURE);
		return 0;
	}
	if ((retcode = pam_set_item(pamh, PAM_TTY, ttyname)) != PAM_SUCCESS)
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
	if (infostr) { free (infostr); infostr = 0; }
	if (errstr)  { free (errstr);  errstr  = 0; }

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
	char *shellname;

	while (*temp)
	{
		if (*temp == '/') base = temp + 1;
		temp++;
	}

	shellname = (char *) calloc(strlen(base)+2, sizeof(char));
	*shellname = '-';
	strcat(shellname, base);

	return shellname;
}

void setEnvironment(struct passwd *pwd)
{
	char *mail;

	environ = (char **) calloc(2, sizeof(char *));
	environ[0] = 0;
	setenv("TERM", "linux", 0);
	setenv("HOME", pwd->pw_dir, 1);
	setenv("SHELL", pwd->pw_shell, 1);
	setenv("USER", pwd->pw_name, 1);
	setenv("LOGNAME", pwd->pw_name, 1);
	mail = (char *) calloc(strlen(_PATH_MAILDIR) + strlen(pwd->pw_name) + 2, sizeof(char));
	strcpy(mail, _PATH_MAILDIR);
	strcat(mail, "/");
	strcat(mail, pwd->pw_name);
	setenv("MAIL", mail, 1);
	chdir(pwd->pw_dir);
}

void switchUser(struct passwd *pwd)
{
	/* Set user id */
	if ((initgroups(pwd->pw_name, pwd->pw_gid) != 0) || (setgid(pwd->pw_gid) != 0) || (setuid(pwd->pw_uid) != 0))
	{
		LogEvent(pwd, CANNOT_SWITCH_USER);
		exit(0);
	}

	/* Execute login command */
	setEnvironment(pwd);
}

void xstrncpy(char *dest, const char *src, size_t n)
{
	strncpy(dest, src, n-1);
	dest[n-1] = 0;
}

/* write login entry to /var/log/lastlog */
void dolastlog(struct passwd *pwd, int quiet)
{
	struct lastlog ll;
	int fd;
	char *hostname = (char *) calloc(UT_HOSTSIZE, sizeof(char));
	char *tty_name = (char *) calloc(UT_LINESIZE, sizeof(char));

	gethostname(hostname, UT_HOSTSIZE);
	strncpy(tty_name, "tty", UT_LINESIZE);
	strncat(tty_name, int_to_str(current_vt), UT_LINESIZE);

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

void Text_Login(struct passwd *pw, char *username)
{
	pid_t proc_id;
	int retval;

	proc_id = fork();
	if (proc_id == -1)
	{
		fprintf(stderr, "session: fatal error: cannot issue fork() command!\n");
		exit(0);
	}
	if (!proc_id)
	{
		/* set up stuff */
		char *args[2];
		args[0] = shell_base_name(pw->pw_shell);
		args[1] = NULL;

		/* write to system logs */
		dolastlog(pw, 0);
#ifdef USE_PAM
		pam_setcred(pamh, PAM_ESTABLISH_CRED);
		pam_open_session(pamh, 0);
#else
		LogEvent(pw, OPEN_SESSION);
#endif

		/* drop root privileges and set user enviroment */
		switchUser(pw);

		/* let's start the shell! */
		execve(pw->pw_shell, args, environ);

		/* execve should never return! */
		fprintf(stderr, "session: fatal error: cannot start your session!\n");
		exit(0);
	}
	set_last_user(username);
	set_last_session(username, "Text Console");
	free(username);

	wait(NULL);
#ifdef USE_PAM
	pam_setcred(pamh, PAM_DELETE_CRED);
	retval = pam_close_session(pamh, 0);
	pam_end(pamh, retval);
	pamh = NULL;
#else
	LogEvent(pw, CLOSE_SESSION);
#endif

	exit(0);
}

/* if somebody else, somewhere else, sometime else
   somehow started some other X servers ;-)        */
int which_X_server(void)
{
	FILE *fp;
	char filename[20];
	int num=1;

	/* we start our search from server :1 (instead of :0)
	   to allow a console user to start his own X server
		 using the default startx command                   */

	strcpy(filename, "/tmp/.X1-lock");
	while( (fp = fopen(filename, "r")) != NULL)
	{
		num++;
		fclose(fp);
		strcpy(filename, "/tmp/.X");
		strcat(filename, int_to_str(num));
		strcat(filename, "-lock");
	}

	return num;
}

void Graph_Login(struct passwd *pw, char *session, char *username)
{
	pid_t proc_id;
	int dest_vt = current_vt + 20;
	int retval;

	proc_id = fork();
	if (proc_id == -1)
	{
		fprintf(stderr, "session: fatal error: cannot issue fork() command!\n");
		exit(0);
	}
	if (!proc_id)
	{
		/* set up stuff */
		char *args[4];
		args[0] = shell_base_name(pw->pw_shell);
		args[1] = (char *) calloc(3, sizeof(char));
		strcpy(args[1], "-c");
		args[2] = (char *) calloc(MAX, sizeof(char));
		strcpy(args[2], XINIT);
		strcat(args[2], " ");
		strcat(args[2], XSESSIONS_DIRECTORY);
		strcat(args[2], session);
		strcat(args[2], " -- :");
		strcat(args[2], int_to_str(which_X_server()));
		strcat(args[2], " vt");
		strcat(args[2], int_to_str(dest_vt));
		strcat(args[2], ">/dev/null 2>/dev/null");
		args[3] = NULL;

		/* write to system logs */
		dolastlog(pw, 1);
#ifdef USE_PAM
		pam_setcred(pamh, PAM_ESTABLISH_CRED);
		pam_open_session(pamh, 0);
#else
		LogEvent(pw, OPEN_SESSION);
#endif

		/* drop root privileges and set user enviroment */
		switchUser(pw);

		/* start X server and selected session! */
		execve(pw->pw_shell, args, environ);

		/* execve should never return! */
		fprintf(stderr, "session: fatal error: cannot start your session!\n");
		exit(0);
	}
	set_last_user(username);
	set_last_session(username, session);
	free(username);

	/* wait a bit, then clear console from X starting messages */
	sleep(3);
	ClearScreen();
	fprintf(stderr, "Switching to X Server...\n");

	/* while X server is active, we wait for user
	   to switch to our tty and redirect him there */
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
#ifdef USE_PAM
	pam_setcred(pamh, PAM_DELETE_CRED);
	retval = pam_close_session(pamh, 0);
	pam_end(pamh, retval);
	pamh = NULL;
#else
	LogEvent(pw, CLOSE_SESSION);
#endif
	if (get_active_tty() == dest_vt) set_active_tty(current_vt);
	free(session);

	exit(0);
}

/* Start the session of your choice */
void start_session(char *username, char *session, int workaround)
{
	struct passwd *pwd = getpwnam(username);

	endpwent();

	if (workaround != -1)
	{
		current_vt = workaround;
		set_active_tty(13);
		set_active_tty(current_vt);
	}
	else current_vt = get_active_tty();

	if (!pwd)
	{
		struct passwd pw;

		pw.pw_name = username;
		LogEvent(&pw, UNKNOWN_USER);
		free(username);
		free(session);
		exit(0);
	}

	ClearScreen();

#ifdef USE_PAM
	if (update_token)
	{
		/* This is a quick hack... for some reason, if I try to
		   do things properly, PAM output on screen when updating
			 authorization token is all garbled, so for now I'll
			 leave this to /bin/login                               */
		printf("You need to update your authorization token...\n");
		printf("After that, log out and in again.\n\n");
		execl("/bin/login", "/bin/login", "--", username, (char *) 0);
		exit(0);
	}
#endif

	if (strcmp(session, "Text Console") == 0)
	{
		free(session);
		Text_Login(pwd, username);
	}
	else Graph_Login(pwd, session, username);

	/* we don't get here unless we couldn't start user session */
	fprintf(stderr, "Couldn't login user '%s'!\n", username);
	exit(0);
}
