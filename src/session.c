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


#define _XOPEN_SOURCE
#define XINIT                   "/usr/X11R6/bin/xinit"
#define STARTUP_SCRIPT		"Scripts/Startup"
#define LOGIN_ROOT_SCRIPT	"Scripts/Login.root"
#define LOGIN_USER_SCRIPT	"Scripts/Login.user"
#define LOGOUT_ROOT_SCRIPT	"Scripts/Logout.root"
#define RESTART_SCRIPT		"Scripts/Restart"
#define POWEROFF_SCRIPT		"Scripts/PowerOff"
#define SUSPEND_SCRIPT		"Scripts/Suspend"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <paths.h>
#include <sys/types.h>
#include <shadow.h>

extern char **environ;


#include "session.h"
#include "chvt.h"

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
	  if ( (strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) )
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

void* alloc(int size) {
   void *tmp;

   tmp = malloc(size);
   if (!tmp) {
      fprintf(stderr, "out of memory, terminating\n");
      exit(0);
   } else {
      memset(tmp, 0, size);
   }

   return tmp;
}

char* stringCombine(const char* str1, const char* str2) {
   char* buffer;

   buffer = alloc(strlen(str1) + strlen(str2) + 1);
   strcpy(buffer, str1);
   strcat(buffer, str2);
   return buffer;
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

void switchUser(char* username, char* path, char* script);

/* Start the session of your choice.
   Returns 0 if starting failed      */
int start_session(int session_id, int workaround)
{
	if (workaround != -1)
	{
	  /* This is to avoid a black display after framebuffer dies */
    set_active_tty(13);
	  set_active_tty(workaround);
	}

	switchUser(username, "/bin/", "bash");
}




static char* baseName(char* name) {
   char *base;

   base = name;
   while (*name) {
      if (*name == '/') {
         base = name + 1;
      }
      ++name;
   }

   return (char*) base;
}

static void execute(struct passwd *pw, char* path, char* script) {
   char* args[4];
   char* shell;
   char* shell_basename;
   char* cmd;

   shell = strdup(pw->pw_shell);
   shell_basename = baseName(shell);
   cmd = stringCombine(path, script);

   args[0] = alloc(strlen(shell_basename) + 2);
   strcpy(args[0], "-");
   strcat(args[0], shell_basename);
   args[1] = "-c";
   args[2] = stringCombine("exec ", cmd);
   args[3] = 0;

   free(cmd);

   execv(shell, args);
}


void setEnvironment(char* username) {
   struct passwd* pw;
   char*          mail;

   /* Get password struct */
   pw = getpwnam(username);
   endpwent();
   if (!pw) {
      fprintf(stderr, "user %s not found in /etc/passwd\n", username);
      exit(0);
   }

   /* Set environment */
   environ = alloc(sizeof(char*) * 2);
   environ[0] = 0;
   setenv("TERM", "vt100", 0);  // TERM=linux?
   setenv("HOME", pw->pw_dir, 1);
   setenv("SHELL", pw->pw_shell, 1);
   setenv("USER", pw->pw_name, 1);
   setenv("LOGNAME", pw->pw_name, 1);
   setenv("DISPLAY", ":0.0", 1);
   mail = alloc(strlen(_PATH_MAILDIR) + strlen(pw->pw_name) + 2);
   strcpy(mail, _PATH_MAILDIR);
   strcat(mail, "/");
   strcat(mail, pw->pw_name);
   setenv("MAIL", mail, 1);
   chdir(pw->pw_dir);
}

void switchUser(char* username, char* path, char* script) {
   struct passwd* pw;

   /* Get password struct */
   pw = getpwnam(username);
   endpwent();
   if (!pw) {
      fprintf(stderr, "user %s not found in /etc/passwd\n", username);
      exit(0);
   }

   /* Set user id */
   if ((initgroups(pw->pw_name, pw->pw_gid) != 0) ||
       (setgid(pw->pw_gid) != 0) || (setuid(pw->pw_uid) != 0)) {
      fprintf(stderr, "could not switch user id\n");
      exit(0);
   }

   /* Execute login command */
   execute(pw, path, script);
   fprintf(stderr, "could not execute login command\n");
   exit(0);
}

