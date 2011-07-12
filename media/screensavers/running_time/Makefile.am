screensaverdir     = @SCREENSAVERS_DIR@

if WANT_SCREEN_SAVERS
screensaver_LTLIBRARIES = running_time.la
else
screensaver_LTLIBRARIES =
endif

running_time_la_SOURCES = running_time.c

INCLUDES   = -I$(top_srcdir) -I$(top_srcdir)/src/DirectFB/objects/
AM_CFLAGS  = @DIRECTFB_CFLAGS@
AM_LDFLAGS = -module -shared -avoid-version -shrext .qss
