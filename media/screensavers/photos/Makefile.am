screensaverdir     = @SCREENSAVERS_DIR@

if WANT_SCREEN_SAVERS
screensaver_PROGRAMS = photos.qss
else
screensaver_PROGRAMS =
endif

photos_qss_SOURCES = photos.c

INCLUDES  = -I../../.. -I../../../src/DirectFB/objects/
AM_CFLAGS = @DIRECTFB_CFLAGS@ -fpic -shared
