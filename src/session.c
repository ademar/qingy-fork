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


//#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <shadow.h>

#include "session.h"
#include "chvt.h"
#include "misc.h"

extern char **environ;
int current_vt;

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

int check_password(void)
{
	char*  encrypted;
	char*  correct;
	struct passwd *pw;
#ifdef SHADOW_PASSWD
	struct spwd* sp;
#endif

	pw = getpwnam(username);
	endpwent();
	if (!pw) return 0;

#ifdef SHADOW_PASSWD
	sp = getspnam(pw->pw_name);
	endspent();
	if(sp) correct = sp->sp_pwdp;
	else
#endif
	correct = pw->pw_passwd;

	if (correct == 0 || correct[0] == '\0') return 1;

	encrypted = crypt(password, correct);
	memset(password, 0, strlen(password));

	if (!strcmp(encrypted, correct)) return 1;
  else return 0;

	return 0;
}

char *baseName(char *name)
{
	char *base;

	base = name;
	while (*name)
	{
		if (*name == '/') base = name + 1;
		++name;
	}

	return (char *) base;
}

void Text_Login(struct passwd *pw)
{
	char *args[2];
	char *shell;
	char *shell_basename;

	shell = strdup(pw->pw_shell);
	shell_basename = baseName(shell);

	args[0] = (char *) calloc(strlen(shell_basename) + 2, sizeof(char));
	strcpy(args[0], "-");
	strcat(args[0], shell_basename);
	args[1] = 0;
	execv(shell,args);

	/* we should never get here... */
	fprintf(stderr, "session: unable to start console session!\n");
	exit(0);
}

/* if somebody else, somewhere else, sometime else
   somehow started some other X servers ;-)        */
int which_X_server(void)
{
	FILE *fp;
	char filename[20];
	int num=0;

	strcpy(filename, "/tmp/.X0-lock");
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

void Graph_Login(struct passwd *pw, char *path, char *script)
{
	char exec[MAX];
	pid_t proc_id;
	int dest_vt = current_vt + 20;

	strcpy(exec, strdup(pw->pw_shell));
	strcat(exec, " -l -c \"");
	strcat(exec, XINIT);
	strcat(exec, " ");
	strcat(exec, path);
	strcat(exec, script);
	strcat(exec, " -- :");
	strcat(exec, int_to_str(which_X_server()));
	strcat(exec, " vt");
	strcat(exec, int_to_str(dest_vt));
	strcat(exec, "\" >/dev/null 2>/dev/null");

	proc_id = fork();
	if (proc_id == -1)
	{
		fprintf(stderr, "session: fatal error: cannot issue fork() command!\n");
		exit(0);
	}
	if (!proc_id)
	{
		system("/usr/bin/clear");
		fprintf(stderr, "Switching to X Server...\n");
		if (system(exec) == -1)
			fprintf(stderr, "session: fatal error: cannot start your session!\n");
		exit(0);
	}
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
	if (get_active_tty() == dest_vt) set_active_tty(current_vt);

	exit(0);
}

void setEnvironment(char *username)
{
	struct passwd *pw;
	char          *mail;

	/* Get password struct */
	pw = getpwnam(username);
	endpwent();
	if (!pw)
	{
		fprintf(stderr, "user %s not found in /etc/passwd\n", username);
		exit(0);
	}

	/* Set environment */
	environ = (char **) calloc(2, sizeof(char *));
	environ[0] = 0;
	setenv("TERM", "vt100", 0);  /* TERM=linux? */
	setenv("HOME", pw->pw_dir, 1);
	setenv("SHELL", pw->pw_shell, 1);
	setenv("USER", pw->pw_name, 1);
	setenv("LOGNAME", pw->pw_name, 1);
	setenv("DISPLAY", ":0.0", 1);
	mail = (char *) calloc(strlen(_PATH_MAILDIR) + strlen(pw->pw_name) + 2, sizeof(char));
	strcpy(mail, _PATH_MAILDIR);
	strcat(mail, "/");
	strcat(mail, pw->pw_name);
	setenv("MAIL", mail, 1);
	chdir(pw->pw_dir);
}

void switchUser(char *username, char *path, char *script)
{
	struct passwd *pw;

	/* Get password struct */
	pw = getpwnam(username);
	endpwent();
	if (!pw)
	{
		fprintf(stderr, "user %s not found in /etc/passwd\n", username);
		exit(0);
	}

	/* Set user id */
	if ((initgroups(pw->pw_name, pw->pw_gid) != 0) || (setgid(pw->pw_gid) != 0) || (setuid(pw->pw_uid) != 0))
	{
		fprintf(stderr, "could not switch user id\n");
		exit(0);
	}

	/* Execute login command */
	setEnvironment(username);
	if (!path && !script) Text_Login(pw);
	else Graph_Login(pw, path, script);
	fprintf(stderr, "could not execute login command\n");
	exit(0);
}

/* Start the session of your choice */
void start_session(int session_id, int workaround)
{
	struct session *curr = &sessions;

	while (curr->id != session_id) curr= curr->next;

	if (workaround != -1)
	{
		current_vt = workaround;
		set_active_tty(13);
		set_active_tty(current_vt);
	}
	else current_vt = get_active_tty();

	if (strcmp(curr->name, "Text Console") == 0)
		switchUser(username, NULL, NULL);
	else switchUser(username, XSESSIONS_DIRECTORY, curr->name);

	/* we don't get here unless we couldn't start user session */
	fprintf(stderr, "Couldn't login user '%s'!\n", username);
	exit(0);
}
