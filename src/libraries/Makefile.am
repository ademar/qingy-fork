## Makefile.am for qingy/src/libraries

AM_YFLAGS     = -d
INCLUDES      = -I$(top_srcdir) -I$(top_srcdir)/src
BUILT_SOURCES = parse_settings.h

AM_CFLAGS = -DSETTINGS_DIR=\"$(sysconfdir)/qingy\" -DSBINDIR=\"$(sbindir)/\"

if !WANT_STATIC
pkglib_LTLIBRARIES = libqingy.la
noinst_LTLIBRARIES =
else
pkglib_LTLIBRARIES = 
noinst_LTLIBRARIES = libqingy_static.la
endif

libqingy_la_SOURCES =                 \
    scan_settings.l  parse_settings.y \
    load_settings.c  load_settings.h  \
    tty_guardian.c   tty_guardian.h   \
    keybindings.c    keybindings.h    \
    session.c        session.h        \
    memmgmt.c        memmgmt.h        \
    logger.c         logger.h         \
    misc.c           misc.h           \
    vt.c             vt.h

if CRYPTO_OPENSSL
libqingy_la_SOURCES += crypto.h crypto_openssl.c
endif
if CRYPTO_LIBGCRYPT
libqingy_la_SOURCES += crypto.h crypto_libgcrypt.c
endif

EXTRA_libqingy_la_SOURCES =           \
    crypto_openssl.c crypto_libgcrypt.c crypto.h

libqingy_la_LIBADD  = @PASSWDLIB@ @CRYPTOLIB@ @NCURSESLIB@ @XLIBS@
libqingy_la_LDFLAGS = -release @VERSION@

libqingy_static_la_SOURCES = $(libqingy_la_SOURCES)
libqingy_static_la_LIBADD  = @PASSWDLIB@ @CRYPTOLIB@ @NCURSESLIB@ @XLIBS@
libqingy_static_la_LDFLAGS = -release @VERSION@

if USE_CONSOLEKIT
AM_CFLAGS                 += @CONSOLEKIT_CFLAGS@
libqingy_la_LIBADD        += @CONSOLEKIT_LIBS@
libqingy_static_la_LIBADD += @CONSOLEKIT_LIBS@
endif
