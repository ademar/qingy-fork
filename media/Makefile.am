## Makefile.am for qingy/media

SUBDIRS = default sessions pam

lisp_LISP = qingy-mode.el

settingsdatadir = $(prefix)/etc/qingy
settingsdata_DATA = \
		settings        \
		welcomes

directfbrcdir = $(prefix)/etc
directfbrc_DATA = directfbrc.qingy

BUILT_SOURCES = directfbrc.qingy
directfbrc.qingy: creatercfile
	./creatercfile directfbrc.qingy @FBSET@

noinst_PROGRAMS = creatercfile
creatercfile_SOURCES = creatercfile.c
