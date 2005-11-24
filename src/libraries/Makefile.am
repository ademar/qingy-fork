## Makefile.am for qingy/src/libraries

AM_YFLAGS     = -d
INCLUDES      = -I../.. -I../
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
    keybindings.c    keybindings.h    \
    session.c        session.h        \
    tty_guardian.c   tty_guardian.h   \
    memmgmt.c        memmgmt.h        \
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

libqingy_la_LIBADD  = @PASSWDLIB@ @CRYPTOLIB@
libqingy_la_LDFLAGS = -release @VERSION@

libqingy_static_la_SOURCES = $(libqingy_la_SOURCES)
libqingy_static_la_LIBADD  = @PASSWDLIB@ @CRYPTOLIB@
libqingy_static_la_LDFLAGS = -release @VERSION@
