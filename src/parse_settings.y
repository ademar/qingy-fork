%{
/*******************************************************************************/
/* parse_settings.y - Qingy theme and settings files parser 		       */
/*  v. 0.3 								       */
/*  Copyright (C) 2004 Paolo Gianrossi - All rights reserved 		       */
/*  by Paolo Gianrossi <paolino.gnu@disi.unige.it>			       */
/* 									       */
/* This program is free software; you can redistribute it and/or 	       */
/* modify  under the terms of the GNU General Public License as 	       */
/* published by the Free Software Foundation; either version 2, or (at 	       */
/* your option) any later version.					       */
/* 									       */
/* This program is distributed in the hope that it will be useful, but 	       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of 		       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 	       */
/* General Public License for more details.				       */
/* 									       */
/* You should have received a copy of the GNU General Public License 	       */
/* along with GNU Emacs; see the file COPYING.  If not, write to the 	       */
/* Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.	       */
/*******************************************************************************/


#include <stdlib.h>
#include <stdio.h>

#include "load_settings.h"
#include "misc.h"
  
#define YYERROR_VERBOSE
  
extern FILE* yyin;
extern int yylex();
extern int in_theme;

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
%token SCREENSAVER_TOK XSESSION_DIR_TOK TXTSESSION_DIR_TOK XINIT_TOK SHUTDOWN_TOK

/* windows && theme blocks */
%token THEME_TOK  WINDOW_TOK 

/* screensaver rvals */
%token PIXEL_TOK PHOTOS_TOK

/* theme  */
%token RAND_TOK
/* theme colors */
%token  DEFAULT_TXT_COL_TOK DEFAULT_CUR_COL_TOK OTHER_TXT_COL_TOK 		     

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

/* typed tokens: */
%token <ival>  ANUM_T 		/* int */
%token <str>   QUOTSTR_T	/* char* */
%token <color> COLOR_T		/* unsigned char[4] */

%%

/* a configuration */
config: /* nothing */
| config ssav  
| config xsessdir
| config txtsessdir
| config xinit
| config theme
| config shutdown
| config window
;

/* Screensaver: "name" or "name" ="path","path"  */
ssav:	SCREENSAVER_TOK QUOTSTR_T { SCREENSAVER = $2;}
| SCREENSAVER_TOK QUOTSTR_T '=' photos  { SCREENSAVER = $2;}
;

photos: QUOTSTR_T	{ add_to_options($1); }
| photos ',' QUOTSTR_T { add_to_options($3); }
;

/* Directory where to look for xsessions. Note that it cannot be in theme file.. */
xsessdir: XSESSION_DIR_TOK '=' QUOTSTR_T 
	{
	  if(in_theme) yyerror("Setting 'x_sessions' is not allowed in theme file.");
	  X_SESSIONS_DIRECTORY = strdup($3);
	};


/* Directory for txtsessions.  Note that it cannot be in theme file..  */
txtsessdir: TXTSESSION_DIR_TOK '=' QUOTSTR_T
	{
	  if(in_theme) yyerror("Setting 'test_sessions' is not allowed in theme file.");
	  TEXT_SESSIONS_DIRECTORY = strdup($3);
	};

/* xinit executable.  Note that it cannot be in theme file..  */
xinit: XINIT_TOK '=' QUOTSTR_T
	{
	  if(in_theme) yyerror("Setting 'xinit' is not allowed in theme file");
	  XINIT = strdup($3);
	};

/* shutdown policies */
shutdown: SHUTDOWN_TOK '=' EVERYONE_TOK
	{
	  if (in_theme) yyerror("Setting 'shutdown_policy' is not allowed in theme file.");
	  SHUTDOWN_POLICY = EVERYONE;
	}
| SHUTDOWN_TOK '=' ONLY_ROOT_TOK
	{
	  fprintf(stderr,"c)INTHEME: %d\n", in_theme);
	  if (in_theme) yyerror("Setting 'shutdown_policy' is not allowed in theme file.");
	  SHUTDOWN_POLICY = ROOT;
	}
| SHUTDOWN_TOK '=' NO_ONE_TOK
	{
	  if (in_theme) yyerror("Setting 'shutdown_policy' is not allowed in theme file.");
	  SHUTDOWN_POLICY = NOONE;
	}
;

 
/* theme: either random, ="themeName" or { definition }  */
theme: THEME_TOK '=' RAND_TOK  { char *temp = get_random_theme(); set_theme(temp); free(temp); }
| THEME_TOK '=' QUOTSTR_T { set_theme($3); }
| THEME_TOK '{' themedefn '}' 
;

/* a theme def */
themedefn: /* nothing */
| themedefn strprop
| themedefn anumprop
| themedefn colorprop
;

/* color assignments */
colorprop: DEFAULT_TXT_COL_TOK '=' COLOR_T
	{
	  fprintf(stderr, "Setting default color to %d\n", *($3));
	  DEFAULT_TEXT_COLOR.R=$3[3]; DEFAULT_TEXT_COLOR.G=$3[2]; 
	  DEFAULT_TEXT_COLOR.B=$3[1]; DEFAULT_TEXT_COLOR.A=$3[0];
	}
| DEFAULT_TXT_COL_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T
	{
	  DEFAULT_TEXT_COLOR.R = $3; DEFAULT_TEXT_COLOR.G= $5;
	  DEFAULT_TEXT_COLOR.B = $7; DEFAULT_TEXT_COLOR.A= $9;
	}
|  DEFAULT_CUR_COL_TOK '=' COLOR_T
	{
	  DEFAULT_CURSOR_COLOR.R=$3[3]; DEFAULT_CURSOR_COLOR.G=$3[2];
	  DEFAULT_CURSOR_COLOR.B=$3[1]; DEFAULT_CURSOR_COLOR.A=$3[0];
	}
|  DEFAULT_CUR_COL_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T
	{
	  DEFAULT_CURSOR_COLOR.R = $3; DEFAULT_CURSOR_COLOR.G= $5; 
	  DEFAULT_CURSOR_COLOR.B = $7; DEFAULT_CURSOR_COLOR.A= $9; 
	}
| OTHER_TXT_COL_TOK '=' COLOR_T
	{
	  OTHER_TEXT_COLOR.R=$3[3]; OTHER_TEXT_COLOR.G=$3[2]; 
	  OTHER_TEXT_COLOR.B=$3[1]; OTHER_TEXT_COLOR.A=$3[0];
	}
| OTHER_TXT_COL_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T
	{
	  OTHER_TEXT_COLOR.R = $3; OTHER_TEXT_COLOR.G= $5; 
	  OTHER_TEXT_COLOR.B = $7; OTHER_TEXT_COLOR.A= $9;
	}
;

/* string properties */
strprop: BG_TOK   '=' QUOTSTR_T { BACKGROUND = StrApp((char**)NULL, THEME_DIR, $3, (char*)NULL); }
| FONT_TOK '=' QUOTSTR_T 	{ FONT       = StrApp((char**)NULL, THEME_DIR, $3, (char*)NULL); }
;

/* numbers in themes */
anumprop: BUTTON_OPAC_TOK '=' ANUM_T 	{ BUTTON_OPACITY          = $3; }
| WIN_OP_TOK      '=' ANUM_T 		{ WINDOW_OPACITY          = $3; }
| SEL_WIN_OP_TOK  '=' ANUM_T 		{ SELECTED_WINDOW_OPACITY = $3; }
;

/* a window in the theme */
window: WINDOW_TOK '{' windefns '}' { add_window_to_list(&wind); }; 

windefns: windefn | windefns windefn;

windefn: 'x'               '=' ANUM_T  	{ wind.x=$3;         		    }
| 'y'               '=' ANUM_T    	{ wind.y=$3;                        }
| WTYPE_TOK         '=' QUOTSTR_T 	{ wind.type     = get_win_type($3); }
| WWIDTH_TOK        '=' ANUM_T    	{ wind.width    = $3;               }
| WHEIGHT_TOK       '=' ANUM_T    	{ wind.height   = $3;               }
| WCOMMAND_TOK      '=' QUOTSTR_T 	{ wind.command  = strdup($3);       }
| WCONTENT_TOK      '=' QUOTSTR_T 	{ wind.content  = strdup($3);       }
| WINDOW_LINK_TOK   '=' QUOTSTR_T 	{ wind.linkto   = strdup($3);       }
| WPOLL_TIME_TOK    '=' ANUM_T    	{ wind.polltime = $3;               }
| WTEXT_ORIENTATION '=' textorientation
| WTEXT_SIZE_TOK    '=' wintextsize
| wincolorprop
;

textorientation: WTEXT_LEFT_TOK   	{ wind.text_orientation = LEFT;   }
| WTEXT_CENTER_TOK 			{ wind.text_orientation = CENTER; }
| WTEXT_RIGHT_TOK  			{ wind.text_orientation = RIGHT;  }
;

wintextsize: WTEXT_SMALL_TOK  	{ wind.text_size = SMALL;  }
| WTEXT_MEDIUM_TOK 		{ wind.text_size = MEDIUM; }
| WTEXT_LARGE_TOK 		{ wind.text_size = LARGE;  }
;

/* local-to-window color properties */
wincolorprop: WTEXT_COLOR_TOK '=' COLOR_T
	{
	  wind.text_color = (color_t *) calloc(1, sizeof(color_t));
	  wind.text_color->R=$3[3]; wind.text_color->G=$3[2]; 
	  wind.text_color->B=$3[1]; wind.text_color->A=$3[0];
	}
| WTEXT_COLOR_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T
	{
	  wind.text_color = (color_t *) calloc(1, sizeof(color_t));
	  wind.text_color->R = $3; wind.text_color->G = $5;
	  wind.text_color->B = $7; wind.text_color->A = $9;
	}
| WCURSOR_COLOR_TOK '=' COLOR_T
	{
	  wind.cursor_color = (color_t *) calloc(1, sizeof(color_t));
	  wind.cursor_color->R=$3[3]; wind.cursor_color->G=$3[2]; 
	  wind.cursor_color->B=$3[1]; wind.cursor_color->A=$3[0];
	}
| WCURSOR_COLOR_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T
	{
	  wind.cursor_color = (color_t *) calloc(1, sizeof(color_t));
	  wind.cursor_color->R = $3; wind.cursor_color->G = $5;
	  wind.cursor_color->B = $7; wind.cursor_color->A = $9;
	}
;

%%

