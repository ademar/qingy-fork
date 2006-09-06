/***************************************************************************
                           misc.c  -  description
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <curses.h>
#include <pwd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <term.h>
#include <time.h>
#include <unistd.h>
#include <utmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#ifdef USE_X
#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>
#endif /* USE_X */

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

#include "misc.h"
#include "memmgmt.h"
#include "load_settings.h"
#include "vt.h"
#include "session.h"
#include "tty_guardian.h"
#include "logger.h"


#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif


int int_log10(int n)
{
  int temp=0;
  while ( (n/=10) >= 1) temp++;
  return temp;
}


char *int_to_str(int n)
{
  char *temp;
  int lun;

  if (n<0) return NULL;
  lun= int_log10(n);
  temp= (char *) calloc((size_t)(lun+2), sizeof(char));
  temp[lun+1]= '\0';
  while (lun>=0)
  {
    temp[lun]= (char)('0' + n%10);
    n/=10; lun--;
  }
  
  return temp;
}


void to_lower(char *string)
{
	int i;
	int len;

	if (!string) return;

	len = strlen(string);
	for (i=0; i<len; i++)
		string[i] = tolower(string[i]);
}


char to_upper(char c)
{
	if (c >= 97 && c <= 122)
		return c - 32;

	return c;
}


void ClearScreen(void)
{
	setupterm((char *) 0, STDOUT_FILENO, (int *) 0);
	tputs(clear_screen, lines > 0 ? lines : 1, putchar);
}


char *get_home_dir(char *user)
{
  struct passwd *pwd;
  
  if (!user) return NULL;
  pwd = getpwnam(user);
  if (!pwd) return NULL;  
  
  return strdup(pwd->pw_dir);
}


char *print_welcome_message(char *preamble, char *postamble)
{
  char *text = (char *) calloc(MAX, sizeof(char));
  int len;
  
  if (preamble) strncpy(text, preamble, MAX-1);
  len = (int)strlen(text);
  gethostname(&(text[len]), MAX-len);
  if (postamble) strncat(text, postamble, MAX-1);
  
  return text;
}


/* append any number of strings to dst */
char *StrApp (char **dst, ...)
{
  int len = 1;
  char *pt, *temp;
  va_list va;

  if (dst) if (*dst) len += strlen(*dst);
  va_start(va, dst);
  for (;;)
	{
		pt = va_arg(va, char *);
		if (!pt) break;
		len += strlen(pt);
	}
  va_end (va);
  temp = (char *) calloc((size_t)len, sizeof(char));
  if (dst) if (*dst)
	{
		strcpy(temp, *dst);
		free(*dst);
	}
  va_start(va, dst);
  for (;;)
	{
		pt = va_arg(va, char *);
		if (!pt) break;
		strcat(temp, pt);
	}
  va_end (va);
  
  if (dst) *dst = temp;
  return temp;
}


void xstrncpy(char *dest, const char *src, size_t n)
{
  strncpy(dest, src, n-1);
  dest[n-1] = 0;
}


int is_a_directory(char *what)
{
  DIR *dir;
  
  if (!what) return 0;
  dir = opendir(what);
  if (!dir) return 0;
  closedir(dir);
  
  return 1;
}


char *get_file_owner(char *file)
{
	struct stat desc;
	struct passwd *pwd;

	if (!file) return NULL;
	if (stat(file, &desc) == -1) return NULL;
	
	pwd = getpwuid(desc.st_uid);
	if (!pwd) return NULL;

	return strdup(pwd->pw_name);
}


#ifdef USE_X
int get_x_idle_time(int x_offset)
{
	static XScreenSaverInfo *xinfo     = NULL;
	static Display          *display   = NULL;
	int                      idle_time = 0;
	int                      event_base;
	int                      error_base;

	if (!display)
	{
		char *x_srv_num = int_to_str(x_offset);
		char *x_server  = StrApp((char**)NULL, ":", x_srv_num, (char*)NULL);
		
		display = XOpenDisplay(x_server);

		free(x_srv_num);
		free(x_server);

		if (!display)
		{
			writelog(ERROR, "Cannot connect to X-Windows server!\n");
			return 0;
		}

		if (!XScreenSaverQueryExtension(display, &event_base, &error_base))
		{
			writelog(ERROR, "No XScreenSaver extension!\n");
			return 0;
		}

		xinfo = XScreenSaverAllocInfo();
	}

	XScreenSaverQueryInfo(display, RootWindow(display, DefaultScreen(display)), xinfo);
	idle_time = (xinfo->idle) / 60000;

	return idle_time;
}
#endif /* USE_X */

