screensaverdir     = @SCREENSAVERS_DIR@

if WANT_SCREEN_SAVERS
screensaver_PROGRAMS   = pixel.qss
else
screensaver_PROGRAMS   =
endif

pixel_qss_SOURCES  = pixel.c

INCLUDES  = -I../../.. -I../../../src/DirectFB/objects/
AM_CFLAGS = @DIRECTFB_CFLAGS@ -fpic -shared
