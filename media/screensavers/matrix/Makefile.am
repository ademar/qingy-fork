pkglibdir          = $(prefix)@SCREENSAVERS_DIR@
screensaverdir     = $(pkglibdir)

pkglib_PROGRAMS    =

if WANT_SCREEN_SAVERS
pkglib_PROGRAMS   += matrix.qss
endif

matrix_qss_SOURCES = matrix.c

INCLUDES  = -I../../../src/DirectFB/objects/
AM_CFLAGS = @DIRECTFB_CFLAGS@ -fpic -shared

screensaver_DATA  =

if WANT_SCREEN_SAVERS
screensaver_DATA += matr.ttf
endif

EXTRA_DIST = matr.ttf
