/***************************************************************************
                           chvt.c  -  description
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


#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <sys/ioctl.h>

#include "misc.h"
#include "chvt.h"

static int is_a_console(int fd)
{
  char arg;
  arg = 0;

	return (ioctl(fd, KDGKBTYPE, &arg) == 0 && ((arg == KB_101) || (arg == KB_84)));
}

static int open_a_console(char *fnam)
{
  int fd;

  fd = open(fnam, O_RDWR);
  if (fd < 0) return -1;
  if (!is_a_console(fd))
	{
    close(fd);
    return -1;
  }

	return fd;
}

int getfd()
{
  int fd;

  fd = open_a_console("/dev/tty");
  if (fd >= 0) return fd;

  fd = open_a_console("/dev/tty0");
  if (fd >= 0) return fd;

  fd = open_a_console("/dev/console");
  if (fd >= 0) return fd;

  for (fd = 0; fd < 3; fd++)
    if (is_a_console(fd))
	    return fd;

  fprintf(stderr, "Couldnt get a file descriptor referring to the console\n");

	return -1;	/* total failure */
}

int switch_to_tty(int tty)
{
	char *ttyname = create_tty_name(tty);

	if (!ttyname) return 0;
  /* we set stdin, stdout and stderr to the new tty */
	stdin  = freopen(ttyname, "r", stdin);
	stdout = freopen(ttyname, "w", stdout);
  stderr = freopen(ttyname, "w", stderr);
  if ( (stdin == NULL) || (stdout == NULL) || (stderr == NULL) ) return 0;

	return 1;
}

/* get the currently active tty */
int get_active_tty(void)
{
	int tty_file_descriptor;
	struct vt_stat terminal_status;

	tty_file_descriptor = getfd();
	if (tty_file_descriptor == -1) return -1;
	if (ioctl (tty_file_descriptor, VT_GETSTATE, &terminal_status) == -1) return -1;

	return terminal_status.v_active;
}

/* jump to another tty */
int set_active_tty(int tty)
{
  int fd;

  /* we switch to /dev/tty<tty> */
	if ( (fd = getfd()) == -1) return 0;
	if (ioctl (fd, VT_ACTIVATE, tty) == -1) return 0;
  if (ioctl (fd, VT_WAITACTIVE, tty) == -1) return 0;
  if (close(fd) != 0) return 0;

	return 1;
}

/* block tty switching */
int lock_tty_switching(void)
{
  int fd;

	if ( (fd = getfd()) == -1) return 0;
	if (ioctl (fd, VT_LOCKSWITCH, 513) == -1) return 0;
  if (close(fd) != 0) return 0;

	return 1;
}

/* allow tty switching */
int unlock_tty_switching(void)
{
  int fd;

	if ( (fd = getfd()) == -1) return 0;
	if (ioctl (fd, VT_UNLOCKSWITCH, 513) == -1) return 0;
  if (close(fd) != 0) return 0;

	return 1;
}

void stderr_disable(void)
{
	stderr = freopen("/dev/null", "w", stderr);
}

void stderr_enable(void)
{
	char *ttyname = create_tty_name(get_active_tty());

	if (!ttyname) return;
	stderr = freopen(ttyname, "w", stderr);
}