/* session idle time, in minutes */
int get_session_idle_time(char *tty, time_t *start_time, int is_x_session, int x_offset)
{
	time_t curr_time = time(NULL);
	struct stat tty_stat;
	int tty_idle_time;

	/* no need to perform expensive checks before due time */
	if (((curr_time - *start_time)/60) < idle_timeout)
		return 0;

#ifdef USE_X
	if (is_x_session)
		return get_x_idle_time(x_offset);
#endif /* USE_X */

	/* return if we cannot get tty stats */
	if (stat(tty, &tty_stat))
		return 0;

	tty_idle_time = (curr_time - tty_stat.st_atime) / 60;

	if (tty_idle_time < idle_timeout)
		return tty_idle_time;

	/* we return idle time of /dev/tty (which is the idle time of
	 * the latest used tty). Not perfect, but it will have to suffice for now...
	 */

	/* return if we cannot get tty stats */
	if (stat("/dev/tty", &tty_stat))
		return 0;

	return (curr_time - tty_stat.st_atime) / 60;
}

int get_system_uptime()
{
	double  uptime_secs;
	FILE   *fp;

	fp = fopen("/proc/uptime", "r");
	if (!fp) return 0;
	
	if (fscanf(fp, "%lf", &uptime_secs) != 1)
	{
		fclose(fp);
		return 0;
	}

	fclose(fp);

	return (int) uptime_secs;
}


char *assemble_message(char *content, char *command)
{
  char   *where;
  char   *prev;
  char   *message = NULL;
  char   *result  = NULL;			
  size_t  len     = 0;
  FILE   *fp;
  
  if (!content) return NULL;
  if (!command) return content;
  
  where = strstr(content, "<INS_CMD_HERE>");
  if (!where) return content;
  
  fp = popen(command, "r");
  getline(&result, &len, fp);
  pclose(fp);
  
  if (!result) return content;
  
  prev = strndup(content, where - content);
  len = strlen(result);
  if (result[len-1] == '\n') result[len-1] = '\0';
  message = StrApp((char**)NULL, prev, result, where+14, (char*)NULL);
  free(prev);
  free(result);
  
  return message;
}


void PrintUsage()
{
  printf("\nqingy version %s\n", PACKAGE_VERSION);
  printf("\nusage: qinqy <ttyname> [options]\n");
  printf("Options:\n");
  printf("\t-t, --text-mode\n");
  printf("\tPerform a text-mode login prompt (no graphics).\n\n");
  printf("\t-f <device>, --fb-device <device>\n");
  printf("\tUse <device> as framebuffer device.\n\n");
  printf("\t-p, --hide-password\n");
  printf("\tDo not show password asterisks.\n\n");
  printf("\t-l, --hide-lastuser\n");
  printf("\tDo not display last user name.\n\n");
  printf("\t-d, --disable-lastuser\n");
  printf("\tDo not remember last user name.\n\n");
  printf("\t-n, --no-shutdown-screen\n");
  printf("\tClose DirectFB mode before shutting down.\n");
  printf("\tThis way you will see system shutdown messages.\n\n");
	printf("\t-r <xres>x<yres>, --resolution <xres>x<yres>\n");
	printf("\tDo not detect framebuffer resolution, use this one instead.\n\n");
	printf("\t-h, --help\n");
	printf("\tPrint this help message.\n\n");
}

/* A good part of the code in this function comes from agetty (util-linux),
 * it is GPL-2 software anyway :-)
 */
