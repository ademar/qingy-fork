## Makefile.am for qingy/src

INCLUDES = -IDirectFB

SUBDIRS = DirectFB

sbin_PROGRAMS = qingy

qingy_SOURCES =	load_settings.c load_settings.h \
								misc.c misc.h \
								chvt.c chvt.h \
								session.c session.h \
								main.c

qingy_LDADD   = DirectFB/libdirectfb.a @DIRECTFB_LIBS@
