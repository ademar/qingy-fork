%{
/*******************************************************************************/
/* parse_settings.y - Qingy theme and settings files parser 		               */
/*        								                                                     */
/*  Copyright (C) 2004 Paolo Gianrossi, (C) 2005-2006 Michele Noberasco        */
/*  All rights reserved                                                        */
/*  by Paolo Gianrossi   <paolino.gnu@disi.unige.it>                           */
/*     Michele Noberasco <michele.noberasco@tiscali.it>                        */
/* 									                                                           */
/* This program is free software; you can redistribute it and/or 	             */
/* modify  under the terms of the GNU General Public License as 	             */
/* published by the Free Software Foundation; either version 2, or (at 	       */
/* your option) any later version.					                                   */
/* 									                                                           */
/* This program is distributed in the hope that it will be useful, but 	       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of 		             */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 	         */
/* General Public License for more details.				                             */
/* 									                                                           */
/* You should have received a copy of the GNU General Public License 	         */
/* along with GNU Emacs; see the file COPYING.  If not, write to the 	         */
/* Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.	         */
/*******************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include "memmgmt.h"
#include "load_settings.h"
#include "logger.h"
#include "misc.h"
#include "keybindings.h"
  
#define YYERROR_VERBOSE
#define TTY_CHECK_COND    if (!intended_tty || intended_tty == current_tty)
#define SSAVER_CHECK_COND if (ssaver_is_set) { erase_options(); ssaver_is_set = 0; }
  
extern FILE* yyin;
extern int yylex();
extern int in_theme;
static int in_window               = 0;
static int clear_background_is_set = 0;
static int intended_tty            = 0;
static int ssaver_is_set           = 0;
static int set_theme_result;

/* key bindings vars */
static actions   action;

static window_t wind =
  {
		0,
    0,
    0,
    0,
    0,
    0,
    LARGE,
    LEFT,
    &default_text_color,
    &default_cursor_color,
    UNKNOWN,
    NULL,
    NULL,
    NULL,
    NULL,
		NULL
  };

static cursor_t curs =
  {
		1,
    NULL,
		0,
		0,
		0,
		NULL
  };

%}

%union
{
  int ival;
  char* str;
  unsigned char color[4];
}

/* settings only lvals */
%token SCREENSAVER_TOK SCRSVRS_DIR_TOK SCRSVR_TIMEOUT_TOK SCRN_POWER_TOK XSESSION_DIR_TOK TXTSESSION_DIR_TOK XINIT_TOK 
%token SHUTDOWN_TOK TTY_TOK THEMES_DIR_TOK X_SERVER_TOK
%token DFB_INTERFACE_TOK RESET_CONSOLE_TOK X_ARGS_TOK TEMP_FILES_DIR_TOK

/* windows && theme blocks */
%token THEME_TOK WINDOW_TOK 

/* theme */
%token RAND_TOK

/* other lvals */
%token CLEAR_BACKGROUND_TOK LOCK_SESSIONS_TOK

/* other rvals */
%token NULL_TOK

/* theme colors */
%token DEFAULT_TXT_COL_TOK DEFAULT_CUR_COL_TOK OTHER_TXT_COL_TOK 		     

/* other theme lvals */
%token BG_TOK FONT_TOK BUTTON_OPAC_TOK WIN_OP_TOK SEL_WIN_OP_TOK THEME_RES_TOK

/* shutdown policies */
%token EVERYONE_TOK ONLY_ROOT_TOK NO_ONE_TOK

/* window specific lvals */
%token WPOLL_TIME_TOK WTEXT_COLOR_TOK WCURSOR_COLOR_TOK WINDOW_LINK_TOK
%token WTYPE_TOK WWIDTH_TOK WHEIGHT_TOK WCOMMAND_TOK WCONTENT_TOK

/* window text size */
%token WTEXT_SIZE_TOK WTEXT_TINY_TOK WTEXT_SMALLER_TOK WTEXT_SMALL_TOK WTEXT_MEDIUM_TOK WTEXT_LARGE_TOK

/* window text orient */
%token WTEXT_ORIENTATION WTEXT_LEFT_TOK WTEXT_CENTER_TOK WTEXT_RIGHT_TOK

/* answer tokens */
%token YES_TOK NO_TOK

/* autologin stuff tokens */
%token AUTOLOGIN_TOK USERNAME_TOK SESSION_TOK RELOGIN_TOK LAST_SESSION_TOK

/* sleep tokens */
%token SLEEP_TOK

/* keybindings tokens */
%token NEXT_TTY_TOK PREV_TTY_TOK POWEROFF_TOK REBOOT_TOK KILL_TOK TEXT_MODE_TOK
%token MENU_KEY_TOK WIN_KEY_TOK ALT_KEY_TOK CTRL_KEY_TOK KEYBINDINGS_TOK

