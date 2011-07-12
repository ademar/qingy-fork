screensaverdir     = @SCREENSAVERS_DIR@

if WANT_SCREEN_SAVERS
screensaver_LTLIBRARIES = photos.la
else
screensaver_LTLIBRARIES =
endif

photos_la_SOURCES = \
    photos.c         \
    wildcards.c      \
    wildcards.h

INCLUDES   = -I$(top_srcdir) -I$(top_srcdir)/src/DirectFB/objects/ -I$(top_srcdir)/src/ -I$(top_srcdir)/src/libraries/
AM_CFLAGS  = @DIRECTFB_CFLAGS@
AM_LDFLAGS = -module -shared -avoid-version -shrext .qss

if !WANT_STATIC
photos_la_LIBADD = ../../../src/libraries/libqingy.la
else
photos_la_LIBADD = ../../../src/libraries/libqingy_static.la
endif
