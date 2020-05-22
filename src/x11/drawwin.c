/*
 *  Drawing to windows (essential operations).
 *
 *  Platform: X-Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Added native draw line interface.
 *  Version: 3.02  2001/10/10  Added native font interface.
 *  Version: 3.06  2001/10/30  Improved temporary font usage.
 *  Version: 3.08  2001/11/11  Expanded native fonts to allow Latin1 text.
 *  Version: 3.21  2002/04/04  Fixed some memory leaks.
 *  Version: 3.22  2002/04/10  Fixed XOR mode copying of bitmaps.
 *  Version: 3.25  2002/07/07  Faster rendering of black/white text.
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
 *  we never rely on the server to do clipping, except if
 *  the destination is a window which is partially obscured.
 *  In that situation we must rely on the server because there
 *  is no way for the program to know which portions of the
 *  window are currently obscured.
 */
int app_window_fill_rect(Graphics *dst, Rect r)
{
	int i, num_rects;
	Rect clipped;
	Rect *rects;
	Display *disp;
	XID dst_id;
	GC dst_gc;

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
	disp = app_extra(dst->app)->display;
	dst_id = win_extra(dst->win)->xid;
	dst_gc = graphics_extra(dst)->gc;

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

	/* draw the clipped rectangles */

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

	return 1;
}

/*
 *  app_window_copy_rect:
 *
 *  Copy an area from a window or bitmap into a window.
 *
 *  Special case: If the source is a partially transparent bitmap
 *  (has a clipmask) only the opaque pixels should be copied.
 *
 *  This function implements client-side clipping; that is,
 *  it looks through the destination clipping region and only
 *  draws where that region allows. If there is no region, it
 *  just clips the output against the rectangle of the destination.
 *  The graphics server would do that clipping for us anyway, but
 *  doing it ourselves gains speed and portability.
 */
int app_window_copy_rect(Graphics *dst, Point dp, Graphics *src, Rect sr)
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
	disp = app_extra(dst->app)->display;
	dst_id = win_extra(dst->win)->xid;
	dst_gc = graphics_extra(dst)->gc;

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
		temp = app_image_to_bitmap(dst->win, src->img);
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
		/* clip destination rectangle to window's boundary */
		num_rects = 1;
		rects = & dst->area;
	}

	/* copy pixels */

	dr = rect(dp.x, dp.y, sr.width, sr.height);

	if (dst->xor_mode) {
		/* xor the dest with the XOR background pixel value */
		/* this makes the final image BG^D^IMG which should */
		/* be just IMG if BG==D */

		XSetForeground(disp, dst_gc, graphics_extra(dst)->bgpixval);
	}
	if (src_mask != None) {
		/* src may have transparency, so use a clipmask */

		XSetClipMask(disp, dst_gc, src_mask);
		XSetClipOrigin(disp, dst_gc, dp.x, dp.y);
	}
	if (src_id != None) {
		/* copy clipped rectangles */

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
	}
	if (src_mask != None) {
		/* remove clipmasking */

		XSetClipMask(disp, dst_gc, None);
		XSetClipOrigin(disp, dst_gc, 0, 0);
	}
	if (dst->xor_mode) {
		/* reset previous drawing colour */

		XSetForeground(disp, dst_gc,
			dst->pixval ^ graphics_extra(dst)->bgpixval);
	}

	/* discard temporary bitmap */
	if (temp)
		app_del_bitmap(temp);

	/* handle GraphicsExpose events before other events */
	if (src->win)
		win_extra(dst->win)->exposed = 100;

	return 1;
}

/*
 *  app_window_draw_utf8:
 *
 *  Draw the characters from a UTF-8 string, loading subfonts as needed.
 *
 *  This function implements client-side clipping to allow partial
 *  glyphs to be drawn.
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
	Display *disp;
	Drawable dst_id;
	GC dst_gc;
	Rect gr;
	int temp_font = 0;
	char *temp_str = NULL;
	Drawable sub_clip = None;
	int done = 0;

	if (dst->colour.alpha > 0x7F)
		return 1; /* nothing to draw if colour is transparent */

	/* correct drawing displacement */
	dp.x += dst->offset.x;
	dp.y += dst->offset.y;

	/* destination is a window */
	disp = app_extra(dst->app)->display;
	dst_id = win_extra(dst->win)->xid;
	dst_gc = graphics_extra(dst)->gc;

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
		/* clip destination rectangle to window's boundary */
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
		app_free(x_rects);
		app_del_region(rgn);

		/* draw the string, clipped */
		XDrawString(disp, dst_id, dst_gc,
				dp.x, dp.y + font_extra(f)->fnt->ascent,
				s, nbytes);

		/* remove clipping */
		XSetClipMask(disp, dst_gc, None);

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
		}
		XSetClipOrigin(disp, dst_gc, dp.x - gr.x, dp.y - gr.y);

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
		}

		if (! right_to_left)
			dp.x += gw;

		/* go to next character */
	}
	XSetClipOrigin(disp, dst_gc, 0, 0);
	XSetClipMask(disp, dst_gc, None);
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
	Display *disp;
	XID dst_id;
	GC dst_gc;

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
	disp = app_extra(dst->app)->display;
	dst_id = win_extra(dst->win)->xid;
	dst_gc = graphics_extra(dst)->gc;

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
		    (p2.y < rects[i].y+rects[i].height)) {
			XDrawLine(disp, dst_id, dst_gc,
					p1.x, p1.y, p2.x, p2.y);
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

			if (app_clip_line_to_rect(rects[i], &p3, &p4))
				XDrawLine(disp, dst_id, dst_gc,
					p3.x, p3.y, p4.x, p4.y);
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

			if (app_clip_line_to_rect(rects[i], &p3, &p4))
				XDrawLine(disp, dst_id, dst_gc,
					p3.x, p3.y, p4.x, p4.y);
		}
	  }
	}

	return 1;
}

/*
 *  Notes on line drawing:
 *
 *  There may be a faster heuristic for drawing lines, as follows:
 *  If the line is short (dx and dy both < 30)
 *	draw using rectangles.
 *  If the line is thick (dst->line_width > 1)
 *	draw using rectangles.
 *  If there is a lot of clipping (dst->clip_num_rects > 50)
 *	draw using rectangles,
 *	since clipping rectangles is faster than clipping lines.
 *  Otherwise,
 *	draw using server-side lines.
 *
 *  This heuristic approach is not exact, but seems to work well
 *  on my computer. The numbers will vary for different computers.
 *  I haven't included code for this method here, since it is
 *  not as exact as the technique used above, but it might be
 *  worth considering for special applications.
 */
