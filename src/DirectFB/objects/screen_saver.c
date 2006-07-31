/***************************************************************************
                       screen_saver.c  -  description
                            --------------------
    begin            : Apr 10 2003
    copyright        : (C) 2003-2006 by Noberasco Michele, Paolo Gianrossi
    e-mail           : michele.noberasco@tiscali.it
                       paolino.gnu@disi.unige.it
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

#ifdef USE_SCREEN_SAVERS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <directfb.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>

#include "memmgmt.h"
#include "load_settings.h"
#include "misc.h"
#include "screen_saver.h"
#include "screensaver_module.h"
#include "logger.h"

char** il2cc(struct _screensaver_options *options)
{
  int max=15;
  int n_str=0;
  char **tbl;
  char* holder;
  
  tbl=(char**)malloc(sizeof(char*) * (max +1));
  for (; options; options=options->next)
	{
		if (!options->option) continue;

		holder=strdup(options->option);
		if (n_str == max)
		{
			char **temp;
			max+= 16;
			temp = (char **) realloc(tbl, (max+1)*sizeof(char *));
			if (!temp)
	    {
	      writelog(ERROR, "Memory allocation failure!\n");
	      abort();
	    }
			tbl = temp;
		}
		*(tbl + n_str) = holder;
		n_str++;
	}
  *(tbl + n_str)= NULL;
  return tbl;
}

void activate_screen_saver(IDirectFBEventBuffer *events)
{
  void* handle;
  char* ssv_name = NULL;
  char* error;
  
  Q_screen_t screenEnv;
  
  /* We put here the actual screen saver function to launch */
  void (*do_screen_saver)(Q_screen_t);

  if (!screen_saver_surface) return;
  if (!events)               return;
  
  screen_saver_surface->GetSize(screen_saver_surface, &(screenEnv.screen_width), &(screenEnv.screen_height));
  screenEnv.surface             = screen_saver_surface;
  screenEnv.dfb                 = screen_saver_dfb;
  screenEnv.screen_saver_events = events;
  screenEnv.params              = il2cc(screensaver_options);
	screenEnv.data_dir            = screensavers_dir;

  /* get what screensaver we want and load it */
  ssv_name = StrApp((char **)NULL, screensavers_dir, "/", screen_saver_kind, ".qss", (char*)NULL);

	WRITELOG(DEBUG, "Trying to open screen saver \"%s\"\n", ssv_name);

  handle   = dlopen(ssv_name, RTLD_NOW); 
  if (!handle)
	{ 
    clear_screen();
    screen_saver_surface->DrawString (screen_saver_surface, 
																			"Not able to open screensaver.",
																			-1, screenEnv.screen_width / 2, screenEnv.screen_height / 2, DSTF_CENTER);
    screen_saver_surface->Flip (screen_saver_surface, NULL, 0);
    sleep(2);
    return;
  }
  
  do_screen_saver = dlsym(handle, "screen_saver_entry");
  if ((error = dlerror()))
	{
    clear_screen();
    screen_saver_surface->DrawString (screen_saver_surface, "Not able to launch screensaver.", 
																			-1, screenEnv.screen_width / 2, screenEnv.screen_height / 2, DSTF_CENTER);
    screen_saver_surface->Flip (screen_saver_surface, NULL, 0);
    sleep(2);
    return;
  }

  (*do_screen_saver)(screenEnv);
  free(screenEnv.params);
    
  dlclose(handle);
}
#endif /* want screen savers */
