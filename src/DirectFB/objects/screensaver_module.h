#ifndef QINGY_SCREENSVR_MOD_H
#define QINGY_SCREENSVR_MOD_H

#define Q_SSM_H_VERSION   0.1


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <directfb.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

typedef struct _ss_env 
{
  IDirectFB *dfb;
  IDirectFBSurface *surface;
  IDirectFBEventBuffer * screen_saver_events;
  int screen_width;
  int screen_height;
  struct _image_paths *image_paths;
  char** params;
} Q_screen_t;

void clear_screen(void);

#endif /* !QINGY_SCREENSVR_MOD_H */

