/***************************************************************************
                           chvt.c  -  Terminal switching functions
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003 by Noberasco Michele
    e-mail               : noberasco.gnu@disi.unige.it
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

/* Working to make it compliant to GNU Standards :P ------------------------- */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "misc.h"
#include "chvt.h"

static int is_a_console(int fd)
{
  char arg = 0;
  int returnval=(ioctl(fd, KDGKBTYPE, &arg) == 0 && ((arg == KB_101) || (arg == KB_84)));
  if(returnval == -1)
    perror("Qingy error: is_a_console");
  return returnval;
}

static int open_a_console(char *fnam) 
{
  int fd; 
  fd = open(fnam, O_RDWR); 
  if (fd < 0) 
    return -1; 
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
  if(!(stdin && stdout && stderr))
    perror("Qingy error: switch_to_tty");
  free(ttyname);
  if (!stdin || !stdout || !stderr) 
    return 0;
  return 1;
}

int get_active_tty(void)
{
  struct vt_stat term_status;
  int fd = getfd();
  
  if (fd == -1) return -1;
  if (ioctl (fd, VT_GETSTATE, &term_status) == -1)
    {
      perror("Qingy error: get_active_tty1");
      close(fd);
      return -1;
    }
  if(close(fd) == -1) 
    perror("Qingy error:  get_active_tty2");
  return term_status.v_active;
}

int set_active_tty(int tty)
{
  int retval = 1;
  int fd;
  
  /* we switch to /dev/tty<tty> */
  if ((fd = getfd()) == -1) return 0;
  if (ioctl (fd, VT_ACTIVATE, tty) == -1){
    perror("Qingy error: set_active_tty1");
    retval = 0;
  }
  if (ioctl (fd, VT_WAITACTIVE, tty) == -1){
    perror("Qingy error : set_active_tty2");
    retval = 0;
  }
  if(close(fd) == -1)
    perror("Qingy error: set_active_tty3");
  return retval;
}

int get_available_tty(void)
{
  int fd = getfd();
  int available;
  
  if (fd == -1) return -1;
  if(ioctl (fd, VT_OPENQRY, &available) == -1)
    perror("Qingy error: get_avail_tty1");
  if(close(fd) == -1)
    perror("Qingy error get_avail_tty1");
  return available;
}

void tty_redraw(void)
{
  int active_tty = get_active_tty();
  int temp_tty = get_available_tty();
  
  /* Actually, this is implemented as a hack
     I do not think that a *real* way to do 
     this even exists...                      */
  
  if (temp_tty != -1) set_active_tty(temp_tty);
  else /* hope this never happens, as it could be an active tty running X */
    set_active_tty(active_tty + 20);
  set_active_tty(active_tty);
}

int disallocate_tty(int tty)
{
  int fd;
  
  /* we switch to /dev/tty<tty> */
  if ((fd = getfd()) == -1) return 0;
  if (ioctl (fd, VT_DISALLOCATE, tty) == -1){
    perror("Qingy error:disalloc tt1 "); 
    return 0;
  }
  if (close(fd) == -1 ) {
    perror("Qingy error disalloc2");
    return 0;
  }
  return 1;
}

int lock_tty_switching(void)
{
  int fd = getfd();
  
  if (fd == -1) return 0;
  if (ioctl (fd, VT_LOCKSWITCH, 513) == -1) { 
    perror("Qingy error lockswitch1");
    return 0;
  }
  
  if(close(fd)==-1)
    perror("Qingy error lockswitc2");
  return 1;
}

int unlock_tty_switching(void)
{
  int fd = getfd();
  
  if (fd == -1) return 0;
  if (ioctl (fd, VT_UNLOCKSWITCH, 513) == -1){
    perror("Qingy error unlockswitc1");
    return 0;
  }
  
  if(close(fd)==-1)
    perror("Qingy error unlockswitch2");
  return 1;
}

void stderr_disable(void)
{
  stderr = freopen("/dev/null", "w", stderr);
  if(!stderr)
    perror("Qingy error stderdis");
}

void stderr_enable(void)
{
  char *ttyname = create_tty_name(get_active_tty());
  
  if (!ttyname) return;
  stderr = freopen(ttyname, "w", stderr);
  if(!stderr) 
    perror("Qingy error stderen");
  free(ttyname);
}