/* last user/session tokens */
%token USER_TOK LAST_USER_POLICY_TOK LAST_SESSION_POLICY_TOK GLOBAL_TOK NONE_TOK

/* scrips that are called before the gui fires up and after it is shut down */
%token PRE_GUI_TOK POST_GUI_TOK

/* X server offset and tty handling */
%token X_SERVER_OFFSET_TOK X_SERVER_TTY_TOK QINGY_TTY_TOK UNUSED_TTY_TOK

/* session timeout tokens */
%token IDLE_TIMEOUT_TOK IDLE_ACTION_TOK LOGOUT_TOK LOCK_TOK

/* loggging cababilities */
%token LOG_LEVEL_TOK DEBUG_TOK ERROR_TOK LOG_FACILITIES_TOK CONSOLE_TOK FILE_TOK SYSLOG_TOK

/* mouse cursor stuff */
%token MOUSE_CURSOR_TOK

/* runlevel checking stuff */
%token CHECK_RUNLEVEL_TOK EXCLUDE_RUNLEVELS_TOK

/* GUI messages stuff */
%token SHUTDOWN_TIMEOUT_MESSAGE_TOK RESTART_TIMEOUT_MESSAGE_TOK SLEEP_TIMEOUT_MESSAGE_TOK
%token SLEEP_FORBIDDEN_MESSAGE_TOK SHUTDOWN_FORBIDDEN_MESSAGE_TOK SLEEP_PASSWORD_MESSAGE_TOK
%token SHUTDOWN_PASSWORD_MESSAGE_TOK SHUTDOWN_MESSAGE_TOK RESTART_MESSAGE_TOK
%token LOGIN_MESSAGE_TOK LOGIN_FAILED_MESSAGE_TOK ABORT_MESSAGE_TOK
%token CAPS_MESSAGE_TOK SLEEP_CMD_MESSAGE_TOK CRYPTO_ERROR_MESSAGE_TOK
%token WELCOME_MESSAGE_TOK

/* timeouts */
%token COUNTDOWN_TIMEOUT_TOK INFO_MESSAGE_TIMEOUT_TOK WELCOME_MESSAGE_TIMEOUT_TOK


/* typed tokens: */
%token <ival>  ANUM_T 		/* int */
%token <str>   QUOTSTR_T	/* char* */
%token <color> COLOR_T		/* unsigned char[4] */

%%

/* a configuration */
config: /* nothing */
| config tty_specific
| config last_user
| config last_session
| config lck_sess
| config scrsvr_timeout
| config scrpvr_timeout
| config scrsvrs_dir
| config themes_dir
| config temp_dir
| config pre_gui
| config post_gui
| config x_serv_offset
| config x_serv_tty
| config ssav { TTY_CHECK_COND ssaver_is_set = 1; }
| config dfb_interface
| config reset_console
| config xsessdir
| config txtsessdir
| config xinit
| config x_server
| config x_args
| config idle_timeout
| config idle_action
| config logging
| config sleep
| config theme { TTY_CHECK_COND got_theme=set_theme_result; }
| config shutdown
| config window
| config keybindings
| config runlevel
| config CLEAR_BACKGROUND_TOK '=' YES_TOK { if (!clear_background_is_set) clear_background = 1; }
| config CLEAR_BACKGROUND_TOK '=' NO_TOK  { if (!clear_background_is_set) clear_background = 0; }
;

/* options that will apply to a specific tty only */
tty_specific: TTY_TOK '=' ANUM_T { intended_tty = $3; } '{' config_tty '}' { intended_tty = 0; };

/* tty specific allowed configuration */
config_tty: /* nothing */
| config_tty last_user
| config_tty last_session
| config_tty lck_sess
| config_tty scrsvr_timeout
| config_tty scrpvr_timeout
| config_tty scrsvrs_dir
| config_tty themes_dir
| config_tty temp_dir
| config_tty pre_gui
| config_tty post_gui
| config_tty x_serv_offset
| config_tty x_serv_tty
| config_tty ssav { TTY_CHECK_COND ssaver_is_set = 1; }
| config_tty dfb_interface
| config_tty reset_console
| config_tty xsessdir
| config_tty txtsessdir
| config_tty xinit
| config_tty x_server
| config_tty x_args
| config_tty idle_timeout
| config_tty idle_action
| config_tty logging_tty
| config_tty sleep
| config_tty theme { TTY_CHECK_COND got_theme=set_theme_result; }
| config_tty shutdown
| config_tty window
| config_tty runlevel
| config_tty CLEAR_BACKGROUND_TOK '=' YES_TOK { TTY_CHECK_COND {if (!clear_background_is_set) clear_background = 1;} }
| config_tty CLEAR_BACKGROUND_TOK '=' NO_TOK  { TTY_CHECK_COND {if (!clear_background_is_set) clear_background = 0;} }
| config_tty autologin { TTY_CHECK_COND do_autologin=1;  }
;

