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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <paths.h>
#include <shadow.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <lastlog.h>

#include "session.h"
#include "chvt.h"
#include "misc.h"
#include "load_settings.h"

#define UNKNOWN_USER   0
#define WRONG_PASSWORD 1
#define OPEN_SESSION   2
#define CLOSE_SESSION  3

int current_vt;
extern char **environ;

int get_sessions(void)
{
	struct session *curr;
	struct dirent *entry;
	DIR *dir;
	int i=0;

	curr= &sessions;

	curr->name = (char *) calloc(13, sizeof(char));
	strcpy(curr->name, "Text Console");
	curr->id = i;
	curr->next = curr;
	curr->prev = curr;

	dir= opendir(XSESSIONS_DIRECTORY);
	if (dir == NULL)
	{
	  fprintf(stderr, "session: unable to open directory \"%s\"\n", XSESSIONS_DIRECTORY);
		return (i+1);
	}
	while ((entry= readdir(dir)) != NULL)
	{
	  if (strcmp(entry->d_name, "." ) != 0)
		if (strcmp(entry->d_name, "..") != 0)
		if (strcmp(entry->d_name, "Xsession") != 0)
		{
			i++;
			curr->next = (struct session *) calloc(1, sizeof(struct session));
			curr->next->prev = curr;
			curr = curr->next;
			curr->name = (char *) calloc(strlen(entry->d_name)+1, sizeof(char));
			strcpy(curr->name, entry->d_name);
			curr->id = i;
		}
	}
	closedir(dir);
	curr->next = &sessions;
	sessions.prev = curr;

	return (i+1);
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
		default:
			syslog(LOG_AUTHPRIV|LOG_WARNING, "Error #666, coder brains are severely damaged!\n");
	}

	closelog();
}

int check_password(char *username, char *password)
{
	char*  encrypted;
	char*  correct;
	struct passwd *pw;
#ifdef SHADOW_PASSWD
	struct spwd* sp;
#endif

	pw = getpwnam(username);
	endpwent();
	if (!pw)
	{
		struct passwd pwd;

		pwd.pw_name = username;
		LogEvent(&pwd, UNKNOWN_USER);
		return 0;
	}

#ifdef SHADOW_PASSWD
	sp = getspnam(pw->pw_name);
	endspent();
	if (sp) correct = sp->sp_pwdp;
	else
#endif
	correct = pw->pw_passwd;

	if (correct == 0 || correct[0] == '\0') return 1;

	encrypted = crypt(password, correct);

	if (!strcmp(encrypted, correct)) return 1;

	LogEvent(pw, WRONG_PASSWORD);
	return 0;
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
	setenv("DISPLAY", ":0.0", 1);
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
		fprintf(stderr, "could not switch user id\n");
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

void Text_Login(struct passwd *pw)
{
	pid_t proc_id;

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
		LogEvent(pw, OPEN_SESSION);

		/* drop root privileges and set user enviroment */
		switchUser(pw);

		/* let's start the shell! */
		execve(pw->pw_shell, args, environ);

		/* execve should never return! */
		fprintf(stderr, "session: fatal error: cannot start your session!\n");
		exit(0);
	}
	wait(NULL);
	LogEvent(pw, CLOSE_SESSION);

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

void Graph_Login(struct passwd *pw, char *script)
{
	pid_t proc_id;
	int dest_vt = current_vt + 20;

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
		strcat(args[2], script);
		strcat(args[2], " -- :");
		strcat(args[2], int_to_str(which_X_server()));
		strcat(args[2], " vt");
		strcat(args[2], int_to_str(dest_vt));
		strcat(args[2], ">/dev/null 2>/dev/null");
		args[3] = NULL;

		/* write to system logs */
		dolastlog(pw, 1);
		LogEvent(pw, OPEN_SESSION);

		/* drop root privileges and set user enviroment */
		switchUser(pw);

		/* start X server and selected session! */
		execve(pw->pw_shell, args, environ);

		/* execve should never return! */
		fprintf(stderr, "session: fatal error: cannot start your session!\n");
		exit(0);
	}

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
	LogEvent(pw, CLOSE_SESSION);
	if (get_active_tty() == dest_vt) set_active_tty(current_vt);

	exit(0);
}

/* Start the session of your choice */
void start_session(char *username, int session_id, int workaround)
{
	struct session *curr = &sessions;
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
		fprintf(stderr, "user %s not found in /etc/passwd\n", username);
		free(username);
		exit(0);
	}
	free(username);

	while (curr->id != session_id) curr= curr->next;
	ClearScreen();

	if (strcmp(curr->name, "Text Console") == 0)
		Text_Login(pwd);
	else Graph_Login(pwd, curr->name);

	/* we don't get here unless we couldn't start user session */
	fprintf(stderr, "Couldn't login user '%s'!\n", username);
	exit(0);
}
