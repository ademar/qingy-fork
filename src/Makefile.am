## Makefile.am for qingy/src

SUBDIRS = libraries DirectFB

pkglibdir = $(prefix)/@LIB_DIR@/
INCLUDES  = -Ilibraries -IDirectFB

sbin_PROGRAMS  = qingy
qingy_SOURCES  =         \
    qingy_constants.h    \
    memmgmt.c            \
    memmgmt.h            \
    vt.c                 \
    vt.h                 \
    misc.c               \
    misc.h               \
    main.c

qingy_LDADD    =
qingy_LDFLAGS  =

if !WANT_STATIC
qingy_LDADD   += DirectFB/libdirectfb.la libraries/libqingy.la
qingy_LDFLAGS += -rdynamic
else
qingy_LDADD   += DirectFB/libdirectfb_static.la libraries/libqingy_static.la
qingy_LDFLAGS += -static
endif
