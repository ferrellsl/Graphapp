/*
 *  Image to Bitmap converter functions.
 *
 *  This file implements conversion from a platform-independent
 *  Image data format into the Windows HBITMAP data format.
 *
 *  Platform: Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Moved a function out.
 *  Version: 3.56  2005/08/09  Note: Pointer truncation here is fine.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

/*
 *  Create a depth-1 bitmap which has 0's for opaque pixels
 *  and 1's for transparent pixels. This is used in various
 *  situations as a 'clipmask' - a stencil through which
 *  drawing occurs only where the pixels are opaque.
 */
HBITMAP app_image_to_clipmask(App *app, Image *img)
{
	HBITMAP bm;
	byte *bits;
	byte *row;
	int x, y, rowbytes;
	Colour col;

	if (! img)
		return 0;
	if ((img->depth != 8) && (img->depth != 32))
		return 0;
	if (! app_image_has_transparent_pixels(img))
		return 0;

	rowbytes = (img->width +15) /16 *2; /* multiple of 16 bits */
	bits = app_zero_alloc(rowbytes * img->height);

	if (img->depth == 8) {
		for (y=0; y < img->height; y++) {
			row = &bits[y * rowbytes];
			for (x=0; x < img->width; x++) {
				col = img->cmap[img->data8[y][x]];
				if (col.alpha > 0x7F)
					row[x/8] |= (1<<(7-(x%8)));
			}
		}
	}
	else if (img->depth == 32) {
		for (y=0; y < img->height; y++) {
			row = &bits[y * rowbytes];
			for (x=0; x < img->width; x++) {
				col = img->data32[y][x];
				if (col.alpha > 0x7F)
					row[x/8] |= (1<<(7-(x%8)));
			}
		}
	}

	bm = CreateBitmap(img->width, img->height, 1, 1, bits);
	app_free(bits);

	return bm;
}

static void app_copy_bits_raw(Graphics *g, Rect dr, Palette *pal,
	unsigned char **rows)
{
	size_t i;
	int size, x, y;
	int row_bytes;
	Colour col;
	unsigned short * cmap;
	unsigned char * data;
	unsigned char * line;
	unsigned char * row;
	BITMAPINFO * bmi;

	size = sizeof(BITMAPINFOHEADER) + 8;

	row_bytes = (((dr.width * 3) + 3) / 4) * 4;
	size = size + (row_bytes * dr.height);

	/* create the block */
	bmi = app_zero_alloc(size); /* fill with zeros */

	/* assign header info */
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = dr.width;
	bmi->bmiHeader.biHeight = dr.height;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = 24;
	bmi->bmiHeader.biCompression = BI_RGB;
	bmi->bmiHeader.biClrUsed = 0;
	bmi->bmiHeader.biClrImportant = 0;

	/* assign no colour table */
	cmap = (unsigned short *) & bmi->bmiColors;

	/* assign the bitmap data itself, align on LONG boundary */
	data = (unsigned char *) cmap;
	i = ((size_t)data) % 4; /* pointer truncation here is fine */
	if (i)
		data += (4 - i);

	for (y=0; y < dr.height; y++) {
		line = data + (row_bytes * y);
		row = rows[dr.height - y - 1]; /* Windows is upside-down */
		for (x=0; x < dr.width; x++) {
			col = pal->element[*row++];
			if (col.alpha > 0x7F) {
				*line++ = 0x00;
				*line++ = 0x00;
				*line++ = 0x00;
			}
			else {
				*line++ = col.blue;
				*line++ = col.green;
				*line++ = col.red;
			}
		}
	}

	/* draw from the DIB data */

	StretchDIBits(graphics_extra(g)->dc,
			dr.x, dr.y, dr.width, dr.height,
			0, 0, dr.width, dr.height,
			data, bmi, DIB_RGB_COLORS, SRCCOPY);

	/* tidy up */
	app_free(bmi);
}

static void app_copy_rgbs_raw(Graphics *g, Rect dr, Colour **rows)
{
	size_t i;
	int size, x, y;
	int row_bytes;
	Colour col;
	unsigned short * cmap;
	unsigned char * data;
	unsigned char * line;
	Colour * row;
	BITMAPINFO * bmi;

	size = sizeof(BITMAPINFOHEADER) + 8;

	row_bytes = (((dr.width * 3) + 3) / 4) * 4;
	size = size + (row_bytes * dr.height);

	/* create the block */
	bmi = app_zero_alloc(size); /* fill with zeros */

	/* assign header info */
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = dr.width;
	bmi->bmiHeader.biHeight = dr.height;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = 24;
	bmi->bmiHeader.biCompression = BI_RGB;
	bmi->bmiHeader.biClrUsed = 0;
	bmi->bmiHeader.biClrImportant = 0;

	/* assign no colour table */
	cmap = (unsigned short *) & bmi->bmiColors;

	/* assign the bitmap data itself, align on LONG boundary */
	data = (unsigned char *) cmap;
	i = ((size_t)data) % 4; /* pointer truncation here is fine */
	if (i)
		data += (4 - i);

	for (y=0; y < dr.height; y++) {
		line = data + (row_bytes * y);
		row = rows[dr.height - y - 1]; /* Windows is upside-down */
		for (x=0; x < dr.width; x++) {
			col = *row++;
			if (col.alpha > 0x7F) {
				*line++ = 0x00;
				*line++ = 0x00;
				*line++ = 0x00;
			}
			else {
				*line++ = col.blue;
				*line++ = col.green;
				*line++ = col.red;
			}
		}
	}

	/* draw from the DIB data */

	StretchDIBits(graphics_extra(g)->dc,
			dr.x, dr.y, dr.width, dr.height,
			0, 0, dr.width, dr.height,
			data, bmi, DIB_RGB_COLORS, SRCCOPY);

	/* tidy up */
	app_free(bmi);
}

