screensaverdir     = @SCREENSAVERS_DIR@

if WANT_SCREEN_SAVERS
screensaver_PROGRAMS   = running_time.qss
else
screensaver_PROGRAMS    =
endif

running_time_qss_SOURCES = running_time.c

INCLUDES  = -I../../.. -I../../../src/DirectFB/objects/
AM_CFLAGS = @DIRECTFB_CFLAGS@ -fpic -shared
