/***************************************************************************
                           misc.c  -  description
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


#define MAX 255

/* Converts an unsigned inteter to a string */
char *int_to_str(int n);

/* Concatenates two strings */
char *stringCombine(const char* str1, const char* str2);

/* Computes the integer part of the base 10 log */
int log10(int n);

/* functions to start and stop gpm */
int stop_gpm(void);
int start_gpm(void);

/* I couldn'd think of an intelligent explanation for this */
void ClearScreen(void);

/* get <user> home directory */
char *get_home_dir(char *user);

/* Reads an entire line from fp */
int get_line(char *tmp, FILE *fp, int max);

/* Prints a welcome message */
char *print_welcome_message(char *preamble, char *postamble);
