%{
/*******************************************************************************/
/* parse_settings.y - Qingy theme and settings files parser 		               */
/*  v. 0.3 								                                                     */
/*  Copyright (C) 2004 Paolo Gianrossi - All rights reserved 		               */
/*  by Paolo Gianrossi <paolino.gnu@disi.unige.it>			                       */
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


#include <stdlib.h>
#include <stdio.h>

#include "load_settings.h"
#include "misc.h"
  
#define YYERROR_VERBOSE
#define TTY_CHECK_COND    if (!intended_tty || intended_tty == current_tty)
#define SSAVER_CHECK_COND if (ssaver_is_set) { erase_options(); ssaver_is_set = 0; }
  
extern FILE* yyin;
extern int yylex();
extern int in_theme;
extern int current_tty;
static int clear_background_is_set = 0;
static int intended_tty            = 0;
static int theme_is_set            = 0;
static int ssaver_is_set           = 0;

static window_t wind =
  {
    0,
    0,
    0,
    0,
    0,
    LARGE,
    LEFT,
    &DEFAULT_TEXT_COLOR,
    &DEFAULT_CURSOR_COLOR,
    UNKNOWN,
    NULL,
    NULL,
    NULL,
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
%token SCREENSAVER_TOK XSESSION_DIR_TOK TXTSESSION_DIR_TOK XINIT_TOK SHUTDOWN_TOK TTY_TOK

/* windows && theme blocks */
%token THEME_TOK WINDOW_TOK 

/* theme */
%token RAND_TOK

/* other lvals */
%token CLEAR_BACKGROUND_TOK LOCK_SESSIONS_TOK

/* theme colors */
%token DEFAULT_TXT_COL_TOK DEFAULT_CUR_COL_TOK OTHER_TXT_COL_TOK 		     

/* other theme lvals */
%token BG_TOK FONT_TOK BUTTON_OPAC_TOK WIN_OP_TOK SEL_WIN_OP_TOK 			     

/* shutdown policies */
%token EVERYONE_TOK ONLY_ROOT_TOK NO_ONE_TOK

/* window specific lvals */
%token WPOLL_TIME_TOK WTEXT_COLOR_TOK WCURSOR_COLOR_TOK WINDOW_LINK_TOK
%token WTYPE_TOK WWIDTH_TOK WHEIGHT_TOK WCOMMAND_TOK WCONTENT_TOK

/* window text size */
%token WTEXT_SIZE_TOK WTEXT_SMALL_TOK WTEXT_MEDIUM_TOK WTEXT_LARGE_TOK

/* window text orient */
%token WTEXT_ORIENTATION WTEXT_LEFT_TOK WTEXT_CENTER_TOK WTEXT_RIGHT_TOK

/* answer tokens */
%token YES_TOK NO_TOK

/* typed tokens: */
%token <ival>  ANUM_T 		/* int */
%token <str>   QUOTSTR_T	/* char* */
%token <color> COLOR_T		/* unsigned char[4] */

%%

/* a configuration */
config: /* nothing */
| config tty_specific
| config lck_sess
| config ssav { TTY_CHECK_COND ssaver_is_set = 1; }
| config xsessdir
| config txtsessdir
| config xinit
| config theme { TTY_CHECK_COND theme_is_set = 1; }
| config shutdown
| config window
| config CLEAR_BACKGROUND_TOK '=' YES_TOK { if (!clear_background_is_set) clear_background = 1; }
| config CLEAR_BACKGROUND_TOK '=' NO_TOK  { if (!clear_background_is_set) clear_background = 0; }
;

/* options that will apply to a specific tty only */
tty_specific: TTY_TOK '=' ANUM_T { intended_tty = $3; } '{' config_tty '}' { intended_tty = 0; };

/* tty specific allowed configuration */
config_tty: /* nothing */
| config_tty lck_sess
| config_tty ssav { TTY_CHECK_COND ssaver_is_set = 1; }
| config_tty xsessdir
| config_tty txtsessdir
| config_tty xinit
| config_tty theme { TTY_CHECK_COND theme_is_set = 1; }
| config_tty shutdown
| config_tty window
| config_tty CLEAR_BACKGROUND_TOK '=' YES_TOK { TTY_CHECK_COND {if (!clear_background_is_set) clear_background = 1;} }
| config_tty CLEAR_BACKGROUND_TOK '=' NO_TOK  { TTY_CHECK_COND {if (!clear_background_is_set) clear_background = 0;} }
;

/* options to enable or disable session locking */
lck_sess: LOCK_SESSIONS_TOK '=' YES_TOK { TTY_CHECK_COND {lock_sessions = 1;} }
|         LOCK_SESSIONS_TOK '=' NO_TOK  { TTY_CHECK_COND {lock_sessions = 0;} }
;

/* Screensaver: "name" or "name" = "option", "option"  */
ssav:	SCREENSAVER_TOK QUOTSTR_T { TTY_CHECK_COND {SSAVER_CHECK_COND SCREENSAVER = $2;} }
| SCREENSAVER_TOK QUOTSTR_T '=' scrsvr_with_options { TTY_CHECK_COND SCREENSAVER = $2;}
;

scrsvr_with_options: QUOTSTR_T      { TTY_CHECK_COND {SSAVER_CHECK_COND add_to_options($1);} }
| scrsvr_with_options ',' QUOTSTR_T { TTY_CHECK_COND {SSAVER_CHECK_COND add_to_options($3);} }
;

/* Directory where to look for xsessions. Note that it cannot be in theme file.. */
xsessdir: XSESSION_DIR_TOK '=' QUOTSTR_T 
	{
	  if(in_theme) yyerror("Setting 'x_sessions' is not allowed in theme file.");
	  TTY_CHECK_COND X_SESSIONS_DIRECTORY = strdup($3);
	};


/* Directory for txtsessions.  Note that it cannot be in theme file..  */
txtsessdir: TXTSESSION_DIR_TOK '=' QUOTSTR_T
	{
	  if(in_theme) yyerror("Setting 'test_sessions' is not allowed in theme file.");
	  TTY_CHECK_COND TEXT_SESSIONS_DIRECTORY = strdup($3);
	};

/* xinit executable.  Note that it cannot be in theme file..  */
xinit: XINIT_TOK '=' QUOTSTR_T
	{
	  if(in_theme) yyerror("Setting 'xinit' is not allowed in theme file");
	  TTY_CHECK_COND XINIT = strdup($3);
	};

/* shutdown policies */
shutdown: SHUTDOWN_TOK '=' EVERYONE_TOK
	{
	  if (in_theme) yyerror("Setting 'shutdown_policy' is not allowed in theme file.");
	  TTY_CHECK_COND SHUTDOWN_POLICY = EVERYONE;
	}
| SHUTDOWN_TOK '=' ONLY_ROOT_TOK
	{
	  fprintf(stderr,"c)INTHEME: %d\n", in_theme);
	  if (in_theme) yyerror("Setting 'shutdown_policy' is not allowed in theme file.");
	  TTY_CHECK_COND SHUTDOWN_POLICY = ROOT;
	}
| SHUTDOWN_TOK '=' NO_ONE_TOK
	{
	  if (in_theme) yyerror("Setting 'shutdown_policy' is not allowed in theme file.");
	  TTY_CHECK_COND SHUTDOWN_POLICY = NOONE;
	}
;

 
/* theme: either random, ="themeName" or { definition }  */
theme: THEME_TOK '=' RAND_TOK { TTY_CHECK_COND {char *temp = get_random_theme(); set_theme(temp); free(temp);} }
| THEME_TOK '=' QUOTSTR_T { TTY_CHECK_COND set_theme($3); }
| THEME_TOK '{' themedefn '}'
;

/* a theme def */
themedefn: /* nothing */
| themedefn strprop
| themedefn anumprop
| themedefn colorprop
| themedefn CLEAR_BACKGROUND_TOK '=' YES_TOK { TTY_CHECK_COND {clear_background = 1; clear_background_is_set = 1;} }
| themedefn CLEAR_BACKGROUND_TOK '=' NO_TOK  { TTY_CHECK_COND {clear_background = 0; clear_background_is_set = 1;} }
;

/* color assignments */
colorprop: DEFAULT_TXT_COL_TOK '=' COLOR_T
	{ 
		TTY_CHECK_COND
		{
			if (!silent) fprintf(stderr, "Setting default color to %d\n", *($3));	  
			DEFAULT_TEXT_COLOR.R=$3[3]; DEFAULT_TEXT_COLOR.G=$3[2]; 
			DEFAULT_TEXT_COLOR.B=$3[1]; DEFAULT_TEXT_COLOR.A=$3[0];
		}
	}
| DEFAULT_TXT_COL_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T
	{
		TTY_CHECK_COND
		{
			DEFAULT_TEXT_COLOR.R = $3; DEFAULT_TEXT_COLOR.G= $5;
			DEFAULT_TEXT_COLOR.B = $7; DEFAULT_TEXT_COLOR.A= $9;
		}
	}
|  DEFAULT_CUR_COL_TOK '=' COLOR_T
	{ 
		TTY_CHECK_COND
		{
			DEFAULT_CURSOR_COLOR.R=$3[3]; DEFAULT_CURSOR_COLOR.G=$3[2];
			DEFAULT_CURSOR_COLOR.B=$3[1]; DEFAULT_CURSOR_COLOR.A=$3[0];
		}
	}
|  DEFAULT_CUR_COL_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T
	{ 
		TTY_CHECK_COND
		{
			DEFAULT_CURSOR_COLOR.R = $3; DEFAULT_CURSOR_COLOR.G= $5; 
			DEFAULT_CURSOR_COLOR.B = $7; DEFAULT_CURSOR_COLOR.A= $9; 
		}
	}
| OTHER_TXT_COL_TOK '=' COLOR_T
	{ 
		TTY_CHECK_COND
		{
			OTHER_TEXT_COLOR.R=$3[3]; OTHER_TEXT_COLOR.G=$3[2]; 
			OTHER_TEXT_COLOR.B=$3[1]; OTHER_TEXT_COLOR.A=$3[0];
		}
	}
| OTHER_TXT_COL_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T
	{ 
		TTY_CHECK_COND
		{
			OTHER_TEXT_COLOR.R = $3; OTHER_TEXT_COLOR.G= $5; 
			OTHER_TEXT_COLOR.B = $7; OTHER_TEXT_COLOR.A= $9;
		}
	}
;

/* string properties */
strprop: BG_TOK '=' QUOTSTR_T { TTY_CHECK_COND BACKGROUND = StrApp((char**)NULL, THEME_DIR, $3, (char*)NULL); }
| FONT_TOK      '=' QUOTSTR_T { TTY_CHECK_COND FONT       = StrApp((char**)NULL, THEME_DIR, $3, (char*)NULL); }
;

/* numbers in themes */
anumprop: BUTTON_OPAC_TOK '=' ANUM_T { TTY_CHECK_COND BUTTON_OPACITY          = $3; }
| WIN_OP_TOK              '=' ANUM_T { TTY_CHECK_COND WINDOW_OPACITY          = $3; }
| SEL_WIN_OP_TOK          '=' ANUM_T { TTY_CHECK_COND SELECTED_WINDOW_OPACITY = $3; }
;

/* a window in the theme */
window: WINDOW_TOK '{' windefns '}'
	{
		TTY_CHECK_COND
		{
			if (theme_is_set)
			{
				destroy_windows_list(windowsList); 
				windowsList  = NULL;
				theme_is_set = 0;
			}
			add_window_to_list(&wind);
		}
	}
; 

windefns: windefn | windefns windefn;

windefn: 'x'        '=' ANUM_T    { TTY_CHECK_COND wind.x=$3;                        }
| 'y'               '=' ANUM_T    { TTY_CHECK_COND wind.y=$3;                        }
| WTYPE_TOK         '=' QUOTSTR_T { TTY_CHECK_COND wind.type     = get_win_type($3); }
| WWIDTH_TOK        '=' ANUM_T    { TTY_CHECK_COND wind.width    = $3;               }
| WHEIGHT_TOK       '=' ANUM_T    { TTY_CHECK_COND wind.height   = $3;               }
| WCOMMAND_TOK      '=' QUOTSTR_T { TTY_CHECK_COND wind.command  = strdup($3);       }
| WCONTENT_TOK      '=' QUOTSTR_T { TTY_CHECK_COND wind.content  = strdup($3);       }
| WINDOW_LINK_TOK   '=' QUOTSTR_T { TTY_CHECK_COND wind.linkto   = strdup($3);       }
| WPOLL_TIME_TOK    '=' ANUM_T    { TTY_CHECK_COND wind.polltime = $3;               }
| WTEXT_ORIENTATION '=' textorientation
| WTEXT_SIZE_TOK    '=' wintextsize
| wincolorprop
;

textorientation: WTEXT_LEFT_TOK { TTY_CHECK_COND wind.text_orientation = LEFT;   }
| WTEXT_CENTER_TOK              { TTY_CHECK_COND wind.text_orientation = CENTER; }
| WTEXT_RIGHT_TOK               { TTY_CHECK_COND wind.text_orientation = RIGHT;  }
;

wintextsize: WTEXT_SMALL_TOK { TTY_CHECK_COND wind.text_size = SMALL;  }
| WTEXT_MEDIUM_TOK           { TTY_CHECK_COND wind.text_size = MEDIUM; }
| WTEXT_LARGE_TOK            { TTY_CHECK_COND wind.text_size = LARGE;  }
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

