pkglibdir          = $(prefix)@SCREENSAVERS_DIR@
screensaverdir     = $(pkglibdir)

pkglib_PROGRAMS    = matrix.qss
matrix_qss_SOURCES = matrix.c

INCLUDES  = -I../../../src/DirectFB/objects/
AM_CFLAGS = @DIRECTFB_CFLAGS@ -fpic -shared

dist_screensaver_DATA = matr.ttf
