/***************************************************************************
                       screen_saver.c  -  description
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <directfb.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <dlfcn.h>

#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

#include "load_settings.h"
#include "misc.h"
#include "screen_saver.h"
#include "screensaver_module.h"

void activate_screen_saver(void)
{
  void* handle;
  int screen_width;
  int screen_height;
  char* ssv_name = NULL;
  char* error;
  
  Q_screen_t screenEnv;
  
  /* We put here the actual screen saver function to launch */
  void (*do_screen_saver)(Q_screen_t);

  if (!screen_saver_surface) return;
  if (!screen_saver_events) return;
  
  screen_saver_surface->GetSize(screen_saver_surface, &(screenEnv.screen_width), &(screenEnv.screen_height));
  screenEnv.surface=screen_saver_surface;
  screenEnv.dfb=screen_saver_dfb;
  screenEnv.screen_saver_events=screen_saver_events;
  screenEnv.params=NULL;

  /* get what screensaver we want and load it */
  ssv_name=StrApp((char **)NULL, DATADIR, "screensavers/", SCREENSAVER, ".qss", (char*)NULL);
  handle=dlopen(ssv_name, RTLD_NOW); 
  if (!handle) { 
    clear_screen();
    screen_saver_surface->DrawString (screen_saver_surface, 
				      "Not able to open screensaver.",
				      -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
    screen_saver_surface->Flip (screen_saver_surface, NULL, 0);
    sleep(2);
    return;
  }
  
  do_screen_saver=dlsym(handle, "screen_saver_entry");
  if((error = dlerror()) != NULL)  {
    clear_screen();
    screen_saver_surface->DrawString (screen_saver_surface, "Not able to launch screensaver.", 
				      -1, screen_width / 2, screen_height / 2, DSTF_CENTER);
    screen_saver_surface->Flip (screen_saver_surface, NULL, 0);
    sleep(2);
    return;
  }

  /* if(!strcmp("photo", screen_saver_kind)){ */
  /* do_screen_saver = photo_screen_saver; */
  /* seconds = 5; */
  /* milli_seconds = 0; */
  /*   } */
  /*   else{ */
  /* fall to default */
  
  (*do_screen_saver)(screenEnv);
  dlclose(handle);
}


/* int is_image(char *filename) */
/* { */
/*   char *temp; */
/*   int lun; */
  
/*   if (!filename) return 0; */
/*   lun = strlen(filename); */
/*   if (lun<5) return 0; */
  
/*   temp = filename + lun - 4; */
/*   if (!strcmp(temp, ".png")) return 1; */
/*   if (!strcmp(temp, ".jpg")) return 1; */
  
/*   return 0;	 */
/* } */

/* int load_images_list(void) */
/* { */
/*   struct _image_paths *paths; */
/*   int n_images = 0; */
/*   int max = 0; */
  
/*   for (paths = image_paths; paths != NULL; paths=paths->next) */
/*     { */
/*       DIR *path; */
/*       struct dirent *entry; */
      
/*       if (!paths->path) continue; */
      
/*       if (!silent) fprintf(stderr, "Loading files from '%s': ", paths->path); */
/*       path = opendir(paths->path); */
/*       if (!path) continue; */
      
/*       while (1) */
/*     {			 */
/*       if (!(entry= readdir(path))) break; */
/*       if (!strcmp(entry->d_name, "." )) continue; */
/*       if (!strcmp(entry->d_name, "..")) continue; */
/*       if (is_image(entry->d_name)) */
/* 	{ */
/* 	  char *temp = calloc(strlen(paths->path)+strlen(entry->d_name)+2, sizeof(char)); */
/* 	  if (n_images == max) */
/* 	    { */
/* 	      char **temp; */
/* 	      max+= 100; */
/* 	      temp = (char **) realloc(images, max*sizeof(char *)); */
/* 	      if (!temp) */
/* 		{ */
/* 		  fprintf(stderr, "screen_saver: memory allocation failure!\n"); */
/* 		  abort(); */
/* 		} */
/* 	      images = temp; */
/* 	    }				 */
/* 	  strcpy(temp, paths->path); */
/* 	  if (*(temp+strlen(temp)-1) != '/') strcat(temp, "/"); */
/* 	  strcat(temp, entry->d_name); */
/* 	  images[n_images] = temp; */
/* 	  n_images++; */
/* 	} */
/*     } */
/*       closedir(path); */
/*       if (!silent) fprintf(stderr, "%d images so far...\n", n_images); */
/*     } */
  
/*   return n_images; */
/* } */

/* void photo_screen_saver(IDirectFB *dfb, IDirectFBSurface *surface, int screen_width, int screen_height) */
/* {	 */
/*   IDirectFBImageProvider *provider;	 */
/*   static int n_images = 0; */
/*   static int error = 0; */
/*   int errorcount = 0; */
/*   int image; */
  
/*   if (!dfb || !surface || !image_paths) return; */
/*   if (!n_images && !error) */
/*     { */
/*       time_t epoch; */
/*       struct tm curr_time; */
      
/*       surface->Clear (surface, 0x00, 0x00, 0x00, 0xFF); */
/*       surface->DrawString (surface, "Loading images list...", -1, screen_width / 2, screen_height / 2, DSTF_CENTER); */
/*       surface->Flip (surface, NULL, DSFLIP_BLIT);		 */
/*       n_images = load_images_list(); */
/*       epoch = time(NULL); */
/*       localtime_r(&epoch, &curr_time); */
/*       srand(curr_time.tm_sec); */
/*     } */
/*   if (!n_images) */
/*     { */
/*       static int toggle = 0; */
/*       surface->Clear (surface, 0x00, 0x00, 0x00, 0xFF); */
/*       if (!toggle) */
/* 	surface->DrawString (surface, "No images to display!", -1, screen_width / 2, screen_height / 2, DSTF_CENTER); */
/*       surface->Flip (surface, NULL, DSFLIP_BLIT); */
/*       toggle = !toggle; */
/*       error = 1; */
/*       return; */
/*     } */
  
/*   while (errorcount < 100) */
/*     { */
/*       image = rand() % n_images; */
/*       if (dfb->CreateImageProvider (dfb, images[image], &provider) == DFB_OK) break; */
/*       errorcount++; */
/*     } */
  
/*   surface->Clear (surface, 0x00, 0x00, 0x00, 0xFF); */
/*   provider->RenderTo (provider, surface, NULL); */
/*   provider->Release (provider); */
/*   surface->Flip (surface, NULL, DSFLIP_BLIT); */
/* } */
