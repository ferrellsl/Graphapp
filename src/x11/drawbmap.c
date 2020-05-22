/*
 *  Drawing to bitmaps (essential operations).
 *
 *  Platform: X-Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Added native draw line interface.
 *  Version: 3.06  2001/10/30  Added native font interface.
 *  Version: 3.08  2001/11/11  Expanded native fonts to allow Latin1 text.
 *  Version: 3.21  2002/04/04  Fixed some memory leaks.
 *  Version: 3.22  2002/04/10  Fixed XOR mode copying of bitmaps.
 *  Version: 3.27  2002/08/14  Faster rendering of black/white text.
 *  Version: 3.48  2003/06/07  Fixed some memory leaks in draw_utf8.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
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
 *  to draw to bitmaps. These implement:
 *	app_fill_rect
 *	app_copy_rect
 *	app_draw_utf8
 *	app_draw_line
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
 *  we never rely on the server to do clipping.
 */
int app_bitmap_fill_rect(Graphics *dst, Rect r)
{
	int i, num_rects;
	Rect clipped;
	Rect *rects;
	Display *disp;
	XID dst_id;
	GC dst_gc;
	Pixmap dst_mask;
	GC dst_mask_gc;

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
	disp = app_extra(dst->app)->display;
	dst_id = bitmap_extra(dst->bmap)->handle;
	dst_gc = graphics_extra(dst)->gc;
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

	/* draw the clipped rectangles */

	if (dst_mask != None)
	{
		/* destination is partially transparent */

		dst_mask_gc = XCreateGC(disp, dst_mask, 0, NULL);
		if (! dst_mask_gc)
			return 0;

		/* opaque -> 1 (opaque) */
		XSetFunction(disp, dst_mask_gc, GXset);

		for (i=0; i < num_rects; i++) {
			clipped = app_clip_rect(r, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			/* fill pixels with colour */

			XFillRectangle(disp, dst_id, dst_gc,
				clipped.x, clipped.y,
				clipped.width, clipped.height);

			/* update transparency information */

			XFillRectangle(disp, dst_mask, dst_mask_gc,
				clipped.x, clipped.y,
				clipped.width, clipped.height);
		}
		XFreeGC(disp, dst_mask_gc);
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

			XFillRectangle(disp, dst_id, dst_gc,
				clipped.x, clipped.y,
				clipped.width, clipped.height);
		}
	}

	/* remove redundant destination clipmask, if any */
	if (dst_mask != None) {
		if (dst->clip == NULL) { /* if no clipping */
			if ((r.x == 0) && (r.y == 0) &&
			    (r.width == dst->area.width) &&
			    (r.height == dst->area.height)) /* all opaque */
			{
				XFreePixmap(disp, dst_mask);
				bitmap_extra(dst->bmap)->clipmask = None;
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
 *  The graphics server would do that clipping for us anyway, but
 *  doing it ourselves gains speed and portability.
 */
int app_bitmap_copy_rect(Graphics *dst, Point dp, Graphics *src, Rect sr)
{
	int i, num_rects;
	Rect dr, clipped;
	Rect *rects;
	Display *disp;
	XID dst_id;
	GC dst_gc;
	XID src_id;
	Pixmap src_mask = None;
	Bitmap *temp = NULL;
	Pixmap dst_mask;
	GC dst_mask_gc;

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
	disp = app_extra(dst->app)->display;
	dst_id = bitmap_extra(dst->bmap)->handle;
	dst_gc = graphics_extra(dst)->gc;
	dst_mask = bitmap_extra(dst->bmap)->clipmask;

	if (src->win) {
		/* source is a window */
		src_id = win_extra(src->win)->xid;

		/* clip source rectangle to window's boundary */
		clipped = app_clip_rect(app_get_window_area(src->win), sr);
		dp.x = dp.x + clipped.x - sr.x;
		dp.y = dp.y + clipped.y - sr.y;
		sr = clipped;
	}
	else if (src->bmap) {
		/* source is a bitmap */
		src_id = bitmap_extra(src->bmap)->handle;
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
		src_id = bitmap_extra(temp)->handle;
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

	/* copy clipped rectangles */

	dr = rect(dp.x, dp.y, sr.width, sr.height);

	if ((dst_mask != None) && (src_mask != None))
	{
		/* both src and dst may be transparent */
		/* perform clipmasked copy area operation */

		dst_mask_gc = XCreateGC(disp, dst_mask, 0, NULL);
		if (! dst_mask_gc)
			return 0;

		/* either 1 -> 1 (opaque) */
		XSetFunction(disp, dst_mask_gc, GXor);

		XSetClipMask(disp, dst_gc, src_mask);
		XSetClipOrigin(disp, dst_gc, dp.x, dp.y);

		/* handle XOR mode */
		if (dst->xor_mode) {
			/* copy image as BG^D^IMG to produce IMG where BG==D */
			XSetForeground(disp, dst_gc,
				graphics_extra(dst)->bgpixval);
		}

		for (i=0; i < num_rects; i++) {
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			/* handle XOR mode */

			if (dst->xor_mode)
				XFillRectangle(disp, dst_id, dst_gc,
					clipped.x, clipped.y,
					clipped.width, clipped.height);

			/* copy the pixels through a clipmask */

			XCopyArea(disp, src_id, dst_id, dst_gc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				clipped.width, clipped.height,
				clipped.x, clipped.y);

			/* merge opaqueness information */

			XCopyArea(disp, src_mask, dst_mask, dst_mask_gc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				clipped.width, clipped.height,
				clipped.x, clipped.y);
		}

		/* reset previous drawing colour */
		if (dst->xor_mode) {
			XSetForeground(disp, dst_gc,
				dst->pixval ^ graphics_extra(dst)->bgpixval);
		}

		/* tidy up */
		XSetClipMask(disp, dst_gc, None);
		XSetClipOrigin(disp, dst_gc, 0, 0);
		XFreeGC(disp, dst_mask_gc);
	}
	else if (dst_mask != None)
	{
		/* src is fully opaque, but dst is not */

		dst_mask_gc = XCreateGC(disp, dst_mask, 0, NULL);
		if (! dst_mask_gc)
			return 0;

		/* dst -> 1 (opaque) */
		XSetFunction(disp, dst_mask_gc, GXset);

		/* handle XOR mode */
		if (dst->xor_mode) {
			/* copy image as BG^D^IMG to produce IMG where BG==D */
			XSetForeground(disp, dst_gc,
				graphics_extra(dst)->bgpixval);
		}

		for (i=0; i < num_rects; i++) {
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			/* handle XOR mode */

			if (dst->xor_mode)
				XFillRectangle(disp, dst_id, dst_gc,
					clipped.x, clipped.y,
					clipped.width, clipped.height);

			/* copy the pixels */

			XCopyArea(disp, src_id, dst_id, dst_gc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				clipped.width, clipped.height,
				clipped.x, clipped.y);

			/* update dst's opaqueness information */
			
			XFillRectangle(disp, dst_mask, dst_mask_gc,
				clipped.x, clipped.y,
				clipped.width, clipped.height);
		}

		/* reset previous drawing colour */
		if (dst->xor_mode) {
			XSetForeground(disp, dst_gc,
				dst->pixval ^ graphics_extra(dst)->bgpixval);
		}

		/* tidy up */
		XFreeGC(disp, dst_mask_gc);
	}
	else if (src_mask != None)
	{
		/* src may have transparency */
		/* so perform clipmasked copy area operation */

		XSetClipMask(disp, dst_gc, src_mask);
		XSetClipOrigin(disp, dst_gc, dp.x, dp.y);

		/* handle XOR mode */
		if (dst->xor_mode) {
			/* copy image as BG^D^IMG to produce IMG where BG==D */
			XSetForeground(disp, dst_gc,
				graphics_extra(dst)->bgpixval);
		}

		for (i=0; i < num_rects; i++) {
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			/* handle XOR mode */

			if (dst->xor_mode)
				XFillRectangle(disp, dst_id, dst_gc,
					clipped.x, clipped.y,
					clipped.width, clipped.height);

			/* copy the pixels through a clipmask */

			XCopyArea(disp, src_id, dst_id, dst_gc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				clipped.width, clipped.height,
				clipped.x, clipped.y);
		}

		/* reset previous drawing colour */
		if (dst->xor_mode) {
			XSetForeground(disp, dst_gc,
				dst->pixval ^ graphics_extra(dst)->bgpixval);
		}

		/* tidy up */
		XSetClipMask(disp, dst_gc, None);
		XSetClipOrigin(disp, dst_gc, 0, 0);
	}
	else {
		/* src and dst are both fully opaque: simple case */

		/* handle XOR mode */
		if (dst->xor_mode) {
			/* copy image as BG^D^IMG to produce IMG where BG==D */
			XSetForeground(disp, dst_gc,
				graphics_extra(dst)->bgpixval);
		}

		for (i=0; i < num_rects; i++) {
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			/* handle XOR mode */

			if (dst->xor_mode)
				XFillRectangle(disp, dst_id, dst_gc,
					clipped.x, clipped.y,
					clipped.width, clipped.height);

			/* copy the pixels */

			XCopyArea(disp, src_id, dst_id, dst_gc,
				sr.x + clipped.x - dr.x,
				sr.y + clipped.y - dr.y,
				clipped.width, clipped.height,
				clipped.x, clipped.y);
		}

		/* reset previous drawing colour */
		if (dst->xor_mode) {
			XSetForeground(disp, dst_gc,
				dst->pixval ^ graphics_extra(dst)->bgpixval);
		}
	}

	/* remove redundant destination clipmask, if any */
	if ((dst_mask != None) && (src_mask == None)) {
		if (dst->clip == NULL) { /* if no clipping */
			if ((dp.x == 0) && (dp.y == 0) &&
			    (sr.width == dst->area.width) &&
			    (sr.height == dst->area.height)) /* all opaque */
			{
				XFreePixmap(disp, dst_mask);
				bitmap_extra(dst->bmap)->clipmask = None;
			}
		}
	}

	/* discard temporary bitmap */
	if (temp)
		app_del_bitmap(temp);

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
	Display *disp;
	Drawable dst_id;
	GC dst_gc;
	Rect gr;
	int temp_font = 0;
	char *temp_str = NULL;
	Drawable sub_clip = None;
	Drawable dst_mask = None;
	GC dst_mask_gc = NULL;
	int done = 0;

	if (dst->colour.alpha > 0x7F)
		return 1; /* nothing to draw if colour is transparent */

	/* correct drawing displacement */
	dp.x += dst->offset.x;
	dp.y += dst->offset.y;

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

	/* destination is a bitmap */
	disp = app_extra(dst->app)->display;
	dst_id = bitmap_extra(dst->bmap)->handle;
	dst_gc = graphics_extra(dst)->gc;
	if (bitmap_extra(dst->bmap)->clipmask != None) {
		dst_mask = bitmap_extra(dst->bmap)->clipmask;
		dst_mask_gc = XCreateGC(disp, dst_mask, 0, NULL);
		if (dst_mask_gc) {
			/* opaque -> 1 */
			XSetFunction(disp, dst_mask_gc, GXset);
		}
	}

	/* set up drawing */

	sp = s;
	src_end = s + nbytes;

	/* if we are using a native font, draw the string specially */

	if (f->style & NATIVE_FONT) /* assume ISO Latin-1 for now */
	{
		Region *rgn;
		XRectangle *x_rects;

		/* convert UTF-8 to Latin-1 */
		if (! app_utf8_is_ascii(s, nbytes))
			s = temp_str = app_utf8_to_latin1(s, &nbytes);

		/* calculate bounding rectangle of string */
		gr.height = f->height;
		gw = XTextWidth(font_extra(f)->fnt, s, nbytes);

		dr = rect(dp.x - gw/2, dp.y - gr.height/2, gw * 2, gr.height * 2);

		rgn = app_new_region();

		/* for each clip rect, intersect with this rect */
		for (i=0; i < num_rects; i++) {
			/* clip to each rectangle separately and draw */
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			app_union_region_with_rect(rgn, clipped, rgn);
		}

		/* set the clipping region */
		x_rects = app_alloc(sizeof(XRectangle) * rgn->num_rects);
		for (i=0; i < rgn->num_rects; i++) {
			x_rects[i].x = rgn->rects[i].x;
			x_rects[i].y = rgn->rects[i].y;
			x_rects[i].width  = rgn->rects[i].width;
			x_rects[i].height = rgn->rects[i].height;
		}
		XSetClipRectangles(disp, dst_gc, 0, 0,
				x_rects, rgn->num_rects, YXSorted);
		if (dst_mask_gc)
			XSetClipRectangles(disp, dst_mask_gc, 0, 0,
				x_rects, rgn->num_rects, YXSorted);
		app_free(x_rects);
		app_del_region(rgn);

		/* draw the string, clipped */
		XDrawString(disp, dst_id, dst_gc,
				dp.x, dp.y + font_extra(f)->fnt->ascent,
				s, nbytes);
		if (dst_mask_gc)
			XDrawString(disp, dst_mask, dst_mask_gc,
				dp.x, dp.y + font_extra(f)->fnt->ascent,
				s, nbytes);

		/* remove clipping */
		XSetClipMask(disp, dst_gc, None);

		/* discard temporary mask GC */
		if (dst_mask_gc)
			XFreeGC(disp, dst_mask_gc);

		/* discard temporary info */
		if (temp_str)
			app_free(temp_str);
		if (temp_font)
			app_del_font(f);

		return 1;
	}

	/* Special case: if drawing black or white text
	 *  (with no XOR mode), we may be able to avoid
	 *  clipmasking by clever use of bitwise blitting modes.
	 *  The clipmask is actually blitted to the destination.
	 *  This seems faster than using XSetClipMask to stencil
	 *  text onto the window.
	 */

	if ((dst->xor_mode == 0) &&
		((dst->pixval == 0) ||
		 (dst->pixval == (0xFFFFFFFFUL >>
		  (32-DefaultDepth(disp, DefaultScreen(disp))) ) )) )
	{
		/* clipmasks are arranged in X-Windows such that
		 * 1 means set the pixel to the FG colour and
		 * 0 means leave it alone. So a text glyph is 1's
		 * on a background of 0's.
		 * XCopyPlane automatically maps SRC=1 to FG and
		 * SRC=0 to BG.
		 * So, if drawing FG pixval 0 (black), we make
		 * BG=~0 (ones) and use AND mode to make pixels 0.
		 * Else, if drawing with FG=~0 (white), we make
		 * BG=0 and use OR mode to make pixels ~0.
		 */
		if (dst->pixval == 0) {
			XSetBackground(disp, dst_gc, ~0L);
			XSetFunction(disp, dst_gc, GXand);
		}
		else {
			XSetBackground(disp, dst_gc, 0L);
			XSetFunction(disp, dst_gc, GXor);
		}

		while (nbytes > 0) {
			cp = &ch;
			if (app_utf8_to_unicode(&sp, src_end, &cp, cp+1)
			    & SourceExhausted)
				break;
			nbytes -= (sp - s);
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

			/* set up bitblt drawing from the clipmask */

			sub_clip = subfont_extra(sub)->clipmask;

			/* blit from the mask to the clipping region */

			dr.width = gr.width;
			dr.height = gr.height;
			for (i=0; i < num_rects; i++) {
				/* clip to each rectangle separately and draw */
				clipped = app_clip_rect(dr, rects[i]);
				if (clipped.width == 0)
					continue; /* nothing visible here */
				if (clipped.height == 0)
					continue; /* nothing visible here */

				XCopyPlane(disp, sub_clip, dst_id, dst_gc,
					clipped.x - dr.x + gr.x,
					clipped.y - dr.y + gr.y,
					clipped.width, clipped.height,
					clipped.x, clipped.y,
					1UL);
			}

			if (! right_to_left)
				dp.x += gw;

			/* go to next character */
		}
		if (dst_mask_gc)
			XFreeGC(disp, dst_mask_gc);
		if (temp_font)
			app_del_font(f);

		XSetFunction(disp, dst_gc, GXcopy); /* restore mode */
		return 1;
	}

	/* draw clipped glyphs */

	while (nbytes > 0) {
		cp = &ch;
		if (app_utf8_to_unicode(&sp, src_end, &cp, cp+1)
		    & SourceExhausted)
			break;
		nbytes -= (sp - s);
		s = sp;

		sub = app_font_char_info(f, ch, &gw);

		if ((sub == NULL) || (gw < 0)) {
			if (right_to_left)
				dp.x -= 6;

			/* character glyph not found, draw a box */
			XSetClipMask(disp, dst_gc, None);
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

		/* set up stencil drawing from the clipmask */

		if (sub_clip != subfont_extra(sub)->clipmask) {
			sub_clip = subfont_extra(sub)->clipmask;
			XSetClipMask(disp, dst_gc, sub_clip);
			if (dst_mask_gc)
				XSetClipMask(disp, dst_mask_gc, sub_clip);
		}
		XSetClipOrigin(disp, dst_gc, dp.x - gr.x, dp.y - gr.y);
		if (dst_mask_gc)
			XSetClipOrigin(disp, dst_mask_gc, dp.x - gr.x, dp.y - gr.y);

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

			XFillRectangle(disp, dst_id, dst_gc,
				clipped.x, clipped.y,
				clipped.width, clipped.height);

			if (dst_mask_gc) {
				/* update transparency information */
				XFillRectangle(disp, dst_mask,
					dst_mask_gc,
					clipped.x, clipped.y,
					clipped.width, clipped.height);
			}
		}

		if (! right_to_left)
			dp.x += gw;

		/* go to next character */
	}
	XSetClipOrigin(disp, dst_gc, 0, 0);
	XSetClipMask(disp, dst_gc, None);
	if (dst_mask_gc)
		XFreeGC(disp, dst_mask_gc);
	if (temp_font)
		app_del_font(f);
	return 1;
}

/*
 *  app_bitmap_draw_line:
 *
 *  Draw a line, using the current line width, in a bitmap.
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
int app_bitmap_draw_line(Graphics *dst, Point p1, Point p2)
{
	int i, num_rects;
	Rect *rects;
	int w, dx, dy;
	Point p3, p4;
	Display *disp;
	XID dst_id;
	GC dst_gc;
	Pixmap dst_mask;
	GC dst_mask_gc;

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

	/* destination is a bitmap */
	disp = app_extra(dst->app)->display;
	dst_id = bitmap_extra(dst->bmap)->handle;
	dst_gc = graphics_extra(dst)->gc;
	dst_mask = bitmap_extra(dst->bmap)->clipmask;

	if (dst_mask != None)
	{
		/* destination is partially transparent */

		dst_mask_gc = XCreateGC(disp, dst_mask, 0, NULL);
		if (! dst_mask_gc)
			return 0;

		/* opaque -> 1 (opaque) */
		XSetFunction(disp, dst_mask_gc, GXset);
	}
	else {
		dst_mask_gc = NULL;
	}

	/* draw the clipped lines */

	if (dx > dy) {
	  /* more horizontal than vertical, line hangs below */

	  for (w=0; w < dst->line_width; w++) {
		for (i=0; i < num_rects; i++) {
			p3.x = p1.x;
			p3.y = p1.y +w;
			p4.x = p2.x;
			p4.y = p2.y +w;

			if (app_clip_line_to_rect(rects[i], &p3, &p4)) {
				XDrawLine(disp, dst_id, dst_gc,
					p3.x, p3.y, p4.x, p4.y);

				if (dst_mask_gc) /* update transparency */
					XDrawLine(disp, dst_mask, dst_mask_gc,
						p3.x, p3.y, p4.x, p4.y);
			}
		}
	  }
	}
	else {
	  /* more vertical than horizontal, line hangs to right */

	  for (w=0; w < dst->line_width; w++) {
		for (i=0; i < num_rects; i++) {
			p3.x = p1.x +w;
			p3.y = p1.y;
			p4.x = p2.x +w;
			p4.y = p2.y;

			if (app_clip_line_to_rect(rects[i], &p3, &p4)) {
				XDrawLine(disp, dst_id, dst_gc,
					p3.x, p3.y, p4.x, p4.y);

				if (dst_mask_gc) /* update transparency */
					XDrawLine(disp, dst_mask, dst_mask_gc,
						p3.x, p3.y, p4.x, p4.y);
			}
		}
	  }
	}

	if (dst_mask_gc)
		XFreeGC(disp, dst_mask_gc);

	return 1;
}
