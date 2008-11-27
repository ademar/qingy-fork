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

#include "qingy_constants.h"

/* Computes the integer part of the base 10 log */
int int_log10(int n);

/* Converts an unsigned integer to a string */
char *int_to_str(int n);

/* make given string lowercase */
void to_lower(char *string);

/* make given char uppercase */
char to_upper(char c);

/* append any number of strings to dst */
char *StrApp(char **dst, ...);

/* like strncpy, but the result is null-terminated */
void xstrncpy(char *dest, const char *src, size_t n);

/* I couldn'd think of an intelligent explanation for this */
void ClearScreen(void);

/* get <user> home directory */
char *get_home_dir(char *user);

/* get id of a given group */
int get_group_id(char *group_name);

/* check if current runlevel is suitable to log user in or show qingy gui */
int check_runlevel();

/* checks wether <what> is a directory */
int is_a_directory(char *what);

/* function name says it all ;-P */
char *get_file_owner(char *file);

/* Get system uptime */
int get_system_uptime();

/* session idle time, in minutes */
int get_session_idle_time(char *tty, time_t *start_time, int is_x_session, int x_offset);

/* guess what :-) */
void execute_script(char *script);

/* other stuff */
char *assemble_message(char *content, char *command);
void text_mode();
void Error(int fatal);
char *get_resolution(char *resolution);
void PrintUsage();
