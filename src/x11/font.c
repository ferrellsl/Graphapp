/*
 *  Native font interface.
 *
 *  Platform: X-Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Added native font interface.
 *  Version: 3.08  2001/11/11  Expanded native fonts to allow Latin1 text.
 *  Version: 3.10  2001/12/01  Fonts can be in a program's resources.
 *  Version: 3.21  2002/04/04  Fixed some memory leaks.
 *  Version: 3.35  2002/12/23  Moved portable code to fontutil.c
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.58  2005/09/07  More font-selection fallback logic.
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
		XFreePixmap(app_extra(f->app)->display,
			subfont_extra(sub)->clipmask);
		subfont_extra(sub)->clipmask = 0;
	}
}

/*
 *  Load a native font. Return 1 on success, 0 on failure.
 *
 *  We construct an X font-name by using the name, style and size fields.
 *  An X font name has the following structure:
 *
 *  -fndry-family-weight-slant-swdth-adstyl-pixelsize-pointsize-
 *  resx-resy-spc-avgWdth-rgstry-encdng
 *
 *  We leave as wildcards those fields we don't wish to specify.
 *  The ones which are important to use are:
 *
 *  family = { courier, fixed, helvetica, lucida, new century schoolbook,
 *		screen, times }
 *  weight = { *, bold, medium }
 *  slant  = { r, i } roman or italic
 *  swdth  = { normal, condensed, double wide, narrow, semicondensed, wide }
 *  adstyl = { *, sans, serif }
 *  pixelsize = any pixel height
 *  pointsize = point size times 10, eg 120 means 12 point.
 *
 *  The other fields can safely be left as the wildcard *
 */
int app_load_native_font(Font *f, const char *name,
	int size, int height, int style)
{
	XFontStruct *fnt;
	char *fontname = NULL;
	char *fmt      = "-*-%s-%s-%s-%s-%s-%s-%s-*-*-*-*-*-*";
	char *wildcard = "*";
	char pixelsize[20];
	char pointsize[20];

	if (! name)
		name = wildcard;

	/* construct X font name */
	fontname = app_alloc(strlen(name) + 85);
	if (! fontname)
		return 0;

	/* fix point and/or pixelsize */

	strcpy(pixelsize, wildcard);
	strcpy(pointsize, wildcard);

	if (size < 0) { /* interpret as point-size */
		sprintf(pointsize, "%d", -size*10);
	}
	else if (size >= 0) { /* interpret as pixel height */
		sprintf(pixelsize, "%d", size-size/8);
		/* -size/8 is a fudge to try to remove interline spaces */
	}

	/* form X font-name string */
	sprintf(fontname, fmt, name,
			(style & BOLD) ? "bold" : "medium",
			(style & ITALIC) ? "i" : "r",
			"normal",
			wildcard,
			pixelsize,
			pointsize);

	/* try to load the font */
	fnt = XLoadQueryFont(app_extra(f->app)->display, fontname);

	/* failed: make it simpler */
	if (! fnt) {
		sprintf(fontname, fmt, name,
			(style & BOLD) ? "bold" : wildcard,
			(style & ITALIC) ? "i" : "r",
			"normal",
			wildcard,
			pixelsize,
			pointsize);
		fnt = XLoadQueryFont(app_extra(f->app)->display, fontname);
	}

	/* failed: make it simpler */
	if (! fnt) {
		sprintf(fontname, fmt, name,
			(style & BOLD) ? "bold" : wildcard,
			wildcard,
			"normal",
			wildcard,
			pixelsize,
			pointsize);
		fnt = XLoadQueryFont(app_extra(f->app)->display, fontname);
	}

	/* failed: make it simpler */
	if (! fnt) {
		sprintf(fontname, fmt, name,
			wildcard,
			wildcard,
			"normal",
			wildcard,
			pixelsize,
			pointsize);
		fnt = XLoadQueryFont(app_extra(f->app)->display, fontname);
	}

	/* failed: make it simpler */
	if (! fnt) {
		sprintf(fontname, fmt, name,
			wildcard,
			wildcard,
			wildcard,
			wildcard,
			pixelsize,
			pointsize);
		fnt = XLoadQueryFont(app_extra(f->app)->display, fontname);
	}

	/* failed: make it smaller */
	while (! fnt) {
		if (size < -9) { /* interpret as point-size */
			++size;
			sprintf(pointsize, "%d", -size*10);
		}
		else if (size >= 9) { /* interpret as pixel height */
			--size;
			sprintf(pixelsize, "%d", size-size/8);
			/* -size/8 is a fudge to remove interline spaces */
		}
		else {
			break; /* too small */
		}

		sprintf(fontname, fmt, name,
			wildcard,
			wildcard,
			wildcard,
			wildcard,
			pixelsize,
			pointsize);
		fnt = XLoadQueryFont(app_extra(f->app)->display, fontname);
	}

	/* failed: make it a lot simpler */
	if (! fnt) {
		strcpy(fontname, "-*-*-*-r-*-sans-*-120-*");
		fnt = XLoadQueryFont(app_extra(f->app)->display, fontname);
	}

	/* failed: go for a fixed font */
	if (! fnt) {
		strcpy(fontname, "fixed");
		fnt = XLoadQueryFont(app_extra(f->app)->display, fontname);
	}

	/* tidy up */
	app_free(fontname);
	font_extra(f)->fnt = fnt;

	if (! font_extra(f)->fnt)
		return 0;
	return 1;
}

/*
 *  Return a native font's actual pixel height.
 */
int app_native_font_height(Font *f)
{
	return font_extra(f)->fnt->ascent + font_extra(f)->fnt->descent;
}

/*
 *  Return a native font's maximum glyph width.
 */
int app_native_font_width(Font *f)
{
	return font_extra(f)->fnt->max_bounds.width;
}

/*
 *  Return the width of a string in pixels, in the given native font.
 */
int app_native_font_string_width(Font *f, const char *s, int nbytes)
{
	int total;
	char *temp_str = NULL;

	/* assume ISO Latin-1 for now */
	if (! app_utf8_is_ascii(s, nbytes))
		s = temp_str = app_utf8_to_latin1(s, &nbytes);
	total = XTextWidth(font_extra(f)->fnt, s, nbytes);
	if (temp_str)
		app_free(temp_str);
	return total;
}

/*
 *  Release memory used by a native font.
 */
void app_release_native_font(Font *f)
{
	if (font_extra(f)->fnt) {
		XFreeFont(app_extra(f->app)->display, font_extra(f)->fnt);
		font_extra(f)->fnt = NULL;
	}
}
