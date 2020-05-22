/*
 *  Cross-platform image type.
 *
 *  Platform: Neutral
 *
 *  Version: 2.30  1997/07/07 Original version by L. Patrick.
 *  Version: 2.40  1998/01/01 Faster drawing, image scaling and cropping.
 *  Version: 3.00  2001/05/15 Names and rows changed to App system.
 *  Version: 3.01  2001/09/09 Added halftoning capability.
 *  Version: 3.58  2005/09/17 Added scale_down_32_bit for better scaling.
 *  Version: 3.60  2007/06/06 scale_down_32_bit now handles alpha better.
 *  Version: 3.61  2010/01/28 halftone_32_bit now handles alpha.
 *  Version: 3.62  2010/02/21 Fixed a bug in app_draw_image.
 *  Version: 3.63  2010/11/21 consts, app_get_image_rect, static to APP_PRIVATE.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

/*
 *  Image
 *  -----
 *  An image is a platform-indepedent representation of a
 *  rectangular picture, in RGB colour format. There are two
 *  possible formats the picture can use: 8-bit indexed colour,
 *  and 32-bit true colour (RGB plus alpha channel).
 *
 *  An image in 8-bit format has the following properties:
 *    - depth is set to 8
 *    - data8 is an array of (height) arrays of (width) bytes
 *    - cmap_size is set to a non-zero value
 *    - cmap is an array of cmap_size rgb values
 *
 *  An image in 32-bit format has the following properties:
 *    - depth is set to 32
 *    - data32 is an array of (height) arrays of (width) rgb values
 *    - cmap_size is zero and cmap is NULL (an empty array)
 *
 *  Any other depth is deliberately not supported.
 *  Transparency is handled in the alpha channel of each rgb value
 *  (either inside the cmap, or in the data32 array itself).
 */

#include "apputils.h"

/*
 *  Create a new image:
 */
Image * app_new_image(int width, int height, int depth)
{
	Image *img;
	int i;

	if ((depth != 8) && (depth != 32))
		return NULL;

	img = app_zero_alloc(sizeof(struct Image));

	if (! img)
		return img;

	img->width  = width;
	img->height = height;

	if (depth == 8) {
		img->depth  = 8;
		img->data8  = app_alloc(height * sizeof(byte *));
		for (i=0; i < height; i++)
			img->data8[i] = app_alloc(width);
	}
	else {
		img->depth  = 32;
		img->data32 = app_alloc(height * sizeof(Colour *));
		for (i=0; i < height; i++)
			img->data32[i] = app_alloc(width * sizeof(Colour));
	}

	return img;
}

/*
 *  Make a new copy of an image:
 */
Image * app_copy_image(const Image *img)
{
	Image *new_img;
	int x, y;

	if (! img)
		return NULL;

	new_img = app_new_image(img->width, img->height, img->depth);

	/* set the pixel values */
	if (img->depth == 8) {
		for (y=0; y < img->height; y++) {
			for (x=0; x < img->width; x++) {
				new_img->data8[y][x] = img->data8[y][x];
			}
		}
		/* copy the palette */
		if (img->cmap_size > 0)
			app_set_image_cmap(new_img, img->cmap_size, img->cmap);
	}
	else if (img->depth == 32) {
		for (y=0; y < img->height; y++) {
			for (x=0; x < img->width; x++) {
				new_img->data32[y][x] = img->data32[y][x];
			}
		}
	}

	return new_img;
}

/*
 *  Delete an image:
 */
void app_del_image(Image *img)
{
	int row;

	if (img->data8) {
		for (row=0; row < img->height; row++)
			app_free(img->data8[row]);
		app_free(img->data8);
	}

	if (img->data32) {
		for (row=0; row < img->height; row++)
			app_free(img->data32[row]);
		app_free(img->data32);
	}

	if (img->cmap)
		app_free(img->cmap);

	app_free(img);
}

/*
 *  Find an image's rectangle.
 */
Rect app_get_image_area(const Image *img) /* Deprecated */
{
	Rect r;

	r.x = 0;
	r.y = 0;
	r.width = img->width;
	r.height = img->height;

	return r;
}

Rect app_get_image_rect(const Image *img)
{
	Rect r;

	r.x = 0;
	r.y = 0;
	r.width = img->width;
	r.height = img->height;

	return r;
}

