## Makefile.am for qingy/src

DIST_SUBDIRS  = libraries DirectFB
SUBDIRS       = libraries

INCLUDES      = -I$(top_srcdir) -I$(srcdir)/libraries

if USE_DIRECTFB
SUBDIRS      += DirectFB
INCLUDES     += -I$(srcdir)/DirectFB
endif

sbin_PROGRAMS   = qingy qingy-reset-console
qingy_SOURCES   =        \
    qingy_constants.h    \
    main.c

qingy_reset_console_SOURCES  =  \
    reset-console.c


if !WANT_STATIC
qingy_LDADD                   = libraries/libqingy.la
qingy_LDFLAGS                 = -export-dynamic
qingy_reset_console_LDADD     = libraries/libqingy.la
qingy_reset_console_LDFLAGS   = -export-dynamic
else
qingy_LDADD                   = libraries/libqingy_static.la
qingy_LDFLAGS                 = -static
qingy_reset_console_LDADD     = libraries/libqingy_static.la
qingy_reset_console_LDFLAGS   = -static
endif



EXTRA_qingy_SOURCES =    \
    libgcrypt-keygen.c


if CRYPTO_LIBGCRYPT # Need to generate a key pair, we have a helper program for this

AM_CFLAGS = -DSETTINGS_DIR=\"$(sysconfdir)/qingy\"

qingy_keygen_SOURCES  = libgcrypt-keygen.c
sbin_PROGRAMS        += qingy-keygen

if !WANT_STATIC
qingy_keygen_LDADD    = libraries/libqingy.la
qingy_keygen_LDFLAGS  = -export-dynamic
else
qingy_keygen_LDADD    = libraries/libqingy_static.la
qingy_keygen_LDFLAGS  = -static
endif

endif #CRYPTO_LIBGCRYPT
