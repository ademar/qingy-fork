%{

#include <stdlib.h>
#include <stdio.h>

#define YYERROR_VERBOSE

extern FILE* yyin;

%}

%union {
  int ival;
  char* str;
  char color[4];
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

ssav:	SCREENSAVER_TOK PIXEL_TOK { printf("Set screensaver as pixel\n"); }
	| SCREENSAVER_TOK PHOTOS_TOK photos { printf("Set screensaver as photos\n"); } ;

photos: 			/* nothing */
	| photos QUOTSTR_T { printf("\tgetting photos at: %s\n", $2); };

xsessdir: XSESSION_DIR_TOK '=' QUOTSTR_T { printf("XSESSION_DIR set as %s\n", $3); };

txtsessdir: TXTSESSION_DIR_TOK '=' QUOTSTR_T { printf("TXT SESSIONS at %s\n", $3); };

xinit:	XINIT_TOK '=' QUOTSTR_T { printf("XINIT_VAL is %s\n", $3); };
 
theme: 	THEME_TOK RAND_TOK {printf("THEME is random\n");} 
	| THEME_TOK '=' QUOTSTR_T  {printf("THEME at %s\n", $3);}
	| THEME_TOK '{' themedefn '}' 
	;

themedefn: 			/* nothing */
	| themedefn strprop
	| themedefn anumprop
	| themedefn colorprop
	;

colorprop: MASK_TXT_COL_TOK COLOR_T { printf("Mask text color is now #%x\n", $2); }
	   | TXT_CUR_COL_TOK COLOR_T  {printf("Cur text color is now #%x\n", $2); }
	   | OTHER_TXT_COL_TOK COLOR_T {printf("Other text color is now #%x\n",$2); }
	   ;

strprop: BG_TOK QUOTSTR_T {printf("Background image will be %s\n", $2); }
	 | FONT_TOK QUOTSTR_T {printf("Font will be %s\n", $2); }
	 ;

anumprop:  BUTTON_OPAC_TOK ANUM_T {printf("Button opacity at %d\n", $2);}
	   | WIN_OP_TOK ANUM_T {printf("Window opacity at %d\n", $2);}
	   | SEL_WIN_OP_TOK ANUM_T {printf("Selected window opacity at %d\n", $2);}
	   ;

%%

yyerror(char* c){printf("%s\n", c);}

int main(int argc, char**argv)
{
  if(argc > 1)
    yyin=fopen(argv[1], "r");
  
  return yyparse();
}

