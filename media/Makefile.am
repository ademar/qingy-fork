## Makefile.am for qingy/media

SUBDIRS = default @PAM@

settingsdatadir = $(prefix)/etc/qingy
settingsdata_DATA = settings

directfbrcdir = $(prefix)/etc
directfbrc_DATA = directfbrc.qingy

BUILT_SOURCES = directfbrc.qingy
directfbrc.qingy: creatercfile
	./creatercfile directfbrc.qingy @FBSET@

noinst_PROGRAMS = creatercfile
creatercfile_SOURCES = creatercfile.c