/*
 *  Change an image's colour map:
 */
void app_set_image_cmap(Image *img, int cmap_size, Colour *cmap)
{
	int i;
	Colour *prev_cmap;

	prev_cmap = img->cmap;

	img->cmap_size = cmap_size;
	img->cmap = app_alloc(cmap_size * sizeof(Colour));

	for (i=0; i < cmap_size; i++)
		img->cmap[i] = cmap[i];

	app_free(prev_cmap);
}

/*
 *  Check an image to see if there are any transparent pixels.
 *  Return 1 as soon as a transparent pixel is found, 0 if none are.
 */
int app_image_has_transparent_pixels(const Image *img)
{
	long x, y, width, height;
	Colour *cmap;
	byte **pixel8;
	Colour **pixel32;
	Colour col;

	if (! img)
		return 0;

	width = img->width;
	height = img->height;

	cmap = img->cmap;

	pixel8 = img->data8;
	pixel32 = img->data32;

	if (img->depth == 8) {
		for (y=0; y < height; y++) {
			for (x=0; x < width; x++) {
				col = cmap[pixel8[y][x]];
				if (col.alpha > 0x7F)
					return 1;
			}
		}
	}
	else if (img->depth == 32) {
		for (y=0; y < height; y++) {
			for (x=0; x < width; x++) {
				col = pixel32[y][x];
				if (col.alpha > 0x7F)
					return 1;
			}
		}
	}

	return 0;
}

/*
 *  Try to generate an 8-bit version of an image:
 *  If there are less than 256 unique colours in a 32-bit
 *  image, this routine will return the corresponding
 *  indexed 8-bit image.
 *  Returns NULL if more than 256 colours are found.
 */
APP_PRIVATE
Image * app_fast_find_cmap (const Image *img)
{
	Image * new_img;
	long    i, x, y;
	int     cmap_size, low, high, mid;
	Colour  col, col2;
	Colour  cmap[256];

	/* the first colour goes into the cmap automatically: */
	cmap_size = 0;
	mid = 0;

	/* create a sorted colour map for speed: */

	for (y=0; y < img->height; y++) {
	  for (x=0; x < img->width; x++) {
		col = img->data32[y][x];
		/* only allow one transparent colour in the cmap: */
		if (col.alpha > 0x7F)
			col = argb(255,255,255,255);	/* transparent */
		else
			col.alpha = 0;	/* opaque */

		/* binary search the cmap: */
		low = 0;
		high = cmap_size - 1;
		while (low <= high) {
			mid = (low+high)/2;
			col2 = cmap[mid];
			if ((col.alpha < col2.alpha) ||
			    (col.red   < col2.red)   ||
			    (col.green < col2.green) ||
			    (col.blue  < col2.blue))
				high = mid - 1;
			else
			if ((col.alpha > col2.alpha) ||
			    (col.red   > col2.red)   ||
			    (col.green > col2.green) ||
			    (col.blue  > col2.blue))
				low  = mid + 1;
			else
				break;
		}

		if (high < low) {
			/* didn't find colour in cmap, insert it: */
			if (cmap_size >= 256)
				return NULL;
			for (i=cmap_size; i > low; i--)
				cmap[i] = cmap[i-1];
			cmap[low] = col;
			cmap_size ++;
		}
	  }
	}

	/* create the 8-bit indexed image: */

	new_img = app_new_image(img->width, img->height, 8);
	if (! new_img)
		return NULL;
	app_set_image_cmap(new_img, cmap_size, cmap);

	/* convert each 32-bit pixel into an 8-bit pixel: */

	for (y=0; y < img->height; y++) {
	  for (x=0; x < img->width; x++) {
		col = img->data32[y][x];

		/* only allow one transparent colour in the cmap: */
		if (col.alpha > 0x7F)
			col = argb(255,255,255,255);	/* transparent */
		else
			col.alpha = 0;	/* opaque */

		/* binary search the cmap: */
		low = 0;
		high = cmap_size - 1;
		while (low <= high) {
			mid = (low+high)/2;
			col2 = cmap[mid];
			if ((col.alpha < col2.alpha) ||
			    (col.red   < col2.red)   ||
			    (col.green < col2.green) ||
			    (col.blue  < col2.blue))
				high = mid - 1;
			else
			if ((col.alpha > col2.alpha) ||
			    (col.red   > col2.red)   ||
			    (col.green > col2.green) ||
			    (col.blue  > col2.blue))
				low  = mid + 1;
			else
				break;
		}

		if (high < low) {
			/* impossible situation */
			app_del_image(new_img);
			return NULL;
		}

		new_img->data8[y][x] = mid;
	  }
	}

	return new_img;
}

