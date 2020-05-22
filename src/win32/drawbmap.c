/*
 *  Drawing to bitmaps (essential operations).
 *
 *  Platform: Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/17  Added native font rendering.
 *  Version: 3.06  2001/10/30  Improved temporary font usage.
 *  Version: 3.08  2001/11/11  Expanded native fonts to allow Latin1 text.
 *  Version: 3.21  2002/04/04  Fixed memory leaks and XOR mode problems.
 *  Version: 3.48  2003/06/07  Fixed some memory leaks in draw_utf8.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced some size_t conversion warnings.
 *  Version: 3.59  2005/10/10  Supports over-sized glyphs.
 *  Version: 3.62  2010/01/10  Native font drawing supports wider glyphs.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

/*
 *  This file implements the essential drawing opertions needed
 *  to draw to bitmaps. These implement:
 *	app_fill_rect
 *	app_copy_rect
 *	app_draw_utf8
 *
 *  Read drawwin.c before trying to understand this file, since
 *  bitmaps can have transparency clipmasks, which involve more
 *  complicated cases than drawing to windows (which are fully
 *  opaque).
 */


/*
 *  app_bitmap_fill_rect:
 *
 *  Fill a rectangle with colour, in a bitmap.
 *
 *  If the destination is a partially transparent bitmap, the
 *  transparency information must be updated so the pixels
 *  we've just changed become opaque.
 *
 *  This function implements client-side clipping, so that
 *  we never rely on the GDI system to do clipping.
 */
