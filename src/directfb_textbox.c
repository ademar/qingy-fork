#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <directfb.h>
#include <directfb_keynames.h>

#include "directfb_textbox.h"
#include "directfb_utils.h"
#include "framebuffer_mode.h"
#include "misc.h"

TextBox *TextBox_Create(IDirectFBDisplayLayer *layer, IDirectFBFont *font, DFBWindowDescription *window_desc)
{
	TextBox *newbox = NULL;
	DFBResult err;

	newbox = (TextBox *) calloc(1, sizeof(TextBox));
	newbox->text     = NULL;
	newbox->xpos     = window_desc->posx;
	newbox->ypos     = window_desc->posy;
	newbox->width    = window_desc->width;
	newbox->height   = window_desc->height;
	newbox->hasfocus = 0;
	newbox->window   = NULL;
	newbox->surface  = NULL;

	DFBCHECK(layer->CreateWindow (layer, window_desc, &(newbox->window)));
	newbox->window->SetOpacity(newbox->window, 0x00 );
	newbox->window->GetSurface(newbox->window, &(newbox->surface));
	newbox->surface->Clear(newbox->surface, 0x00, 0x00, 0x00, 0x00);
	newbox->surface->Flip(newbox->surface, NULL, 0);
	newbox->surface->SetFont (newbox->surface, font);
	newbox->surface->SetColor (newbox->surface, MASK_TEXT_COLOR);
	newbox->window->RequestFocus(newbox->window);
	newbox->window->RaiseToTop(newbox->window);

	return newbox;
}

int parse_input(int *input, char *buffer, int *length)
{

	if (*input == BACKSPACE)
	{
		if ( !(*length) ) return 0;
		(*length)--;
		buffer[*length] = '\0';
		return 1;
	}

	if ( (*input >= 32) && (*input <=255) ) /* ASCII */
	{
		if (*length == MAX) return 0;
		buffer[*length] = *input;
		(*length)++;
		buffer[*length] = '\0';
		return 1;
	}

	/*if (*input == ARROW_LEFT)
	if (*input == ARROW_RIGHT)*/

	return 0;
}

void TextBox_KeyEvent(TextBox *thiz, int ascii_code, int show)
{
	char *buffer = thiz->text;
	int length;
	IDirectFBSurface *window_surface= thiz->surface;

	if (!buffer)
	{
		buffer = (char *) calloc(MAX, sizeof(char));
		thiz->text = buffer;
	}
	length = strlen(thiz->text);

	if (parse_input(&ascii_code, buffer, &length))
	{
		window_surface->Clear (window_surface, 0x00, 0x00, 0x00, 0x00);
		if (show) window_surface->DrawString (window_surface, buffer, -1, 0, 0, DSTF_LEFT|DSTF_TOP);
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

void TextBox_SetFocus(TextBox *thiz, int focus)
{
	IDirectFBWindow *window = thiz->window;

	if (focus)
	{
		window->SetOpacity(window, SELECTED_WINDOW_OPACITY);
		thiz->hasfocus = 1;
		return;
	}
	else
	{
		window->SetOpacity(window, WINDOW_OPACITY);
		thiz->hasfocus = 0;
		return;
	}
}

void TextBox_Hide(TextBox *thiz)
{
	thiz->window->SetOpacity(thiz->window, 0x00);
}

void TextBox_Destroy(TextBox *thiz)
{
	if (!thiz) return;
	if (!!(thiz->text)) free(thiz->text);
	if (thiz->surface) thiz->surface->Release (thiz->surface);
	if (thiz->window) thiz->window->Release (thiz->window);
	free(thiz);
}
