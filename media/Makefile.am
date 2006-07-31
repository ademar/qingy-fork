## Makefile.am for qingy/media

SUBDIRS = default gpm-lock sessions pam screensavers

if WANT_EMACS_STUFF
lisp_LISP  = qingy-mode.el
else
lisp_LISP  =
endif
EXTRA_DIST = qingy-mode.el

settingsdatadir = $(sysconfdir)/qingy
dist_settingsdata_DATA   = welcomes
nodist_settingsdata_DATA = settings

settings: Makefile
	@echo -n "Creating config file... "
	@if test -f "settings"; then rm settings; fi
	@echo "# Directory containing X sessions" >> settings
	@echo "x_sessions = \"@X_SESSIONS_DIR@/\"" >> settings
	@echo "# Directory containing text mode sessions" >> settings
	@echo "text_sessions = \"@TEXT_SESSIONS_DIR@/\"" >> settings
	@echo "# Directory where qingy should put its temporary files" >> settings
	@echo "temp_files_dir = \"@TEMP_FILES_DIR@\"" >> settings
	@echo "" >> settings
	@echo "# Full path to the X server" >> settings
	@echo "# x_server = \"@XFREE@\"" >> settings
	@echo "# Full path to the 'xinit' executable" >> settings
	@echo "xinit = \"@XINIT@\"" >> settings
	@echo "" >> settings
	@echo "# Parameter we should pass to the X server" >> settings
	@echo "x_args = \"-nolisten tcp\"" >> settings
	@echo "" >> settings
	@echo "# How verbose should qingy be?" >> settings
	@echo "# Possible values are debug, error" >> settings
	@echo "# Default value is error" >> settings
	@echo "log_level = error" >> settings
	@echo "" >> settings
	@echo "# Where should qingy messages be logged?" >> settings
	@echo "# Values can be one or more of the following:" >> settings
	@echo "# console, file, syslog" >> settings
	@echo "# Default value is console" >> settings
	@echo "# log_facilities = console, file" >> settings
	@echo "log_facilities = console" >> settings
	@echo "" >> settings
	@echo "# Offset to search for an available X server number." >> settings
	@echo "# This number affects the DISPLAY env variable." >> settings
	@echo "# Default is 1, setting it to 0 will make buggy OpenGL implementations" >> settings
	@echo "# (like the ATI one) work with qingy, but it will also make it impossible" >> settings
	@echo "# to start an X server from console using startx without passing it" >> settings
	@echo "# extra parameters." >> settings
	@echo "#x_server_offset = 1" >> settings
	@echo "" >> settings
	@echo "# Scripts that should be executed just before/after qingy GUI is fired up/shut down" >> settings
if WANT_GPM_LOCK
	@echo "pre_gui_script  = \"$(settingsdatadir)/pre_GUI.sh\"" >> settings
	@echo "post_gui_script = \"$(settingsdatadir)/post_GUI.sh\"" >> settings
else
	@echo "#pre_gui_script  = \"/path/to/pre_guiscript.sh\"" >> settings
	@echo "#post_gui_script = \"/path/to/post_gui_script.sh\"" >> settings
endif
	@echo "" >> settings
if WANT_SCREEN_SAVERS
	@echo "# Where are the screen savers?" >> settings
	@echo "screensavers_dir = \"@SCREENSAVERS_DIR@\"" >> settings
	@echo "" >> settings
	@echo "#screensaver \"pixel\"" >> settings
	@echo "screensaver \"running_time\"#=\"%H:%S:%M\"" >> settings
	@echo "" >> settings
