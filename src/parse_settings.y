%{

#include <stdlib.h>
#include <stdio.h>

#include "load_settings.h"
#include "misc.h"

#define YYERROR_VERBOSE

extern FILE* yyin;
extern int yylex();
extern int in_theme;

%}

%union
{
  int ival;
  char* str;
  unsigned char color[4];
}

%token SCREENSAVER_TOK PIXEL_TOK PHOTOS_TOK XSESSION_DIR_TOK TXTSESSION_DIR_TOK XINIT_TOK    
%token THEME_TOK RAND_TOK MASK_TXT_COL_TOK TXT_CUR_COL_TOK OTHER_TXT_COL_TOK 		     
%token BG_TOK FONT_TOK BUTTON_OPAC_TOK WIN_OP_TOK SEL_WIN_OP_TOK 			     
      											     
%token <ival>  ANUM_T 									     
%token <str>   QUOTSTR_T
%token <color> COLOR_T                                                                       

%%

config: 
	| config ssav 
	| config xsessdir
	| config txtsessdir
	| config xinit
	| config theme
;

ssav:	SCREENSAVER_TOK PIXEL_TOK { SCREENSAVER=PIXEL_SCREENSAVER; }
	| SCREENSAVER_TOK PHOTOS_TOK '=' photos { SCREENSAVER=PHOTO_SCREENSAVER; } ;

photos: QUOTSTR_T	{add_to_paths($1); }		/* nothing */
	| photos ',' QUOTSTR_T { add_to_paths($3); };

xsessdir: XSESSION_DIR_TOK '=' QUOTSTR_T { if(in_theme){ yyerror("Not allowed in theme file");}  X_SESSIONS_DIRECTORY=strdup($3); };

txtsessdir: TXTSESSION_DIR_TOK '=' QUOTSTR_T { if(in_theme){ yyerror("Not allowed in theme file");} TEXT_SESSIONS_DIRECTORY=strdup($3); };

xinit:	XINIT_TOK '=' QUOTSTR_T { if(in_theme){ yyerror("Not allowed in theme file");} XINIT=strdup($3); };
 
theme: 	THEME_TOK RAND_TOK { set_theme(get_random_theme());} 
	| THEME_TOK '=' QUOTSTR_T  {set_theme($3);}
	| THEME_TOK '{' themedefn '}' 
	;

themedefn: 			/* nothing */
	| themedefn strprop
	| themedefn anumprop
	| themedefn colorprop
	;

colorprop: MASK_TXT_COL_TOK '=' COLOR_T { MASK_TEXT_COLOR_R=$3[3]; MASK_TEXT_COLOR_G=$3[2]; 
					  MASK_TEXT_COLOR_B=$3[1]; MASK_TEXT_COLOR_A=$3[0];}
	  | MASK_TXT_COL_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T {
                                              MASK_TEXT_COLOR_R = $3; MASK_TEXT_COLOR_G= $5; 
					      MASK_TEXT_COLOR_B = $7; MASK_TEXT_COLOR_A= $9; 
	   }
 
	   | TXT_CUR_COL_TOK '=' COLOR_T  { TEXT_CURSOR_COLOR_R=$3[3]; TEXT_CURSOR_COLOR_G=$3[2]; 
   					    TEXT_CURSOR_COLOR_B=$3[1]; TEXT_CURSOR_COLOR_A=$3[0];}
	   | TXT_CUR_COL_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T {
	     				      TEXT_CURSOR_COLOR_R = $3; TEXT_CURSOR_COLOR_G= $5; 
					      TEXT_CURSOR_COLOR_B = $7; TEXT_CURSOR_COLOR_A= $9; 
	   }

	   | OTHER_TXT_COL_TOK '=' COLOR_T { OTHER_TEXT_COLOR_R=$3[3]; OTHER_TEXT_COLOR_G=$3[2]; 
    					     OTHER_TEXT_COLOR_B=$3[1]; OTHER_TEXT_COLOR_A=$3[0]; }
	   | OTHER_TXT_COL_TOK '=' ANUM_T ',' ANUM_T ',' ANUM_T ',' ANUM_T {
	     				      OTHER_TEXT_COLOR_R = $3; OTHER_TEXT_COLOR_G= $5; 
					      OTHER_TEXT_COLOR_B = $7; OTHER_TEXT_COLOR_A= $9; 
	   }
	   ;

strprop: BG_TOK '=' QUOTSTR_T {BACKGROUND=StrApp((char**)0, THEME_DIR, $3, (char*)0); }
	 | FONT_TOK '=' QUOTSTR_T {FONT=StrApp((char**)0, THEME_DIR, $3, (char*)0);}
	 ;

anumprop:  BUTTON_OPAC_TOK '=' ANUM_T {BUTTON_OPACITY=$3;}
| WIN_OP_TOK '=' ANUM_T {WINDOW_OPACITY=$3;}
	   | SEL_WIN_OP_TOK '=' ANUM_T {SELECTED_WINDOW_OPACITY=$3;}
	   ;

%%
