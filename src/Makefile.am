## Makefile.am for qingy/src
BUILT_SOURCES=scan_settings.c parse_settings.c parse_settings.h

AM_YFLAGS= -d

INCLUDES = -IDirectFB

SUBDIRS = DirectFB

sbin_PROGRAMS = qingy

qingy_SOURCES =	load_settings.c load_settings.h \
								misc.c misc.h \
								chvt.c chvt.h \
								session.c session.h \
								main.c \
								scan_settings.l parse_settings.y

qingy_LDADD   = DirectFB/libdirectfb.a @DIRECTFB_LIBS@
