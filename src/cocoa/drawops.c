/*
 *  Drawing operations on pixel surfaces.
 *
 *  Platform: macOS.
 *
 *  This file implements the essential drawing operations for windows
 *  and bitmaps:
 *	fill_rect
 *	copy_rect
 *	draw_utf8
 *  (draw_line is the portable implementation, built on fill_rect).
 *
 *  Windows and bitmaps are both Surfaces of 0x00RRGGBB pixels, so one
 *  implementation serves both; the app_window_* and app_bitmap_* names
 *  are the ones the Graphics function pointers refer to.
 *
 *  All clipping is done here, against the Graphics' clip region (or its
 *  whole area if it has none) and against the surface bounds.
 *
 *  XOR mode follows the Windows semantics GraphApp was written against:
 *  the drawing colour is stored as (colour ^ background), and drawing
 *  XORs that onto the destination, so drawing twice restores the pixels.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

/*
 *  Surfaces.
 */

int app_surface_create(Surface *s, int width, int height)
{
	if (width < 1)
		width = 1;
	if (height < 1)
		height = 1;
	memset(s, 0, sizeof(*s));
	s->pixels = calloc((size_t) width * (size_t) height, sizeof(uint32_t));
	if (s->pixels == NULL)
		return 0;
	s->width = width;
	s->height = height;
	return 1;
}

void app_surface_destroy(Surface *s)
{
	free(s->pixels);
	memset(s, 0, sizeof(*s));
}

void app_surface_mark_dirty(Surface *s, Rect r)
{
	int x2, y2;

	if ((r.width <= 0) || (r.height <= 0))
		return;
	if (! s->has_dirty) {
		s->dirty = r;
		s->has_dirty = 1;
		return;
	}
	x2 = s->dirty.x + s->dirty.width;
	y2 = s->dirty.y + s->dirty.height;
	if (r.x < s->dirty.x)
		s->dirty.x = r.x;
	if (r.y < s->dirty.y)
		s->dirty.y = r.y;
	if (r.x + r.width > x2)
		x2 = r.x + r.width;
	if (r.y + r.height > y2)
		y2 = r.y + r.height;
	s->dirty.width = x2 - s->dirty.x;
	s->dirty.height = y2 - s->dirty.y;
}

void app_mask_free(Mask *m)
{
	if (m) {
		free(m->bits);
		free(m);
	}
}

/*
 *  Helpers.
 */

static uint32_t pack_rgb(Colour c)
{
	return ((uint32_t) c.red << 16) | ((uint32_t) c.green << 8) | c.blue;
}

static Rect surface_bounds(const Surface *s)
{
	return rect(0, 0, s->width, s->height);
}

/* Fetch the clip rectangles to draw through. */
static void get_clip(Graphics *g, Rect **rects, int *num)
{
	if (g->clip) {
		*num = g->clip->num_rects;
		*rects = g->clip->rects;
	}
	else {
		*num = 1;
		*rects = &g->area;
	}
}

/* Clip r to a clip rectangle and to the surface; width/height 0 if empty. */
static Rect clip_to(Rect r, Rect clip, Rect bounds)
{
	Rect c;

	c = app_clip_rect(r, clip);
	if ((c.width <= 0) || (c.height <= 0))
		return rect(0, 0, 0, 0);
	c = app_clip_rect(c, bounds);
	if ((c.width <= 0) || (c.height <= 0))
		return rect(0, 0, 0, 0);
	return c;
}

static void fix_negative_rect(Rect *r)
{
	if (r->width < 0) {
		r->x += r->width;
		r->width = 0 - r->width;
	}
	if (r->height < 0) {
		r->y += r->height;
		r->height = 0 - r->height;
	}
}

static uint32_t blend_pixel(uint32_t dst, uint32_t src, unsigned a)
{
	unsigned inv = 255 - a;
	unsigned r = (((src >> 16) & 0xFF) * a + ((dst >> 16) & 0xFF) * inv + 127) / 255;
	unsigned g = (((src >>  8) & 0xFF) * a + ((dst >>  8) & 0xFF) * inv + 127) / 255;
	unsigned b = (((src      ) & 0xFF) * a + ((dst      ) & 0xFF) * inv + 127) / 255;

	return (r << 16) | (g << 8) | b;
}

/*
 *  app_fill_rect: fill a rectangle with the current colour.
 */

