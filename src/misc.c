/***************************************************************************
                           misc.h  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003 by Noberasco Michele
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
#include "chvt.h"
#include "load_settings.h"


#ifdef defined_my_calloc
#undef calloc
#undef free
#undef exit
#undef strdup
#define redef_calloc 1
#endif
void *my_calloc(size_t nmemb, size_t size)
{	
  void *temp = calloc(nmemb, size);
  if (!temp)
    {
      perror("Qingy error - FATAL");
      abort();
    }
  
  return temp;
}
void my_free(void *ptr)
{
  if (!ptr) return;
  free(ptr);
  ptr=NULL;
}
void my_exit(int n)
{
  /* We reenable VT switching if it is disabled */
  unlock_tty_switching();

  my_free(DATADIR);
  my_free(SETTINGS);
  my_free(LAST_USER);
  my_free(TEXT_SESSIONS_DIRECTORY);
  my_free(X_SESSIONS_DIRECTORY);
  my_free(XINIT);
  my_free(FONT);
  my_free(BACKGROUND);
  my_free(THEME_DIR);
  
  exit(n);
}
char *my_strdup(const char *s)
{
  char *temp;
  
  if (!s) return NULL;
  temp = strdup(s);
  if (!temp) {
    perror("Qingy error - FATAL");
    abort();
  }
  return temp;
}

#ifdef redef_calloc
#define calloc my_calloc
#define free   my_free
#define exit   my_exit
#define strdup my_strdup
#undef redef_calloc
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

#ifdef USE_PAM
int StrDup (char **dst, const char *src)
{
  if (src)
    {
      int len = strlen (src);
      
      if (!(*dst = malloc (len + 1))) return 0;
      memcpy (*dst, src, len);
      (*dst)[len] = 0;
    }
  else *dst = 0;
  
  return 1;
}
#endif