/*
 *  Try to generate an 8-bit version of an image:
 *  This routine will approximate a 32-bit image using a
 *  6x6x6 colour cube.
 *  If it runs out of memory, it returns NULL.
 */
APP_PRIVATE
Image * app_fast_generate_cmap (const Image *img)
{
	Image * new_img;
	long    i, x, y;
	int     r, g, b, value;
	Colour  col;
	Colour  cmap[256];

	/* Generate the colour map: */

	for (r=0; r<6; r++)		/* 6x6x6 colour cube */
	  for (g=0; g<6; g++)
	    for (b=0; b<6; b++)
		cmap[r*36 + g*6 + b] = rgb(r,g,b);

	for (i=0; i<39; i++)		/* greyscale ramp */
	{
		value = 255 * i / 38;
		cmap [216 + i] = rgb(value, value, value);
	}

	cmap[255] = argb(255,255,255,255);	/* transparent */

	/* Generate the 8-bit indexed image: */

	new_img = app_new_image(img->width, img->height, 8);
	if (! new_img)
		return new_img;
	app_set_image_cmap(new_img, 256, cmap);

	/* Translate the pixels from 32-bit to 8-bit: */

	for (y=0; y < img->height; y++) {
	  for (x=0; x < img->width; x++) {
		col = img->data32[y][x];
		r = col.red;
		g = col.green;
		b = col.blue;

		if (col.alpha > 0x7F)  /* transparent */
			value = 255;

		else if ((r == g) && (r == b))	/* grey */
		{
			g = g * 38 / 255;
			if (g == 0)
				value = 0;	/* black */
			else
				value = 216 + g;
		}

		else	/* map to 6x6x6 colour cube */
		{
			r = r * 5 / 255;
			g = g * 5 / 255;
			b = b * 5 / 255;

			value = r*36 + g*6 + b;
		}

		new_img->data8[y][x] = value;
	  }
	}

	return new_img;
}

/*
 *  Try to generate an 8-bit version of a 32-bit image:
 *  Return NULL on failure.
 */
Image * app_image_convert_32_to_8 (const Image *img)
{
	Image *new_img;

	if (img->depth == 8)
		return app_copy_image(img);

	new_img = app_fast_find_cmap(img);
	if (! new_img)
		new_img = app_fast_generate_cmap(img);

	return new_img;
}

/*
 *  Try to generate a 32-bit version of an 8-bit image:
 *  Return NULL if there is no memory left.
 */
Image * app_image_convert_8_to_32 (const Image *img)
{
	int x, y;
	Image *new_img;
	byte value;

	new_img = app_new_image(img->width, img->height, 32);
	if (! new_img)
		return new_img;

	for (y=0; y < img->height; y++) {
	  for (x=0; x < img->width; x++) {
		value = img->data8[y][x];
		new_img->data32[y][x] = img->cmap[value];
	  }
	}

	return new_img;
}

/*
 *  Sort an image's colour map, eliminating redudancies.
 *  This operation transforms an existing image.
 */

typedef int (*qsort_func)(const void *a, const void *b);

APP_PRIVATE
int app_cmp_freq(unsigned long *a, unsigned long *b)
{
	unsigned long freq_a, freq_b;
	int value_a, value_b;

	freq_a = (*a) >> 8;
	freq_b = (*b) >> 8;
	value_a = (*a) & 0x00FF;
	value_b = (*b) & 0x00FF;

	if (freq_a < freq_b)        return (+1);
	else if (freq_a > freq_b)   return (-1);
	else                        return (value_a - value_b);
}