static int surface_fill_rect(Graphics *dst, Rect r)
{
	int i, num_rects, x, y;
	Rect *rects;
	Rect c, bounds;
	Surface *s;
	uint32_t pixval;
	uint32_t *row;

	if (dst->colour.alpha > 0x7F)
		return 1; /* nothing to draw if colour is transparent */

	s = graphics_extra(dst)->surf;
	if (s == NULL)
		return 0;
	pixval = graphics_extra(dst)->pixval;
	bounds = surface_bounds(s);

	/* correct drawing displacement */
	r.x += dst->offset.x;
	r.y += dst->offset.y;
	fix_negative_rect(&r);

	get_clip(dst, &rects, &num_rects);

	for (i=0; i < num_rects; i++) {
		c = clip_to(r, rects[i], bounds);
		if (c.width == 0)
			continue;

		for (y=c.y; y < c.y + c.height; y++) {
			row = s->pixels + (size_t) y * s->width + c.x;
			if (dst->xor_mode) {
				for (x=0; x < c.width; x++)
					row[x] ^= pixval;
			}
			else {
				for (x=0; x < c.width; x++)
					row[x] = pixval;
			}
		}
		app_surface_mark_dirty(s, c);
	}
	return 1;
}

int app_window_fill_rect(Graphics *dst, Rect r)
{
	return surface_fill_rect(dst, r);
}

int app_bitmap_fill_rect(Graphics *dst, Rect r)
{
	return surface_fill_rect(dst, r);
}

/*
 *  app_copy_rect: copy an area from a window, bitmap or image.
 *
 *  The source is first described as a block of pixels (plus, optionally,
 *  a stencil), addressed relative to the top-left of the clipped source
 *  rectangle.  Sources that could overlap the destination (scrolling a
 *  window onto itself) and images (which need converting) are copied to
 *  a temporary block first.
 *
 *  Where a stencil is present, only its opaque pixels are copied.
 *  In XOR mode the pixels copied are  D ^ S ^ background.
 */

typedef struct SrcBlock
{
	const uint32_t *	px;
	int			stride;
	const unsigned char *	mask;	/* NULL if fully opaque */
	int			mstride;
	uint32_t *		temp_px;	/* to free, if allocated */
	unsigned char *		temp_mask;	/* to free, if allocated */
} SrcBlock;

static void free_src_block(SrcBlock *b)
{
	free(b->temp_px);
	free(b->temp_mask);
}

/*
 *  Convert an image region to a block.  Returns 0 if out of memory.
 */
static int image_to_block(Image *img, Rect sr, SrcBlock *b)
{
	int x, y;
	Colour col;
	uint32_t *px;
	unsigned char *mask;

	px = malloc((size_t) sr.width * sr.height * sizeof(uint32_t));
	mask = malloc((size_t) sr.width * sr.height);
	if (px == NULL || mask == NULL) {
		free(px);
		free(mask);
		return 0;
	}
	for (y=0; y < sr.height; y++) {
		for (x=0; x < sr.width; x++) {
			if (img->depth == 8)
				col = img->cmap[img->data8[sr.y+y][sr.x+x]];
			else
				col = img->data32[sr.y+y][sr.x+x];
			px[y * sr.width + x] = pack_rgb(col);
			mask[y * sr.width + x] = (col.alpha > 0x7F) ? 0 : 1;
		}
	}
	b->px = b->temp_px = px;
	b->stride = sr.width;
	b->mask = b->temp_mask = mask;
	b->mstride = sr.width;
	return 1;
}

