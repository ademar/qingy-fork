## Makefile.am for qingy/src/DirectFB

INCLUDES  = -I$(top_srcdir) -I$(srcdir)/objects -I$(srcdir)/.. -I$(srcdir)/../libraries
AM_CFLAGS = @DIRECTFB_CFLAGS@

sbin_PROGRAMS          = qingy-DirectFB
qingy_DirectFB_SOURCES =                                           \
    objects/screensaver_module.h                                   \
    objects/button.c         objects/button.h                      \
    objects/label.c          objects/label.h                       \
    objects/textbox.c        objects/textbox.h                     \
    objects/combobox.c       objects/combobox.h                    \
    objects/utils.c          objects/utils.h                       \
    objects/screen_saver.c   objects/screen_saver.h                \
    pm.c                     pm.h                                  \
    directfb_mode.c          directfb_mode.h

if !WANT_STATIC
qingy_DirectFB_LDADD   = ../libraries/libqingy.la @DIRECTFB_LIBS@
qingy_DirectFB_LDFLAGS = -export-dynamic
else
qingy_DirectFB_LDADD   = ../libraries/libqingy_static.la @DIRECTFB_LIBS@
qingy_DirectFB_LDFLAGS = -static
endif
