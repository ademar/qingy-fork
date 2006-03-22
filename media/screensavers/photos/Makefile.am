screensaverdir     = @SCREENSAVERS_DIR@

if WANT_SCREEN_SAVERS
screensaver_PROGRAMS = photos.qss
else
screensaver_PROGRAMS =
endif

photos_qss_SOURCES = \
    photos.c         \
    wildcards.c      \
    wildcards.h

INCLUDES  = -I../../.. -I../../../src/DirectFB/objects/ -I../../../src/ -I../../../src/libraries/
AM_CFLAGS = @DIRECTFB_CFLAGS@ -fpic -shared

if !WANT_STATIC
photos_qss_LDADD    = ../../../src/libraries/libqingy.la
photos_qss_LDFLAGS  = -rdynamic
else
photos_qss_LDADD    = ../../../src/libraries/libqingy_static.la
photos_qss_LDFLAGS  = -static
endif