runlevel: check_runlevel
|         exclude_runlevels
;

check_runlevel: CHECK_RUNLEVEL_TOK '=' YES_TOK { TTY_CHECK_COND do_runlevel_check = 1; }
|               CHECK_RUNLEVEL_TOK '=' NO_TOK  { TTY_CHECK_COND do_runlevel_check = 0; }
;

exclude_runlevels: EXCLUDE_RUNLEVELS_TOK '=' do_exclude_runlevels;

do_exclude_runlevels: do_exclude_runlevel
|                     do_exclude_runlevel ',' do_exclude_runlevels
;

do_exclude_runlevel: ANUM_T
{
	TTY_CHECK_COND add_to_excluded_runlevels($1);
}

autologin: AUTOLOGIN_TOK '{' config_autologin '}';

config_autologin: username
| config_autologin session
| config_autologin RELOGIN_TOK '=' YES_TOK { TTY_CHECK_COND auto_relogin = 1; }
| config_autologin RELOGIN_TOK '=' NO_TOK  { TTY_CHECK_COND auto_relogin = 0; }
;

username: USERNAME_TOK '=' QUOTSTR_T 
	{
	  TTY_CHECK_COND autologin_username = strdup($3);
	}

session: SESSION_TOK '=' QUOTSTR_T { TTY_CHECK_COND autologin_session = strdup($3); }
|        SESSION_TOK '=' LAST_SESSION_TOK { TTY_CHECK_COND autologin_session = strdup("LAST"); }

dfb_interface: DFB_INTERFACE_TOK '=' QUOTSTR_T
	{
	  if(in_theme) yyerror("Setting 'qingy_DirectFB' is not allowed in theme file.");
	  TTY_CHECK_COND { if (dfb_interface) free(dfb_interface); dfb_interface = strdup($3); }
	}

reset_console: RESET_CONSOLE_TOK '=' QUOTSTR_T
	{
	  if(in_theme) yyerror("Setting 'qingy_reset_console' is not allowed in theme file.");
	  TTY_CHECK_COND { if (reset_console_utility) free(reset_console_utility); reset_console_utility = strdup($3); }
	}

/* logging support */
logging: LOG_LEVEL_TOK      '=' loglevels
|        LOG_FACILITIES_TOK '=' logfacilities
;

logging_tty: LOG_LEVEL_TOK      '=' loglevels
|            LOG_FACILITIES_TOK '=' logfacilities_tty
;

loglevels: DEBUG_TOK { TTY_CHECK_COND max_loglevel = DEBUG; }
|          ERROR_TOK { TTY_CHECK_COND max_loglevel = ERROR; }
;

logfacilities: logfacility
|              logfacility ',' logfacilities
;

logfacilities_tty: logfacility_tty
|                  logfacility_tty ',' logfacilities_tty
;

logfacility: CONSOLE_TOK { TTY_CHECK_COND log_facilities |= LOG_TO_CONSOLE; }
|            FILE_TOK    { TTY_CHECK_COND log_facilities |= LOG_TO_FILE;    }
|            SYSLOG_TOK  { TTY_CHECK_COND log_facilities |= LOG_TO_SYSLOG;  }
;

logfacility_tty: CONSOLE_TOK { TTY_CHECK_COND log_facilities_tty |= LOG_TO_CONSOLE; }
|                FILE_TOK    { TTY_CHECK_COND log_facilities_tty |= LOG_TO_FILE;    }
|                SYSLOG_TOK  { TTY_CHECK_COND log_facilities_tty |= LOG_TO_SYSLOG;  }
;

/* sleep cupport */
sleep: SLEEP_TOK '=' QUOTSTR_T
	{
	  if(in_theme) yyerror("Setting 'sleep' is not allowed in theme file.");
	  TTY_CHECK_COND { if (sleep_cmd) free(sleep_cmd); sleep_cmd = strdup($3); }
	}

/* options to enable or disable session locking */
lck_sess: LOCK_SESSIONS_TOK '=' YES_TOK { TTY_CHECK_COND lock_sessions = 1; }
|         LOCK_SESSIONS_TOK '=' NO_TOK  { TTY_CHECK_COND lock_sessions = 0; }
;

