## Makefile.am for qingy/src/DirectFB

pkglibdir = $(prefix)/@LIB_DIR@/

INCLUDES  = -Iobjects -I../ -I../libraries
AM_CFLAGS = @DIRECTFB_CFLAGS@

pkglib_LTLIBRARIES  =
noinst_LTLIBRARIES  =

if !WANT_STATIC
pkglib_LTLIBRARIES += libdirectfb.la
else
noinst_LTLIBRARIES += libdirectfb_static.la
endif

libdirectfb_la_SOURCES =	                          \
    objects/button.c objects/button.h               \
    objects/label.c objects/label.h                 \
    objects/textbox.c objects/textbox.h             \
    objects/combobox.c objects/combobox.h           \
    objects/utils.c objects/utils.h                 \
    objects/screen_saver.c objects/screen_saver.h   \
    objects/screensaver_module.h                    \
    directfb_mode.c directfb_mode.h
libdirectfb_static_la_SOURCES = $(libdirectfb_la_SOURCES)
libdirectfb_static_la_LIBADD  = @DIRECTFB_LIBS@
libdirectfb_la_LDFLAGS = @DIRECTFB_LIBS@
libdirectfb_la_LIBADD  = ../libraries/libsession.la
libdirectfb_la_DEPENDENCIES  = cleanup

cleanup:
	rm -f libdirectfb.la
