screensaverdir     = @SCREENSAVERS_DIR@

if WANT_SCREEN_SAVERS
screensaver_PROGRAMS   = matrix.qss
else
screensaver_PROGRAMS   =
endif

matrix_qss_SOURCES = matrix.c

INCLUDES  = -I../../.. -I../../../src/DirectFB/objects/
AM_CFLAGS = @DIRECTFB_CFLAGS@ -fpic -shared

if WANT_SCREEN_SAVERS
screensaver_DATA  = matr.ttf
else
screensaver_DATA  =
endif

EXTRA_DIST = matr.ttf
