## Makefile.am for qingy/media

SUBDIRS = default sessions pam screensavers

if WANT_EMACS_STUFF
lisp_LISP  = qingy-mode.el
endif
EXTRA_DIST = qingy-mode.el

settingsdatadir = $(prefix)/etc/qingy
dist_settingsdata_DATA   = welcomes
nodist_settingsdata_DATA = settings
