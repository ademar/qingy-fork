/***************************************************************************
                      directfb_textbox.c  -  description
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <directfb.h>

#include "directfb_textbox.h"
#include "framebuffer_mode.h"
#include "load_settings.h"
#include "misc.h"


int parse_input(int *input, char *buffer, int *length, int *position)
{
	int i;

	switch (*input)
	{
		case BACKSPACE:
			if ( !(*length) || !(*position) ) return 0;
			(*position)--; (*length)--;
			for (i=*position; i<*length; i++) buffer[i] = buffer[i+1];
			buffer[*length] = '\0';
			return 1;
		case DELETE:
			if ( !(*length) || (*position == *length) ) return 0;
			(*length)--;
			for (i=*position; i<*length; i++) buffer[i] = buffer[i+1];
			buffer[*length] = '\0';
			return 1;
		case ARROW_LEFT:
			if (*position == 0) return 0;
			(*position)--;
			return 1;
		case ARROW_RIGHT:
			if (*position == *length) return 0;
			(*position)++;
			return 1;
		case HOME:
			if (*position == 0) return 0;
			(*position) = 0;
			return 1;
		case END:
			if (*position == *length) return 0;
			*position = *length;
			return 1;
		case REDRAW:
			return 1;
	}

	if ( (*input >= 32) && (*input <=255) ) /* ASCII */
	{
		if (*length == MAX) return 0;
		for (i=*length; i>*position; i--) buffer[i] = buffer[i-1];
		buffer[*position] = *input;
		(*position)++;
		(*length)++;
		buffer[*length] = '\0';
		return 1;
	}

	return 0;
}

void DrawCursor(TextBox *thiz)
{
	static IDirectFBFont *font = NULL;
	IDirectFBSurface *where;
	DFBRectangle dest1, dest2, dest;
	char *text;
	int free_text = 0;

	if (thiz->mask_text)
	{
		int length = strlen(thiz->text);
		text = (char *) calloc(length+1, sizeof(char));
		text[length] = '\0';
		for(length--; length>=0; length--) text[length] = '*';
		free_text = 1;
	}
	else text = thiz->text;

	if (!font) thiz->surface->GetFont(thiz->surface, &font);

	font->GetStringExtents(font, text, thiz->position, &dest1, NULL);
	font->GetStringExtents(font, text, thiz->position+1, &dest2, NULL);
	font->GetStringWidth (font, text, thiz->position, &(dest.x));
	dest.y = 0;
	dest.w = dest2.w - dest1.w;
	dest.h = dest1.h;
	thiz->surface->GetSubSurface(thiz->surface, &dest, &where);
	where->SetColor (where, TEXT_CURSOR_COLOR_R, TEXT_CURSOR_COLOR_G, TEXT_CURSOR_COLOR_B, TEXT_CURSOR_COLOR_A);
	where->FillRectangle (where, 0, 0, dest.w, dest.h);
	if (free_text) free(text);
}

void TextBox_KeyEvent(TextBox *thiz, int ascii_code, int draw_cursor)
{
	char *buffer = thiz->text;
	int length;
	int *position = &thiz->position;
	IDirectFBSurface *window_surface= thiz->surface;

	if (!buffer)
	{
		buffer = (char *) calloc(MAX, sizeof(char));
		thiz->text = buffer;
	}
	length = strlen(thiz->text);

	if (parse_input(&ascii_code, buffer, &length, position))
	{
		window_surface->Clear (window_surface, 0x00, 0x00, 0x00, 0x00);
		if (draw_cursor) DrawCursor(thiz);
		if (!(thiz->mask_text)) window_surface->DrawString (window_surface, buffer, -1, 0, 0, DSTF_LEFT|DSTF_TOP);
		else
		{
			char *tmp = (char *) calloc(length+1, sizeof(char));
			tmp[length] = '\0';
			for(length--; length>=0; length--) tmp[length] = '*';
			window_surface->DrawString (window_surface, tmp, -1, 0, 0, DSTF_LEFT|DSTF_TOP);
			free(tmp);
		}
		window_surface->Flip(window_surface, NULL, 0);
	}
}

