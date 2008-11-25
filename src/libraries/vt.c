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

/* Working to make it compliant to GNU Standards :P ---------------------- */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/kd.h>
#include <sys/vt.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>

#include "memmgmt.h"
#include "vt.h"
#include "load_settings.h"
#include "misc.h"
#include "logger.h"

static int is_a_console(int fd)
{
  char arg = 0;

  return (ioctl(fd, KDGKBTYPE, &arg) == 0 && ((arg == KB_101) || (arg == KB_84)));
}

static int open_a_console(char *fnam) 
{
  int fd = open(fnam, O_RDWR);

  if (fd == -1) return -1; 
  if (!is_a_console(fd))
	{ 
		close(fd); 
		return -1; 
	} 

  return fd; 
} 

static int getfd()
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
  
  writelog(ERROR, "Couldnt get a file descriptor referring to the console\n");
  
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
  free(ttyname);
  if (!stdin || !stdout || !stderr) return 0;

  return 1;
}

int get_active_tty(void)
{
  static struct vt_stat *term_status = NULL;
  int fd = getfd();
  
  if (fd == -1) return -1;

	if (!term_status)
		term_status = (struct vt_stat *) calloc(1, sizeof(struct vt_stat));

  if (ioctl (fd, VT_GETSTATE, term_status) == -1)
	{
		close(fd);
		return -1;
	}

	close(fd);

  return term_status->v_active;
}

int set_active_tty(int tty)
{
  int fd;
  
  /* we switch to /dev/tty<tty> */
  if ((fd = getfd()) == -1) return 0;
  if (ioctl (fd, VT_ACTIVATE,   tty) == -1) return 0;
  if (ioctl (fd, VT_WAITACTIVE, tty) == -1) return 0;
  if (close(fd) == -1) return 0;
	
	return 1;
}

int get_available_tty(void)
{
  int fd = getfd();
  int available;
  
  if (fd == -1) return -1;
  if (ioctl (fd, VT_OPENQRY, &available) == -1) return -1;
  if (close(fd) == -1) return -1;

  return available;
}

int disallocate_tty(int tty)
{
  int fd = getfd();
  
  if (fd == -1) return 0;
  if (ioctl (fd, VT_DISALLOCATE, tty) == -1) return 0;
  if (close(fd) == -1) return 0;

  return 1;
}

int lock_tty_switching(void)
{
  int fd = getfd();
  
  if (fd == -1) return 0;
  if (ioctl (fd, VT_LOCKSWITCH, 513) == -1) return 0;
  if (close(fd) == -1) return 0;

  return 1;
}

int unlock_tty_switching(void)
{
  int fd = getfd();
  
  if (fd == -1) return 0;
  if (ioctl (fd, VT_UNLOCKSWITCH, 513) == -1) return 0;
  if (close(fd) == -1) return 0;
  
	return 1;
}

void stderr_disable(void)
{
	fclose(stderr);
}

void stderr_enable(int *vt)
{
  char *ttyname;

	if (!vt)
		ttyname = create_tty_name(get_active_tty());
	else
		ttyname = create_tty_name(*vt);
  
  if (!ttyname) return;

	stderr = fopen(ttyname, "w");
  free(ttyname);
}

char *get_fb_resolution(char *fb_device)
{	
	char *result, *temp1, *temp2;
	struct fb_var_screeninfo fb_var;
	int fb;	

	if (!fb_device) return NULL;

	fb = open(fb_device, O_RDWR);
	if (fb == -1)
	{
		writelog(ERROR, "Cannot get console framebuffer resolution!\n");
		return NULL;
	}
	if (ioctl(fb, FBIOGET_VSCREENINFO, &fb_var) == -1)
	{
		close(fb);
		writelog(ERROR, "Cannot get console framebuffer resolution!\n");
		return NULL;
	}
	if (close(fb) == -1)
	{	/* I don't see why this should fail, though */
		writelog(ERROR, "I won't use your system because it sucks!\n");
		abort();	
	}

	temp1  = int_to_str(fb_var.xres);
	temp2  = int_to_str(fb_var.yres);
	result = StrApp((char**)NULL, temp1, "x", temp2, (char*)NULL);
	free(temp1); free(temp2);

	return result;
}

int is_tty_available(int tty)
{
	int fd = getfd();
	struct vt_stat vtstat;

	if (ioctl(fd, VT_GETSTATE, &vtstat) < 0)
	{
		writelog(ERROR, "VT_GETSTATE failed\n");
		exit(EXIT_FAILURE);
	}
	close(fd);
	
	/* works only for tty < 16 ?!? */
	if (vtstat.v_state & (1 << tty)) return 0;

	return 1;
}

static void restore_console(int fd, struct termios *termios_backup)
{
	ioctl(fd, KDSKBMODE, K_XLATE);
	ioctl(fd, KDSETMODE, KD_TEXT);
	write(fd, "\033[?25h\033[?0c", 11);

	if (termios_backup)
		tcsetattr(fd, TCSANOW, termios_backup);
}

static int zap_console(int fd, struct termios *termios_backup)
{
	struct termios t;
	int            flags;
	int            result = 0;
	
	if (tcgetattr(fd, &t) == -1)
		writelog(ERROR, "Could not get console attributes\n");
	else
	{
		memcpy(termios_backup, &t, sizeof(t));

		t.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
		t.c_lflag &= ~(ISIG|ICANON|ECHO);
		t.c_cc[VTIME] = 0;
		t.c_cc[VMIN] = 0;
		tcsetattr(fd, TCSANOW, &t);

		result = 1;
	}

	ioctl(fd, KDSKBMODE, K_MEDIUMRAW);
	fcntl(fd, F_SETOWN, getpid());

	flags = O_RDONLY | O_ASYNC | O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);

	return result;
}

void reset_console(int do_fork)
{
	if (!do_fork)
	{
		struct termios termios_backup;
		int dest_vt = get_available_tty();
		int have_backup;
		int fd;

		if ((fd = open("/dev/console", O_RDWR)) == -1)
			writelog(ERROR, "Could not open /dev/console\n");
		
		have_backup = zap_console(fd, &termios_backup);

		if (have_backup)
			restore_console(fd, &termios_backup);
		else
			restore_console(fd, NULL);

		unlock_tty_switching();
		set_active_tty(dest_vt);

		return;
	}

	switch ((int)fork())
	{
		case -1: /* error */
			writelog(ERROR, "Cannot issue fork() command!\n");
			sleep(2);
			exit(EXIT_FAILURE);
		case 0: /* child */
			reset_console(0);
			exit(EXIT_SUCCESS);
		default: /* parent */
			wait(NULL);
	}
}
