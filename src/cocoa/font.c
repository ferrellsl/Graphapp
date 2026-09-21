/*
 *  Native font interface.
 *
 *  Platform: macOS (CoreText, via the bridge).
 *
 *  A native font is a CTFont whose size is chosen so that its line
 *  height (ascent + descent) equals the requested pixel height.
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
		app_mask_free(subfont_extra(sub)->clipmask);
		subfont_extra(sub)->clipmask = NULL;
	}
}

/*
 *  Load a native font. Return 1 on success, 0 on failure.
 */
int app_load_native_font(Font *f, const char *name,
	int size, int height, int style)
{
	int ascent, descent, max_width;

	font_extra(f)->handle = gab_font_create(name,
			(style & BOLD) ? 1 : 0, (style & ITALIC) ? 1 : 0,
			height, &ascent, &descent, &max_width);
	if (! font_extra(f)->handle)
		return 0;
	font_extra(f)->ascent = ascent;
	font_extra(f)->descent = descent;
	f->maximum_width = max_width;
	return 1;
}

/*
 *  Return a native font's actual pixel height.
 */
int app_native_font_height(Font *f)
{
	f->height = font_extra(f)->ascent + font_extra(f)->descent;
	return f->height;
}

/*
 *  Return a native font's maximum glyph width.
 */
int app_native_font_width(Font *f)
{
	return f->maximum_width;
}

/*
 *  Return the width of a string in pixels, in the given native font.
 */
int app_native_font_string_width(Font *f, const char *s, int nbytes)
{
	return gab_font_string_width(font_extra(f)->handle, s, nbytes);
}

/*
 *  Release memory used by a native font.
 */
void app_release_native_font(Font *f)
{
	if (font_extra(f)->handle) {
		gab_font_release(font_extra(f)->handle);
		font_extra(f)->handle = NULL;
	}
}
