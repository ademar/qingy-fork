pkglibdir          = $(prefix)/etc/qingy/screensavers
screensaverdir     = $(prefix)/etc/qingy/screensavers
dist_screensaver_DATA   = matr.ttf matrix01.ttf
pkglib_PROGRAMS    = matrix.qss
matrix_qss_SOURCES = matrix.c
INCLUDES  = -Iobjects -I../ -I/etc/qingy/screensavers -I../../../src/DirectFB/objects/
AM_CFLAGS = @DIRECTFB_CFLAGS@ -fpic -shared