static int surface_copy_rect(Graphics *dst, Point dp, Graphics *src, Rect sr)
{
	int i, num_rects, x, y;
	Rect *rects;
	Rect dr, c, bounds, src_area;
	Surface *ds, *ss = NULL;
	Mask *smask = NULL;
	SrcBlock b;
	uint32_t bgpix;
	uint32_t *drow;
	const uint32_t *srow;
	const unsigned char *mrow;

	ds = graphics_extra(dst)->surf;
	if (ds == NULL)
		return 0;
	memset(&b, 0, sizeof(b));

	/* correct drawing displacement */
	dp.x += dst->offset.x;
	dp.y += dst->offset.y;
	sr.x += src->offset.x;
	sr.y += src->offset.y;
	fix_negative_rect(&sr);

	/* locate the source, and clip the source rectangle to its bounds */
	if (src->win) {
		ss = &win_extra(src->win)->surf;
		src_area = surface_bounds(ss);
	}
	else if (src->bmap) {
		ss = &bitmap_extra(src->bmap)->surf;
		smask = bitmap_extra(src->bmap)->clipmask;
		src_area = surface_bounds(ss);
	}
	else if (src->img) {
		src_area = app_get_image_area(src->img);
	}
	else
		return 0;

	c = app_clip_rect(src_area, sr);
	dp.x = dp.x + c.x - sr.x;
	dp.y = dp.y + c.y - sr.y;
	sr = c;
	if ((sr.width <= 0) || (sr.height <= 0))
		return 1;

	/* describe the source pixels */
	if (ss == NULL) {
		/* image: convert to a block */
		if (! image_to_block(src->img, sr, &b))
			return 0;
	}
	else if (ss == ds) {
		/* same surface: copy first, in case areas overlap */
		b.temp_px = malloc((size_t) sr.width * sr.height * sizeof(uint32_t));
		if (b.temp_px == NULL)
			return 0;
		for (y=0; y < sr.height; y++)
			memcpy(b.temp_px + (size_t) y * sr.width,
				ss->pixels + (size_t) (sr.y + y) * ss->width + sr.x,
				(size_t) sr.width * sizeof(uint32_t));
		b.px = b.temp_px;
		b.stride = sr.width;
		b.mask = NULL;
	}
	else {
		b.px = ss->pixels + (size_t) sr.y * ss->width + sr.x;
		b.stride = ss->width;
		if (smask && smask->bits && smask->width == ss->width) {
			b.mask = smask->bits + (size_t) sr.y * smask->width + sr.x;
			b.mstride = smask->width;
		}
	}

	bgpix = pack_rgb(graphics_extra(dst)->bg);
	bounds = surface_bounds(ds);
	get_clip(dst, &rects, &num_rects);
	dr = rect(dp.x, dp.y, sr.width, sr.height);

	for (i=0; i < num_rects; i++) {
		c = clip_to(dr, rects[i], bounds);
		if (c.width == 0)
			continue;

		for (y=0; y < c.height; y++) {
			drow = ds->pixels + (size_t) (c.y + y) * ds->width + c.x;
			srow = b.px + (size_t) (c.y - dr.y + y) * b.stride
					+ (c.x - dr.x);
			mrow = b.mask ? b.mask + (size_t) (c.y - dr.y + y) * b.mstride
					+ (c.x - dr.x) : NULL;

			if (dst->xor_mode) {
				for (x=0; x < c.width; x++)
					if (mrow == NULL || mrow[x])
						drow[x] ^= srow[x] ^ bgpix;
			}
			else if (mrow) {
				for (x=0; x < c.width; x++)
					if (mrow[x])
						drow[x] = srow[x];
			}
			else {
				memcpy(drow, srow, (size_t) c.width * sizeof(uint32_t));
			}
		}
		app_surface_mark_dirty(ds, c);
	}

	free_src_block(&b);
	return 1;
}

int app_window_copy_rect(Graphics *dst, Point dp, Graphics *src, Rect sr)
{
	return surface_copy_rect(dst, dp, src, sr);
}

int app_bitmap_copy_rect(Graphics *dst, Point dp, Graphics *src, Rect sr)
{
	return surface_copy_rect(dst, dp, src, sr);
}

/*
 *  app_draw_utf8: draw the characters of a UTF-8 string.
 *
 *  Native fonts are rendered by CoreText into an 8-bit coverage bitmap,
 *  which is blended (or, in XOR mode, thresholded) onto the surface.
 *  Portable fonts draw each glyph from its stencil.
 */

/* Draw a coverage bitmap with its top-left at (dx, dy). */
static void draw_coverage(Graphics *dst, int dx, int dy,
	const uint8_t *cov, int cw, int ch)
{
	int i, num_rects, x, y;
	Rect *rects;
	Rect dr, c, bounds;
	Surface *s = graphics_extra(dst)->surf;
	uint32_t pixval = graphics_extra(dst)->pixval;
	uint32_t *row;
	const uint8_t *crow;
	unsigned a;

	bounds = surface_bounds(s);
	dr = rect(dx, dy, cw, ch);
	get_clip(dst, &rects, &num_rects);

	for (i=0; i < num_rects; i++) {
		c = clip_to(dr, rects[i], bounds);
		if (c.width == 0)
			continue;

		for (y=0; y < c.height; y++) {
			row = s->pixels + (size_t) (c.y + y) * s->width + c.x;
			crow = cov + (size_t) (c.y - dr.y + y) * cw + (c.x - dr.x);
			for (x=0; x < c.width; x++) {
				a = crow[x];
				if (a == 0)
					continue;
				if (dst->xor_mode) {
					if (a > 0x7F)
						row[x] ^= pixval;
				}
				else if (a == 255)
					row[x] = pixval;
				else
					row[x] = blend_pixel(row[x], pixval, a);
			}
		}
		app_surface_mark_dirty(s, c);
	}
}

