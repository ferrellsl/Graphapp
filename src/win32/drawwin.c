/*
 *  Drawing to windows (essential operations).
 *
 *  Platform: Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/17  Added native font and line rendering.
 *  Version: 3.06  2001/10/30  Improved temporary font usage.
 *  Version: 3.08  2001/11/11  Expanded native fonts to allow Latin1 text.
 *  Version: 3.21  2002/04/04  Fixed memory leaks and XOR mode problems.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced some size_t conversion warnings.
 *  Version: 3.59  2005/10/10  Supports over-sized glyphs.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

/*
 *  This file implements the essential drawing opertions needed
 *  to draw to windows. These implement:
 *	app_fill_rect
 *	app_copy_rect
 *	app_draw_utf8
 *	app_draw_line
 *
 *  Pointers to the functions in this file are stored into
 *  Graphics objects created to draw to a window, and are later
 *  called to implement these basic drawing operations.
 */


/*
 *  app_window_fill_rect:
 *
 *  Fill a rectangle with colour, in a window.
 *
 *  This function implements client-side clipping, so that
 *  we never rely on the GDI system to do clipping, except if
 *  the destination is a window which is partially obscured.
 *  In that situation we must rely on the GDI system because there
 *  is no way for the program to know which portions of the
 *  window are currently obscured.
 */
int app_window_fill_rect(Graphics *dst, Rect r)
{
	int i, num_rects;
	Rect clipped;
	Rect *rects;
	unsigned long mode;
	HDC dst_dc;

	if (dst->colour.alpha > 0x7F)
		return 1; /* nothing to draw if colour is transparent */

	/* correct drawing displacement */
	r.x += dst->offset.x;
	r.y += dst->offset.y;

	/* fix negative spaces */
	if (r.width < 0) {
		r.x += r.width;
		r.width = 0 - r.width;
	}
	if (r.height < 0) {
		r.y += r.height;
		r.height = 0 - r.height;
	}

	/* destination is a window */

	dst_dc = graphics_extra(dst)->dc;

	/* set up clipping */
	if (dst->clip) {
		/* clip to the selected clipping region */
		num_rects = dst->clip->num_rects;
		rects = dst->clip->rects;
	}
	else {
		/* clip destination rectangle to window's boundary */
		num_rects = 1;
		rects = & dst->area;
	}

	/* handle XOR mode */
	if (dst->xor_mode)
		mode = PATINVERT;
	else
		mode = PATCOPY;

	/* draw the clipped rectangles */

	for (i=0; i < num_rects; i++) {
		clipped = app_clip_rect(r, rects[i]);
		if (clipped.width == 0)
			continue; /* nothing visible here */
		if (clipped.height == 0)
			continue; /* nothing visible here */

		/* fill pixels with colour */

		PatBlt(dst_dc, clipped.x, clipped.y,
			clipped.width, clipped.height, mode);
	}

	return 1;
}

/*
 *  app_window_copy_rect:
 *
 *  Copy an area from a window or bitmap into a window.
 *
 *  Special case: If the source and destination are both the same
 *  window, we use the ScrollDC and UpdateWindow functions to
 *  perform proper scrolling.
 *
 *  This function implements client-side clipping; that is,
 *  it looks through the destination clipping region and only
 *  draws where that region allows. If there is no region, it
 *  just clips the output against the rectangle of the destination.
 *  The GDI system would do that clipping for us anyway, but
 *  doing it ourselves gains speed and portability.
 *
 *                       transparent  opaque
 *  Clipmask        C       1 1 1 1  0 0 0 0    (1=trans, 0=opaque)
 *  Source          S       0 0 0 0  1 1 0 0
 *  Destination     D       1 0 1 0  1 0 1 0    Windows mode
 *  --------------- ----    ----------------    ------------
 *  MASK stage 1    D'=C&D  1 0 1 0  0 0 0 0    SRCAND    (0x008800C6)
 *  MASK stage 2    D"=S|D' 1 0 1 0  1 1 0 0    SRCPAINT  (0x00EE0086)
 *
 *  COPY (no C)     D'=S    X X X X  1 1 0 0    SRCCOPY   (0x00CC0020)
 *
 *  XOR (if no C)           1 0 0 1  0 1 1 0    D^S^BG    (0x00960169)
 *
 *  XOR (if C)      T=D                         SRCCOPY
 *      stage 2    T'=XOR   1 0 0 1  0 1 1 0    D^S^BG    (0x00960169)
 *      stage 3    T"=T'&~C 0 0 0 0 unchanged   T^~C      (0x00220326)
 *      stage 4    D'=D&C  unchanged 0 0 0 0    SRCAND
 *      stage 5    D"=T"|D'                     SRCPAINT
 */
