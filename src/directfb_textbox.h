/***************************************************************************
                      directfb_textbox.h  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003 by Noberasco Michele
    e-mail               : noberasco.gnu@educ.disi.unige.it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.              *
 *                                                                         *
 ***************************************************************************/


#define BACKSPACE       8
#define TAB             9
#define RETURN         13
#define ESCAPE         27
#define DELETE        127
#define ARROW_LEFT  61440
#define ARROW_RIGHT 61441
#define ARROW_UP    61442
#define ARROW_DOWN  61443
#define HOME        61445
#define END         61446

typedef struct
{
	char *text;
	unsigned int xpos, ypos;
	unsigned int width, height;
	int hasfocus;
	int mask_text;
	int position;
	IDirectFBWindow	*window;
	IDirectFBSurface *surface;
} TextBox;

TextBox *TextBox_Create
(
	IDirectFBDisplayLayer *layer,
	IDirectFBFont *font,
	DFBWindowDescription *window_desc
);

void TextBox_KeyEvent(TextBox *thiz, int ascii_code, int draw_cursor);
void TextBox_SetFocus(TextBox *thiz, int focus);
void TextBox_SetText(TextBox *thiz, char *text);
void TextBox_ClearText(TextBox *thiz);
void TextBox_Hide(TextBox *thiz);
void TextBox_Show(TextBox *thiz);
void TextBox_Destroy(TextBox *thiz);
