/***************************************************************************
                          memmgmt.c  -  description
                            --------------------
    begin                : Feb 16 2004
    copyright            : (C) 2004-2005 by Noberasco Michele
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vt.h"

void *my_calloc(size_t nmemb, size_t size)
{	
  void *temp = calloc(nmemb, size);
  if (!temp)
	{
		perror("qingy: allocation error - FATAL");
		abort();
	}
  
  return temp;
}

void my_free(void *ptr)
{
  if (!ptr) return;
  free(ptr);
  ptr = NULL;
}

void my_exit(int n)
{
	unlock_tty_switching();
  exit(n);
}

char *my_strdup(const char *s)
{
  char *temp;
  
  if (!s) return NULL;
  temp = strdup(s);
  if (!temp)
	{
    perror("qingy: allocation error - FATAL");
    abort();
  }
  return temp;
}

char *my_strndup(const char *s, size_t len)
{
  char *temp;
  
  if (!s) return NULL;
  temp = strndup(s, len);
  if (!temp)
	{
    perror("qingy: allocation error - FATAL");
    abort();
  }
  return temp;
}