/* How much time should we wait befire firing up the screen saver */
scrsvr_timeout: SCRSVR_TIMEOUT_TOK '=' ANUM_T
	{
		if(in_theme) yyerror("Setting 'screensaver_timeout' is not allowed in theme file");
		TTY_CHECK_COND
		{
			if ($3 < 0)
			{
				writelog(ERROR, "Invalid screen saver timeout: screensaver will be disabled.\n");
				use_screensaver     = 0;
				screensaver_timeout = 0;
			}
			else
			{
				use_screensaver     = 1;
				screensaver_timeout = $3;
				WRITELOG(DEBUG, "You chose a screen saver timeout of %d minute%s", screensaver_timeout, (screensaver_timeout == 1) ? "" : "s");
				if ($3 == 0)
				{
					use_screensaver = 0;
					writelog(DEBUG, ", thus disabling them");
				}
				writelog(DEBUG, ".\n");
			}
		}
	}

/* How much time should we wait befire the screen enters power management mode */
scrpvr_timeout: SCRN_POWER_TOK '=' ANUM_T
	{
		if(in_theme) yyerror("Setting 'screen_powersaving_timeout' is not allowed in theme file");
		TTY_CHECK_COND
		{
			if ($3 < 0)
			{
				writelog(ERROR, "Invalid screen power management timeout: feature will be disabled.\n");
				use_screen_power_management     = 0;
				screen_power_management_timeout = 0;
			}
			else
			{
				use_screen_power_management     = 1;
				screen_power_management_timeout = $3;
				WRITELOG(DEBUG, "You chose a screen power management timeout of %d minute%s", screen_power_management_timeout, (screen_power_management_timeout == 1) ? "" : "s");
				if ($3 == 0)
				{
					use_screen_power_management = 0;
					writelog(DEBUG, ", thus disabling the feature");
				}
				writelog(DEBUG, ".\n");
			}
		}
	}

/* where are located the screen savers? */
scrsvrs_dir: SCRSVRS_DIR_TOK '=' QUOTSTR_T
	{
		if(in_theme) yyerror("Setting 'screensavers_dir' is not allowed in theme file.");
		TTY_CHECK_COND screensavers_dir = strdup($3);
	}

/* where are located the themes? */
themes_dir: THEMES_DIR_TOK '=' QUOTSTR_T
	{
		if(in_theme) yyerror("Setting 'themes_dir' is not allowed in theme file.");
		TTY_CHECK_COND themes_dir = strdup($3);
	}

/* where should we put the temp files? */
temp_dir: TEMP_FILES_DIR_TOK '=' QUOTSTR_T
	{
		if(in_theme) yyerror("Setting 'temp_files_dir' is not allowed in theme file.");
		TTY_CHECK_COND { if (tmp_files_dir) free(tmp_files_dir); tmp_files_dir = strdup($3); };
	}

/* script that is called before the gui is fired up */
pre_gui: PRE_GUI_TOK '=' QUOTSTR_T
	{
		if(in_theme) yyerror("Setting 'pre_gui_script' is not allowed in theme file.");
		TTY_CHECK_COND { if (pre_gui_script) free(pre_gui_script); pre_gui_script = strdup($3); };
	}

/* script that is called after the gui is shut down */
post_gui: POST_GUI_TOK '=' QUOTSTR_T
	{
		if(in_theme) yyerror("Setting 'post_gui_script' is not allowed in theme file.");
		TTY_CHECK_COND { if (post_gui_script) free(post_gui_script); post_gui_script = strdup($3); };
	}

/* Screensaver: "name" or "name" = "option", "option"  */
ssav:	SCREENSAVER_TOK QUOTSTR_T { TTY_CHECK_COND {SSAVER_CHECK_COND screensaver_name = $2;} }
| SCREENSAVER_TOK QUOTSTR_T '=' scrsvr_with_options { TTY_CHECK_COND screensaver_name = $2;}
;

scrsvr_with_options: QUOTSTR_T      { TTY_CHECK_COND {SSAVER_CHECK_COND add_to_options($1);} }
| scrsvr_with_options ',' QUOTSTR_T { TTY_CHECK_COND {SSAVER_CHECK_COND add_to_options($3);} }
;

/* Directory where to look for xsessions. Note that it cannot be in theme file.. */
xsessdir: XSESSION_DIR_TOK '=' QUOTSTR_T 
	{
	  if(in_theme) yyerror("Setting 'x_sessions' is not allowed in theme file.");
	  TTY_CHECK_COND x_sessions_directory = strdup($3);
	};


/* Directory for txtsessions.  Note that it cannot be in theme file..  */
txtsessdir: TXTSESSION_DIR_TOK '=' QUOTSTR_T
	{
	  if(in_theme) yyerror("Setting 'test_sessions' is not allowed in theme file.");
	  TTY_CHECK_COND text_sessions_directory = strdup($3);
	};

