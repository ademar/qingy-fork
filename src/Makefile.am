## Makefile.am for qingy/src

AM_CFLAGS = \
	@DIRECTFB_CFLAGS@

LIBADDS = \
	@DIRECTFB_LIBS@

sbin_PROGRAMS = qingy

qingy_SOURCES =	load_settings.c load_settings.h \
								misc.c misc.h \
								chvt.c chvt.h \
								directfb_button.c directfb_button.h \
								directfb_utils.c directfb_utils.h \
								directfb_label.c directfb_label.h \
								directfb_textbox.c directfb_textbox.h \
								directfb_combobox.c directfb_combobox.h \
								framebuffer_mode.c framebuffer_mode.h \
								session.c session.h \
								main.c

qingy_LDADD  = $(LIBADDS)
