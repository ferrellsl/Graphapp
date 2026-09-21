/*
 *  Bitmaps.
 *
 *  Platform: macOS.
 *
 *  A Bitmap is an off-screen Surface plus an optional stencil.
 *  A stencil marks which pixels are opaque when the bitmap is copied
 *  onto something else; a bitmap with no stencil is fully opaque.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

static Bitmap *app_alloc_bitmap(Window *w, int width, int height)
{
	Bitmap *b;

	b = app_zero_alloc(sizeof(struct Bitmap));
	if (b == NULL)
		return NULL;
	b->win = w;
	b->area = rect(0, 0, width, height);
	b->extra = app_zero_alloc(sizeof(struct BitmapExtra));
	if (b->extra == NULL) {
		app_free(b);
		return NULL;
	}

	if ((width <= 0) || (height <= 0)) {
		b->area = rect(0,0,0,0);
		width = height = 1;
	}

	if (! app_surface_create(&bitmap_extra(b)->surf, width, height)) {
		app_free(b->extra);
		app_free(b);
		return NULL;
	}
	return b;
}

/*
 *  Create a fully transparent bitmap.
 *  The bitmap is transparent because its stencil says so: every
 *  pixel is marked transparent (0), so nothing shows when it is copied
 *  until a stencil is supplied (as app_image_to_bitmap does).
 */
Bitmap *app_new_bitmap(Window *w, int width, int height)
{
	Bitmap *b;
	Mask *m;
	int sw, sh;

	b = app_alloc_bitmap(w, width, height);
	if (b == NULL)
		return NULL;

	sw = bitmap_extra(b)->surf.width;
	sh = bitmap_extra(b)->surf.height;
	m = calloc(1, sizeof(Mask));
	if (m)
		m->bits = calloc((size_t) sw * sh, 1);
	if (m == NULL || m->bits == NULL) {
		app_mask_free(m);
		app_del_bitmap(b);
		return NULL;
	}
	m->width = sw;
	m->height = sh;
	bitmap_extra(b)->clipmask = m;
	return b;
}

/*
 *  Create a white fully-opaque bitmap.
 *  The bitmap is opaque because there is no stencil.
 */
Bitmap *app_new_white_bitmap(Window *w, int width, int height)
{
	Bitmap *b;
	Surface *s;
	size_t i, n;

	b = app_alloc_bitmap(w, width, height);
	if (b == NULL)
		return NULL;

	s = &bitmap_extra(b)->surf;
	n = (size_t) s->width * s->height;
	for (i=0; i < n; i++)
		s->pixels[i] = 0x00FFFFFF;
	return b;
}

/*
 *  Delete a bitmap.
 */
void app_del_bitmap(Bitmap *b)
{
	app_mask_free(bitmap_extra(b)->clipmask);
	app_surface_destroy(&bitmap_extra(b)->surf);
	app_free(bitmap_extra(b));
	app_free(b);
}

/*
 *  Determine the bitmap's rectangular extent.
 */
Rect app_get_bitmap_area(Bitmap *b)
{
	return b->area;
}