void parse_etc_issue(void)
{
	FILE           *fd;
	int             c;
	struct utsname  uts;

	if (max_loglevel >= DEBUG) printf("\n");

	(void) uname(&uts);
	(void) write(1, "\r\n", 2);			/* start a new line */
	fd = fopen("/etc/issue", "r");

	if (fd)
	{
		while ((c = getc(fd)) != EOF)
		{
	    if (c == '\\')
			{
				c = getc(fd);

				switch (c)
				{
					case 's':
						(void) printf ("%s", uts.sysname);
						break;
		    
					case 'n':
						(void) printf ("%s", uts.nodename);
						break;
		    
					case 'r':
						(void) printf ("%s", uts.release);
						break;
		    
					case 'v':
						(void) printf ("%s", uts.version);
						break;
		    
					case 'm':
						(void) printf ("%s", uts.machine);
						break;

					case 'o':
					{
						char domainname[256];
#ifdef HAVE_getdomainname
						getdomainname(domainname, sizeof(domainname));
#else
						strcpy(domainname, "unknown_domain");
#endif
						domainname[sizeof(domainname)-1] = '\0';
						printf ("%s", domainname);
					}
					break;

					case 'O':
					{
						char *domain = NULL;
						char host[HOST_NAME_MAX + 1];
						struct hostent *hp = NULL;
			
						if (gethostname(host, HOST_NAME_MAX) || !(hp = gethostbyname(host)))
						{
							domain = "	 unknown_domain";
						}
						else
						{
							/* get the substring after the first . */
							domain = strchr(hp->h_name, '.');
							if (domain == NULL)
								domain = ".(none)";
						}
						printf("%s", ++domain);
					}  
					break;

					case 'd':
					case 't':
					{
						char *weekday[] = { "Sun", "Mon", "Tue", "Wed", "Thu",
																"Fri", "Sat" };
						char *month[] = { "Jan", "Feb", "Mar", "Apr", "May",
															"Jun", "Jul", "Aug", "Sep", "Oct",
															"Nov", "Dec" };
						time_t now;
						struct tm *tm;

						(void) time (&now);
						tm = localtime(&now);

						if (c == 'd')
							(void) printf ("%s %s %d  %d",
														 weekday[tm->tm_wday], month[tm->tm_mon],
														 tm->tm_mday, 
														 tm->tm_year < 70 ? tm->tm_year + 2000 :
														 tm->tm_year + 1900);
						else
							(void) printf ("%02d:%02d:%02d",
														 tm->tm_hour, tm->tm_min, tm->tm_sec);
		      
						break;
					}

					case 'l':
						(void) printf ("/dev/tty%d", current_tty);
						break;

/* 'b' option not supported for now */
/* 					case 'b': */
/* 					{ */
/* 						int i; */

/* 						for (i = 0; speedtab[i].speed; i++) { */
/* 							if (speedtab[i].code == (tp->c_cflag & CBAUD)) { */
/* 								printf("%ld", speedtab[i].speed); */
/* 								break; */
/* 							} */
/* 						} */
/* 						break; */
/* 					} */
					case 'u':
					case 'U':
					{
						int users = 0;
						struct utmp *ut;
						setutent();
						while ((ut = getutent()))
							if (ut->ut_type == USER_PROCESS)
								users++;
						endutent();
						printf ("%d ", users);
						if (c == 'U')
							printf ((users == 1) ? "user" : "users");
						break;
					}
					default:
						putchar(c);
				}
			}
	    else
				putchar(c);
		}

		fflush(stdout);
		(void) fclose(fd);
	}
}

