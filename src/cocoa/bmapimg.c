/*
 *  Converting between images and bitmaps.
 *
 *  Platform: macOS.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

/*
 *  Create a stencil for an image: nonzero for opaque pixels, zero for
 *  transparent ones (alpha > 0x7F).  Returns NULL if the image is
 *  fully opaque, or on error.  This is used in various situations as a
 *  'clipmask' - a stencil through which drawing occurs only where the
 *  pixels are opaque.
 */
Mask *app_image_to_clipmask(App *app, Image *img)
{
	Mask *m;
	int x, y;
	Colour col;

	if (! img)
		return NULL;
	if ((img->depth != 8) && (img->depth != 32))
		return NULL;
	if (! app_image_has_transparent_pixels(img))
		return NULL;

	m = calloc(1, sizeof(Mask));
	if (m == NULL)
		return NULL;
	m->bits = malloc((size_t) img->width * img->height);
	if (m->bits == NULL) {
		free(m);
		return NULL;
	}
	m->width = img->width;
	m->height = img->height;

	for (y=0; y < img->height; y++) {
		for (x=0; x < img->width; x++) {
			if (img->depth == 8)
				col = img->cmap[img->data8[y][x]];
			else
				col = img->data32[y][x];
			m->bits[(size_t) y * img->width + x] =
				(col.alpha > 0x7F) ? 0 : 1;
		}
	}
	return m;
}

/*
 *  Create a bitmap from an image.  The bitmap is transparent wherever
 *  the image is.
 */
Bitmap *app_image_to_bitmap(Window *win, Image *img)
{
	Bitmap *b;
	Surface *s;
	int x, y;
	Colour col;
	uint32_t *row;

	if ((img->depth != 8) && (img->depth != 32))
		return NULL;

	b = app_new_bitmap(win, img->width, img->height);
	if (! b)
		return NULL;

	s = &bitmap_extra(b)->surf;
	for (y=0; y < img->height && y < s->height; y++) {
		row = s->pixels + (size_t) y * s->width;
		for (x=0; x < img->width && x < s->width; x++) {
			if (img->depth == 8)
				col = img->cmap[img->data8[y][x]];
			else
				col = img->data32[y][x];
			row[x] = ((uint32_t) col.red << 16) |
				 ((uint32_t) col.green << 8) | col.blue;
		}
	}

	/* replace the all-transparent stencil with the image's own */
	app_mask_free(bitmap_extra(b)->clipmask);
	bitmap_extra(b)->clipmask = app_image_to_clipmask(win->app, img);
	return b;
}

/*
 *  Fetch the data from a bitmap and store it into an image.
 */
Image *app_bitmap_to_image(Bitmap *b)
{
	Surface *s = &bitmap_extra(b)->surf;
	Rect br = app_get_bitmap_area(b);
	Image *img;
	int x, y;
	uint32_t p;
	Colour *cp;

	img = app_new_image(br.width, br.height, 32);
	if (! img)
		return NULL;

	for (y=0; y < img->height; y++) {
		cp = img->data32[y];
		for (x=0; x < img->width; x++) {
			p = s->pixels[(size_t) y * s->width + x];
			cp[x].alpha = 0;
			cp[x].red   = (byte) (p >> 16);
			cp[x].green = (byte) (p >> 8);
			cp[x].blue  = (byte) p;
		}
	}
	return img;
}