void app_image_sort_palette(Image *img)
{
	long	i, j, x, y;
	int 	old_value, new_value;
	int 	new_size;
	Colour 	col, col2;
	Colour *new_cmap;
	unsigned long * histogram;
	unsigned char * translate;

	if (img->depth > 8)
		return;

	histogram = app_zero_alloc(256 * sizeof(long));
	translate = app_zero_alloc(256);

	/* Generate a colour histogram: */
	for (y=0; y < img->height; y++)
		for (x=0; x < img->width; x++)
			histogram[img->data8[y][x]] ++;

	/* Place colour indexes in low byte of histogram: */
	for (i=0; i < 256; i++) {
		histogram[i] <<= 8;
		histogram[i] |= i;
	}

	/* Sort the histogram in decreasing frequency order: */
	qsort(histogram, 256, sizeof(long), (qsort_func) app_cmp_freq);

	/* Generate a colour translation table: */
	new_size = img->cmap_size;

	for (i=255; i >= 0; i--)
	{
		old_value = histogram[i] & 0x00FFL;
		new_value = i;
		col = img->cmap[i];

		/* coalesce identical colours in cmap */
		for (j=i-1; j >= 0; j--) {
			col2 = img->cmap[j];
			if ((col.alpha == col2.alpha) &&
			    (col.red   == col2.red)   &&
			    (col.green == col2.green) &&
			    (col.blue  == col2.blue))
				new_value = j;
		}

		translate[old_value] = new_value;

		/* find smallest useless colour */
		if ((histogram[i] >> 8) == 0)
			new_size = i;
	}

	/* Generate a sorted colour map: */
	new_cmap = app_alloc(new_size * sizeof(Colour));

	for (i=0; i < new_size; i++) {
		old_value = histogram[i] & 0x00FFL;
		new_value = i;
		new_cmap[new_value] = img->cmap[old_value];
	}

	/* Change the existing colour map: */
	img->cmap_size = new_size;
	for (i=0; i < new_size; i++)
		img->cmap[i] = new_cmap[i];
	app_free(new_cmap);

	/* Translate the pixels to the new colour map: */
	for (y=0; y < img->height; y++)
		for (x=0; x < img->width; x++)
			img->data8[y][x] = translate[img->data8[y][x]];

	/* Clean up: */
	app_free(translate);
	app_free(histogram);
}

/*
 *  Changing a Colour's value:
 */

Colour app_darker(Colour c)
{
	int r, g, b;

	r = c.red;
	g = c.green;
	b = c.blue;

	return argb(c.alpha,(r+1)*3/4,(g+1)*3/4,(b+1)*3/4);
}

Colour app_brighter(Colour c)
{
	int r, g, b;

	r = c.red;
	g = c.green;
	b = c.blue;

	r = r * 4 / 3; if (r > 255) r = 255;
	g = g * 4 / 3; if (g > 255) g = 255;
	b = b * 4 / 3; if (b > 255) b = 255;
	return rgb(r,g,b);
}

APP_PRIVATE
Colour app_monochrome(Colour c)
{
	int min, max, g, b;

	max = min = c.red;
	g = c.green;
	if      (g < min) min = g;
	else if (g > max) max = g;
	b = c.blue;
	if      (b < min) min = b;
	else if (b > max) max = b;

	if (min > 0xE0) 	c = argb(c.alpha,255,255,255);
	else if (max < 0x10)	c = argb(c.alpha,0,0,0);
	else if (max < 0x60)	c = argb(c.alpha,0,0,0);
	else if (max < 0xD0)	c = argb(c.alpha,0,0,0);
	else			c = argb(c.alpha,255,255,255);

	return c;
}

APP_PRIVATE
Colour app_greyscale(Colour c)
{
	int min, max, g, b;

	max = min = c.red;
	g = c.green;
	if      (g < min) min = g;
	else if (g > max) max = g;
	b = c.blue;
	if      (b < min) min = b;
	else if (b > max) max = b;

	if (min > 0xE0) 	c = argb(c.alpha,255,255,255);
	else if (max < 0x10)	c = argb(c.alpha,0,0,0);
	else if (max < 0x60)	c = argb(c.alpha,96,96,96);
	else if (max < 0xD0)	c = argb(c.alpha,128,128,128);
	else			c = argb(c.alpha,192,192,192);

	return c;
}

/*
 *  Determine pixel values from an image:
 */

APP_PRIVATE
Colour app_get_image_pixel(const Image *img, int x, int y)
{
	int value;
	Colour c;

	if ((x < 0) || (x >= img->width) ||
	     (y < 0) || (y >= img->height))
		return argb(255,255,255,255); /* transparent */

	if (img->depth <= 8) {
		value = img->data8[y][x];
		c = img->cmap[value];
	} else {
		c = img->data32[y][x];
	}

	return c;
}

