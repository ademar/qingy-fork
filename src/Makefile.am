## Makefile.am for qingy/src

AM_CFLAGS = \
	@DIRECTFB_CFLAGS@

LIBADDS = \
	@DIRECTFB_LIBS@

sbin_PROGRAMS = qingy

qingy_SOURCES =	load_settings.c misc.c chvt.c directfb_button.c directfb_utils.c directfb_label.c directfb_textbox.c directfb_combobox.c framebuffer_mode.c session.c main.c
qingy_LDADD  = $(LIBADDS)
