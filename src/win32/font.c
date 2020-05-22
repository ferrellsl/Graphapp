/*
 *  Native font interface.
 *
 *  Platform: Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/17  Added native font interface.
 *  Version: 3.08  2001/11/11  Expanded native fonts to allow Latin1 text.
 *  Version: 3.10  2001/12/01  Fonts can be in a program's resources.
 *  Version: 3.21  2002/04/04  Fixed some memory leaks.
 *  Version: 3.35  2002/12/23  Moved portable code to fontutil.c
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

/*
 *  Release memory used by a native font's subfont.
 */
void app_release_native_subfont(Font *f, Subfont *sub)
{
	if (subfont_extra(sub)->clipmask) {
		DeleteObject(subfont_extra(sub)->clipmask);
		subfont_extra(sub)->clipmask = 0;
	}
}

/*
 *  Load a native font. Return 1 on success, 0 on failure.
 */
int app_load_native_font(Font *f, const char *name,
	int size, int height, int style)
{
	font_extra(f)->fnt = CreateFont(height, 0, 0, 0,
				(style & BOLD) ? FW_BOLD : FW_NORMAL,
				(style & ITALIC) ? 1 : 0,
				0, 0,
				ANSI_CHARSET,
				OUT_DEFAULT_PRECIS,
				CLIP_DEFAULT_PRECIS,
				DRAFT_QUALITY,
				DEFAULT_PITCH,
				name);
	if (! font_extra(f)->fnt)
		return 0;
	return 1;
}

/*
 *  Return a native font's actual pixel height.
 */
int app_native_font_height(Font *f)
{
	HDC dc;
	HFONT old;
	TEXTMETRIC tm;

	dc = GetDC(NULL);
	old = SelectObject(dc, font_extra(f)->fnt);
	GetTextMetrics(dc, &tm);
	SelectObject(dc, old);
	ReleaseDC(NULL, dc);
	f->height = tm.tmHeight;
	return f->height;
}

/*
 *  Return a native font's maximum glyph width.
 */
int app_native_font_width(Font *f)
{
	HDC dc;
	HFONT old;
	TEXTMETRIC tm;

	dc = GetDC(NULL);
	old = SelectObject(dc, font_extra(f)->fnt);
	GetTextMetrics(dc, &tm);
	SelectObject(dc, old);
	ReleaseDC(NULL, dc);
	f->maximum_width = tm.tmMaxCharWidth;
	return f->maximum_width;
}

/*
 *  Return the width of a string in pixels, in the given native font.
 */
int app_native_font_string_width(Font *f, const char *s, int nbytes)
{
	HDC dc;
	HFONT old;
	SIZE size;
	char *temp_str = NULL;

	/* assume ISO Latin-1 for now */
	if (! app_utf8_is_ascii(s, nbytes))
		s = temp_str = app_utf8_to_latin1(s, &nbytes);
	dc = GetDC(NULL);
	old = SelectObject(dc, font_extra(f)->fnt);
	GetTextExtentPoint32(dc, s, nbytes, &size);
	SelectObject(dc, old);
	ReleaseDC(NULL, dc);
	if (temp_str)
		app_free(temp_str);
	return size.cx;
}

/*
 *  Release memory used by a native font.
 */
void app_release_native_font(Font *f)
{
	if (font_extra(f)->fnt) {
		DeleteObject(font_extra(f)->fnt);
		font_extra(f)->fnt = NULL;
	}
}
