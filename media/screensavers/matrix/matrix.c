/***********************************************************************/
/*  matrix.c - Matrix screensaver module for Qingy                     */
/*  v. 0.1                                                             */
/*  Copyright (C) 2004 Paolo Gianrossi - All rights reserved           */
/*  by Paolo Gianrossi <paolino.gnu@disi.unige.it>                     */
/*                                                                     */
/* This program is free software; you can redistribute it and/or       */
/* modify it under the terms of the GNU General Public License as      */
/* published by the Free Software Foundation; either version 2 of the  */
/* License, or (at your option) any later version.                     */
/*                                                                     */
/* This program is distributed in the hope that it will be useful, but */
/* WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   */
/* General Public License for more details.                            */
/*                                                                     */
/* You should have received a copy of the GNU General Public License   */
/* along with this program; if not, write to the Free Software         */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-    */
/* 1307, USA.                                                          */
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
#define ROWS 24
void 
screen_saver_entry(Q_screen_t env)
{
	char *font_path;
  unsigned int seconds=0;
  unsigned int milli_seconds=100;
  int i, j;
  int cel_w, cel_h;
  
  char Matrix[ROWS][COLS];
  char speeds[COLS];
  char curspeed[COLS];
  IDirectFBFont *font;
  DFBFontDescription font_dsc;

	font_path = (char *) calloc(strlen(env.data_dir)+10, sizeof(char));
	strcpy(font_path, env.data_dir);
	strcat(font_path, "/matr.ttf");

  /* initialize matrix to 0 */
  for(i=0; i<COLS; i++){
    for(j=1; j<ROWS; j++){
      Matrix[j][i]=0;
    }
  }
  
  for(i=0; i<COLS; i++){
    speeds[i]=rand()%100;
    curspeed[i]=0;
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
			if(!rand()%3) 
				Matrix[0][i]='\0';
			else 
				Matrix[0][i]=(rand()%(127-32))+32;
		}
		font_dsc.flags = DFDESC_HEIGHT;
		font_dsc.height = 30;
		env.dfb->CreateFont(env.dfb, font_path, &font_dsc, &font);
		env.surface->SetFont(env.surface, font);
		env.surface->Clear (env.surface, 0x00, 0x00, 0x00, 0x01);
		env.surface->SetColor (env.surface, 0x10, 0xA0, 0x10, 0xff);
		for(i=0; i<COLS; i++){
			if(speeds[i] > curspeed[i])
			{
				curspeed[i]++;
				j=rand()%ROWS;
				Matrix[j][i]='\0';
	    
			}
			else{
				curspeed[i]=0;
				speeds[i]=rand()%30;
			}
			for(j=0; j<ROWS; j++){
				env.surface->SetColor(env.surface, 0x10, 0xA0, 0x10, 0x01);
				if(Matrix[j][i])
					env.surface->DrawGlyph (env.surface,
																	Matrix[j][i],
																	i* cel_h ,  j * 3 * cel_w + rand() % 20 , DSTF_LEFT);
			}
		}
		env.surface->Flip (env.surface, NULL, DSFLIP_BLIT);
		for(i=ROWS-1; i; i--){
			for(j=0; j<COLS; j++){
				Matrix[i][j]=Matrix[i-1][j];
			}
		}
		env.screen_saver_events->WaitForEventWithTimeout(env.screen_saver_events, seconds, milli_seconds);
		if (env.screen_saver_events->HasEvent(env.screen_saver_events) == DFB_OK)
			break;
	}
}
