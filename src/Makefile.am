## Makefile.am for qingy/src

prefix= $(QINGY_PREFIX)

AM_CFLAGS = \
	@DIRECTFB_CFLAGS@

LIBADDS = \
	@DIRECTFB_LIBS@

sbin_PROGRAMS = qingy

qingy_SOURCES =	misc.c chvt.c directfb_utils.c framebuffer_mode.c session.c main.c
qingy_LDADD  = $(LIBADDS) -lcrypt
