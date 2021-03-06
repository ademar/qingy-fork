/***************************************************************************
                      vt.c  -  Terminal handling functions
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003-2007 by Noberasco Michele
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

#ifndef CHVT_H
#define CHVT_H

#define Switch_TTY                                                                          \
	if (!switch_to_tty(our_tty_number))                                                       \
	{                                                                                         \
		fprintf(stderr, "\nUnable to switch to virtual terminal /dev/tty%d\n", our_tty_number); \
		exit(EXIT_FAILURE);                                                                     \
	}

/* NOTE: should be an inline func, and StrApp should be probably fixed */
#define create_tty_name(tty) StrApp((char**)NULL, "/dev/tty", int_to_str(tty), (char*)NULL)
#define create_vc_name(tty)  StrApp((char**)NULL, "/dev/vc/", int_to_str(tty), (char*)NULL)

/* keyboard and relative leds status (numlock, ...) */
#define onoff(a) ((a) ? "on " : "off")
typedef struct _KB_status
{
	char leds;
	char flags;

} KB_status;

#define LED_NLOCK  1
#define LED_CMPOSE 2
#define LED_SCRLCK 4
#define LED_CLOCK  8

/* change stdin, stdout and stderr to a new tty */
int switch_to_tty(int tty);

/* get the currently active tty */
int get_active_tty(void);

/* jump to another tty */
int set_active_tty(int tty);

/* get the number of an unused tty */
int get_available_tty(void);

/* checks whether a tty is in use */
int is_tty_available(int tty);

/* allow to lock/unlock tty switching */
int lock_tty_switching(void);
int unlock_tty_switching(void);

/* disallocate tty */
int disallocate_tty(int tty);

/* enable or disable stderr */
void stderr_disable(void);
void stderr_enable(int *vt);

/* get console framebuffer resolution */
char *get_fb_resolution(char *fb_device);

/* reset framebuffer console if DirectFB hangs... */
void reset_console(int do_fork);

#endif /* !CHVT_H */