APP_PRIVATE
Colour app_get_monochrome_pixel(const Image *img, int x, int y)
{
	return app_monochrome(app_get_image_pixel(img, x, y));
}

APP_PRIVATE
Colour app_get_grey_pixel(const Image *img, int x, int y)
{
	return app_greyscale(app_get_image_pixel(img, x, y));
}

/*
 *  Return an image scaled to a new width and/or height.
 *  The source rectangle sr can be used to crop the source image.
 *  The returned image should be deleted using app_del_image() when
 *  it is no longer needed.
 */

APP_PRIVATE
void app_scale_8_bit_image(Image *dest, const Image *src, Rect dr, Rect sr)
{
	int value, t;
	long x, y;
	long dx, dy, sx, sy;
	long dw, dh, sw, sh;
	long hscale, vscale;
	byte ** src_pixels = src->data8;
	byte ** dest_pixels = dest->data8;

	dw = dest->width;
	dh = dest->height;
	sw = src->width;
	sh = src->height;

	for (t=0; t < dest->cmap_size; t++)
		if (dest->cmap[t].alpha == 0xFF)
			break;
	if (t == dest->cmap_size) /* no transparent colour found */
		t = 0;

	hscale = (dr.width > 1 ? dr.width-1 : 1);
	vscale = (dr.height > 1 ? dr.height-1 : 1);

	for (y=0; y < dr.height; y++) {
	  for (x=0; x < dr.width; x++) {
		sy = sr.y + y * (sr.height-1) / vscale;
		sx = sr.x + x * (sr.width-1) / hscale;
		if ((sx >= 0) && (sx < sw) && (sy >= 0) && (sy < sh))
			value = src_pixels[sy][sx];
		else
			value = t;
		dy = dr.y + y;
		dx = dr.x + x;
		if ((dx >= 0) && (dx < dw) && (dy >= 0) && (dy < dh))
			dest_pixels[dy][dx] = value;
	  }
	}
}

APP_PRIVATE
void app_scale_32_bit_image(Image *dest, const Image *src, Rect dr, Rect sr)
{
	Colour value;
	long x, y;
	long dx, dy, sx, sy;
	long dw, dh, sw, sh;
	long hscale, vscale;
	Colour ** src_pixels = src->data32;
	Colour ** dest_pixels = dest->data32;

	dw = dest->width;
	dh = dest->height;
	sw = src->width;
	sh = src->height;

	hscale = (dr.width > 1 ? dr.width-1 : 1);
	vscale = (dr.height > 1 ? dr.height-1 : 1);

	for (y=0; y < dr.height; y++) {
	  for (x=0; x < dr.width; x++) {
		sy = sr.y + y * (sr.height-1) / vscale;
		sx = sr.x + x * (sr.width-1) / hscale;
		if ((sx >= 0) && (sx < sw) && (sy >= 0) && (sy < sh))
			value = src_pixels[sy][sx];
		else
			value = argb(255,255,255,255);
		dy = dr.y + y;
		dx = dr.x + x;
		if ((dx >= 0) && (dx < dw) && (dy >= 0) && (dy < dh))
			dest_pixels[dy][dx] = value;
	  }
	}
}

