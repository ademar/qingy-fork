%{

#include <string.h>
#include "parse_settings.h"

#define MAX_STR_CONST 256

char string_buf[MAX_STR_CONST];
char *string_buf_ptr;



%}

%x stringa

%%

\"      string_buf_ptr = string_buf; BEGIN(stringa);

<stringa>\"        { 
  /* saw closing quote - all done */
  BEGIN(INITIAL);
  *string_buf_ptr = '\0';
  yylval.str=strdup(string_buf);
  return QUOTSTR_T;
}

<stringa>\n        {
  /* error - unterminated string constant */
  /* generate error message */
}

<stringa>\\[0-7]{1,3} {
                   /* octal escape sequence */
  unsigned int result;
  
  (void) sscanf( yytext + 1, "%o", &result );
  
  if ( result > 0xff )
    /* error, constant is out-of-bounds */
    
    *string_buf_ptr++ = result;
}

<stringa>\\[0-9]+ {
  /* generate error - bad escape sequence; something
   * like '\48' or '\0777777'
   */
}

<stringa>\\n  *string_buf_ptr++ = '\n';
<stringa>\\t  *string_buf_ptr++ = '\t';
<stringa>\\r  *string_buf_ptr++ = '\r';
<stringa>\\b  *string_buf_ptr++ = '\b';
<stringa>\\f  *string_buf_ptr++ = '\f';

<stringa>\\(.|\n)  *string_buf_ptr++ = yytext[1];

<stringa>[^\\\n\"]+        {
  char *yptr = yytext;
  
  while ( *yptr )
    *string_buf_ptr++ = *yptr++;
}

screensaver             return SCREENSAVER_TOK;
pixel                   return PIXEL_TOK;
photos                  return PHOTOS_TOK;
x_sessions              return XSESSION_DIR_TOK;
text_sessions           return TXTSESSION_DIR_TOK;
xinit                   return XINIT_TOK;
theme                   return THEME_TOK;
random                  return RAND_TOK;
mask_text_color         return MASK_TXT_COL_TOK;
cursor_text_color       return TXT_CUR_COL_TOK;
other_text_color        return OTHER_TXT_COL_TOK;
background              return BG_TOK;
font                    return FONT_TOK;
button_opacity          return BUTTON_OPAC_TOK;
window_opacity          return WIN_OP_TOK;
selected_window_opacity return SEL_WIN_OP_TOK;

"#"[^\n]*"\n" ;			/* skip comments */

[0-9]{1,3} { sscanf(yytext, "%i", &(yylval.ival)); return ANUM_T;}

[0-9]+	   {  sscanf(yytext, "%i", &(yylval.ival)); return LONGNUM_T;}	

"\["[0-9a-fA-F]{8}"\]" { sscanf(yytext, "[%x]", (unsigned int *)yylval.color); return COLOR_T;}

[ \n\t] ; 			/* skip blanks */

. return *yytext;

%%

int yywrap(){return 1;}
