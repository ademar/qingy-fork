/***************************************************************************
                          photos.c  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003-2006 by Noberasco Michele
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <directfb.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <screensaver_module.h>

#include <memmgmt.h>
#include <misc.h>

#include "wildcards.h"

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
# include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
# include <sys/dir.h>
# endif
# if HAVE_NDIR_H
# include <ndir.h>
# endif
#endif


static char **images;


int is_image(char *filename)
{
	char *temp;
	int lun;

	if (!filename) return 0;
	lun = strlen(filename);
	if (lun<5) return 0;

	temp = filename + lun - 4;
	if (!strcasecmp(temp, ".png")) return 1;
	if (!strcasecmp(temp, ".jpg")) return 1;

	temp = filename + lun - 5;
	if (!strcasecmp(temp, ".jpeg")) return 1;

	return 0;
}

int load_images_list(char **params, int silent)
{
	int n_images = 0;
	int max      = 0;
	int i        = 0;

  for (; params[i]; i++)
  {
		char *param;
    DIR  *path;
    struct dirent *entry;

		while ((param=expand_path(params[i])))
		{
			if (!silent) fprintf(stderr, "Loading files from '%s': ", param);
			path = opendir(param);
			if (!path) continue;

			while (1)
			{
				if (!(entry= readdir(path))) break;
				if (!strcmp(entry->d_name, "." )) continue;
				if (!strcmp(entry->d_name, "..")) continue;
				if (is_image(entry->d_name))
				{
					char *temp = calloc(strlen(param)+strlen(entry->d_name)+2, sizeof(char));
					if (n_images == max)
					{
						char **temp;
						max+= 100;
						temp = (char **) realloc(images, max*sizeof(char *));
						if (!temp)
						{
							fprintf(stderr, "screen_saver: memory allocation failure!\n");
							abort();
						}
						images = temp;
					}
					strcpy(temp, param);
					if (*(temp+strlen(temp)-1) != '/') strcat(temp, "/");
					strcat(temp, entry->d_name);
					images[n_images] = temp;
					n_images++;
				}
			}
			closedir(path);
			if (!silent) fprintf(stderr, "%d images so far...\n", n_images);
		}
  }

	return n_images;
}

void screen_saver_entry(Q_screen_t env)
{
	static int n_images = 0;

	DFBSurfaceDescription desc;
	IDirectFBImageProvider *provider=NULL;
	DFBRectangle dest_rectangle;
	int errorcount = 0;
	int image;
  unsigned int seconds=5;
  unsigned int milli_seconds=0;
	int image_width,  image_height;
	float xy_ratio;
	IDirectFBSurface *dest_surface=NULL;


  if (!env.dfb || !env.surface) return;
  /* we clear event buffer to avoid being bailed out immediately */
  env.screen_saver_events->Reset(env.screen_saver_events);

	if (!n_images)
	{
		time_t epoch;
		struct tm curr_time;
		
		env.surface->Clear (env.surface, 0x00, 0x00, 0x00, 0xFF);
		env.surface->DrawString (env.surface, "Loading images list...", -1, env.screen_width / 2, env.screen_height / 2, DSTF_CENTER);
		env.surface->Flip (env.surface, NULL, DSFLIP_BLIT);
		n_images = load_images_list(env.params, env.silent);
		epoch = time(NULL);
		localtime_r(&epoch, &curr_time);
		srand(curr_time.tm_sec);
	}

  /* do screen saver until an input event arrives */
  while (1)
	{
		if (!n_images)
		{
			static int toggle = 0;
			env.surface->Clear (env.surface, 0x00, 0x00, 0x00, 0xFF);
			if (!toggle)
				env.surface->DrawString (env.surface, "No images to display!", -1, env.screen_width / 2, env.screen_height / 2, DSTF_CENTER);
			env.surface->Flip (env.surface, NULL, DSFLIP_BLIT);
			toggle = !toggle;
			seconds       = 2;
			milli_seconds = 0;
			env.screen_saver_events->WaitForEventWithTimeout(env.screen_saver_events, seconds, milli_seconds);
			if (env.screen_saver_events->HasEvent(env.screen_saver_events) == DFB_OK) return;
			continue;
		}

		while (errorcount < 100)
		{
			image = rand() % n_images;

			if (env.dfb->CreateImageProvider (env.dfb, images[image], &provider) == DFB_OK) break;
			errorcount++;
			if (errorcount == 100)
			{
				env.surface->Clear (env.surface, 0x00, 0x00, 0x00, 0xFF);
				env.surface->DrawString (env.surface, "Error loading images!", -1, env.screen_width / 2, env.screen_height / 2, DSTF_CENTER);
				env.surface->Flip (env.surface, NULL, DSFLIP_BLIT);
				sleep(5);
				return;
			}
		}

		/* Let's make sure that the image will not be distorted */
		provider->GetSurfaceDescription(provider, &desc);
		image_width  = desc.width;
		image_height = desc.height;
		xy_ratio = (float)image_width / (float)image_height;

		if (image_width > env.screen_width)
		{
			image_width  = env.screen_width;
			image_height = image_width/xy_ratio;
		}

		if (image_height > env.screen_height)
		{
			image_height = env.screen_height;
			image_width  = image_height*xy_ratio;
		}

		if (image_width > image_height)
		{
			dest_rectangle.x=(env.screen_width-image_width)/2;
			dest_rectangle.w=image_width;
			dest_rectangle.h=dest_rectangle.w/xy_ratio;
			dest_rectangle.y=(env.screen_height-dest_rectangle.h)/2;
		}
		else
		{
			dest_rectangle.y=(env.screen_height-image_height)/2;
			dest_rectangle.h=image_height;
			dest_rectangle.w=dest_rectangle.h*xy_ratio;
			dest_rectangle.x=(env.screen_width-dest_rectangle.w)/2;
		}

		/* now we display the image */
		env.surface->GetSubSurface(env.surface, &dest_rectangle, &dest_surface);
		env.surface->Clear (env.surface, 0x00, 0x00, 0x00, 0xFF);
		provider->RenderTo (provider, dest_surface, NULL);
		provider->Release (provider);
		provider = NULL;
		env.surface->Flip (env.surface, NULL, DSFLIP_BLIT);
		dest_surface->Release(dest_surface);
		dest_surface = NULL;

		env.screen_saver_events->WaitForEventWithTimeout(env.screen_saver_events, seconds, milli_seconds);
		if (env.screen_saver_events->HasEvent(env.screen_saver_events) == DFB_OK) return;
	}
}
