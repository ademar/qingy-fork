/***************************************************************************
                       load_settings.h  -  description
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


#define DATADIR "/etc/qingy/"
#define SETTINGS DATADIR"settings"
#define LAST_USER DATADIR"lastuser"

char *XSESSIONS_DIRECTORY;
char *XINIT;
char *FONT;

/* get program settings from config file */
int load_settings(void);

/* get/set name of last user that logged in */
char *get_last_user(void);
int set_last_user(char *user);

/* get/save last session used by <user> */
char *get_last_session(char *user);
int set_last_session(char *user, char *session);