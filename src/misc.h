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


/* safe allocation and deallocation */
#define calloc my_calloc
#define free   my_free
#define exit   my_exit
#define strdup my_strdup
#define defined_my_calloc 1
void *my_calloc(size_t  nmemb, size_t size);
void  my_free  (void   *ptr);
void  my_exit  (int     n);
char *my_strdup(const char *s);

/* Computes the integer part of the base 10 log */
int int_log10(int n);

/* Converts an unsigned inteter to a string */
char *int_to_str(int n);

/* append any number of strings to dst */
char *StrApp(char **dst, ...);

/* like strncpy, but the result is null-terminated */
void xstrncpy(char *dest, const char *src, size_t n);

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

/* checks wether <what> is a directory */
int is_a_directory(char *what);

#ifdef USE_PAM
/* duplicate src */
int StrDup (char **dst, const char *src);
#endif