void TextBox_SetText(TextBox *thiz, char *text)
{
	if (!thiz || !text) return;
	if (strlen(text) >= MAX-1) return;
	if (!thiz->text) thiz->text = (char *) calloc(MAX, sizeof(char));
	strcpy(thiz->text, text);
	thiz->position = strlen(thiz->text);
	if (thiz->hasfocus) TextBox_KeyEvent(thiz, REDRAW, 1);
	else TextBox_KeyEvent(thiz, REDRAW, 0);
}

void TextBox_ClearText(TextBox *thiz)
{
	if (!thiz) return;
	if (thiz->text) (thiz->text)[0] = '\0';
	thiz->position = 0;
	if (thiz->hasfocus) TextBox_KeyEvent(thiz, REDRAW, 1);
	else TextBox_KeyEvent(thiz, REDRAW, 0);
}

void TextBox_SetFocus(TextBox *thiz, int focus)
{
	if (!thiz) return;

	if (focus)
	{
		thiz->window->RequestFocus(thiz->window);
		thiz->hasfocus=1;
		thiz->window->SetOpacity(thiz->window, SELECTED_WINDOW_OPACITY);
		if (thiz->text == NULL) thiz->position = 0;
		else thiz->position = strlen(thiz->text);
		TextBox_KeyEvent(thiz, REDRAW, 1);
		return;
	}

	thiz->hasfocus = 0;
	thiz->window->SetOpacity(thiz->window, WINDOW_OPACITY);
	TextBox_KeyEvent(thiz, REDRAW, 0);
	return;
}

void TextBox_Hide(TextBox *thiz)
{
	thiz->window->SetOpacity(thiz->window, 0x00);
}

void TextBox_Show(TextBox *thiz)
{
	if (thiz->hasfocus) thiz->window->SetOpacity(thiz->window, SELECTED_WINDOW_OPACITY);
	else thiz->window->SetOpacity(thiz->window, WINDOW_OPACITY);
}

void TextBox_Destroy(TextBox *thiz)
{
	if (!thiz) return;
	if (thiz->text) free(thiz->text);
	if (thiz->surface) thiz->surface->Release (thiz->surface);
	if (thiz->window) thiz->window->Release (thiz->window);
	free(thiz);
}

TextBox *TextBox_Create(IDirectFBDisplayLayer *layer, IDirectFBFont *font, DFBWindowDescription *window_desc)
{
	TextBox *newbox = NULL;
	DFBResult err;

	newbox = (TextBox *) calloc(1, sizeof(TextBox));
	newbox->text     = NULL;
	newbox->xpos     = (unsigned int) window_desc->posx;
	newbox->ypos     = (unsigned int) window_desc->posy;
	newbox->width    = window_desc->width;
	newbox->height   = window_desc->height;
	newbox->hasfocus = 0;
	newbox->mask_text= 0;
	newbox->position = 0;
	newbox->window   = NULL;
	newbox->surface  = NULL;
	newbox->KeyEvent = TextBox_KeyEvent;
	newbox->SetFocus = TextBox_SetFocus;
	newbox->SetText  = TextBox_SetText;
	newbox->ClearText= TextBox_ClearText;
	newbox->Hide     = TextBox_Hide;
	newbox->Show     = TextBox_Show;
	newbox->Destroy  = TextBox_Destroy;

	DFBCHECK(layer->CreateWindow (layer, window_desc, &(newbox->window)));
	newbox->window->SetOpacity(newbox->window, 0x00 );
	newbox->window->GetSurface(newbox->window, &(newbox->surface));
	newbox->surface->Clear(newbox->surface, 0x00, 0x00, 0x00, 0x00);
	newbox->surface->Flip(newbox->surface, NULL, 0);
	newbox->surface->SetFont (newbox->surface, font);
	newbox->surface->SetColor (newbox->surface, MASK_TEXT_COLOR_R, MASK_TEXT_COLOR_G, MASK_TEXT_COLOR_B, MASK_TEXT_COLOR_A);
	newbox->window->RaiseToTop(newbox->window);

	return newbox;
}
