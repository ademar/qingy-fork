/***************************************************************************
                           misc.h  -  description
                            --------------------
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


#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#include "misc.h"

int log10(int n)
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
  lun= log10(n);
  temp= (char *) calloc(lun+2, sizeof(char));
  temp[lun+1]= '\0';
  while (lun>=0)
  {
    temp[lun]= '0'+ n%10;
    n/=10; lun--;
  }

  return temp;
}

int stop_gpm(void)
{
	int retval;

	retval = system("/etc/init.d/gpm stop >/dev/null 2>/dev/null");
	if (retval == 0) return 1;

	return 0;
}

int start_gpm(void)
{
	int retval;
  struct timespec delay;

  /* We set up a delay of 0.5 seconds */
  delay.tv_sec= 0;
  delay.tv_nsec= 500000000;

	retval = system("/etc/init.d/gpm start >/dev/null 2>/dev/null");
	/* let's give gpm some time to initialize correctly */
	nanosleep(&delay, NULL);
	if (retval == 0) return 1;

	return 0;
}

char *stringCombine(const char* str1, const char* str2)
{
   char* buffer;

   buffer = (char *) calloc(strlen(str1) + strlen(str2) + 1, sizeof(char));
   strcpy(buffer, str1);
   strcat(buffer, str2);
   return buffer;
}

void ClearScreen(void)
{
	system("/usr/bin/clear 2>/dev/null");
}

char *get_home_dir(char *user)
{
	char *homedir;
	struct passwd *pwd;

	if (!user) return NULL;
	pwd = getpwnam(user);
	endpwent();
	if (!pwd) return NULL;
	homedir = pwd->pw_dir;

	return homedir;
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
	len = strlen(text);
	gethostname(&(text[len]), MAX-len);
	if (postamble) strncat(text, postamble, MAX-1);

	return text;
}

#ifdef USE_PAM
#include <stdarg.h>
/* append any number of strings to dst */
int StrApp (char **dst, ...)
{
	int len;
	char *bk, *pt, *dp;
	va_list va;

	len = 1;
	if (*dst) len += strlen(*dst);
	va_start (va, dst);
	for (;;)
	{
		pt = va_arg (va, char *);
		if (!pt) break;
		len += strlen (pt);
	}
	va_end (va);
	if (!(bk = malloc (len))) return 0;
	dp = bk;
	if (*dst)
	{
		len = strlen(*dst);
		memcpy (dp, *dst, len);
		dp += len;
		free(*dst);
	}
	va_start (va, dst);
	for (;;)
	{
		pt = va_arg (va, char *);
		if (!pt) break;
		len = strlen(pt);
		memcpy (dp, pt, len);
		dp += len;
	}
	va_end (va);
	*dp = '\0';
	*dst = bk;
	return 1;
}

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

void WipeStr (char *str)
{
	if (str)
	{
		memset((char *)str, 0, strlen(str));
		free (str);
	}
}
#endif
