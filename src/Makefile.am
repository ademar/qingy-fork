## Makefile.am for qingy/src

SUBDIRS = libraries DirectFB

INCLUDES  = -I.. -Ilibraries -IDirectFB

sbin_PROGRAMS  = qingy
qingy_SOURCES  =         \
    qingy_constants.h    \
    main.c

if !WANT_STATIC
qingy_LDADD   = libraries/libqingy.la
qingy_LDFLAGS = -rdynamic
else
qingy_LDADD   = libraries/libqingy_static.la @PASSWDLIB@
qingy_LDFLAGS = -static
endif
