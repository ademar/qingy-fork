## Makefile.am for qingy/src

SUBDIRS = libraries DirectFB

INCLUDES  = -I.. -Ilibraries -IDirectFB

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

if !WANT_STATIC
qingy_LDADD   = DirectFB/libqingyui_directfb.la libraries/libqingy.la
qingy_LDFLAGS = -rdynamic
else
qingy_LDADD   = DirectFB/libqingyui_directfb_static.la libraries/libqingy_static.la @PASSWDLIB@
qingy_LDFLAGS = -static
endif
