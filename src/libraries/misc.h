/***************************************************************************
                           misc.c  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003-2005 by Noberasco Michele
    e-mail               : s4t4n@gentoo.org
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

#include "qingy_constants.h"

/* Computes the integer part of the base 10 log */
int int_log10(int n);

/* Converts an unsigned integer to a string */
char *int_to_str(int n);

/* make given string lowercase */
void to_lower(char *string);

/* append any number of strings to dst */
char *StrApp(char **dst, ...);

/* like strncpy, but the result is null-terminated */
void xstrncpy(char *dest, const char *src, size_t n);

#ifdef USE_GPM_LOCK
/* functions to start and stop gpm */
int stop_gpm(void);
int start_gpm(void);
#endif

/* I couldn'd think of an intelligent explanation for this */
void ClearScreen(void);

/* get <user> home directory */
char *get_home_dir(char *user);

/* Prints a welcome message */
char *print_welcome_message(char *preamble, char *postamble);

/* checks wether <what> is a directory */
int is_a_directory(char *what);

/* function name says it all ;-P */
char *get_file_owner(char *file);

/* Get system uptime */
int get_system_uptime();

/* other stuff */
char *assemble_message(char *content, char *command);
void text_mode();
void Error(int fatal);
char *get_resolution(char *resolution);
void PrintUsage();
