/***************************************************************************
                        keybindings.h  -  description
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

#ifndef HAVE_KEYBINGINGS_H
#define HAVE_KEYBINGINGS_H 1

#define ESCAPE    27 /* ASCII code of ESCAPE key                         */
#define MENU   61984 /* ASCII code of MENU   key, at least on my machine */
#define WIN    61968 /* ASCII code of WIN    key, at least on my machine */

typedef enum 
{
	NONE=0,
	SHIFT,
	CONTROL,
	ALT,
	ALTGR,
/*META,   I need these as keys, */
/*SUPER,  not as modifiers      */
/*HYPER,                        */
	CTRLALT

} modifiers;

typedef enum 
{
	DO_NOTHING=0,
	DO_SLEEP,
  DO_POWEROFF,
  DO_REBOOT,
	DO_NEXT_TTY,
	DO_PREV_TTY,
	DO_KILL,
	DO_SCREEN_SAVER,
	DO_TEXT_MODE

} actions;

/* add a new keybinding */
int add_to_keybindings(actions action, char *key);

/* wipe out all keybindings */
void destroy_keybindings_list();

/* check wether a particular key combo has an action associated to it */
actions search_keybindings(modifiers modifier, int key);

/* print name of these elements */
char *print_action  (actions   action  );
char *print_modifier(modifiers modifier);
char *print_key     (int       key     );

#endif /* HAVE_KEYBINGINGS_H */