/* xinit executable.  Note that it cannot be in theme file..  */
xinit: XINIT_TOK '=' QUOTSTR_T
	{
	  if(in_theme) yyerror("Setting 'xinit' is not allowed in theme file");
	  TTY_CHECK_COND xinit = strdup($3);
	};

/* xinit executable.  Note that it cannot be in theme file..  */
x_server: X_SERVER_TOK '=' QUOTSTR_T
	{
	  if(in_theme) yyerror("Setting 'xinit' is not allowed in theme file");
	  TTY_CHECK_COND x_server = strdup($3);
	};

/* args to be passed to the X server.  Note that it cannot be in theme file..  */
x_args: X_ARGS_TOK '=' QUOTSTR_T
	{
	  if(in_theme) yyerror("Setting 'x_args' is not allowed in theme file");
	  TTY_CHECK_COND x_args = strdup($3);
	};

/* Offset the search for an available X server should start with  */
x_serv_offset: X_SERVER_OFFSET_TOK '=' ANUM_T
	{
	  if(in_theme) yyerror("Setting 'x_server_offset' is not allowed in theme file");
	  TTY_CHECK_COND x_server_offset = $3;
	};

/* Handling of the tty the X server should be started in  */
x_serv_tty: X_SERVER_TTY_TOK  '=' QINGY_TTY_TOK
	{
	  if (in_theme) yyerror("Setting 'x_server_tty' is not allowed in theme file.");
	  TTY_CHECK_COND x_serv_tty_mgmt = QINGY_TTY;
	}
| X_SERVER_TTY_TOK '=' UNUSED_TTY_TOK
	{
	  if (in_theme) yyerror("Setting 'x_server_tty' is not allowed in theme file.");
	  TTY_CHECK_COND x_serv_tty_mgmt = UNUSED_TTY;
	}
;

/* shutdown policies */
shutdown: SHUTDOWN_TOK '=' EVERYONE_TOK
	{
	  if (in_theme) yyerror("Setting 'shutdown_policy' is not allowed in theme file.");
	  TTY_CHECK_COND shutdown_policy = EVERYONE;
	}
| SHUTDOWN_TOK '=' ONLY_ROOT_TOK
	{
	  if (in_theme) yyerror("Setting 'shutdown_policy' is not allowed in theme file.");
	  TTY_CHECK_COND shutdown_policy = ROOT;
	}
| SHUTDOWN_TOK '=' NO_ONE_TOK
	{
	  if (in_theme) yyerror("Setting 'shutdown_policy' is not allowed in theme file.");
	  TTY_CHECK_COND shutdown_policy = NOONE;
	}
;

/* last user policies */
last_user: LAST_USER_POLICY_TOK '=' GLOBAL_TOK
	{
	  if (in_theme) yyerror("Setting 'last_user_policy' is not allowed in theme file.");
	  TTY_CHECK_COND last_user_policy = LU_GLOBAL;
	}
| LAST_USER_POLICY_TOK '=' TTY_TOK
	{
	  if (in_theme) yyerror("Setting 'last_user_policy' is not allowed in theme file.");
	  TTY_CHECK_COND last_user_policy = LU_TTY;
	}
| LAST_USER_POLICY_TOK '=' NONE_TOK
	{
	  if (in_theme) yyerror("Setting 'last_user_policy' is not allowed in theme file.");
	  TTY_CHECK_COND last_user_policy = LU_NONE;
	}
;

/* last session policies */
last_session: LAST_SESSION_POLICY_TOK '=' USER_TOK
	{
	  if (in_theme) yyerror("Setting 'last_session_policy' is not allowed in theme file.");
	  TTY_CHECK_COND last_session_policy = LS_USER;
	}
| LAST_SESSION_POLICY_TOK '=' TTY_TOK
	{
	  if (in_theme) yyerror("Setting 'last_session_policy' is not allowed in theme file.");
	  TTY_CHECK_COND last_session_policy = LS_TTY;
	}
| LAST_SESSION_POLICY_TOK '=' NONE_TOK
	{
	  if (in_theme) yyerror("Setting 'last_session_policy' is not allowed in theme file.");
	  TTY_CHECK_COND last_session_policy = LS_NONE;
	}
;

/* session idle timeout */
idle_timeout: IDLE_TIMEOUT_TOK '=' ANUM_T
{
	if (in_theme) yyerror("Setting 'idle_timeout' is not allowed in theme file.");
	TTY_CHECK_COND idle_timeout = $3;
}
 
