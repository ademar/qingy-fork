## Makefile.am for qingy/src/DirectFB

pkglibdir = $(prefix)/@LIB_DIR@/

INCLUDES  = -Iobjects -I../ -I../libraries
AM_CFLAGS = @DIRECTFB_CFLAGS@

noinst_LTLIBRARIES     =
pkglib_LTLIBRARIES     =

if !WANT_STATIC
pkglib_LTLIBRARIES     += libdirectfb.la
else
noinst_LTLIBRARIES     += libdirectfb_static.la
endif

libdirectfb_la_SOURCES =	   objects/screensaver_module.h   \
    objects/button.c         objects/button.h               \
    objects/label.c          objects/label.h                \
    objects/textbox.c        objects/textbox.h              \
    objects/combobox.c       objects/combobox.h             \
    objects/utils.c          objects/utils.h                \
    objects/screen_saver.c   objects/screen_saver.h         \
    directfb_mode.c          directfb_mode.h

libdirectfb_static_la_SOURCES = $(libdirectfb_la_SOURCES)
libdirectfb_static_la_LIBADD  = @DIRECTFB_LIBS@
libdirectfb_la_LDFLAGS = @DIRECTFB_LIBS@
