## Makefile.am for qingy/src/libraries

pkglibdir = $(prefix)/@LIB_DIR@/

AM_YFLAGS     = -d
INCLUDES      = -I../
BUILT_SOURCES = parse_settings.h

pkglib_LTLIBRARIES = 
noinst_LTLIBRARIES =

if !WANT_STATIC
pkglib_LTLIBRARIES += libmemmgmt.la        libvt.la         libmisc.la          \
                      libsettings.la       libsession.la
else
noinst_LTLIBRARIES += libqingy.la
endif


# Should we create a nice set of small dlopened libraries...
libmemmgmt_la_SOURCES      = memmgmt.c        memmgmt.h
libvt_la_SOURCES           = vt.c             vt.h
libmisc_la_SOURCES         = misc.c           misc.h
libsettings_la_SOURCES     = scan_settings.l  parse_settings.y \
                             load_settings.c  load_settings.h
libsession_la_SOURCES      = session.c        session.h        \
                             tty_guardian.c   tty_guardian.h

# ...or a big fat library to include into qingy?
libqingy_la_SOURCES  =            \
    $(libmemmgmt_la_SOURCES)      \
    $(libvt_la_SOURCES)           \
    $(libmisc_la_SOURCES)         \
    $(libsession_la_SOURCES)      \
    $(libsettings_la_SOURCES)



libmisc_la_LIBADD          = libmemmgmt.la
libvt_la_LIBADD            = libmisc.la
libsettings_la_LIBADD      = libvt.la
libsession_la_LIBADD       = libsettings.la
