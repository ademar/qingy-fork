/***********************************************************************/
/*  matrix.c - Matrix screensaver module for Qingy 	       	       */
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

#include <screensaver_module.h>

#define COLS 80
#define ROWS 10
void 
screen_saver_entry(Q_screen_t env)
{
  static int toggle = 1;
  int posx;
  int posy;
  unsigned int seconds=0;
  unsigned int milli_seconds=50;
  int i, j;
  int cel_w, cel_h;
  
  char Matrix[ROWS][COLS];
  char *firstone=Matrix[0];
 

   /* initialize matrix to 0 */
  for(i=1; i<ROWS; i++){
    for(j=0; j<COLS; j++){
      Matrix[i][j]=0;
    }
  }
  /* our rect dims: */
  cel_w=env.screen_width / COLS;
  cel_h=env.screen_height / ROWS;
  
  if (!env.dfb || !env.surface) return;
  /* we clear event buffer to avoid being bailed out immediately */
  env.screen_saver_events->Reset(env.screen_saver_events);
  
   
  /* do screen saver until an input event arrives */
  while (1)
    {
      for(i=0; i<COLS; i++){
	if(!rand()%4) firstone[i]='\0';
	firstone[i]=(rand()%(127-32))+32;
      }
      env.surface->Clear (env.surface, 0x00, 0x00, 0x00, 0xFF);
      env.surface->SetColor (env.surface, 0xFF, 0x50, 0x50, 0xFF);
      for(i=0; i<ROWS; i++){
	for(j=0; j<COLS; j++){
	  if(*(firstone+(i*COLS)+j))
	    env.surface->DrawGlyph (env.surface,
				    *(firstone+(i*COLS)+j)
				    , env.screen_width - j*cel_h, env.screen_height - i*cel_w, DSTF_LEFT);
	}
      }
      env.surface->Flip (env.surface, NULL, DSFLIP_BLIT);
      firstone+=COLS;
      firstone = (firstone >= &Matrix[ROWS][COLS]? Matrix[0]:firstone);
      
      env.screen_saver_events->WaitForEventWithTimeout(env.screen_saver_events, seconds, milli_seconds);
      if (env.screen_saver_events->HasEvent(env.screen_saver_events) == DFB_OK)
	break;
    }
}
