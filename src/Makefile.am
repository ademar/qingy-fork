## Makefile.am for qingy/src

SUBDIRS = DirectFB

pkglibdir = $(prefix)/@LIB_DIR@/
AM_YFLAGS = -d
INCLUDES  = -IDirectFB $(LTDLINCL)


BUILT_SOURCES       = parse_settings.h
pkglib_LTLIBRARIES  = libqingy.la
libqingy_la_SOURCES =	                \
		scan_settings.l parse_settings.y  \
		load_settings.c load_settings.h   \
		session.c session.h               \
		misc.c misc.h                     \
		chvt.c chvt.h                     \
		tty_guardian.c tty_guardian.h     \
		qingy_constants.h

sbin_PROGRAMS      = qingy
qingy_SOURCES      = main.c
qingy_LDADD        = libqingy.la
qingy_LDFLAGS      = -rdynamic -ldl