endif
	@echo "# Where are the themes?" >> settings
	@echo "themes_dir = \"@THEMES_DIR@\"" >> settings
	@echo "" >> settings
	@echo "# What theme do you want (you can also specify 'random')" >> settings
	@echo "# theme = random" >> settings
	@echo "theme = \"default\"" >> settings
	@echo "" >> settings
	@echo "# Who is allowed to shut down the system?" >> settings
	@echo "# Allowed options are 'everyone', 'root', 'noone'" >> settings
	@echo "# default policy is everyone" >> settings
	@echo "# shutdown_policy = everyone" >> settings
	@echo "" >> settings
	@echo "# How should latest user be calculated?" >> settings
	@echo "# global means get latest user that logged in using qingy from whichever tty" >> settings
	@echo "# tty    means get latest user that logged in using current tty" >> settings
	@echo "# none   means do not get (and set) latest user" >> settings
	@echo "# default policy is global" >> settings
	@echo "#last_user_policy = global" >> settings
	@echo "" >> settings
	@echo "# How should latest user session be calculated?" >> settings
	@echo "# user means get last session of each user" >> settings
	@echo "# tty  means get last session of current tty" >> settings
	@echo "# none means do not get (and set) latest session" >> settings
	@echo "# default policy is user" >> settings
	@echo "#last_session_policy = user" >> settings
	@echo "" >> settings
	@echo "# What happens when we press the 'sleep' button?" >> settings
	@echo "#sleep = \"/usr/local/sbin/hibernate\"" >> settings
	@echo "" >> settings
	@echo "# whether we should clear background image during dialogs (default is no)..." >> settings
	@echo "# this is the default setting, it gets overridden if the theme you are using" >> settings
	@echo "# sets the same setting differently..." >> settings
	@echo "# clear_background = yes" >> settings
	@echo "" >> settings
	@echo "# whether to allow session locking; if you enable this, when you try to" >> settings
	@echo "# switch to a qingy-controlled tty whose owner is not your current" >> settings
	@echo "# user, you will be asked for the password of that user before being" >> settings
	@echo "# allowed to continue. If you are root, of course, you can switch" >> settings
	@echo "# to any tty you chose to. Default setting is 'no'." >> settings
	@echo "#lock_sessions = yes" >> settings
	@echo "" >> settings
	@echo "# whether to allow session timeout; if you enable this, after the amount" >> settings
	@echo "# of minutes specified in idle_timeout variable, idle_action will be" >> settings
	@echo "# performed. Allowed actions are:" >> settings
	@echo "# lock     will lock user session asking you for your password" >> settings
	@echo "# logout   will close your session" >> settings
	@echo "#idle_timeout = 30" >> settings
	@echo "#idle_action  = lock" >> settings
	@echo "" >> settings
	@echo "# How many times should we retry to fire app qingy interface" >> settings
	@echo "# before reverting to text mode? Default is 0..." >> settings
	@echo "#retries = 1" >> settings
	@echo "" >> settings
	@echo "# these options are valid only if qingy is started from tty3" >> settings
	@echo "#tty = 3" >> settings
	@echo "#{" >> settings
	@echo "#	theme = \"fireplace\"" >> settings
if WANT_SCREEN_SAVERS
	@echo "#	screensaver \"pixel\"" >> settings
endif
	@echo "" >> settings
	@echo "#	# Should we auto log in?" >> settings
	@echo "#	# Totally insecure, but very convenient ;-)" >> settings
	@echo "#	# Note that this section must be put inside a tty=n{} block" >> settings
	@echo "#	# Also, if you decide to use this feature, it is better" >> settings
	@echo "#	# that you also make this settings file readable only by root" >> settings
	@echo "#	autologin" >> settings
	@echo "#	{" >> settings
	@echo "#		username = \"myuser\"" >> settings
	@echo "#		password = \"mypassword\"" >> settings
	@echo "#		# You can also use 'session=lastsession' to automatically choose last user session" >> settings
	@echo "#		session  = \"Text: emacs\"" >> settings
	@echo "#		# if set to 'no', qingy will autologin only once every system restart" >> settings
	@echo "#		relogin  = no" >> settings
	@echo "#	}" >> settings
	@echo "#}" >> settings
	@echo "" >> settings
	@echo "keybindings" >> settings
	@echo "{" >> settings
	@echo "	prev_tty    = \"win\"      # switch to left tty" >> settings
	@echo "	next_tty    = \"menu\"     # switch to right tty" >> settings
	@echo "	poweroff    = \"ALT-p\"    # shutdown your system" >> settings
	@echo "	reboot      = \"ALT-r\"    # restart your system" >> settings
	@echo "	screensaver = \"ALT-s\"    # activate screen saver" >> settings
	@echo "	sleep       = \"ALT-z\"    # put machine to sleep" >> settings
	@echo "#	kill        = \"CTRL-c\"   # kill qingy" >> settings
	@echo "	text_mode   = \"CTRL-ESC\" # Revert to text mode" >> settings
	@echo "}" >> settings
	@echo "done"