int app_bitmap_fill_rect(Graphics *dst, Rect r)
{
	int i, num_rects;
	Rect clipped;
	Rect *rects;
	unsigned long mode;
	HDC dst_dc, dst_mask_dc;
	HBITMAP dst_mask = 0;

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

	/* destination is a bitmap */

	dst_dc = graphics_extra(dst)->dc;
	dst_mask = bitmap_extra(dst->bmap)->clipmask;

	/* set up clipping */
	if (dst->clip) {
		/* clip to the selected clipping region */
		num_rects = dst->clip->num_rects;
		rects = dst->clip->rects;
	}
	else {
		num_rects = 1;
		rects = & dst->area;
	}

	/* handle XOR mode */
	if (dst->xor_mode)
		mode = PATINVERT;
	else
		mode = PATCOPY;

	/* draw the clipped rectangles */

	if (dst_mask != 0)
	{
		/* destination is partially transparent */

		dst_mask_dc = CreateCompatibleDC(dst_dc);
		if (! dst_mask_dc)
			return 0;
		SelectObject(dst_mask_dc, dst_mask);

		for (i=0; i < num_rects; i++) {
			clipped = app_clip_rect(r, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			/* fill pixels with colour */

			PatBlt(dst_dc, clipped.x, clipped.y,
				clipped.width, clipped.height, mode);

			/* update transparency information */

			PatBlt(dst_mask_dc, clipped.x, clipped.y,
				clipped.width, clipped.height, BLACKNESS);
		}
		DeleteDC(dst_mask_dc);
	}
	else {
		/* destination is fully opaque: simple case */

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
	}

	/* remove redundant destination clipmask, if any */
	if (dst_mask != 0) {
		if (dst->clip == NULL) { /* if no clipping */
			if ((r.x == 0) && (r.y == 0) &&
			    (r.width == dst->area.width) &&
			    (r.height == dst->area.height)) /* all opaque */
			{
				DeleteObject(dst_mask);
				bitmap_extra(dst->bmap)->clipmask = 0;
			}
		}
	}

	return 1;
}

/*
 *  app_bitmap_copy_rect:
 *
 *  Copy an area from a window or bitmap into a bitmap.
 *
 *  There are a number of special cases to handle here.
 *  If the source is a partially transparent bitmap (has a clipmask)
 *  only the opaque pixels should be copied. If the destination
 *  is a partially transparent bitmap, its clipmask must become
 *  opaque wherever we've just drawn opaque pixels.
 *
 *  If the destination was semi-transparent, and has now been made
 *  fully opaque, it would be nice to delete the clipmask bitmap,
 *  since it is now redundant. The general way to do this is to
 *  read the bits out of the clipmask and check they are all now
 *  signalling opaqueness, but this is clearly too slow. So this
 *  function just implements one special case: if the source is
 *  fully opaque, and the destination rectangle is the entire
 *  destination, delete the destination's clipmask.
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
 *  MASK stage 1    C&D     1 0 1 0  0 0 0 0    SRCAND    (0x008800C6)
 *  MASK stage 2    S|D     1 0 1 0  1 1 0 0    SRCPAINT  (0x00EE0086)
 *
 *  COPY (no C)     S       X X X X  1 1 0 0    SRCCOPY   (0x00CC0020)
 *
 *  XOR (if no C)           1 0 0 1  0 1 1 0    D^S^BG    (0x00960169)
 *
 *  XOR (if C)      T=D                         SRCCOPY
 *      stage 2    T'=XOR   1 0 0 1  0 1 1 0    D^S^BG    (0x00960169)
 *      stage 3    T"=T'&~C 0 0 0 0 unchanged   T^~C      (0x00220326)
 *      stage 4    D'=D&C  unchanged 0 0 0 0    SRCAND
 *      stage 5    D"=T"|D'                     SRCPAINT
 */

/*
 *  Windows notes on app_bitmap_copy_rect:
 *
 *  If the source is a bitmap which has no clipmask, or a
 *  window, we can just use BitBlt with a mode of SRCCOPY,
 *  which overwrites the destination with the source pixels.
 *  If using XOR mode, just use the mode SRCINVERT (S^D).
 *
 *  To copy a bitmap which has a clipmask (transparency),
 *  do this:
 *
 *  Assume the 1-bpp clipmask has 1's for transparent, and
 *  0's for opaque.
 *
 *  Assume the bitmap has 0's in transparent areas. There can
 *  be 0's elsewhere too; it's the combination of 1's in the
 *  clipmask and 0's in the bitmap that is indicates transparency.
 *
 *  First, BitBlt the clipmask into the destination,
 *  using a mode of SRCAND (S & D) which turns all
 *  pixels where the clipmask has 0's (opaque) into 0's
 *  (since 0 & D == 0, and 1 & D == D).
 *  All other pixels will remains unchanged in D.
 *
 *  Then BitBlt the bitmap into the destination,
 *  using a mode of SRCPAINT (S | D) which does this:
 *  all the pixels in D which were just changed to 0
 *  will now be ORed with data from the bitmap.
 *  The bitmap has 0's for transparent areas (note this is
 *  opposite to the clipmask which uses 1's for transparency).
 *  Because the bitmap has 0's for transparent, the OR
 *  operation doesn't affect anything which is transparent.
 *  Only the pixels which were just changed to 0's will
 *  now become the opaque pixels from the bitmap.
 *
 *  If the destination is a bitmap which has no clipmask,
 *  this suffices, since the destination is fully opaque, so
 *  no clipmask needs to be produced. If, on the other hand,
 *  the destination also has a clipmask, we must modify that
 *  clipmask to have 0's (opaque) wherever we've just written
 *  data. This amounts to a final BitBlt, copying the source
 *  clipmask into the destination clipmask with a mode of
 *  SRCAND (S & D), since this only produces 1's (transparent)
 *  where both source and dest clipmasks are 1 (transparent);
 *  all else becomes 0 (opaque).
 *
 *  There is one other possibility; that the destination has
 *  a clipmask, but the source does not. In that case we use
 *  a BitBlt with a mode of BLACKNESS to fill the correct area
 *  in the destination clipmask with 0's (opaque).
 */
int app_bitmap_copy_rect(Graphics *dst, Point dp, Graphics *src, Rect sr)
{
	int i, num_rects;
	Rect dr, clipped;
	Rect *rects;
	unsigned long mode;
	HDC src_dc, src_mask_dc;
	HDC dst_dc, dst_mask_dc;
	HBITMAP src_mask = 0;
	HBITMAP dst_mask = 0;
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

	/* destination is a bitmap */

	dst_dc = graphics_extra(dst)->dc;
	dst_mask = bitmap_extra(dst->bmap)->clipmask;

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
		temp = app_image_to_bitmap(dst->bmap->win, src->img);
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
		num_rects = 1;
		rects = & dst->area;
	}

	/* handle XOR mode */
	if (dst->xor_mode)
		mode = 0x00960169;	/* S xor D xor BG */
	else
		mode = SRCCOPY;		/* S = copy pixels */

	/* copy clipped rectangles */

	dr = rect(dp.x, dp.y, sr.width, sr.height);

	if ((dst_mask == 0) && (src_mask == 0) && (dst->xor_mode == 0))
	{
		/* src and dst are both fully opaque: simple case */

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
				mode);
		}
	}
	else if ((src_mask == 0) && (dst->xor_mode == 0))
	{
		/* src is fully opaque, but dst has a clipmask */

		dst_mask_dc = CreateCompatibleDC(dst_dc);
		if (! dst_mask_dc)
			return 0;
		SelectObject(dst_mask_dc, dst_mask);

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
				mode);

			/* set opaque areas in destination mask to 0: */

			BitBlt(dst_mask_dc,
				clipped.x, clipped.y,
				clipped.width, clipped.height,
				dst_mask_dc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				BLACKNESS);
		}
		DeleteDC(dst_mask_dc);
	}
	else if ((dst_mask == 0) && (dst->xor_mode == 0))
	{
		/* src may have transparency, but dst is opaque */
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

			/* two-stage bitblt: */

			BitBlt(dst_dc,
				clipped.x, clipped.y,
				clipped.width, clipped.height,
				src_mask_dc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				SRCAND);

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
	else if (dst->xor_mode == 0)
	{
		/* both src and dst may be transparent (have clipmasks) */
		/* perform clipmasked copy area operation */

		src_mask_dc = CreateCompatibleDC(src_dc);
		if (! src_mask_dc)
			return 0;

		dst_mask_dc = CreateCompatibleDC(dst_dc);
		if (! dst_mask_dc) {
			DeleteDC(src_mask_dc);
			return 0;
		}

		SelectObject(src_mask_dc, src_mask);
		SelectObject(dst_mask_dc, dst_mask);

		for (i=0; i < num_rects; i++) {
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			/* two-stage bitblt: */

			BitBlt(dst_dc,
				clipped.x, clipped.y,
				clipped.width, clipped.height,
				src_mask_dc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				SRCAND);

			BitBlt(dst_dc,
				clipped.x, clipped.y,
				clipped.width, clipped.height,
				src_dc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				SRCPAINT);

			/* intersect transparency information: */

			BitBlt(dst_mask_dc,
				clipped.x, clipped.y,
				clipped.width, clipped.height,
				src_mask_dc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				SRCAND);
		}
		DeleteDC(dst_mask_dc);
		DeleteDC(src_mask_dc);
	}
	else if ((dst_mask == 0) && (src_mask == 0) && (dst->xor_mode == 1)) {
		/* using XOR mode with no clipmasks, simple case */

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
	else if ((src_mask == 0) && (dst->xor_mode == 1)) {
		/* using XOR mode: src is opaque but dsk has clipmask */

		HBRUSH brush, old_br;
		Colour bg;

		dst_mask_dc = CreateCompatibleDC(dst_dc);
		if (! dst_mask_dc)
			return 0;
		SelectObject(dst_mask_dc, dst_mask);

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

			/* set opaque areas in destination mask to 0: */

			BitBlt(dst_mask_dc,
				clipped.x, clipped.y,
				clipped.width, clipped.height,
				dst_mask_dc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				BLACKNESS);
		}
		SelectObject(dst_dc, old_br);
		DeleteObject(brush);

		DeleteDC(dst_mask_dc);
	}
	else if ((dst_mask == 0) && (dst->xor_mode == 1)) {
		/* using XOR mode: src has clipmask, dst is opaque */
		/* use temporary bitmap */

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
	else if (dst->xor_mode == 1) {
		/* using XOR mode: src has clipmask, dst has clipmask */
		/* use temporary bitmap */

		HDC temp_dc;
		HBITMAP temp_bm, old_bmp;
		HBRUSH brush, old_br;
		Colour bg;

		src_mask_dc = CreateCompatibleDC(src_dc);
		if (! src_mask_dc)
			return 0;

		dst_mask_dc = CreateCompatibleDC(dst_dc);
		if (! dst_mask_dc)
			return 0;

		SelectObject(src_mask_dc, src_mask);
		SelectObject(dst_mask_dc, dst_mask);

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

			/* intersect transparency information: */

			BitBlt(dst_mask_dc,
				clipped.x, clipped.y,
				clipped.width, clipped.height,
				src_mask_dc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				SRCAND);
		}

		SelectObject(temp_dc, old_br);
		DeleteObject(brush);
		SelectObject(temp_dc, old_bmp);
		DeleteDC(temp_dc);
		DeleteObject(temp_bm);

		DeleteDC(dst_mask_dc);
		DeleteDC(src_mask_dc);
	}

	/* remove redundant destination clipmask, if any */
	if ((dst_mask != 0) && (src_mask == 0)) {
		if (dst->clip == NULL) { /* if no clipping */
			if ((dp.x == 0) && (dp.y == 0) &&
			    (sr.width == dst->area.width) &&
			    (sr.height == dst->area.height)) /* all opaque */
			{
				DeleteObject(dst_mask);
				bitmap_extra(dst->bmap)->clipmask = 0;
			}
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
 *  app_bitmap_draw_utf8:
 *
 *  Draw the characters from a UTF-8 string, loading subfonts as needed.
 *
 *  This function implements client-side clipping to allow partial
 *  glyphs to be drawn, and also updates bitmap transparency
 *  information.
 *
 *	Colour Pattern  P       1 1 1 1  0 0 0 0    (if XORing, P=fg^bg)
 *	Font Clipmask   C       1 1 0 0  1 1 0 0    (1=trans, 0=opaque)
 *	Destination     D       1 0 1 0  1 0 1 0    Windows mode name
 *	--------------- ----    ----------------    -----------------
 *	COPY     ((D^P)&C)^P    1 0 1 1  1 0 0 0    0x00B8074A (PSDPxax)
 *
 *	XOR      ((~C)&P)^D)    1 0 0 1  1 0 1 0    0x009A0709 (DPSnax)
 *
 */
int app_bitmap_draw_utf8(Graphics *dst, Point dp, const char *s, int nbytes)
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
	HDC dst_dc, dst_mask_dc = 0;
	HBITMAP dst_mask = 0;
	HBITMAP sub_clip = 0;
	HBITMAP old_dst_mask = 0;
	HBITMAP old_sub_clip = 0;
	int done = 0;

	if (dst->colour.alpha > 0x7F)
		return 1; /* nothing to draw if colour is transparent */

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

	/* correct drawing displacement */
	dp.x += dst->offset.x;
	dp.y += dst->offset.y;

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

	/* destination is a bitmap */
	dst_dc = graphics_extra(dst)->dc;
	if (bitmap_extra(dst->bmap)->clipmask != 0) {
		dst_mask = bitmap_extra(dst->bmap)->clipmask;
		dst_mask_dc = CreateCompatibleDC(dst_dc);
		old_dst_mask = SelectObject(dst_mask_dc, dst_mask);
	}

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

		dr = rect(dp.x - gw, dp.y - gr.height, gw * 3, gr.height * 3);
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

		/* clean up */
		if (dst_mask_dc) {
			SetBkMode(dst_mask_dc, 1);
			TextOut(dst_mask_dc, dp.x, dp.y, s, nbytes);

			SelectObject(dst_mask_dc, old_dst_mask);
			DeleteDC(dst_mask_dc);
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

			if (dst_mask_dc) {
				/* update transparency information */
				/* 0 -> 0, 1 -> no change */
				BitBlt(dst_mask_dc, clipped.x, clipped.y,
					clipped.width, clipped.height,
					src_dc,
					gr.x + clipped.x - dr.x,
					gr.y + clipped.y - dr.y,
					SRCAND);
			}
		}

		if (! right_to_left)
			dp.x += gw;

		/* go to next character */
	}
	if (old_sub_clip)
		SelectObject(src_dc, old_sub_clip);
	DeleteDC(src_dc);
	if (dst_mask_dc) {
		SelectObject(dst_mask_dc, old_dst_mask);
		DeleteDC(dst_mask_dc);
	}
	if (temp_font)
		app_del_font(f);
	return 1;
}

