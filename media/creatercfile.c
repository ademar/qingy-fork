/***************************************************************************
                       creatercfile.c  -  description
                            --------------------
    begin                : Sep 21 2003
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

/* 
   Create a directfbrc file with a line
   mode=XRESxYRES
*/	 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX 128

int is_digit(char c)
{
	int test = c - '0';
	
	if (test < 0) return 0;
	if (test > 9) return 0;
	
	return 1;
}

char *get_resolution(char *source)
{
	int i;
	
	if (!source) return NULL;

	for (i=0; is_digit(*(source+i)); i++);
	if (*(source+i) != 'x') return NULL;
	for (i++; is_digit(*(source+i)); i++);
	if (*(source+i) != '-')
		if (*(source+i) != '\"')
			return NULL;
	
	*(source+i) = '\0';
	
	return source;
}

int main(int argc, char *argv[])
{
	char *file_name = NULL;
	char *program = NULL;
	char temp[MAX];
	char *resolution = NULL;
	FILE *fp;

	if (argc != 3) return EXIT_FAILURE;
	file_name = argv[1];
	program = argv[2];
	
	if (!(fp = popen(program, "r"))) return EXIT_FAILURE;
	
	while (1)
	{
		if (!fgets(temp, MAX, fp)) break;
		if (!strncmp(temp, "mode \"", 6))
		{
			resolution = get_resolution(temp+6);
			break;
		}
	}	
	pclose(fp);
	
	if (!resolution) return EXIT_FAILURE;
	if (!(fp = fopen(file_name, "w"))) return EXIT_FAILURE;
	
	fprintf(fp, "mode=%s\n", resolution);
	fclose(fp);
	
	return EXIT_SUCCESS;
}