/* idle timeout actions */
idle_action: IDLE_ACTION_TOK '=' LOGOUT_TOK
	{
	  if (in_theme) yyerror("Setting 'idle_action' is not allowed in theme file.");
	  TTY_CHECK_COND timeout_action = ST_LOGOUT;
	}
| IDLE_ACTION_TOK '=' LOCK_TOK
	{
	  if (in_theme) yyerror("Setting 'idle_action' is not allowed in theme file.");
	  TTY_CHECK_COND timeout_action = ST_LOCK;
	}
| IDLE_ACTION_TOK '=' NONE_TOK
	{
	  if (in_theme) yyerror("Setting 'idle_action' is not allowed in theme file.");
	  TTY_CHECK_COND timeout_action = ST_NONE;
	}
;

/* theme: either random, ="themeName" or { definition }  */
theme: THEME_TOK '=' RAND_TOK { TTY_CHECK_COND {char *temp = get_random_theme(); set_theme_result=set_theme(temp); free(temp);} }
| THEME_TOK '=' QUOTSTR_T { TTY_CHECK_COND set_theme_result=set_theme($3); }
| THEME_TOK '{' themedefn '}'
;

/* a theme def */
themedefn: /* nothing */
| themedefn strprop
| themedefn anumprop
| themedefn colorprop
| themedefn CLEAR_BACKGROUND_TOK '=' YES_TOK { TTY_CHECK_COND {clear_background = 1; clear_background_is_set = 1;} }
| themedefn CLEAR_BACKGROUND_TOK '=' NO_TOK  { TTY_CHECK_COND {clear_background = 0; clear_background_is_set = 1;} }
| themedefn resolutionprop
| themedefn mousecursorprop
| themedefn guimessageprop
| themedefn guitimers
;

/* GUI timers */
guitimers: COUNTDOWN_TIMEOUT_TOK       '=' ANUM_T { TTY_CHECK_COND countdown_timeout       = $3; }
|          INFO_MESSAGE_TIMEOUT_TOK    '=' ANUM_T { TTY_CHECK_COND info_message_timeout    = $3; }
|          WELCOME_MESSAGE_TIMEOUT_TOK '=' ANUM_T { TTY_CHECK_COND welcome_message_timeout = $3; }
;

/* GUI message properties */
guimessageprop: SHUTDOWN_TIMEOUT_MESSAGE_TOK   '=' QUOTSTR_T { TTY_CHECK_COND shutdown_timeout_message   = strdup($3); }
|               RESTART_TIMEOUT_MESSAGE_TOK    '=' QUOTSTR_T { TTY_CHECK_COND restart_timeout_message    = strdup($3); }
|               SLEEP_TIMEOUT_MESSAGE_TOK      '=' QUOTSTR_T { TTY_CHECK_COND sleep_timeout_message      = strdup($3); }
|               SLEEP_FORBIDDEN_MESSAGE_TOK    '=' QUOTSTR_T { TTY_CHECK_COND sleep_forbidden_message    = strdup($3); }
|               SHUTDOWN_FORBIDDEN_MESSAGE_TOK '=' QUOTSTR_T { TTY_CHECK_COND shutdown_forbidden_message = strdup($3); }
|               SLEEP_PASSWORD_MESSAGE_TOK     '=' QUOTSTR_T { TTY_CHECK_COND sleep_password_message     = strdup($3); }
|               SHUTDOWN_PASSWORD_MESSAGE_TOK  '=' QUOTSTR_T { TTY_CHECK_COND shutdown_password_message  = strdup($3); }
|               SHUTDOWN_MESSAGE_TOK           '=' QUOTSTR_T { TTY_CHECK_COND shutdown_message           = strdup($3); }
|               RESTART_MESSAGE_TOK            '=' QUOTSTR_T { TTY_CHECK_COND restart_message            = strdup($3); }
|               LOGIN_MESSAGE_TOK              '=' QUOTSTR_T { TTY_CHECK_COND login_message              = strdup($3); }
|               LOGIN_FAILED_MESSAGE_TOK       '=' QUOTSTR_T { TTY_CHECK_COND login_failed_message       = strdup($3); }
|               ABORT_MESSAGE_TOK              '=' QUOTSTR_T { TTY_CHECK_COND abort_message              = strdup($3); }
|               CAPS_MESSAGE_TOK               '=' QUOTSTR_T { TTY_CHECK_COND caps_message               = strdup($3); }
|               SLEEP_CMD_MESSAGE_TOK          '=' QUOTSTR_T { TTY_CHECK_COND sleep_cmd_message          = strdup($3); }
|               CRYPTO_ERROR_MESSAGE_TOK       '=' QUOTSTR_T { TTY_CHECK_COND crypto_error_message       = strdup($3); }
|               WELCOME_MESSAGE_TOK            '=' QUOTSTR_T { TTY_CHECK_COND welcome_message            = strdup($3); }
;