Bitmap * app_image_to_bitmap(Window *win, Image *img)
{
	Bitmap *b;
	Graphics *g;
	Palette *p;

	b = app_new_bitmap(win, img->width, img->height);
	if (! b)
		return NULL;
	g = app_get_bitmap_graphics(b);
	if (! g) {
		app_del_bitmap(b);
		return NULL;
	}

	if (img->depth == 8) {
		p = app_new_palette(img->cmap_size, img->cmap);
		if (! p) {
			app_del_graphics(g);
			app_del_bitmap(b);
			return NULL;
		}
		app_copy_bits_raw(g, app_get_bitmap_area(b), p, img->data8);
		app_free(p);
	}
	else if (img->depth == 32) {
		app_copy_rgbs_raw(g, app_get_bitmap_area(b), img->data32);
	}

	app_del_graphics(g);
	if (bitmap_extra(b)->clipmask)
		DeleteObject(bitmap_extra(b)->clipmask);
	bitmap_extra(b)->clipmask = app_image_to_clipmask(win->app, img);
	return b;
}

/*
 *  Fetch the data from a bitmap and store it into an image.
 */
Image *app_bitmap_to_image(Bitmap *b)
{
	HDC hdc;
	HBITMAP hb;
	Graphics *g;
	BITMAPINFO *bmi;
	size_t i;
	long row_bytes;
	unsigned char *data;
	Image *img;
	int size, x, y, numlines;
	Rect br;

	g = app_get_bitmap_graphics(b);
	hdc = graphics_extra(g)->dc;
	hb = bitmap_extra(b)->handle;
	br = app_get_bitmap_area(b);

	/* assign header info */
	size = sizeof(BITMAPINFOHEADER) + 8;

	row_bytes = (((br.width * 3) + 3) / 4) * 4;
	size = size + row_bytes * 3; /* x3 in case it's 64- or 32-bit not 24 */

	/* create the block */
	bmi = app_zero_alloc(size); /* fill with zeros */

	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = br.width;
	bmi->bmiHeader.biHeight = br.height;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = 0; /* set to zero, not 24, to query this bitmap */
	bmi->bmiHeader.biCompression = BI_RGB;
	bmi->bmiHeader.biClrUsed = 0;
	bmi->bmiHeader.biClrImportant = 0;

	/* query the bitmap */
	GetDIBits(hdc, hb, 0, 0, NULL, bmi, DIB_RGB_COLORS);

	if (bmi->bmiHeader.biPlanes != 1)
		return NULL;
	if ((bmi->bmiHeader.biBitCount != 24)
	 && (bmi->bmiHeader.biBitCount != 32))
		return NULL;
	if ((bmi->bmiHeader.biCompression != BI_RGB)
	 && (bmi->bmiHeader.biCompression != BI_BITFIELDS))
		return NULL;
	if (bmi->bmiHeader.biClrUsed != 0)
		return NULL;

	/* actually, let's just use the pixel array at the end of the struct */
	data = (char *) & bmi->bmiColors;

	/* align the temporary on a 4-byte boundary */
	i = ((size_t)data) % 4; /* pointer truncation here is fine */
	if (i)
		data += (4 - i);

	/* skip bit-field definitions */
	if (bmi->bmiHeader.biCompression == BI_BITFIELDS)
	{
		Colour c;
		Colour *cp = &c;
		byte *bp = data;
		for (i = 0; i < 3; i++)
		{
			cp->blue  = *bp++;
			cp->green = *bp++;
			cp->red   = *bp++;
			cp->alpha = 0;
			if (bmi->bmiHeader.biBitCount == 32)
				bp++;
		}
		data = bp;
	}

	/* create the image */
	img = app_new_image(br.width, br.height, 32);
	if (! img)
		return NULL;

	/* copy the pixels out of each bitmap row and into the image */
	numlines = 1;
	for (y = 0; y < img->height; y++)
	{
		Colour *cp = img->data32[img->height - y - 1];
		byte *bp = data;

		GetDIBits(hdc, hb, y, numlines, data, bmi, DIB_RGB_COLORS);
		for (x = 0; x < img->width; x++)
		{
			cp->blue  = *bp++;
			cp->green = *bp++;
			cp->red   = *bp++;
			cp->alpha = 0;
			if (bmi->bmiHeader.biBitCount == 32)
				bp++;
			cp++;
		}
	}

	/* tidy up */
	app_del_graphics(g);
	return img;
}
