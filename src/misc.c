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
#include <sys/types.h>


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
