pkglibdir          = $(prefix)@SCREENSAVERS_DIR@

pkglib_PROGRAMS    =

if WANT_SCREEN_SAVERS
pkglib_PROGRAMS   += photos.qss
endif

photos_qss_SOURCES = photos.c

INCLUDES  = -I../../../src/DirectFB/objects/
AM_CFLAGS = @DIRECTFB_CFLAGS@ -fpic -shared
