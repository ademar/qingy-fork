## Makefile.am for qingy/src

SUBDIRS = libraries DirectFB

pkglibdir = $(prefix)/@LIB_DIR@/
INCLUDES  = -Ilibraries -IDirectFB

sbin_PROGRAMS  = qingy
qingy_SOURCES  = main.c qingy_constants.h

qingy_LDADD    =
qingy_LDFLAGS  =

if !WANT_STATIC
qingy_LDADD   += libraries/libvt.la DirectFB/libdirectfb.la
qingy_LDFLAGS += -rdynamic
else
qingy_LDADD   += DirectFB/libdirectfb_static.la libraries/libqingy.la
qingy_LDFLAGS += -static
endif