APP_PRIVATE
void app_scale_down_32_bit_image(Image *dest, const Image *src, Rect dr, Rect sr)
{
	Colour value, v;
	long x, y;
	long a, r, g, b, count, counta;
	long x1, y1, x2, y2;
	long dx, dy, sx, sy;
	long dw, dh, sw, sh;
	long hscale, vscale;
	Colour ** src_pixels = src->data32;
	Colour ** dest_pixels = dest->data32;

	dw = dest->width;
	dh = dest->height;
	sw = src->width;
	sh = src->height;

	hscale = (dr.width > 1 ? dr.width-1 : 1);
	vscale = (dr.height > 1 ? dr.height-1 : 1);

	for (y=0; y < dr.height; y++) {
	  for (x=0; x < dr.width; x++) {
		a = r = g = b = count = counta = 0;

		y1 = sr.y + y * (sr.height-1) / vscale;
		x1 = sr.x + x * (sr.width-1) / hscale;
		y2 = sr.y + (y + 1) * (sr.height-1) / vscale;
		x2 = sr.x + (x + 1) * (sr.width-1) / hscale;

		for (sy=y1; sy < y2; sy++) {
		  for (sx=x1; sx < x2; sx++){
			if ((sx >= 0) && (sx < sw) && (sy >= 0) && (sy < sh))
				v = src_pixels[sy][sx];
			else
				v = argb(255,255,255,255);

			a += v.alpha;
			counta++;
			if (v.alpha < 255)
			{
				r += v.red;
				g += v.green;
				b += v.blue;
				count += 1;
			}
		  }
		}
		if (count == 0) {
			r = g = b = 255;
			count = 1;
		}
		if (counta == 0) {
			a = 0;
			counta = 1;
		}

		value.alpha = a / counta;
		value.red   = r / count;
		value.green = g / count;
		value.blue  = b / count;

		dy = dr.y + y;
		dx = dr.x + x;
		if ((dx >= 0) && (dx < dw) && (dy >= 0) && (dy < dh))
			dest_pixels[dy][dx] = value;
	  }
	}
}

APP_PRIVATE
void app_halftone_32_bit_image(Image *dest, const Image *src, Rect dr, Rect sr)
{
	Colour value, v1, v2, v3, v4;
	long x, y;
	long dx, dy, sx, sy;
	long dw, dh, sw, sh;
	long hscale, vscale;
	Colour ** src_pixels = src->data32;
	Colour ** dest_pixels = dest->data32;

	dw = dest->width;
	dh = dest->height;
	sw = src->width;
	sh = src->height;

	hscale = (dr.width > 1 ? dr.width-1 : 1);
	vscale = (dr.height > 1 ? dr.height-1 : 1);

	for (y=0; y < dr.height; y++) {
	  for (x=0; x < dr.width; x++) {
		sy = sr.y + y * (sr.height-1) / vscale;
		sx = sr.x + x * (sr.width-1) / hscale;
		if ((sx >= 0) && (sx < sw) && (sy >= 0) && (sy < sh))
			v1 = src_pixels[sy][sx];
		else
			v1 = argb(255,255,255,255);

		//sy = sr.y + y * (sr.height-1) / vscale;
		//sx = sr.x + x * (sr.width-1) / hscale +1;
		sx += 1;
		if ((sx >= 0) && (sx < sw) && (sy >= 0) && (sy < sh))
			v2 = src_pixels[sy][sx];
		else
			v2 = argb(255,255,255,255);

		//sy = sr.y + y * (sr.height-1) / vscale +1;
		//sx = sr.x + x * (sr.width-1) / hscale;
		sy += 1;
		sx -= 1;
		if ((sx >= 0) && (sx < sw) && (sy >= 0) && (sy < sh))
			v3 = src_pixels[sy][sx];
		else
			v3 = argb(255,255,255,255);

		//sy = sr.y + y * (sr.height-1) / vscale +1;
		//sx = sr.x + x * (sr.width-1) / hscale +1;
		sx += 1;
		if ((sx >= 0) && (sx < sw) && (sy >= 0) && (sy < sh))
			v4 = src_pixels[sy][sx];
		else
			v4 = argb(255,255,255,255);

		value.alpha = (v1.alpha*4L+v2.alpha*2L+v3.alpha*2L+v4.alpha*1L)/9;
		value.red   = (v1.red  *4L+v2.red  *2L+v3.red  *2L+v4.red  *1L)/9;
		value.green = (v1.green*4L+v2.green*2L+v3.green*2L+v4.green*1L)/9;
		value.blue  = (v1.blue *4L+v2.blue *2L+v3.blue *2L+v4.blue *1L)/9;

		dy = dr.y + y;
		dx = dr.x + x;
		if ((dx >= 0) && (dx < dw) && (dy >= 0) && (dy < dh))
			dest_pixels[dy][dx] = value;
	  }
	}
}