int app_window_copy_rect(Graphics *dst, Point dp, Graphics *src, Rect sr)
{
	int i, num_rects;
	Rect dr, clipped;
	Rect *rects;
	HDC src_dc, src_mask_dc;
	HDC dst_dc;
	HBITMAP src_mask = 0;
	Bitmap *temp = NULL;
	Graphics *g = NULL;

	/* correct drawing displacement */
	dp.x += dst->offset.x;
	dp.y += dst->offset.y;
	sr.x += src->offset.x;
	sr.y += src->offset.y;

	/* fix negative spaces */
	if (sr.width < 0) {
		sr.x += sr.width;
		sr.width = 0 - sr.width;
	}
	if (sr.height < 0) {
		sr.y += sr.height;
		sr.height = 0 - sr.height;
	}

	/* destination is a window */

	dst_dc = graphics_extra(dst)->dc;

	if (src->win) {
		/* source is a window */
		src_dc = graphics_extra(src)->dc;

		/* clip source rectangle to window's boundary */
		clipped = app_clip_rect(app_get_window_area(src->win), sr);
		dp.x = dp.x + clipped.x - sr.x;
		dp.y = dp.y + clipped.y - sr.y;
		sr = clipped;
	}
	else if (src->bmap) {
		/* source is a bitmap */
		src_dc = graphics_extra(src)->dc;
		src_mask = bitmap_extra(src->bmap)->clipmask;

		/* clip source rectangle to bitmap's boundary */
		clipped = app_clip_rect(app_get_bitmap_area(src->bmap), sr);
		dp.x = dp.x + clipped.x - sr.x;
		dp.y = dp.y + clipped.y - sr.y;
		sr = clipped;
	}
	else {
		/* source is an image */
		temp = app_image_to_bitmap(dst->win, src->img);
		g = app_get_bitmap_graphics(temp);
		src_dc = graphics_extra(g)->dc;
		src_mask = bitmap_extra(temp)->clipmask;

		/* clip source rectangle to image's boundary */
		clipped = app_clip_rect(app_get_image_area(src->img), sr);
		dp.x = dp.x + clipped.x - sr.x;
		dp.y = dp.y + clipped.y - sr.y;
		sr = clipped;
	}

	/* set up clipping */
	if (dst->clip) {
		/* clip to the selected clipping region */
		num_rects = dst->clip->num_rects;
		rects = dst->clip->rects;
	}
	else {
		/* clip destination rectangle to window's boundary */
		num_rects = 1;
		rects = & dst->area;
	}

	/* copy clipped rectangles */

	dr = rect(dp.x, dp.y, sr.width, sr.height);

	if ((src_mask != 0) && (dst->xor_mode == 0))
	{
		/* src may have transparency (thus not a window) */
		/* so perform clipmasked copy area operation */

		src_mask_dc = CreateCompatibleDC(src_dc);
		if (! src_mask_dc)
			return 0;
		SelectObject(src_mask_dc, src_mask);

		for (i=0; i < num_rects; i++) {
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			/* two-stage bitblt: first, opaque pixels -> 0 */

			BitBlt(dst_dc,
				clipped.x, clipped.y,
				clipped.width, clipped.height,
				src_mask_dc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				SRCAND);

			/* then OR the opaque pixels with target */

			BitBlt(dst_dc,
				clipped.x, clipped.y,
				clipped.width, clipped.height,
				src_dc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				SRCPAINT);
		}
		DeleteDC(src_mask_dc);
	}
	else if ((src_dc == dst_dc) && (dst->xor_mode == 0))
	{
		/* source and destination are both the same window */
		HRGN rgn;
		RECT scroll;

		rgn = CreateRectRgn(0,0,0,0);

		for (i=0; i < num_rects; i++) {
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			scroll.left   = sr.x + clipped.x - dr.x;
			scroll.top    = sr.y + clipped.y - dr.y;
			scroll.right  = scroll.left + clipped.width;
			scroll.bottom = scroll.top + clipped.height;

			ScrollDC(dst_dc,
				clipped.x - scroll.left,
				clipped.y - scroll.top,
				&scroll, NULL, rgn, NULL);
			InvalidateRgn(win_extra(dst->win)->hwnd, rgn, 0);
		}

		UpdateWindow(win_extra(dst->win)->hwnd);
		DeleteObject(rgn);
	}
	else if ((src_mask == 0) && (dst->xor_mode == 1)) {
		/* using XOR mode with no clipmask, simple case */

		HBRUSH brush, old_br;
		Colour bg;

		bg = graphics_extra(dst)->bg;
		brush = CreateSolidBrush(PALETTERGB(bg.red, bg.green, bg.blue));
		old_br = SelectObject(dst_dc, brush);

		for (i=0; i < num_rects; i++) {
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			/* copy the whole source rectangle: */

			BitBlt(dst_dc,
				clipped.x, clipped.y,
				clipped.width, clipped.height,
				src_dc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				0x00960169);
		}

		SelectObject(dst_dc, old_br);
		DeleteObject(brush);
	}
	else if ((src_mask != 0) && (dst->xor_mode == 1)) {
		/* using XOR mode: use temporary bitmap */

		HDC temp_dc;
		HBITMAP temp_bm, old_bmp;
		HBRUSH brush, old_br;
		Colour bg;

		src_mask_dc = CreateCompatibleDC(src_dc);
		if (! src_mask_dc)
			return 0;
		SelectObject(src_mask_dc, src_mask);

		temp_bm = CreateCompatibleBitmap(dst_dc, sr.width, sr.height);
		temp_dc = CreateCompatibleDC(dst_dc);
		old_bmp = SelectObject(temp_dc, temp_bm);
		bg = graphics_extra(dst)->bg;
		brush = CreateSolidBrush(PALETTERGB(bg.red, bg.green, bg.blue));
		old_br = SelectObject(temp_dc, brush);

		for (i=0; i < num_rects; i++) {
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			/* copy the whole source rectangle: */

			BitBlt(temp_dc,
				0, 0,
				clipped.width, clipped.height,
				dst_dc,
				clipped.x,
				clipped.y,
				SRCCOPY); /* TEMP = D */
			BitBlt(temp_dc,
				0, 0,
				clipped.width, clipped.height,
				src_dc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				0x00960169); /* TEMP = BG==D?S:D==S?BG:D */
			BitBlt(temp_dc,
				0, 0,
				clipped.width, clipped.height,
				src_mask_dc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				0x00220326); /* TEMP = TEMP&~C  transparent pixels => 0 */
			BitBlt(dst_dc,
				clipped.x, clipped.y,
				clipped.width, clipped.height,
				src_mask_dc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				SRCAND); /* D = D&C  opaque pixels => 0 */
			BitBlt(dst_dc,
				clipped.x, clipped.y,
				clipped.width, clipped.height,
				temp_dc,
				0, 0,
				SRCPAINT); /* D = D|TEMP */
		}

		SelectObject(temp_dc, old_br);
		DeleteObject(brush);
		SelectObject(temp_dc, old_bmp);
		DeleteDC(temp_dc);
		DeleteObject(temp_bm);
		DeleteDC(src_mask_dc);
	}
	else {
		/* src is fully opaque: simple case */

		for (i=0; i < num_rects; i++) {
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			/* copy the whole source rectangle: */

			BitBlt(dst_dc,
				clipped.x, clipped.y,
				clipped.width, clipped.height,
				src_dc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				SRCCOPY);
		}
	}

	/* discard temporary bitmap */
	if (temp) {
		app_del_graphics(g);
		app_del_bitmap(temp);
	}

	return 1;
}

