/***************************************************************************
                          memmgmt.h  -  description
                            --------------------
    begin                : Feb 16 2004
    copyright            : (C) 2004-2005 by Noberasco Michele
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

/* safe allocation and deallocation functions */
#define calloc  my_calloc
#define free    my_free
#define exit    my_exit
#define strdup  my_strdup
#define strndup my_strndup
void *my_calloc (size_t  nmemb, size_t size);
void  my_free   (void   *ptr);
void  my_exit   (int     n);
char *my_strdup (const char *s);
char *my_strndup(const char *s, size_t len);
