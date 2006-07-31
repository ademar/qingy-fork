/***************************************************************************
                        keybindings.c  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003-2005 by Noberasco Michele
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

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "keybindings.h"
#include "memmgmt.h"
#include "load_settings.h"
#include "logger.h"
#include "misc.h"

typedef struct _keybinding
{
	actions             action;
	modifiers           modifier;
	int                 key;
	struct _keybinding *next;
	
} keybinding;

static keybinding *keybindings = NULL;
char ret[2] = {'0', '\0'};

char *print_modifier(modifiers modifier)
{
	switch (modifier)
	{
		case CONTROL:
			return "CTRL-";
		case ALT:
			return "ALT-";
		case CTRLALT:
			return "CTRL-ALT-";
		default: /* nothing */
			break;
	}

	return "";
}

char *print_action(actions action)
{
	switch (action)
	{
		case DO_SLEEP:
			return "put machine to sleep";
		case DO_POWEROFF:
			return "poweroff machine";
		case DO_REBOOT:
			return "reboot machine";
		case DO_PREV_TTY:
			return "switch to left tty";
		case DO_NEXT_TTY:
			return "switch to right tty";
		case DO_KILL:
			return "kill qingy";
		case DO_SCREEN_SAVER:
			return "activate screen saver";
		case DO_TEXT_MODE:
			return "revert to text mode";
		default: /* nothing */
			break;
	}

	return "";
}

char *print_key(int key)
{
	if (key == MENU  ) return "menu";
	if (key == WIN   ) return "win";
	if (key == ESCAPE) return "ESC";

	ret[0] = key;

	return ret;
}

modifiers get_modifier(char *source)
{
	char *delim = strchr(source, '-');

	if (!delim) return NONE;
	
	if (!strncmp(source, "alt",  3)) return ALT;
	if (!strncmp(source, "ctrl", 4)) return CONTROL;

	return NONE;
}

int get_key(char *source)
{
	char *delim = strchr(source, '-');

	if (!delim) delim = source;
	else        delim++;

	if (!strcmp(delim, "menu")) return MENU;
	if (!strcmp(delim, "win" )) return WIN;
	if (!strcmp(delim, "esc" )) return ESCAPE;
	
	return delim[0];
}

int check_dupe_keybinding(actions action, modifiers modifier, int key)
{
	keybinding *my_keybinding = keybindings;

	for (; my_keybinding != NULL; my_keybinding = my_keybinding->next)
	{
		if (action   == my_keybinding->action      )
		{
			WRITELOG(ERROR, "Cannot add keybinding: action \"%s\" already defined!\n", print_action(action));
			return 0;
		}
		if (modifier == my_keybinding->modifier &&
				key      == my_keybinding->key         )
		{
			WRITELOG(ERROR, "Cannot add keybinding: key combination '%s%s' already defined!\n", print_modifier(modifier), print_key(key));
			return 0;
		}
	}

	return 1;
}

int add_to_keybindings(actions action, char *string)
{
	keybinding *new_keybinding;
	int         key;
	modifiers   modifier;

	if (!string) return 0;
	to_lower(string); /* let's sanitize out input... */

	modifier = get_modifier(string);
	key      = get_key     (string);

	if (!check_dupe_keybinding(action, modifier, key)) return 0;

	if (!keybindings)
	{
		keybindings    = (keybinding *) calloc(1, sizeof(keybinding));
		new_keybinding = keybindings;
	}
	else
	{
		for (new_keybinding = keybindings; new_keybinding->next != NULL; new_keybinding = new_keybinding->next);
		new_keybinding->next = (keybinding *) calloc(1, sizeof(keybinding));
		new_keybinding       = new_keybinding->next;
	}

	new_keybinding->action   = action;
	new_keybinding->modifier = modifier;
	new_keybinding->key      = key;
	new_keybinding->next     = NULL;

	WRITELOG(DEBUG, "added keybinding: '%s%s' will %s...\n", print_modifier(modifier), print_key(key), print_action(action));

	return 1;
}

void destroy_keybindings_list()
{
	while (keybindings)
	{
		keybinding *my_keybinding = keybindings;
		keybindings = keybindings->next;
		
		free(my_keybinding);
	}
}

actions search_keybindings(modifiers modifier, int key)
{
	keybinding *my_keybinding = keybindings;

	for (; my_keybinding != NULL; my_keybinding = my_keybinding->next)
	{
		if (my_keybinding->modifier == modifier &&
				my_keybinding->key      == key        )
			return my_keybinding->action;
	}

	return DO_NOTHING;
}