/*
 *  app_window_draw_utf8:
 *
 *  Draw the characters from a UTF-8 string, loading subfonts as needed.
 *
 *  This function implements client-side clipping to allow partial
 *  glyphs to be drawn.
 *
 *	Colour Pattern  P       1 1 1 1  0 0 0 0    (if XORing, P=fg^bg)
 *	Font Clipmask   C       1 1 0 0  1 1 0 0    (1=trans, 0=opaque)
 *	Destination     D       1 0 1 0  1 0 1 0    Windows mode name
 *	--------------- ----    ----------------    -----------------
 *	COPY     ((D^P)&C)^P    1 0 1 1  1 0 0 0    0x00B8074A (PSDPxax)
 *
 *	XOR      ((~C)&P)^D)    1 0 0 1  1 0 1 0    0x009A0709 (DPSnax)
 *
 *  Note: clipmasks are 1-bit-per-pixel bitmaps (monochrome).
 *  Windows translates such bitmaps as follows:
 *    0 becomes the current text colour
 *    1 becomes the current background colour
 *  This is a pain, because if the text colour is set to the
 *  drawing colour (P), columns 3 and 4 in the above table
 *  never occur when COPYing, since C is effectively 1111 1100
 *  instead of 1100 1100 (i.e. opaque clipmask pixels become P).
 *  To fix this problem, we only ever set a DC's text colour
 *  when using native fonts, and we only do it just before we
 *  need to do TextOut, and we set it back to the default
 *  colour immediately afterwards.
 *  The defaults for text colour and background colour
 *  are 0 and 1 (and we never change the background anyway)
 *  so normal use of clipmasks works just fine.
 */

