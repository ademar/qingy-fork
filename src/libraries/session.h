/***************************************************************************
                         session.h  -  description
                            -------------------
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

/* get info about available sessions */
char *get_sessions(void);

/* Password autentication */
int gui_check_password(char *username, char *password, char *session, int pppid);
int check_password(char *username, char *password);

/* Start the session of your choice */
void start_session(char *username, char *session);

/* sort sessions */
void sort_sessions(char **sessions, int n_items);

/* PAM initialization for autologin */
int autologin_pam_start(char *username);
