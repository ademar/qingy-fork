AM_CFLAGS = \
	@DIRECTFB_CFLAGS@

LIBADDS = \
	@DIRECTFB_LIBS@

bin_PROGRAMS = qingy

qingy_SOURCES =	misc.c chvt.c directfb_utils.c framebuffer_mode.c session.c main.c
qingy_LDADD  = $(LIBADDS) -lcrypt
