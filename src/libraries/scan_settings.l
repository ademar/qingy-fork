/* -*-c-*- */
%{
/**-*-c-*-****************************************************************/
/* scan_settings.l - config file scanner for qingy                       */
/*                                                                       */
/*  Copyright (C) 2003      Paolo Gianrossi   - All rights reserved      */
/*            (C) 2004-2005 Michele Noberasco                            */
/*  by Paolo Gianrossi   <paolino.gnu@disi.unige.it>                     */
/*  by Michele Noberasco <s4t4n@gentoo.org>                              */
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include "parse_settings.h"

#include "load_settings.h"
#include "misc.h"

#define MAX_STR_CONST 256

char string_buf[MAX_STR_CONST];
char *string_buf_ptr;
YY_BUFFER_STATE settings_buf=NULL;
FILE *theme_fp=NULL;

extern int in_theme;
extern int yyparse();

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
x_sessions              return XSESSION_DIR_TOK;
text_sessions           return TXTSESSION_DIR_TOK;
xinit                   return XINIT_TOK;
x_server                return X_SERVER_TOK;
theme                   return THEME_TOK;
native_resolution       return THEME_RES_TOK;
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
tiny                    return WTEXT_TINY_TOK;
smaller                 return WTEXT_SMALLER_TOK;
small                   return WTEXT_SMALL_TOK;
medium                  return WTEXT_MEDIUM_TOK;
large                   return WTEXT_LARGE_TOK;
text_orientation        return WTEXT_ORIENTATION;
left                    return WTEXT_LEFT_TOK;
center                  return WTEXT_CENTER_TOK;
right                   return WTEXT_RIGHT_TOK;
yes                     return YES_TOK;
no                      return NO_TOK;
clear_background        return CLEAR_BACKGROUND_TOK;
tty                     return TTY_TOK;
lock_sessions           return LOCK_SESSIONS_TOK;
screensavers_dir        return SCRSVRS_DIR_TOK;
themes_dir              return THEMES_DIR_TOK;
qingy_DirectFB          return DFB_INTERFACE_TOK;
retries                 return RETRIES_TOK;
x_args                  return X_ARGS_TOK;
NULL                    return NULL_TOK;
autologin               return AUTOLOGIN_TOK;
username                return USERNAME_TOK;
password                return PASSWORD_TOK;
session                 return SESSION_TOK;
relogin                 return RELOGIN_TOK;
lastsession             return LAST_SESSION_TOK;
temp_files_dir          return TEMP_FILES_DIR_TOK;
sleep                   return SLEEP_TOK;
next_tty                return NEXT_TTY_TOK;
prev_tty                return PREV_TTY_TOK;
poweroff                return POWEROFF_TOK;
reboot                  return REBOOT_TOK;
kill                    return KILL_TOK;
text_mode               return TEXT_MODE_TOK;
menu                    return MENU_KEY_TOK;
win                     return WIN_KEY_TOK;
keybindings             return KEYBINDINGS_TOK;
user                    return USER_TOK;
last_user_policy        return LAST_USER_POLICY_TOK;
last_session_policy     return LAST_SESSION_POLICY_TOK;
global                  return GLOBAL_TOK;
pre_gui_script          return PRE_GUI_TOK;
post_gui_script         return POST_GUI_TOK;
x_server_offset         return X_SERVER_OFFSET_TOK;


"#"[^\n]*"\n" ;			/* skip comments */

 /* numbers */
[0-9]+	   {  sscanf(yytext, "%i", &(yylval.ival)); return ANUM_T;}	

 /* colors */
"\["[0-9a-fA-F]{8}"\]" { sscanf(yytext, "[%x]", (unsigned int *)yylval.color); return COLOR_T;}

[ \n\t] ; 			/* skip blanks */

. return *yytext;

%%

 /* do nothing at EOF */
int yywrap()
{
  if(settings_buf)
	{
    yy_delete_buffer( YY_CURRENT_BUFFER );
    yy_switch_to_buffer(settings_buf);
    settings_buf=NULL;
    got_theme=1;
    in_theme=0;
		fclose(theme_fp);
    return 0;
  }
  return 1;
}


int set_theme(char *theme)
{
	FILE *fp;
  char *file;

  if (!theme) return 0;
  
  theme_dir = StrApp((char**)NULL, themes_dir, "/", theme, "/", (char*)NULL);
  file = StrApp((char**)NULL, theme_dir, "theme", (char*)NULL);

  fp = fopen(file, "r");
	free(file);
  if (!fp)
	{
		if (!silent) fprintf(stderr, "load_settings: theme '%s' does not exist in directory '%s'.\n", theme, theme_dir);
		return 0;
	}
	settings_buf=YY_CURRENT_BUFFER;
	theme_fp = yyin = fp;
  yy_switch_to_buffer(yy_create_buffer( yyin, YY_BUF_SIZE ) );
  in_theme = 1;
  return 1;
}

