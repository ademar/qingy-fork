## Makefile.am for qingy/src/DirectFB

INCLUDES  = -I../.. -Iobjects -I../ -I../libraries
AM_CFLAGS = @DIRECTFB_CFLAGS@

if !WANT_STATIC
pkglib_LTLIBRARIES     = libqingyui_directfb.la
noinst_LTLIBRARIES     =
else
pkglib_LTLIBRARIES     =
noinst_LTLIBRARIES     = libqingyui_directfb_static.la
endif

libqingyui_directfb_la_SOURCES =    objects/screensaver_module.h   \
    objects/button.c         objects/button.h                      \
    objects/label.c          objects/label.h                       \
    objects/textbox.c        objects/textbox.h                     \
    objects/combobox.c       objects/combobox.h                    \
    objects/utils.c          objects/utils.h                       \
    objects/screen_saver.c   objects/screen_saver.h                \
    directfb_mode.c          directfb_mode.h

libqingyui_directfb_static_la_SOURCES = $(libqingyui_directfb_la_SOURCES)
libqingyui_directfb_static_la_LIBADD  = @DIRECTFB_LIBS@
libqingyui_directfb_la_LDFLAGS = @DIRECTFB_LIBS@