int app_window_draw_utf8(Graphics *dst, Point dp, const char *s, int nbytes)
{
	int i, gw;
	int right_to_left;
	int num_rects;
	unsigned long ch;
	unsigned long *cp;
	const char *sp;
	const char *src_end;
	Rect dr, clipped;
	Rect *rects;
	Subfont *sub;
	Font *f;
	Rect gr;
	unsigned long mode;
	int temp_font = 0;
	char *temp_str = NULL;
	HDC src_dc;
	HDC dst_dc;
	HBITMAP sub_clip = 0;
	HBITMAP old_sub_clip = 0;
	int done = 0;

	if (dst->colour.alpha > 0x7F)
		return 1; /* nothing to draw if colour is transparent */

	/* correct drawing displacement */
	dp.x += dst->offset.x;
	dp.y += dst->offset.y;

	/* destination is a window */
	dst_dc = graphics_extra(dst)->dc;

	/* obtain font to use when drawing */
	f = dst->font;
	if (f == NULL) {
		if (dst->app == NULL)
			temp_font = 1;
		f = app_find_default_font(dst->app);
		app_set_font(dst, f);
	}
	if (f == NULL)
		return 0;

	/* correct drawing direction */
	if (dst->text_direction & RL_TB)
		right_to_left = 1;
	else
		right_to_left = 0;

	/* set up clipping */

	if (dst->clip) {
		/* clip to the selected clipping region */
		num_rects = dst->clip->num_rects;
		rects = dst->clip->rects;
		clipped = dst->clip->extents;
	}
	else {
		num_rects = 1;
		rects = & dst->area;
		clipped = dst->area;
	}

	/* check if we are completely outside the clipping region */

	if (dp.y + f->height <= clipped.y)
		done = 1;	/* completely above */
	else if (dp.y >= clipped.y + clipped.height)
		done = 1;	/* completely below */
	if (right_to_left) {
		if (dp.x <= clipped.x)
			done = 1;	/* completely to the left */
	}
	else {
		if (dp.x >= clipped.x + clipped.width)
			done = 1;	/* completely to the right */
	}
	if (done) {
		if (temp_font)
			app_del_font(f);
		return 1;
	}

	/* handle XOR mode */
	if (dst->xor_mode)
		mode = 0x009A0709L;	/* D -> D^S where font opaque */
	else
		mode = 0x00B8074AL;	/* D -> P where font opaque */

	/* if we are using a native font, draw the string specially */

	if (f->style & NATIVE_FONT) { /* assume ISO Latin-1 for now */
		HRGN rgn1, rgn2;
		HFONT oldfnt;
		SIZE size;

		/* convert UTF-8 to Latin-1 */
		if (! app_utf8_is_ascii(s, nbytes))
			s = temp_str = app_utf8_to_latin1(s, &nbytes);

		/* calculate bounding rectangle of string */
		oldfnt = SelectObject(dst_dc, font_extra(f)->fnt);
		GetTextExtentPoint32(dst_dc, s, nbytes, &size);
		gw = size.cx;
		gr.height = f->height;

		dr = rect(dp.x - gw/2, dp.y - gr.height/2, gw * 2, gr.height * 2);
		rgn1 = CreateRectRgn(dp.x, dp.y, dp.x, dp.y);

		/* for each clip rect, intersect with this rect */
		for (i=0; i < num_rects; i++) {
			/* clip to each rectangle separately and draw */
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			rgn2 = CreateRectRgn(clipped.x, clipped.y,
					clipped.x+clipped.width,
					clipped.y+clipped.height);
			CombineRgn(rgn1, rgn1, rgn2, RGN_OR);
			DeleteObject(rgn2);
		}

		/* set the clipping region */
		SelectClipRgn(dst_dc, rgn1);
		DeleteObject(rgn1);

		/* draw the string, clipped */
		if (! dst->xor_mode) {
			COLORREF oldclr;

			oldclr = SetTextColor(dst_dc, dst->pixval);
			TextOut(dst_dc, dp.x, dp.y, s, nbytes);
			SetTextColor(dst_dc, oldclr);
		}
		else {
			/* XOR mode requires drawing the text into a bitmap */
			/* then copying that bitmap to the destination */
			HBITMAP temp_bm;
			HDC temp_dc;
			HBITMAP old_bmp;
			HFONT old_fnt;

			temp_bm = CreateCompatibleBitmap(dst_dc, gw, gr.height);
			temp_dc = CreateCompatibleDC(dst_dc);
			old_bmp = SelectObject(temp_dc, temp_bm);
			old_fnt = SelectObject(temp_dc, font_extra(f)->fnt);
			PatBlt(temp_dc, 0, 0, gw, gr.height, WHITENESS);
			TextOut(temp_dc, 0, 0, s, nbytes);
			BitBlt(dst_dc, dp.x, dp.y, gw, gr.height,
				temp_dc, 0, 0, mode); /* XOR mode */
			SelectObject(temp_dc, old_fnt);
			SelectObject(temp_dc, old_bmp);
			DeleteDC(temp_dc);
			DeleteObject(temp_bm);
		}

		/* remove clipping and temporary settings */
		SelectClipRgn(dst_dc, NULL);
		SelectObject(dst_dc, oldfnt);

		/* discard temporary info */
		if (temp_str)
			app_free(temp_str);
		if (temp_font)
			app_del_font(f);

		return 1;
	}

	/* set up drawing */
	sp = s;
	src_end = s + nbytes;
	src_dc = CreateCompatibleDC(dst_dc);

	/* draw clipped glyphs */

	while (nbytes > 0) {
		cp = &ch;
		if (app_utf8_to_unicode(&sp, src_end, &cp, cp+1)
			& SourceExhausted)
			break;
		nbytes -= (int) (sp - s);
		s = sp;

		sub = app_font_char_info(f, ch, &gw);

		if ((sub == NULL) || (gw < 0)) {
			if (right_to_left)
				dp.x -= 6;

			/* character glyph not found, draw a box */
			app_draw_rect(dst, rect(dp.x - dst->offset.x + 1,
						dp.y - dst->offset.y + 1,
						4, f->height-2));

			if (! right_to_left)
				dp.x += 6;

			continue; /* go to next character */
		}
		/* else, character glyph exists */

		gr.width = sub->img->width / 32;
		gr.height = sub->img->height / 8;
		gr.x = (ch%32) * gr.width;
		gr.y = ((ch/32)%8) * gr.height;

		if (right_to_left)
			dp.x -= gw;

		if (gr.height > f->height) {
			/* glyphs are centered within each glyph box */
			dr.x = dp.x - (gr.width - gw) / 2;
			dr.y = dp.y - (gr.height - f->height) / 2;
		}
		else {
			/* glyphs are in the top left of each glyph box */
			dr.x = dp.x;
			dr.y = dp.y;
			gr.width = gw;
		}

		/* clip to the clipping region */

		dr.width = gr.width;
		dr.height = gr.height;
		for (i=0; i < num_rects; i++) {
			/* clip to each rectangle separately and draw */
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			if (sub_clip != subfont_extra(sub)->clipmask) {
				sub_clip = subfont_extra(sub)->clipmask;
				old_sub_clip = SelectObject(src_dc, sub_clip);
			}

			/* 0 -> current colour, 1 -> dest (transparent) */
			BitBlt(dst_dc, clipped.x, clipped.y,
				clipped.width, clipped.height,
				src_dc,
				gr.x + clipped.x - dr.x,
				gr.y + clipped.y - dr.y,
				mode);
		}

		if (! right_to_left)
			dp.x += gw;

		/* go to next character */
	}
	if (old_sub_clip)
		SelectObject(src_dc, old_sub_clip);
	DeleteDC(src_dc);
	if (temp_font)
		app_del_font(f);
	return 1;
}