/* mouse cursor properties */
mousecursorprop: MOUSE_CURSOR_TOK '=' YES_TOK
  {
    curs.enable = 1;
		curs.path   = NULL;
		curs.x_off  = 0;
		curs.y_off  = 0;
		if (!in_window)
			curs.window_id = -1;
		else
			curs.window_id = wind.id;
		add_cursor_to_list(&curs);
  }
| MOUSE_CURSOR_TOK '=' NO_TOK
  {
    curs.enable = 0;
		curs.path   = NULL;
		curs.x_off  = 0;
		curs.y_off  = 0;
		if (!in_window)
			curs.window_id = -1;
		else
			curs.window_id = wind.id;
		add_cursor_to_list(&curs);
  }
| MOUSE_CURSOR_TOK '=' '{' cursordefn '}'
;

/* mouse cursor definition */
cursordefn: QUOTSTR_T ',' ANUM_T ',' ANUM_T
	{
		curs.enable    = 1;
		curs.path      = $1;
		curs.x_off     = $3;
		curs.y_off     = $5;
		if (!in_window)
			curs.window_id = -1;
		else
			curs.window_id = wind.id;
		add_cursor_to_list(&curs);
	}
;

/* color assignments */
colorprop: DEFAULT_TXT_COL_TOK '=' COLOR_T
	{ 
		TTY_CHECK_COND
		{
			default_text_color.R=$3[3]; default_text_color.G=$3[2]; 
			default_text_color.B=$3[1]; default_text_color.A=$3[0];
		}
	}
| DEFAULT_TXT_COL_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T
	{
		TTY_CHECK_COND
		{
			default_text_color.R = $3; default_text_color.G= $5;
			default_text_color.B = $7; default_text_color.A= $9;
		}
	}
|  DEFAULT_CUR_COL_TOK '=' COLOR_T
	{ 
		TTY_CHECK_COND
		{
			default_cursor_color.R=$3[3]; default_cursor_color.G=$3[2];
			default_cursor_color.B=$3[1]; default_cursor_color.A=$3[0];
		}
	}
|  DEFAULT_CUR_COL_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T
	{ 
		TTY_CHECK_COND
		{
			default_cursor_color.R = $3; default_cursor_color.G= $5; 
			default_cursor_color.B = $7; default_cursor_color.A= $9; 
		}
	}
| OTHER_TXT_COL_TOK '=' COLOR_T
	{ 
		TTY_CHECK_COND
		{
			other_text_color.R=$3[3]; other_text_color.G=$3[2]; 
			other_text_color.B=$3[1]; other_text_color.A=$3[0];
		}
	}
| OTHER_TXT_COL_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T
	{ 
		TTY_CHECK_COND
		{
			other_text_color.R = $3; other_text_color.G= $5; 
			other_text_color.B = $7; other_text_color.A= $9;
		}
	}
;

/* native theme resolution */
resolutionprop: THEME_RES_TOK '=' ANUM_T 'x' ANUM_T
	{
		TTY_CHECK_COND
		{
			theme_xres = $3; theme_yres = $5;
		}
	}
| THEME_RES_TOK '=' ANUM_T 'X' ANUM_T
	{
		TTY_CHECK_COND
		{
			theme_xres = $3; theme_yres = $5;
		}
	}
;

/* string properties */
strprop: BG_TOK '=' QUOTSTR_T { TTY_CHECK_COND background = StrApp((char**)NULL, theme_dir, $3, (char*)NULL); }
| FONT_TOK      '=' QUOTSTR_T { TTY_CHECK_COND font       = StrApp((char**)NULL, theme_dir, $3, (char*)NULL); }
;

/* numbers in themes */
anumprop: BUTTON_OPAC_TOK '=' ANUM_T { TTY_CHECK_COND button_opacity          = $3; }
| WIN_OP_TOK              '=' ANUM_T { TTY_CHECK_COND window_opacity          = $3; }
| SEL_WIN_OP_TOK          '=' ANUM_T { TTY_CHECK_COND selected_window_opacity = $3; }
;

/* key bindings */
keybindings: KEYBINDINGS_TOK '{' keybindingsdefns '}'
	{
	  if(in_theme) yyerror("Setting 'key_bindings' is not allowed in theme file.");
	}
;

keybindingsdefns: /* nothing */
| keybindingsdefns actiondefns '=' QUOTSTR_T
	{
		if (!add_to_keybindings(action, $4))
			yyerror("An error occurred while parsing your keybingings configuration.");
	}
;

