## Makefile.am for qingy/src/libraries

pkglibdir = $(prefix)/@LIB_DIR@/

AM_YFLAGS     = -d
INCLUDES      = -I../
BUILT_SOURCES = parse_settings.h

pkglib_LTLIBRARIES = 
noinst_LTLIBRARIES =

if !WANT_STATIC
pkglib_LTLIBRARIES += libqingy.la
else
noinst_LTLIBRARIES += libqingy_static.la
endif

libqingy_la_SOURCES = scan_settings.l  parse_settings.y \
                      load_settings.c  load_settings.h  \
                      session.c        session.h        \
                      tty_guardian.c   tty_guardian.h

libqingy_static_la_SOURCES = $(libqingy_la_SOURCES)