/*
 *  app_window_draw_line:
 *
 *  Draw a line, using the current line width, in a window.
 *
 *  This function implements client-side clipping, using
 *  the Newman and Sproull clipping algorithm to clip the
 *  line against each rectangle in the clipping region
 *  (or just against the entire destination rectangle if no
 *  clipping region is given).
 *
 *  Line width is implemented by drawing multiple lines of
 *  width 1, side by side. This is not the fastest way,
 *  but is simple and allows clipping to work as expected.
 */
int app_window_draw_line(Graphics *dst, Point p1, Point p2)
{
	int i, num_rects;
	Rect *rects;
	int w, dx, dy;
	Point p3, p4;
	POINT p[3];
	HDC dst_dc;

	if (dst->colour.alpha > 0x7F)
		return 1; /* nothing to draw if colour is transparent */

	/* determine line distances */
	dx = p2.x - p1.x;
	dy = p2.y - p1.y;
	if (dx < 0)
		dx = -dx;
	if (dy < 0)
		dy = -dy;

	/* Use rectangles if they are faster than lines. */

	w = dst->line_width;

	if ((dx < w) || (dy < w))
		return app_portable_draw_line(dst, p1, p2);

	/* set up clipping */
	if (dst->clip) {
		/* clip to the selected clipping region */
		num_rects = dst->clip->num_rects;
		rects = dst->clip->rects;
	}
	else {
		/* clip destination rectangle to window's boundary */
		num_rects = 1;
		rects = & dst->area;
	}

	/* If any part of a thick line is clipped at right angles */
	/* then use the slow portable way of drawing lines. */
	/* Otherwise, clipping makes the lines slightly non-parallel, */
	/* leaving small gaps. */

	if (w > 1)
	{
	  if (dx > dy) {
		/* More horizontal than vertical, line hangs below. */
		/* The start and end of the line should be vertical. */

	    for (i=0; i < num_rects; i++) {
		if ((p1.y+dst->offset.y   < rects[i].y) ||
		    (p1.y+dst->offset.y+w > rects[i].y+rects[i].height) ||
			(p2.y+dst->offset.y   < rects[i].y) ||
		    (p2.y+dst->offset.y+w > rects[i].y+rects[i].height))
			return app_portable_draw_line(dst, p1, p2);
		}
	  }
	  else {
		/* More vertical than horizontal, line hangs to right. */
		/* The start and end of the line should be horizontal. */

	    for (i=0; i < num_rects; i++) {
		if ((p1.x+dst->offset.x   < rects[i].x) ||
			(p1.x+dst->offset.x+w > rects[i].x+rects[i].width) ||
		    (p2.x+dst->offset.x   < rects[i].x) ||
			(p2.x+dst->offset.x+w > rects[i].x+rects[i].width))
			return app_portable_draw_line(dst, p1, p2);
		}
	  }
	}

	/* correct drawing displacement */
	p1.x += dst->offset.x;
	p1.y += dst->offset.y;
	p2.x += dst->offset.x;
	p2.y += dst->offset.y;

	/* destination is a window */
	dst_dc = graphics_extra(dst)->dc;

	/* handle XOR mode */
	if (dst->xor_mode)
		SetROP2(dst_dc, R2_XORPEN);

	/* draw the clipped lines */

	/* special optimised case: if no clipping, draw and finish */
	if (w == 1) {
	    for (i=0; i < num_rects; i++) {
		if ((p1.x >= rects[i].x) &&
		    (p2.x >= rects[i].x) &&
		    (p1.y >= rects[i].y) &&
		    (p2.y >= rects[i].y) &&
		    (p1.x < rects[i].x+rects[i].width) &&
		    (p2.x < rects[i].x+rects[i].width) &&
		    (p1.y < rects[i].y+rects[i].height) &&
		    (p2.y < rects[i].y+rects[i].height))
		{
			p[0].x = p1.x;
			p[0].y = p1.y;
			p[1].x = p2.x;
			p[1].y = p2.y;
			p[2].x = p[1].x+1; /* to draw end pixel */
			p[2].y = p[1].y;
			Polyline(dst_dc, p, 3);
			return 1;
		}
	    }
	}

	if (dx > dy) {
	  /* More horizontal than vertical, line hangs below. */

	  for (w=0; w < dst->line_width; w++) {
		for (i=0; i < num_rects; i++) {
			p3.x = p1.x;
			p3.y = p1.y +w;
			p4.x = p2.x;
			p4.y = p2.y +w;

			if (app_clip_line_to_rect(rects[i], &p3, &p4)) {
				p[0].x = p3.x;
				p[0].y = p3.y;
				p[1].x = p4.x;
				p[1].y = p4.y;
				p[2].x = p[1].x+1; /* to draw end pixel */
				p[2].y = p[1].y;
				Polyline(dst_dc, p, 3);
			}
		}
	  }
	}
	else {
	  /* More vertical than horizontal, line hangs to right. */

	  for (w=0; w < dst->line_width; w++) {
		for (i=0; i < num_rects; i++) {
			p3.x = p1.x +w;
			p3.y = p1.y;
			p4.x = p2.x +w;
			p4.y = p2.y;

			if (app_clip_line_to_rect(rects[i], &p3, &p4)) {
				p[0].x = p3.x;
				p[0].y = p3.y;
				p[1].x = p4.x;
				p[1].y = p4.y;
				p[2].x = p[1].x+1; /* to draw end pixel */
				p[2].y = p[1].y;
				Polyline(dst_dc, p, 3);
			}
		}
	  }
	}

	if (dst->xor_mode)
		SetROP2(dst_dc, R2_COPYPEN);

	return 1;
}
