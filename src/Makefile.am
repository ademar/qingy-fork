## Makefile.am for qingy/src

SUBDIRS = DirectFB

AM_YFLAGS= -d
INCLUDES = -IDirectFB

sbin_PROGRAMS = qingy

BUILT_SOURCES = parse_settings.h

qingy_SOURCES =	scan_settings.l parse_settings.y \
		load_settings.c load_settings.h              \
		session.c session.h                          \
		misc.c misc.h                                \
		chvt.c chvt.h                                \
		tty_guardian.c tty_guardian.h                \
		main.c qingy_constants.h

qingy_LDADD   = DirectFB/libdirectfb.a @DIRECTFB_LIBS@