actiondefns: SLEEP_TOK { action = DO_SLEEP;        }
| NEXT_TTY_TOK         { action = DO_NEXT_TTY;     }
| PREV_TTY_TOK         { action = DO_PREV_TTY;     }
| POWEROFF_TOK         { action = DO_POWEROFF;     }
| REBOOT_TOK           { action = DO_REBOOT;       }
| SCREENSAVER_TOK      { action = DO_SCREEN_SAVER; }
| KILL_TOK             { action = DO_KILL;         }
| TEXT_MODE_TOK        { action = DO_TEXT_MODE;    }
;

/* a window in the theme */
window: WINDOW_TOK '{' windefns '}'
	{
		TTY_CHECK_COND
		{
			if (got_theme)
			{
				destroy_windows_list(windowsList); 
				windowsList = NULL;
				got_theme   = 0;
			}
			add_window_to_list(&wind);
			in_window=0;
		}
	}
; 

windefns: windefn | windefns windefn { in_window=1; };

windefn: 'x'        '=' ANUM_T    { TTY_CHECK_COND wind.x=$3;                        }
| 'y'               '=' ANUM_T    { TTY_CHECK_COND wind.y=$3;                        }
| WTYPE_TOK         '=' QUOTSTR_T { TTY_CHECK_COND wind.type     = get_win_type($3); }
| WWIDTH_TOK        '=' ANUM_T    { TTY_CHECK_COND wind.width    = $3;               }
| WHEIGHT_TOK       '=' ANUM_T    { TTY_CHECK_COND wind.height   = $3;               }
| WCOMMAND_TOK      '=' buttoncommand
| WCONTENT_TOK      '=' QUOTSTR_T { TTY_CHECK_COND wind.content  = strdup($3);       }
| WINDOW_LINK_TOK   '=' QUOTSTR_T { TTY_CHECK_COND wind.linkto   = strdup($3);       }
| WPOLL_TIME_TOK    '=' ANUM_T    { TTY_CHECK_COND wind.polltime = $3;               }
| WTEXT_ORIENTATION '=' textorientation
| WTEXT_SIZE_TOK    '=' wintextsize
| wincolorprop
| mousecursorprop
;

buttoncommand: NULL_TOK { TTY_CHECK_COND wind.command = NULL;       }
| QUOTSTR_T             { TTY_CHECK_COND wind.command = strdup($1); }
;

textorientation: WTEXT_LEFT_TOK { TTY_CHECK_COND wind.text_orientation = LEFT;   }
| WTEXT_CENTER_TOK              { TTY_CHECK_COND wind.text_orientation = CENTER; }
| WTEXT_RIGHT_TOK               { TTY_CHECK_COND wind.text_orientation = RIGHT;  }
;

wintextsize: WTEXT_TINY_TOK { TTY_CHECK_COND wind.text_size = TINY;    }
| WTEXT_SMALLER_TOK         { TTY_CHECK_COND wind.text_size = SMALLER; }
| WTEXT_SMALL_TOK           { TTY_CHECK_COND wind.text_size = SMALL;   }
| WTEXT_MEDIUM_TOK          { TTY_CHECK_COND wind.text_size = MEDIUM;  }
| WTEXT_LARGE_TOK           { TTY_CHECK_COND wind.text_size = LARGE;   }
;

/* local-to-window color properties */
wincolorprop: WTEXT_COLOR_TOK '=' COLOR_T
	{ 
		TTY_CHECK_COND
		{
			wind.text_color = (color_t *) calloc(1, sizeof(color_t));
			wind.text_color->R=$3[3]; wind.text_color->G=$3[2]; 
			wind.text_color->B=$3[1]; wind.text_color->A=$3[0];
		}
	}
| WTEXT_COLOR_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T
	{ 
		TTY_CHECK_COND
		{
			wind.text_color = (color_t *) calloc(1, sizeof(color_t));
			wind.text_color->R = $3; wind.text_color->G = $5;
			wind.text_color->B = $7; wind.text_color->A = $9;
		}
	}
| WCURSOR_COLOR_TOK '=' COLOR_T
	{	
		TTY_CHECK_COND
		{
			wind.cursor_color = (color_t *) calloc(1, sizeof(color_t));
			wind.cursor_color->R=$3[3]; wind.cursor_color->G=$3[2]; 
			wind.cursor_color->B=$3[1]; wind.cursor_color->A=$3[0];
		}
	}
| WCURSOR_COLOR_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T
	{
		TTY_CHECK_COND
		{
			wind.cursor_color = (color_t *) calloc(1, sizeof(color_t));
			wind.cursor_color->R = $3; wind.cursor_color->G = $5;
			wind.cursor_color->B = $7; wind.cursor_color->A = $9;
		}
	}
;

%%
