#include "misc.h"

typedef struct _TextBox TextBox;
struct _TextBox
{
	char *text;
	unsigned int xpos, ypos;
	unsigned int width, height;
	int hasfocus;
	IDirectFBWindow	*window;
	IDirectFBSurface *surface;
};

TextBox *TextBox_Create
(
	IDirectFBDisplayLayer *layer,
	IDirectFBFont *font,
	DFBWindowDescription *window_desc
);

void TextBox_KeyEvent(TextBox *thiz, int ascii_code, int show);
void TextBox_SetFocus(TextBox *thiz, int focus);
void TextBox_Hide(TextBox *thiz);
void TextBox_Destroy(TextBox *thiz);