Image * app_scale_image(const Image *src, Rect dr, Rect sr)
{
	Image *dest;

	dest = app_new_image(dr.width, dr.height, src->depth);
	if (! dest)
		return NULL;

	if (src->depth == 8) {
		app_set_image_cmap(dest, src->cmap_size, src->cmap);
		app_scale_8_bit_image(dest, src, dr, sr);
	}
	else if ((src->depth == 32) &&
		 ((dr.width < sr.width) && (dr.height < sr.height))) {
		app_scale_down_32_bit_image(dest, src, dr, sr);
	}
	else if ((src->depth == 32) &&
		 ((dr.width < sr.width) || (dr.height < sr.height))) {
		app_halftone_32_bit_image(dest, src, dr, sr);
	}
	else if (src->depth == 32) {
		app_scale_32_bit_image(dest, src, dr, sr);
	}
	else {
		app_del_image(dest);
		dest = NULL;
	}

	return dest;
}

/*
 *  Functions for drawing an image:
 */

APP_PRIVATE
int app_get_mono_pixval(Image *src, Image *dest, int x, int y)
{
	int i;
	Colour col;
	Colour pixel = app_get_monochrome_pixel(src, x, y);

	for (i=0; i < dest->cmap_size; i++) {
		col = dest->cmap[i];
		if ((pixel.alpha == col.alpha) &&
		    (pixel.red   == col.red)   &&
		    (pixel.green == col.green) &&
		    (pixel.blue  == col.blue))
			return i;
	}
	return dest->cmap_size - 1;
}

APP_PRIVATE
int app_get_grey_pixval(Image *src, Image *dest, int x, int y)
{
	int i;
	Colour col;
	Colour pixel = app_get_grey_pixel(src, x, y);

	for (i=0; i < dest->cmap_size; i++) {
		col = dest->cmap[i];
		if ((pixel.alpha == col.alpha) &&
		    (pixel.red   == col.red)   &&
		    (pixel.green == col.green) &&
		    (pixel.blue  == col.blue))
			return i;
	}
	return dest->cmap_size - 1;
}

int app_draw_image_monochrome(Graphics *g, Rect dr, Image *src, Rect sr)
{
	int x, y;
	int result;
	Image *dest;
	Colour cmap[3] = {
		{  0,  0,  0,  0},	/* black */
		{  0,255,255,255},	/* white */
		{255,255,255,255}	/* transparent */
	};

	dest = app_new_image(src->width, src->height, 8);
	app_set_image_cmap(dest, 3, cmap);

	for (y=0; y < dest->height; y++)
	  for (x=0; x < dest->width; x++)
		dest->data8[y][x] = app_get_mono_pixval(src, dest, x, y);
	result = app_draw_image(g, dr, dest, sr);
	app_del_image(dest);
	return result;
}

int app_draw_image_greyscale(Graphics *g, Rect dr, Image *src, Rect sr)
{
	int x, y;
	int result;
	Image *dest;
	Colour cmap[6] = {
		{  0,  0,  0,  0},	/* black */
		{  0, 96, 96, 96},	/* dark grey */
		{  0,128,128,128},	/* grey */
		{  0,192,192,192},	/* light grey */
		{  0,255,255,255},	/* white */
		{255,255,255,255}	/* transparent */
	};

	dest = app_new_image(src->width, src->height, 8);
	app_set_image_cmap(dest, 6, cmap);

	for (y=0; y < dest->height; y++)
	  for (x=0; x < dest->width; x++)
		dest->data8[y][x] = app_get_grey_pixval(src, dest, x, y);
	result = app_draw_image(g, dr, dest, sr);
	app_del_image(dest);
	return result;
}

int app_draw_image_darker(Graphics *g, Rect dr, Image *src, Rect sr)
{
	int i, x, y;
	int result;
	Image *dest = NULL;
	Colour *newcmap = NULL, *oldcmap = NULL;
	Colour **pixels;

	if (src->depth == 8) {
		newcmap = app_alloc(src->cmap_size * sizeof(Colour));
		for (i=0; i < src->cmap_size; i++)
			newcmap[i] = app_darker(src->cmap[i]);
		oldcmap = src->cmap;
		src->cmap = newcmap;
		dest = src;
	}
	else if (src->depth == 32) {
		dest = app_new_image(src->width, src->height, 32);
		if (dest) {
		  pixels = dest->data32;
		  for (y=0; y < dest->height; y++)
		    for (x=0; x < dest->width; x++)
			pixels[y][x]
			  = app_darker(app_get_image_pixel(src, x, y));
		} else {
			dest = src;
		}
	}
	result = app_draw_image(g, dr, dest, sr);
	if (dest != src)
		app_del_image(dest);
	if (src->cmap == newcmap)
		src->cmap = oldcmap;
	app_free(newcmap);
	return result;
}

