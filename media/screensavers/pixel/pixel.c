/***********************************************************************/
/*  pixel_screensvr.c - Pixel screensaver module for Qingy             */
/*  v. 0.1                                                             */
/*  Copyright (C) 2004 Paolo Gianrossi, Michele Noberasco              */
/* - All rights reserved                                               */
/*                                                                     */	
/*    Michele Noberasco <noberasco.gnu@disi.unige.it>                  */	
/*    Paolo Gianrossi <paolino.gnu@disi.unige.it>                      */
/*                                                                     */
/* This program is free software; you can redistribute it and/or       */
/* modify it under the terms of the GNU General Public License as      */
/* published by the Free Software Foundation; either version 2 of the  */
/* License, or (at your option) any later version.                     */
/*                                                                     */
/* This program is distributed in the hope that it will be useful, but */
/* WITHOUT ANY WARRANTY; without even the implied warranty of 	       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   */
/* General Public License for more details.                            */
/*                                                                     */
/* You should have received a copy of the GNU General Public License   */
/* along with this program; if not, write to the Free Software 	       */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-    */
/* 1307, USA.                                                          */
/***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <directfb.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <screensaver_module.h>

void screen_saver_entry(Q_screen_t env)
{
  static int toggle = 1;
  int width  = env.screen_width  / 200;
  int height = env.screen_height / 300;
  int posx;
  int posy;
  unsigned int seconds=0;
  unsigned int milli_seconds=500;
  
  srand((unsigned)time(NULL));
  
  if (!env.dfb || !env.surface) return;
  /* we clear event buffer to avoid being bailed out immediately */
  env.screen_saver_events->Reset( env.screen_saver_events);
  
  /* do screen saver until an input event arrives */
  while (1)
	{
		if (toggle)
		{
			posx = rand() % env.screen_width;
			posy = rand() % env.screen_height;
	  
			env.surface->Clear (env.surface, 0x00, 0x00, 0x00, 0xFF);
			env.surface->SetColor (env.surface, 0x50, 0x50, 0x50, 0xFF);
			env.surface->FillRectangle (env.surface, posx, posy, width, height);
			env.surface->Flip (env.surface, NULL, DSFLIP_BLIT);
			toggle = 0;
		}
		else
		{
			env.surface->Clear (env.surface, 0x00, 0x00, 0x00, 0xFF);
			env.surface->Flip (env.surface, NULL, DSFLIP_BLIT);
			toggle = 1;
		}
		env.screen_saver_events->WaitForEventWithTimeout(env.screen_saver_events, seconds, milli_seconds);
		if (env.screen_saver_events->HasEvent(env.screen_saver_events) == DFB_OK)
			break;
	}
}
