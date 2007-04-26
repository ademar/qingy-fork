/***************************************************************************
                       tty_guardian.h  -  description
                            --------------------
    begin                : Feb 04 2004
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


/* activate post login features if user wants (any of) them.
 * Now supported: tty guardian, session timeout with lock/logout.
 */
void watch_over_session(pid_t proc_id, char *username, int session_vt, int x_vt, int is_x_session, int x_offset);

/* read user password without echoing it */
char *read_password(int tty);
