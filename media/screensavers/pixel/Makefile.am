pkglibdir          = $(prefix)@SCREENSAVERS_DIR@

pkglib_PROGRAMS    = pixel.qss
pixel_qss_SOURCES  = pixel.c

INCLUDES  = -I../../../src/DirectFB/objects/
AM_CFLAGS = @DIRECTFB_CFLAGS@ -fpic -shared
