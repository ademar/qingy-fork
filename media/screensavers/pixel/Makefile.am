pkglibdir          = $(prefix)@SCREENSAVERS_DIR@

pkglib_PROGRAMS    =

if WANT_SCREEN_SAVERS
pkglib_PROGRAMS   += pixel.qss
endif

pixel_qss_SOURCES  = pixel.c

INCLUDES  = -I../../../src/DirectFB/objects/
AM_CFLAGS = @DIRECTFB_CFLAGS@ -fpic -shared
