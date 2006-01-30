## Makefile.am for qingy/media/gpm-lock

gpmlockdatadir = $(sysconfdir)/qingy

if WANT_GPM_LOCK
gpmlockdata_SCRIPTS = @GPM_LOCK@/pre_GUI.sh @GPM_LOCK@/post_GUI.sh
else
gpmlockdata_SCRIPTS =
endif

EXTRA_DIST =                             \
    gentoo/pre_GUI.sh gentoo/post_GUI.sh \
    fedora/pre_GUI.sh fedora/post_GUI.sh
