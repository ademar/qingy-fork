pkglibdir          = $(prefix)@SCREENSAVERS_DIR@

pkglib_PROGRAMS    =

if WANT_SCREEN_SAVERS
pkglib_PROGRAMS   += running_time.qss
endif

running_time_qss_SOURCES = running_time.c

INCLUDES  = -I../../../src/DirectFB/objects/
AM_CFLAGS = @DIRECTFB_CFLAGS@ -fpic -shared
