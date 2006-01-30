## Makefile.am for qingy/src

SUBDIRS = libraries DirectFB

INCLUDES  = -I.. -Ilibraries -IDirectFB

sbin_PROGRAMS   = qingy
qingy_SOURCES   =        \
    qingy_constants.h    \
    main.c


if !WANT_STATIC
qingy_LDADD     = libraries/libqingy.la
qingy_LDFLAGS   = -rdynamic
else
qingy_LDADD     = libraries/libqingy_static.la
qingy_LDFLAGS   = -static
endif



EXTRA_qingy_SOURCES =           \
    libgcrypt-keygen.c gpgme-keygen.c


if CRYPTO_LIBGCRYPT # Need to generate a key pair, we have a helper program for this

AM_CFLAGS = -DSETTINGS_DIR=\"$(sysconfdir)/qingy\"

qingy_keygen_SOURCES  = libgcrypt-keygen.c
sbin_PROGRAMS        += qingy-keygen

if !WANT_STATIC
qingy_keygen_LDADD    = libraries/libqingy.la
qingy_keygen_LDFLAGS  = -rdynamic
else
qingy_keygen_LDADD    = libraries/libqingy_static.la
qingy_keygen_LDFLAGS  = -static
endif

endif #CRYPTO_LIBGCRYPT


#if CRYPTO_GPGME # Need to generate a key pair, we have a helper program for this

#settingsdatadir           = $(sysconfdir)/qingy
#nodist_settingsdata_DATA  = private_key public_key

#gpgme_keygen_SOURCES  = gpgme-keygen.c
#noinst_PROGRAMS       = gpgme-keygen

#if !WANT_STATIC
#gpgme_keygen_LDADD    = libraries/libqingy.la
#gpgme_keygen_LDFLAGS  = -rdynamic
#else
#gpgme_keygen_LDADD    = libraries/libqingy_static.la
#gpgme_keygen_LDFLAGS  = -static
#endif

#public_key:	private_key
#private_key:	gpgme-keygen
#	if [ ! -f private_key ] && [ ! -f public_key ]; then ./gpgme-keygen; fi

#endif #CRYPTO_GPGME
