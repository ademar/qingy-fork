/***************************************************************************
                     framebuffer_mode.h  -  description
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


/* constants that we make use of */
#define BUTTON_OPACITY 0xFF
#define WINDOW_OPACITY 0x80
#define SELECTED_WINDOW_OPACITY 0xCF
#define MASK_TEXT_COLOR   0xFF, 0x00, 0x00, 0xFF
#define TEXT_CURSOR_COLOR 0x80, 0x00, 0x00, 0xDD
#define OTHER_TEXT_COLOR  0x40, 0x40, 0x40, 0xFF
#define TEXT_MODE -2

/* Init framebuffer mode */
int framebuffer_mode(int argc, char *argv[], int workaround);
