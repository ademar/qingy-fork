/* -*-c-*- */
%{
/**-*-c-*-****************************************************************/
/* scan_settings.l - config file scanner for qingy                       */
/*  v. 1.0                                                               */
/*  Copyright (C) 2003 Paolo Gianrossi - All rights reserved             */
/*  by Paolo Gianrossi <paolino.gnu@disi.unige.it>                       */
/*                                                                       */
/* This program is free software; you can redistribute it and/or         */
/* modify  under the terms of the GNU General Public License as          */
/* published by the Free Software Foundation; either version 2, or (at   */
/* your option) any later version.                                       */
/*                                                                       */
/* This program is distributed in the hope that it will be useful, but   */
/* WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     */
/* General Public License for more details.                              */
/*                                                                       */
/* You should have received a copy of the GNU General Public License     */
/* along with GNU Emacs; see the file COPYING.  If not, write to the     */
/* Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.     */
/*************************************************************************/

#include <string.h>
#include "parse_settings.h"

#define MAX_STR_CONST 256

char string_buf[MAX_STR_CONST];
char *string_buf_ptr;

%}

%x stringa

%%

/* quted string defn */

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
/* end of quoted string defn */

/* keywords */
screensaver             return SCREENSAVER_TOK;
pixel                   return PIXEL_TOK;
photos                  return PHOTOS_TOK;
x_sessions              return XSESSION_DIR_TOK;
text_sessions           return TXTSESSION_DIR_TOK;
xinit                   return XINIT_TOK;
theme                   return THEME_TOK;
random                  return RAND_TOK;
default_text_color      return DEFAULT_TXT_COL_TOK;
default_cursor_color    return DEFAULT_CUR_COL_TOK;
other_text_color        return OTHER_TXT_COL_TOK;
background              return BG_TOK;
font                    return FONT_TOK;
button_opacity          return BUTTON_OPAC_TOK;
window_opacity          return WIN_OP_TOK;
selected_window_opacity return SEL_WIN_OP_TOK;
shutdown_policy         return SHUTDOWN_TOK;
everyone                return EVERYONE_TOK;
root                    return ONLY_ROOT_TOK;
noone                   return NO_ONE_TOK;
type                    return WTYPE_TOK;
width                   return WWIDTH_TOK;
height                  return WHEIGHT_TOK;
command                 return WCOMMAND_TOK;
content                 return WCONTENT_TOK;
window                  return WINDOW_TOK;
time                    return WPOLL_TIME_TOK;
text_color              return WTEXT_COLOR_TOK;
cursor_color            return WCURSOR_COLOR_TOK;
linkto                  return WINDOW_LINK_TOK;
text_size               return WTEXT_SIZE_TOK;
small                   return WTEXT_SMALL_TOK;
medium                  return WTEXT_MEDIUM_TOK;
large                   return WTEXT_LARGE_TOK;
text_orientation        return WTEXT_ORIENTATION;
left                    return WTEXT_LEFT_TOK;
center                  return WTEXT_CENTER_TOK;
right                   return WTEXT_RIGHT_TOK;


"#"[^\n]*"\n" ;			/* skip comments */

/* numbers */
[0-9]+	   {  sscanf(yytext, "%i", &(yylval.ival)); return ANUM_T;}	

/* colors */
"\["[0-9a-fA-F]{8}"\]" { sscanf(yytext, "[%x]", (unsigned int *)yylval.color); return COLOR_T;}

[ \n\t] ; 			/* skip blanks */

. return *yytext;

%%

/* do nothing at EOF */
int yywrap(){return 1;}