/* Draw one glyph cell of a stencil, with its top-left at (dr.x, dr.y). */
static void draw_glyph_mask(Graphics *dst, Rect dr, const Mask *m, int gx, int gy)
{
	int i, num_rects, x, y;
	Rect *rects;
	Rect c, bounds;
	Surface *s = graphics_extra(dst)->surf;
	uint32_t pixval = graphics_extra(dst)->pixval;
	uint32_t *row;
	const unsigned char *mrow;
	int mx, my;

	bounds = surface_bounds(s);
	get_clip(dst, &rects, &num_rects);

	for (i=0; i < num_rects; i++) {
		c = clip_to(dr, rects[i], bounds);
		if (c.width == 0)
			continue;

		for (y=0; y < c.height; y++) {
			my = gy + c.y - dr.y + y;
			mx = gx + c.x - dr.x;
			if ((my < 0) || (my >= m->height))
				continue;
			row = s->pixels + (size_t) (c.y + y) * s->width + c.x;
			mrow = m->bits + (size_t) my * m->width;
			for (x=0; x < c.width; x++) {
				if ((mx + x < 0) || (mx + x >= m->width))
					continue;
				if (! mrow[mx + x])
					continue;
				if (dst->xor_mode)
					row[x] ^= pixval;
				else
					row[x] = pixval;
			}
		}
		app_surface_mark_dirty(s, c);
	}
}

static int surface_draw_utf8(Graphics *dst, Point dp, const char *s, int nbytes)
{
	int gw;
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
	int temp_font = 0;
	int done = 0;

	if (dst->colour.alpha > 0x7F)
		return 1; /* nothing to draw if colour is transparent */
	if (graphics_extra(dst)->surf == NULL)
		return 0;

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
	right_to_left = (dst->text_direction & RL_TB) ? 1 : 0;

	/* check if we are completely outside the clipping region */
	get_clip(dst, &rects, &num_rects);
	clipped = dst->clip ? dst->clip->extents : dst->area;

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

	/* native font: render the whole string with CoreText */
	if (f->style & NATIVE_FONT) {
		uint8_t *cov = NULL;
		int cw, chh;

		if (gab_font_render(font_extra(f)->handle, s, nbytes,
				&cov, &cw, &chh) && cov)
			draw_coverage(dst, dp.x, dp.y, cov, cw, chh);
		free(cov);
		if (temp_font)
			app_del_font(f);
		return 1;
	}

	/* portable font: draw clipped glyphs */
	sp = s;
	src_end = s + nbytes;

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
		dr.width = gr.width;
		dr.height = gr.height;

		if (subfont_extra(sub)->clipmask)
			draw_glyph_mask(dst, dr, subfont_extra(sub)->clipmask,
					gr.x, gr.y);

		if (! right_to_left)
			dp.x += gw;

		/* go to next character */
	}
	if (temp_font)
		app_del_font(f);
	return 1;
}

int app_window_draw_utf8(Graphics *dst, Point dp, const char *s, int nbytes)
{
	return surface_draw_utf8(dst, dp, s, nbytes);
}

int app_bitmap_draw_utf8(Graphics *dst, Point dp, const char *s, int nbytes)
{
	return surface_draw_utf8(dst, dp, s, nbytes);
}

/*
 *  Lines: use the portable Bresenham implementation, which is built on
 *  fill_rect and so gets clipping, line width and XOR mode for free.
 */

int app_window_draw_line(Graphics *dst, Point p1, Point p2)
{
	return app_portable_draw_line(dst, p1, p2);
}

int app_bitmap_draw_line(Graphics *dst, Point p1, Point p2)
{
	return app_portable_draw_line(dst, p1, p2);
}
