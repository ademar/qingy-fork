/***************************************************************************
                           misc.h  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003-2004 by Noberasco Michele
    e-mail               : noberasco.gnu@disi.unige.it
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

#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
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

#include "misc.h"
#include "memmgmt.h"
#include "load_settings.h"
#include "vt.h"


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


void ClearScreen(void)
{
  system("/usr/bin/clear 2>/dev/null");
}


char *get_home_dir(char *user)
{
  struct passwd *pwd;
  
  if (!user) return NULL;
  pwd = getpwnam(user);
  if (!pwd) return NULL;  
  
  return strdup(pwd->pw_dir);
}


int get_line(char *tmp, FILE *fp, int max)
{
  int result = 0;
  int temp;
  
  if (!tmp || !fp || max<1) return 0;
  while((temp = fgetc(fp)) != EOF)
	{
		if (temp == '\n') break;
		if (result == max-1) break;
		tmp[result] = (char) temp;
		result++;
	}
  tmp[result] = '\0';
  return result;
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


#ifdef USE_GPM_LOCK
int stop_gpm(void)
{
  int retval;
  char *filename = StrApp((char**)0, DATADIR, "gpm", (char*)0);
  FILE *fp;
  
  retval = system("/etc/init.d/gpm stop >/dev/null 2>/dev/null");
  if (!retval)
	{
		/* we create this file so that even if we crash
			 our next instance will know that gpm is to be restarted */
		if (filename)
		{
			fp = fopen(filename, "w");
			fclose(fp);
			free(filename);
		}
		return 1;
	}
  
  /* we then check wether a 'restart gpm' file exists...
     ... see above, in case we crashed                   */
  if (filename)
	{
		int does_not_exist = access(filename, F_OK);
		free(filename);
		if (!does_not_exist) return 1;
	}
  
  return 0;
}


int start_gpm(void)
{
  int retval;
  struct timespec delay;
  char *filename = StrApp((char**)0, DATADIR, "gpm", (char*)0);
  
  /* We set up a delay of 0.5 seconds */
  delay.tv_sec= 0;
  delay.tv_nsec= 500000000;
  
  retval = system("/etc/init.d/gpm start >/dev/null 2>/dev/null");
  /* let's give gpm some time to initialize correctly */
  nanosleep(&delay, NULL);
  if (!retval)
	{
		/* let's remove the gpm file if it exists... */
		if (filename)
		{
			remove (filename);
			free(filename);
		}
		return 1;
	}
  if (filename) free(filename);
  
  return 0;
}
#endif


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


/*int uptime(int *days, int *hours, int *mins)*/
int get_system_uptime()
{
	double  uptime_secs;
/* 	int     seconds_in_days; */
	FILE   *fp;

/* 	if (!days)  return 0; */
/* 	if (!hours) return 0; */
/* 	if (!mins)  return 0; */

	fp = fopen("/proc/uptime", "r");
	if (!fp) return 0;
	
	if (fscanf(fp, "%lf", &uptime_secs) != 1)
	{
		fclose(fp);
		return 0;
	}

	fclose(fp);

/* 	*days  = uptime_secs / (60 * 60 * 24); */
/* 	seconds_in_days = *days * 60 * 60 * 24; */

/* 	*hours = (uptime_secs - seconds_in_days) / (60 * 60); */
/* 	*mins  = (uptime_secs - (seconds_in_days + (*hours * 60 * 60))) / 60; */

/* 	return 1; */
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
  printf("\t-f <device>, --fb-device <device>\n");
  printf("\tUse <device> as framebuffer device.\n\n");
  printf("\t-p, --hide-password\n");
  printf("\tDo not show password asterisks.\n\n");
  printf("\t-l, --hide-lastuser\n");
  printf("\tDo not display last user name.\n\n");
  printf("\t-d, --disable-lastuser\n");
  printf("\tDo not remember last user name.\n\n");
  printf("\t-v, --verbose\n");
  printf("\tDisplay some diagnostic messages on stderr.\n\n");
  printf("\t-n, --no-shutdown-screen\n");
  printf("\tClose DirectFB mode before shutting down.\n");
  printf("\tThis way you will see system shutdown messages.\n\n");
  printf("\t-s <timeout>, --screensaver <timeout>\n");
  printf("\tActivate screensaver after <timeout> minutes (default is 5).\n");
  printf("\tA value of 0 disables screensaver completely.\n\n");
	printf("\t-r <xres>x<yres>, --resolution <xres>x<yres>\n");
	printf("\tDo not detect framebuffer resolution, use this one instead.\n\n");
	printf("\t-h, --help\n");
	printf("\tPrint this help.\n\n");
}


void text_mode()
{
  execl("/bin/login", "/bin/login", NULL);

  /* We should never get here... */
  fprintf(stderr, "\nCannot exec \"/bin/login\"...\n");
  exit(EXIT_FAILURE);
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
		fprintf(stderr, "%s will be restarted automatically in %d seconds\r", program_name, countdown);
		fflush(stderr);
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
