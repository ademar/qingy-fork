screensaverdir     = @SCREENSAVERS_DIR@

if WANT_SCREEN_SAVERS
screensaver_LTLIBRARIES = pixel.la
else
screensaver_LTLIBRARIES =
endif

pixel_la_SOURCES  = pixel.c

INCLUDES   = -I$(top_srcdir) -I$(top_srcdir)/src/DirectFB/objects/
AM_CFLAGS  = @DIRECTFB_CFLAGS@
AM_LDFLAGS = -module -shared -avoid-version -shrext .qss
