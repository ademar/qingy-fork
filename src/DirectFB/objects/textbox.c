/***************************************************************************
                          textbox.c  -  description
                            --------------------
    begin                : Apr 10 2003
    copyright            : (C) 2003-2005 by Noberasco Michele
    e-mail               : michele.noberasco@tiscali.it
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <directfb.h>
#include <directfb_keynames.h>

#include "memmgmt.h"
#include "load_settings.h"
#include "textbox.h"
#include "directfb_mode.h"
#include "misc.h"
#include "utils.h"

int insert_char(char *buffer, int *length, int *position, char c)
{
	int i;

	if (*length == MAX) return 0;
	for (i=*length; i>*position; i--) buffer[i] = buffer[i-1];
	buffer[*position] = c;
	(*position)++;
	(*length)++;
	buffer[*length] = '\0';
	return 1;
}

int delete_left_char(char *buffer, int *length, int *position)
{
	int i;

	if ( !(*length) || !(*position) ) return 0;
	(*position)--; (*length)--;
	for (i=*position; i<*length; i++) buffer[i] = buffer[i+1];
	buffer[*length] = '\0';
	return 1;
}

int delete_right_char(char *buffer, int *length, int *position)
{
 	int i;

	if ( !(*length) || (*position == *length) ) return 0;
	(*length)--;
	for (i=*position; i<*length; i++) buffer[i] = buffer[i+1];
	buffer[*length] = '\0';
	return 1;
}

int parse_input(int *input, char *buffer, int modifier, int *length, int *position)
{
	if (modifier == CONTROL)
		switch (*input)
		{
			case 'u':
				buffer[*position = *length = 0] = '\0';
				return 1;
			case 'h':
				return delete_left_char(buffer, length, position);
		}

	switch (*input)
	{
		case BACKSPACE:
			return delete_left_char(buffer, length, position);
		case DELETE:
			return delete_right_char(buffer, length, position);
		case ARROW_LEFT:
			if (!(*position)) return 0;
			(*position)--;
			return 1;
		case ARROW_RIGHT:
			if (*position == *length) return 0;
			(*position)++;
			return 1;
		case HOME:
			if (!(*position)) return 0;
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
		return insert_char(buffer, length, position, *input);

	return 0;
}

void DrawCursor(TextBox *thiz)
{
	static IDirectFBFont *font = NULL;
	IDirectFBSurface *where;
	DFBRectangle dest1, dest2, dest;
	char *text;
	int free_text = 0;
	int position;

	if (!thiz) return;
	position = thiz->position;

	if (thiz->mask_text)
	{
		int length = strlen(thiz->text);
		text = (char *) calloc(length+1, sizeof(char));
		text[length] = '\0';
		for(length--; length>=0; length--) text[length] = '*';
		free_text = 1;
	}
	else if (thiz->hide_text)
	{
		text = (char *) calloc(1, sizeof(char));
		*text = '\0';
		free_text = 1;
		position = 0;
	}
	else text = thiz->text;

	if (!font) thiz->surface->GetFont(thiz->surface, &font);

	font->GetStringExtents(font, text, position, &dest1, NULL);
	font->GetStringExtents(font, text, position+1, &dest2, NULL);
	font->GetStringWidth (font, text, position, &(dest.x));
	dest.y = 0;
	dest.w = dest2.w - dest1.w;
	dest.h = dest1.h;
	thiz->surface->GetSubSurface(thiz->surface, &dest, &where);
	where->SetColor (where, thiz->cursor_color.R, thiz->cursor_color.G, thiz->cursor_color.B, thiz->cursor_color.A);
	where->FillRectangle (where, 0, 0, dest.w, dest.h);
	where->Release(where);
	if (free_text) free(text);
}

void TextBox_KeyEvent(TextBox *thiz, int ascii_code, int modifier, int draw_cursor)
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

	if (parse_input(&ascii_code, buffer, modifier, &length, position))
	{
		window_surface->Clear (window_surface, 0x00, 0x00, 0x00, 0x00);
		if (draw_cursor) DrawCursor(thiz);
		if (thiz->mask_text)
		{
			char *tmp = (char *) calloc(length+1, sizeof(char));			
			tmp[length] = '\0';
			for(length--; length>=0; length--) tmp[length] = '*';
			window_surface->DrawString (window_surface, tmp, -1, 0, 0, DSTF_LEFT|DSTF_TOP);
			free(tmp);
		}
		else if (thiz->hide_text) window_surface->DrawString (window_surface, "", -1, 0, 0, DSTF_LEFT|DSTF_TOP);
		else window_surface->DrawString (window_surface, buffer, -1, 0, 0, DSTF_LEFT|DSTF_TOP);
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
	if (thiz->hasfocus) TextBox_KeyEvent(thiz, REDRAW, NONE, 1);
	else TextBox_KeyEvent(thiz, REDRAW, NONE, 0);
}

void TextBox_ClearText(TextBox *thiz)
{
	if (!thiz) return;
	if (thiz->text) memset(thiz->text, '\0', sizeof(thiz->text));
	thiz->position = 0;
	if (thiz->hasfocus) TextBox_KeyEvent(thiz, REDRAW, NONE, 1);
	else TextBox_KeyEvent(thiz, REDRAW, NONE, 0);
}

void TextBox_SetTextColor(TextBox *thiz, color_t *text_color)
{
	if (!thiz)       return;
	if (!text_color) return;

	thiz->text_color.R = text_color->R;
	thiz->text_color.G = text_color->G;
	thiz->text_color.B = text_color->B;
	thiz->text_color.A = text_color->A;
	thiz->surface->SetColor (thiz->surface, text_color->R, text_color->G, text_color->B, text_color->A);	
}

void TextBox_SetCursorColor(TextBox *thiz, color_t *cursor_color)
{
	if (!thiz)       return;
	if (!cursor_color) return;

	thiz->cursor_color.R = cursor_color->R;
	thiz->cursor_color.G = cursor_color->G;
	thiz->cursor_color.B = cursor_color->B;
	thiz->cursor_color.A = cursor_color->A;
}

void TextBox_SetFocus(TextBox *thiz, int focus)
{
	if (!thiz) return;

	if (focus)
	{
		thiz->window->RequestFocus(thiz->window);
		thiz->hasfocus = 1;
		thiz->window->SetOpacity(thiz->window, selected_window_opacity);
		if (!thiz->text) thiz->position = 0;
		else thiz->position = strlen(thiz->text);
		TextBox_KeyEvent(thiz, REDRAW, NONE, 1);
		return;
	}

	thiz->hasfocus = 0;
	thiz->window->SetOpacity(thiz->window, window_opacity);
	TextBox_KeyEvent(thiz, REDRAW, NONE, 0);
	return;
}

void TextBox_Hide(TextBox *thiz)
{
	thiz->window->SetOpacity(thiz->window, 0x00);
}

void TextBox_Show(TextBox *thiz)
{
	if (thiz->hasfocus) thiz->window->SetOpacity(thiz->window, selected_window_opacity);
	else thiz->window->SetOpacity(thiz->window, window_opacity);
}

void TextBox_Destroy(TextBox *thiz)
{
	if (!thiz) return;
	if (thiz->text) free(thiz->text);	
	if (thiz->surface) thiz->surface->Release (thiz->surface);
	if (thiz->window) thiz->window->Release (thiz->window);
	free(thiz);
}

TextBox *TextBox_Create(IDirectFBDisplayLayer *layer, IDirectFBFont *font, color_t *text_color, color_t *cursor_color, DFBWindowDescription *window_desc)
{
	TextBox *newbox = NULL;

	newbox = (TextBox *) calloc(1, sizeof(TextBox));
	newbox->text           = NULL;
	newbox->xpos           = (unsigned int) window_desc->posx;
	newbox->ypos           = (unsigned int) window_desc->posy;
	newbox->width          = window_desc->width;
	newbox->height         = window_desc->height;
	newbox->hasfocus       = 0;
	newbox->mask_text      = 0;
	newbox->hide_text      = 0;
	newbox->position       = 0;
	newbox->text_color.R   = text_color->R;
	newbox->text_color.G   = text_color->G;
	newbox->text_color.B   = text_color->B;
	newbox->text_color.A   = text_color->A;
	newbox->cursor_color.R = cursor_color->R;
	newbox->cursor_color.G = cursor_color->G;
	newbox->cursor_color.B = cursor_color->B;
	newbox->cursor_color.A = cursor_color->A;
	newbox->window         = NULL;
	newbox->surface        = NULL;
	newbox->KeyEvent       = TextBox_KeyEvent;
	newbox->SetFocus       = TextBox_SetFocus;
	newbox->SetText        = TextBox_SetText;
	newbox->ClearText      = TextBox_ClearText;
	newbox->Hide           = TextBox_Hide;
	newbox->Show           = TextBox_Show;
	newbox->Destroy        = TextBox_Destroy;

	if (layer->CreateWindow(layer, window_desc, &(newbox->window)) != DFB_OK) return NULL;
	newbox->window->SetOpacity(newbox->window, 0x00);
	newbox->window->GetSurface(newbox->window, &(newbox->surface));
	newbox->surface->Clear(newbox->surface, 0x00, 0x00, 0x00, 0x00);
	newbox->surface->Flip(newbox->surface, NULL, 0);
	newbox->surface->SetFont (newbox->surface, font);
	newbox->surface->SetColor (newbox->surface, newbox->text_color.R, newbox->text_color.G, newbox->text_color.B, newbox->text_color.A);
	newbox->window->RaiseToTop(newbox->window);

	return newbox;
}
