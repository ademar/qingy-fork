/***********************************************************************/
/*  running_time.c - Pixel screensaver module for Qingy 	       */
/*  v. 0.1 							       */
/*  Copyright (C) 2004 Paolo Gianrossi - All rights reserved 	       */
/*  by Paolo Gianrossi <paolino.gnu@disi.unige.it> 		       */
/*  								       */
/* This program is free software; you can redistribute it and/or       */
/* modify it under the terms of the GNU General Public License as      */
/* published by the Free Software Foundation; either version 2 of the  */
/* License, or (at your option) any later version.		       */
/* 								       */
/* This program is distributed in the hope that it will be useful, but */
/* WITHOUT ANY WARRANTY; without even the implied warranty of 	       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   */
/* General Public License for more details.			       */
/*  								       */
/* You should have received a copy of the GNU General Public License   */
/* along with this program; if not, write to the Free Software 	       */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-    */
/* 1307, USA.							       */
/***********************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <directfb.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void 
screen_saver_entry(IDirectFB *dfb, IDirectFBSurface *surface,
		   IDirectFBEventBuffer * screen_saver_events, 
		   int screen_width, int screen_height)
{
  static int toggle = 1;
  int width  = screen_width  / 200;
  int height = screen_height / 300;
  int posx;
  int posy;
  unsigned int seconds=0;
  unsigned int milli_seconds=50;
  
  char string[15];
  
  int x_slope;
  int y_slope;
  time_t t;
  srand((unsigned)time(NULL));
  
  if (!dfb || !surface) return;
  /* we clear event buffer to avoid being bailed out immediately */
  screen_saver_events->Reset(screen_saver_events);
  
  posx= rand() % (screen_width-200)+100;
  posy = rand() % (screen_height-20)+10;
  
  x_slope=(rand() %2) +1 ;
  y_slope=(3-x_slope) ;

  /* do screen saver until an input event arrives */
  while (1)
    {
      t=time(NULL);
      strftime (string, 15, "%I:%M:%S %p", localtime(&t)); 
      surface->Clear (surface, 0x00, 0x00, 0x00, 0xFF);
      surface->SetColor (surface, 0xFF, 0x50, 0x50, 0xFF);
      surface->DrawString (surface,
			   string, -1, posx, posy, DSTF_CENTER);
      surface->Flip (surface, NULL, DSFLIP_BLIT);

      if(posx <100 || posx > (screen_width -100) || 
	 posy < 10 || posy > (screen_height -10))
	{
	  x_slope=(rand() %2) +1 ;
	  y_slope=(3-x_slope) ;
	  
	  x_slope=((rand()%2)==0)?x_slope:-x_slope;
	  y_slope=((rand()%2)==0)?y_slope:-y_slope;
	  
	  if(posx < 100) 
	    x_slope=(x_slope < 0) ? -x_slope : x_slope;
	  if(posx > (screen_width - 100)) 
	    x_slope=(x_slope > 0) ? -x_slope : x_slope;
	  if(posy < 10) 
	    y_slope=(y_slope < 0) ? -y_slope : y_slope;
	  if(posy > (screen_height - 10))
	    y_slope=(y_slope > 0) ? -y_slope : y_slope;
	}
      posx += x_slope;
      posy += y_slope;

      screen_saver_events->WaitForEventWithTimeout(screen_saver_events, seconds, milli_seconds);
      if (screen_saver_events->HasEvent(screen_saver_events) == DFB_OK)
	break;
    }
}