int app_draw_image_brighter(Graphics *g, Rect dr, Image *src, Rect sr)
{
	int i, x, y;
	int result;
	Image *dest = NULL;
	Colour *newcmap = NULL, *oldcmap = NULL;
	Colour **pixels;

	if (src->depth == 8) {
		newcmap = app_alloc(src->cmap_size * sizeof(Colour));
		for (i=0; i < src->cmap_size; i++)
			newcmap[i] = app_brighter(src->cmap[i]);
		oldcmap = src->cmap;
		src->cmap = newcmap;
		dest = src;
	}
	else if (src->depth == 32) {
		dest = app_new_image(src->width, src->height, 32);
		if (dest) {
		  pixels = dest->data32;
		  for (y=0; y < dest->height; y++)
		    for (x=0; x < dest->width; x++)
			pixels[y][x]
			  = app_brighter(app_get_image_pixel(src, x, y));
		} else {
			dest = src;
		}
	}
	result = app_draw_image(g, dr, dest, sr);
	if (dest != src)
		app_del_image(dest);
	if (src->cmap == newcmap)
		src->cmap = oldcmap;
	app_free(newcmap);
	return result;
}

/*
 *  Draw an image:
 */

int app_draw_image(Graphics *g, Rect dr, Image *img, Rect sr)
{
	Graphics *src;
	Bitmap *b = NULL;
	Window *win = NULL;
	Image *i = img;
	int result;

	if ((dr.width != sr.width) || (dr.height != sr.height)) {
		i = app_scale_image(img, rect(0,0,dr.width,dr.height), sr);
		sr = rect(0, 0, dr.width, dr.height);
	}
	if (g->win)
		win = g->win;
	else if (g->bmap)
		win = g->bmap->win;

	if (win) {
		b = app_image_to_bitmap(win, i);
		src = app_get_bitmap_graphics(b);
	}
	else
		src = app_get_image_graphics(i);

	result = app_copy_rect(g, pt(dr.x, dr.y), src, sr);
	app_del_graphics(src);
	if (b)
		app_del_bitmap(b);
	if (i != img)
		app_del_image(i);
	return result;
}

/*
 *  Copy indexed colours to a window or bitmap.
 */
int app_copy_bits(Graphics *g, Rect dr, Palette *pal, byte **rows)
{
	int x, y;
	Colour col;
	Image *img;
	int result = 1;

	img = app_zero_alloc(sizeof(Image));
	if (img) {
		img->depth = 8;
		img->width = dr.width;
		img->height = dr.height;
		img->cmap_size = pal->size;
		img->cmap = pal->element;
		img->data8 = rows;
		result = app_draw_image(g, dr, img, rect(0,0,dr.width,dr.height));
		app_free(img);
	}
	else for (y=0; y < dr.height; y++) {
		for (x=0; x < dr.width; x++) {
			col = pal->element[rows[y][x]];
			app_set_rgb(g, col);
			result &= app_fill_rect(g, rect(dr.x+x,dr.y+y,1,1));
		}
	}
	return result;
}

/*
 *  Copy 32-bit colour values to a window or bitmap.
 */
int app_copy_rgbs(Graphics *g, Rect dr, Colour **rows)
{
	int x, y;
	Colour col;
	Image *img;
	int result = 1;

	img = app_zero_alloc(sizeof(Image));
	if (img) {
		img->depth = 32;
		img->width = dr.width;
		img->height = dr.height;
		img->data32 = rows;
		result = app_draw_image(g, dr, img, rect(0,0,dr.width,dr.height));
		app_free(img);
	}
	else for (y=0; y < dr.height; y++) {
		for (x=0; x < dr.width; x++) {
			col = rows[y][x];
			app_set_rgb(g, col);
			result &= app_fill_rect(g, rect(dr.x+x,dr.y+y,1,1));
		}
	}
	return result;
}

/*
 *  Find nearest matching indexed colour from image's palette.
 */
int app_image_find_colour(const Image *img, Colour col)
{
	unsigned char nearest[1];
	Palette p;

	if (img->cmap_size > 0) {
		p.size = img->cmap_size;
		p.element = img->cmap;
		app_palette_translation(&p, nearest, 1, &col);
		return nearest[0];
	}
	return 0;
}