void text_mode()
{
	char           *username   =  NULL;
	char           *password   =  NULL;
	size_t          len        =  0;
	int             n_sessions =  0;
	int             selected   = -999;
	char            session    =  0;
	char          **sessions;
	char           *last_session;

#ifdef __linux__
	char            hn[HOST_NAME_MAX+1];

	gethostname(hn, HOST_NAME_MAX);
#endif

	parse_etc_issue();

	while (!username)
	{
#ifdef __linux__
		write(1, hn, strlen(hn));
		write(1, " ", strlen(" "));
#endif

		fprintf(stdout, "login: ");
		fflush(stdout);

		if (getline(&username, &len, stdin) == -1)
		{
			fprintf(stdout, "\nCould not read user name... aborting!\n");
			fflush(stdout);
			sleep(3);
			exit(EXIT_FAILURE);
		}

		if (!username)
		{
			fprintf(stdout, "\nInvalid user name!\n\n");
			fflush(stdout);
		}

		if (username)
		{
			len = strlen(username);

			if (len < 2)
			{
				fprintf(stdout, "\nInvalid user name!\n\n");
				fflush(stdout);
				free(username);
				username=NULL;
			}
		}

		if (username)
		{
			username[len-1] = '\0';
		}
	}

	fprintf(stdout, "Password: ");
	fflush(stdout);

	password = read_password(current_tty);
	fprintf(stdout, "\n");
	fflush(stdout);

	if (!check_password(username, password))
	{
		fprintf(stdout, "\nLogin failed!\n");
		fflush(stdout);
		sleep(3);
		exit(EXIT_SUCCESS);
	}

	/* wipe password */
	memset(password, '\0', sizeof(password));
	free(password);

	/* get available sessions */
	sessions    = (char **)calloc(1, sizeof(char *));
	sessions[0] = get_sessions();
	while (sessions[n_sessions])
	{
		n_sessions++;
		sessions = (char **)realloc(sessions, (n_sessions+1)*sizeof(char*));
		sessions[n_sessions] = get_sessions();
	}
	sort_sessions(sessions, n_sessions);

	/* get latest session if available */
	last_session = get_last_session(username);
	if (!last_session)
		last_session = strdup("Text: Console");
	else
	{
		for (; session<n_sessions; session++)
			if (!strcmp(sessions[(int)session], last_session))
				break;

		if (session == n_sessions)
		{
			free(last_session);
			last_session = strdup("Text: Console");
		}
	}

	/* present sessions list and ask user to choose */
	initscr();
	cbreak();

	while(1)
	{
		n_sessions = 0;
		session    = 'a';

		erase();

		if (selected != -999)
			printw("Invalid choice '%c': please select a valid session...\n\n", (char)selected+'a');
		else
			printw("Welcome, %s, please select a session...\n\n", username);

		while (sessions[n_sessions])
			printw("(%c) %s\n", session++, sessions[n_sessions++]);

		printw("\nYour choice (just press ENTER for '%s'): ", last_session);

		session = getch();

		/* if user just pressed enter, select latest session */
		if ((int)session == 10)
		{
			selected=n_sessions;
			while (selected)
				if (!strcmp(sessions[--selected], last_session))
					break;
		}
		else
			selected = (int)(session-'a');

		if (selected >= 0 && selected <= (n_sessions-1))
			break;
	}
	
	erase();
	refresh();
	reset_shell_mode();
	free(last_session);

	start_session(username, sessions[selected]);
}

void Error(int fatal)
{
	/* seconds before we die: let's be kind to init! */
	int countdown = 16;

  /* We reenable VT switching if it is disabled */
  unlock_tty_switching();   
  
  PrintUsage();
  if (!fatal) text_mode();
  
  /*
   * we give the user some time to read the message
   * and change VT before dying
	 * ED (paolino): was sleep(5), too fast to read errors
	 * ED (michele): all right, but we should let them know what's happening!
   */
	while (--countdown)
	{
		fprintf(stdout, "%s will be restarted automatically in %d seconds\r", program_name, countdown);
		fflush(stdout);
		sleep(1);
	}
  exit(EXIT_FAILURE);
}

char *get_resolution(char *resolution)
{
	char *result;
	char *temp;
	char *temp2;
	int   width  = 0;
	int   height = 0;
	int  *value  = &width;

	if (!resolution) return NULL;
	for(temp = resolution; (int)*temp; temp++)
		switch (*temp)
		{
			case 'x': case 'X':
				if (!width) return NULL;
				if (value == &height) return NULL;
				value = &height;
				break;
			default:
				if ((*temp) > '9' || (*temp) < '0') return NULL;
				(*value) *= 10;
				(*value) += (int)(*temp) - (int)'0';
		}
	if (!width)  return NULL;
	if (!height) return NULL;

	temp   = int_to_str(width);
	temp2  = int_to_str(height);
	result = StrApp((char**)NULL, temp, "x", temp2, (char*)NULL);
	free(temp); free(temp2);

	return result;
}

void execute_script(char *script)
{
	if (!script) return;

	if (access(script, X_OK))
	{
		WRITELOG(ERROR, "Could not execute your user defined command '%s'!\n", script);
		return;
	}

	switch ((int)fork())
	{
		case -1: /* error */
			writelog(ERROR, "Cannot issue fork() command!\n");
			sleep(2);
			exit(EXIT_FAILURE);
		case 0: /* child */
			execve(script, NULL, NULL);
			WRITELOG(ERROR, "qingy: could not execute your user defined command '%s'!\n", script);
			sleep(4);
		default: /* parent */
			wait(NULL);
	}
}
