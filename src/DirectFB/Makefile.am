## Makefile.am for qingy/src

pkglibdir = $(prefix)/@LIB_DIR@/

INCLUDES = -Iobjects -I../
AM_CFLAGS = @DIRECTFB_CFLAGS@

pkglib_LTLIBRARIES = libdirectfb.la

libdirectfb_la_SOURCES =	\
				objects/button.c objects/button.h \
				objects/label.c objects/label.h \
				objects/textbox.c objects/textbox.h \
				objects/combobox.c objects/combobox.h \
				objects/utils.c objects/utils.h \
				objects/screen_saver.c objects/screen_saver.h objects/screensaver_module.h \
				directfb_mode.c directfb_mode.h

libdirectfb_la_LDFLAGS      = @DIRECTFB_LIBS@
libdirectfb_la_LIBADD       = ../libqingy.la
libdirectfb_la_DEPENDENCIES = libqingy.la

libqingy.la:
	cd ..
	make --directory=../ libqingy.la
