/***************************************************************************
                         session.h  -  description
                            -------------------
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

#include "misc.h"

#define XSESSIONS_DIRECTORY "/etc/X11/Sessions/"
#define XINIT               "/usr/X11R6/bin/xinit"

struct session
{
	char *name;
	int id;
	struct session *next;
	struct session *prev;
};

struct session sessions;
char username[MAX];
char password[MAX];

/* get info about available sessions */
int get_sessions(void);

/* Password autentication */
int check_password(void);

/* Start the session of your choice */
void start_session(int session_id, int workaround);
