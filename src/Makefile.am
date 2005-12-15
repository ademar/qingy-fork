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



if CRYPTO_LIBGCRYPT # Need to generate a key pair, we have a helper program for this

settingsdatadir           = $(sysconfdir)/qingy
nodist_settingsdata_DATA  = private_key public_key

libgcrypt_keygen_SOURCES  = libgcrypt-keygen.c
noinst_PROGRAMS           = libgcrypt-keygen

if !WANT_STATIC
libgcrypt_keygen_LDADD    = libraries/libqingy.la
libgcrypt_keygen_LDFLAGS  = -rdynamic
else
libgcrypt_keygen_LDADD    = libraries/libqingy_static.la
libgcrypt_keygen_LDFLAGS  = -static
endif

public_key:	private_key
private_key:	libgcrypt-keygen
	if [ ! -f private_key ] && [ ! -f public_key ]; then ./libgcrypt-keygen; fi

endif #CRYPTO_LIBGCRYPT
