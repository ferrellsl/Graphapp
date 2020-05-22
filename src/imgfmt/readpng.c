/*
 *  PNG reading functions.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.56  2005/08/09  Silenced a size_t conversion warning.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdlib.h>
#include <stdio.h>

#include "app.h"
#include <png.h>

/*
 * This code reads a PNG graphics file, using libpng.
 * There are a number of cases to handle:
 *   - the image is greyscale, indexed or colour (G, I or RGB)
 *   - the image has an alpha channel (GA or RGBA)
 *   - the image has a palette (PLTE chunk)
 *   - the image has a histogram (hIST chunk)
 *   - the image has transparency (tRNS chunks)
 *   - the image has a background chunk (bKGD chunk)
 *   - the user requested 8 bit or 32 bit colour (8-bit or 32-bit)
 *   - the user supplied a palette to use (pal_given or no_pal)
 *
 * These cases combine in the following solutions
 * for 8-bit output, no palette given by user:
 *
 *   G               => generate a greyscale palette
 *   G, tRNS         => generate a greyscale palette with 1 transp (bKGD)
 *   RGB             => quantize to produce a palette
 *   RGB, PLTE       => copy image's PLTE
 *   RGB, tRNS       => quantize to produce a palette with 1 transp (bKGD)
 *   RGB, PLTE, tRNS => copy image's PLTE, add 1 transparent entry (bKGD)
 *   I, PLTE         => copy image's PLTE
 *   I, PLTE, tRNS   => copy image's PLTE with transparency information
 *   GA              => generate a greyscale palette with 1 transparent
 *   GA, PLTE        => copy image's PLTE, add 1 transparent (bKGD) entry
 *   RGBA            => quantize to produce a palette with 1 transparent
 *   RGBA, PLTE      => copy image's PLTE, add 1 transparent (bKGD) entry
 *
 * For 8-bit output, palette given by user:
 *
 *   G               => dither to given palette
 *   G, tRNS         => dither to given palette, tRNS maps to bKGD
 *   RGB             => dither to given palette
 *   RGB, PLTE       => match PLTE to given palette
 *   RGB, tRNS       => dither to given palette, tRNS maps to bKGD
 *   RGB, PLTE, tRNS => match PLTE to given palette, tRNS maps to bKGD
 *   I, PLTE         => match PLTE to given palette
 *   I, PLTE, tRNS   => match PLTE to given palette, tRNS maps to bKGD
 *   GA              => dither to given palette, transparent maps to bKGD
 *   GA, PLTE        => match PLTE to given palette, transparent maps to bKGD
 *   RGBA            => dither to given palette, transparent maps to bKGD
 *   RGBA, PLTE      => match PLTE to given palette, transparent maps to bKGD
 *
 * For 32-bit output, always just convert to ARGB format.
 *
 * The general algorithm for the above is:
 *
 *  if (user_wants_8_bits)
 *    if (user_given_palette)
 *      png_set_dither(user_given_palette)
 *      if (tRNS)
 *        transparent pixels map to a single bgcolor
 *    else (no user_given_palette)
 *      if (PLTE) # indexed color or otherwise
 *         png_set_dither(PLTE)
 *         if (tRNS)
 *           transparent pixels map to a single bgcolor
 *      else (no PLTE)
 *         if (greyscale)
 *           generate_greyscale_palette(depth, tRNS=None, bgcolor=None)
 *         else (must be RGB or RGBA)
 *           quantize_image_to_palette(image)
 *  else (user_wants_32_bits)
 *    expand all to ARGB format
 *
 */

typedef struct Transformation
{
	/* Destination: */
	long      width, height, rowbytes;

	/* For colour cubes: */
	int       side, volume;

	/* For greyscale ramps: */
	int       greys, grey_start;
	int       black_index, white_index;

	/* For transparency: */
	int       transparent;
	int       transparent_index;
	Colour    transparent_rgb;

	/* For indexed colour: */
	Palette * pal;

	/* For PLTE files: */
	Palette * file_pal;
} Transform;

typedef void (*TransformFunc)(Transform *, byte *, byte *);

static int palette_size(int bit_depth)
{
	switch (bit_depth) {
		case 1: return 2;
		case 2: return 4;
		case 4: return 16;
		case 8: return 256;
		default: return 256;
	}
}

static Palette *generate_greyscale_palette(int max_size, int transparent,
	Transform *transform)
{
	Palette *pal;
	int i, loc, value, denom, greys;

	if (transparent)
		transparent = 1;

	pal = app_new_palette(max_size, NULL);

	denom = max_size - 1;
	if (transparent)
		denom--;
	if (denom <= 0)
		denom = 1;
	greys = max_size - transparent;
	for (i=0, loc=0; i < greys; i++) {
		value = (255L * i) / denom;
		pal->element[loc++] = rgb(value, value, value);
	}
	if (transparent)
		if (max_size > 0) {
			pal->element[loc] = argb(255,255,255,255);
		}
	if (transform) {
		transform->side = 0;
		transform->volume = 0;
		transform->greys = greys;
		transform->grey_start = 0;
		transform->black_index = 0;
		transform->white_index = max_size - transparent - 1;
		transform->transparent = transparent;
		if (transparent) {
			transform->transparent_index = loc;
			transform->transparent_rgb = pal->element[loc];
		}
		transform->pal = pal;
	}
	return pal;
}

static Palette *generate_colour_cube(int max_size, int transparent,
	Transform *transform)
{
	Palette *pal;
	int i, loc, x, y, z, r, g, b;
	int side, denom, volume, greys;

	if (transparent)
		transparent = 1;

	/* Create palette */
	pal = app_new_palette(max_size, NULL);

	/* Determine cube size */
	greys = 0;
	for (side=6; side > 0; side--) {
		volume = side * side * side;
		if (volume <= max_size - transparent) {
			greys = max_size - transparent - volume;
			break;
		}
	}

	/* Generate colour cube */
	loc = 0;
	denom = side - 1;
	if (denom == 0)
		denom = 1;
	for (x=0; x < side; x++)
	{
		r = (255L * x) / denom;
		for (y=0; y < side; y++)
		{
			g = (255L * y) / denom;
			for (z=0; z < side; z++)
			{
				b = (255L * z) / denom;
				pal->element[loc++] = rgb(r,g,b);
			}
		}
	}

	/* Add a greyscale ramp at the end. We already have black and white,
	 * so we omit those from the range generated. */
	denom = greys + 1;
	for (i=1; i <= greys; i++)
	{
		g = (255L * i) / denom;
		pal->element[loc++] = rgb(g,g,g);
	}
	if (transparent)
		if (max_size > 0) {
			pal->element[loc] = argb(255,255,255,255);
		}

	if (transform) {
		transform->side = side;
		transform->volume = volume;
		transform->greys = greys;
		transform->grey_start = volume;
		transform->black_index = 0;
		transform->white_index = volume - 1;
		transform->transparent = transparent;
		if (transparent) {
			transform->transparent_index = loc;
			transform->transparent_rgb = pal->element[loc];
		}
		transform->pal = pal;
	}
	return pal;
}

/*
 * Implement transformations to 8-bit paletted output:
 */

void transform_copy(Transform *transform, byte *row, byte *dest)
{
	/* No transformation, just copy the data. */
	long width = transform->width;

	memcpy(dest, row, width);
}

void transform_g_to_ramp(Transform *transform, byte *row, byte *dest)
{
	/* Greyscale data, 8-bit samples, significant bits highest. */
	/* Transform to a palette which is a greyscale ramp black to white. */
	long i;
	long width = transform->width;
	long max_grey = transform->greys - 1;

	for (i=0; i < width; i++)
		*dest++ = (byte) (((*row++) * max_grey) / 255);
}

void transform_ga_to_ramp(Transform *transform, byte *row, byte *dest)
{
	/* Greyscale + alpha, 8-bit samples, significant bits highest. */
	/* Transform to a palette which is a greyscale ramp black to white. */
	/* Assume there is a transparent entry in the palette. */
	long i;
	int g, a;
	long width = transform->width;
	long max_grey = transform->greys - 1;
	int transparent_index = transform->transparent_index;

	for (i=0; i < width; i++) {
		g = *row++;
		a = *row++;
		if (a > 0x7f)
			*dest++ = transparent_index;
		else
			*dest++ = (byte) ((g * max_grey) / 255);
	}
}

void transform_rgb_to_cube(Transform *transform, byte *row, byte *dest)
{
	/* RGB data, 8-bit samples. */
	/* Transform to a colour cube + greyscale ramp. */
	long i;
	long width = transform->width;
	int r, g, b, value;
	int side = transform->side;
	int greys = transform->greys;
	int grey_start = transform->grey_start;
	int grey_end = grey_start + greys;
	int black_index = transform->black_index;
	int white_index = transform->white_index;
	long max_side = side - 1;
	long max_grey = greys - 1;

	for (i=0; i < width; i++) {
		r = *row++;
		g = *row++;
		b = *row++;
		if ((r == g) && (g == b) && (greys > 0)) {
			value = grey_start - 1 + ((g * max_grey) / 255);
			if (value < grey_start)
				value = black_index;
			else if (value >= grey_end)
				value = white_index;
			*dest++ = value;
		}
		else {
			value = (r * max_side) / 255;
			value *= side;
			value += (g * max_side) / 255;
			value *= side;
			value += (b * max_side) / 255;
			*dest++ = value;
		}
	}
}

void transform_rgba_to_cube(Transform *transform, byte *row, byte *dest)
{
	/* RGBA data, 8-bit samples. */
	/* Transform to a colour cube + greyscale ramp + transparent entry. */
	long i;
	long width = transform->width;
	int r, g, b, a, value;
	int side = transform->side;
	int greys = transform->greys;
	int grey_start = transform->grey_start;
	int grey_end = grey_start + greys;
	int black_index = transform->black_index;
	int white_index = transform->white_index;
	int transparent = transform->transparent;
	int transparent_index = transform->transparent_index;
	long max_side = side - 1;
	long max_grey = greys - 1;

	for (i=0; i < width; i++) {
		r = *row++;
		g = *row++;
		b = *row++;
		a = *row++;
		if ((a > 0x7F) && (transparent)) {
			*dest++ = transparent_index;
		}
		else if ((r == g) && (g == b) && (greys > 0)) {
			value = grey_start - 1 + (g * max_grey / 255);
			if (value < grey_start)
				value = black_index;
			else if (value >= grey_end)
				value = white_index;
			*dest++ = value;
		}
		else {
			value = (r * max_side) / 255;
			value *= side;
			value += (g * max_side) / 255;
			value *= side;
			value += (b * max_side) / 255;
			*dest++ = value;
		}
	}
}

void dither_g(Transform *transform, byte *row, byte *dest)
{
	/* Greyscale data, 8-bit samples, significant bits highest. */
	/* Dither to an arbitrary given palette. */
	long i;
	long width = transform->width;
	int g;
	Colour *rgb_row;

	rgb_row = app_alloc(width * sizeof(Colour));
	for (i=0; i < width; i++) {
		g = (*row++);
		rgb_row[i] = rgb(g,g,g);
	}
	app_palette_translation(transform->pal, dest, width, rgb_row);
	app_free(rgb_row);
}

void dither_ga(Transform *transform, byte *row, byte *dest)
{
	/* Greyscale + alpha data, 8-bit samples, significant bits highest. */
	/* Dither to an arbitrary given palette. */
	long i;
	long width = transform->width;
	int g, a;
	Colour *rgb_row;

	rgb_row = app_alloc(width * sizeof(Colour));
	for (i=0; i < width; i++) {
		g = (*row++);
		a = (*row++);
		rgb_row[i] = rgb(g,g,g);
		rgb_row[i].alpha = a;
	}
	app_palette_translation(transform->pal, dest, width, rgb_row);
	app_free(rgb_row);
}

void dither_rgb(Transform *transform, byte *row, byte *dest)
{
	/* RGB data, 8-bit samples, significant bits highest. */
	/* Dither to an arbitrary given palette. */
	long i;
	long width = transform->width;
	int r, g, b;
	Colour *rgb_row;

	rgb_row = app_alloc(width * sizeof(Colour));
	for (i=0; i < width; i++) {
		r = (*row++);
		g = (*row++);
		b = (*row++);
		rgb_row[i] = rgb(r,g,b);
	}
	app_palette_translation(transform->pal, dest, width, rgb_row);
	app_free(rgb_row);
}

void dither_rgba(Transform *transform, byte *row, byte *dest)
{
	/* RGB data, 8-bit samples, significant bits highest. */
	/* Dither to an arbitrary given palette. */
	long i;
	long width = transform->width;
	int r, g, b, a;
	Colour *rgb_row;

	rgb_row = app_alloc(width * sizeof(Colour));
	for (i=0; i < width; i++) {
		r = (*row++);
		g = (*row++);
		b = (*row++);
		a = (*row++);
		rgb_row[i] = rgb(r,g,b);
		rgb_row[i].alpha = a;
	}
	app_palette_translation(transform->pal, dest, width, rgb_row);
	app_free(rgb_row);
}

/*
 * Read a PNG file.
 * Assume the file has been opened and is known to be a PNG file.
 */

int app_read_png(ImageReader *reader)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned int sig_read = reader->bytes_read;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	/* png_color_16 my_background, *image_background; */
	char *gamma_str;
	double screen_gamma;
	int intent, number_passes;
	int rowbytes;
	byte *temp_data;
	byte **data_ptr;
	unsigned int row;
	int pass;
	unsigned int y;
	Transform transform;
	TransformFunc transform_data;

	reader->state = STOPPED;
	if (reader->file == NULL)
		return IMAGE_ERROR;

	/* Set starting state, call startup function. */
	reader->state = STARTING;
	if (reader->startup_func)
		if (! reader->startup_func(reader)) {
			return IMAGE_ERROR;
	}

	/* Create and initialize the png_struct with the desired error handler
	 * functions.  If you want to use the default stderr and longjmp method,
	 * you can supply NULL for the last three parameters. We also supply the
	 * the compiler header file version, so that we know if the application
	 * was compiled with a compatible version of the library.  REQUIRED
	 */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL)
	{
		return IMAGE_ERROR;
	}

	/* Allocate/initialize the memory for image information.  REQUIRED. */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		return IMAGE_ERROR;
	}

	/* Set error handling using the setjmp/longjmp method (this is the
	 * normal method of doing things with libpng).  REQUIRED unless you set
	 * up your own error handlers in the png_create_read_struct() earlier.
	 */
	if (setjmp(png_ptr->jmpbuf))
	{
		/* Free all memory associated with the png_ptr and info_ptr */
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		/* If we get here, we had a problem reading the file */
		reader->state = IMAGE_ERROR;
		if (reader->error_func)
			reader->error_func(reader);
		return IMAGE_ERROR;
	}

	/* Initialise some variables to stop spurious complaints */
	data_ptr = NULL;
	transform_data = transform_copy;

	/* Set up the input control if you are using standard C streams */
	png_init_io(png_ptr, reader->file);

	/* If we have already read some of the signature */
	png_set_sig_bytes(png_ptr, sig_read);

	/* The call to png_read_info() gives us all of the information from the
	 * PNG file before the first IDAT (image data chunk).  REQUIRED
	 */
	png_read_info(png_ptr, info_ptr);

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		 &interlace_type, NULL, NULL);

	reader->width = width;
	reader->height = height;
	reader->max_stages = 1;	/* dummy values for now */
	reader->stage = 0;
	reader->row = 0;
	reader->rows_done = 0;
	reader->row_height = 1;

	/* tell libpng to strip 16 bit/color files down to 8 bits/color */
	png_set_strip_16(png_ptr);

	/* Make the alpha channel measure transparency, not opaqueness,
	 * by inverting the alpha channel.
	 */
	png_set_invert_alpha(png_ptr);

	/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
	 * byte into separate bytes (useful for paletted and grayscale images).
	 */
	png_set_packing(png_ptr);

	/* Extract grayscale pixels with bit depths of 1, 2, and 4 from a single
	 * byte into separate bytes.
	 */
	png_set_gray_1_2_4_to_8(png_ptr);

	if (reader->required_depth == 8)
	{
		/* Expand paletted colors into true RGBA quartets */
		if (color_type == PNG_COLOR_TYPE_PALETTE) {
			png_set_palette_to_rgb(png_ptr);
			if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
				png_set_tRNS_to_alpha(png_ptr);
			png_set_filler(png_ptr, 0x00, PNG_FILLER_AFTER);
		}
	}
	else if (reader->required_depth == 32)
	{
		/* Expand paletted colors into true RGB triplets */
		if (color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_palette_to_rgb(png_ptr);

		/* Expand greyscale images to RGB format. */
		if (color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
				png_set_gray_to_rgb(png_ptr);

		/* Expand paletted or RGB images with transparency to full alpha
		 * channels so the data will be available as RGBA quartets.
		 */
		if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
			png_set_tRNS_to_alpha(png_ptr);

		/* Add filler (alpha) byte before each RGB triplet, if needed */
		png_set_filler(png_ptr, 0x00, PNG_FILLER_BEFORE);

		/* swap the RGBA or GA data to ARGB or AG (or BGRA to ABGR) */
		png_set_swap_alpha(png_ptr);
	}

	/* Set the background color to draw transparent and alpha images over.
	 * It is possible to set the red, green, and blue components directly
	 * for paletted images instead of supplying a palette index.  Note that
	 * even if the PNG file supplies a background, you are not required to
	 * use it - you should use the (solid) application background if it has one.
	 */

	/*
	my_background.red = 255;
	my_background.green = 255;
	my_background.blue = 255;
	image_background = &my_background;

	if (png_get_bKGD(png_ptr, info_ptr, &image_background))
		png_set_background(png_ptr, image_background,
						   PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
	else
		png_set_background(png_ptr, &my_background,
						   PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
	*/

	/* Some suggestions as to how to get a screen gamma value */

	/* Note that screen gamma is the display_exponent, which includes
	 * the CRT_exponent and any correction for viewing conditions */
	if (0) /* We have a user-defined screen gamma value */
	{
		screen_gamma = 2.2; /* user-defined screen_gamma; */
	}
	/* This is one way that applications share the same screen gamma value */
	else if ((gamma_str = getenv("SCREEN_GAMMA")) != NULL)
	{
		screen_gamma = atof(gamma_str);
	}
	/* If we don't have another value */
	else
	{
		screen_gamma = 2.2;  /* A good guess for a PC monitors in a dimly
								lit room */
	}

	/* Tell libpng to handle the gamma conversion for you.  The second call
	 * is a good guess for PC generated images, but it should be configurable
	 * by the user at run time by the user.  It is strongly suggested that
	 * your application support gamma correction.
	 */

	if (png_get_sRGB(png_ptr, info_ptr, &intent))
		png_set_sRGB(png_ptr, info_ptr, intent);
	else
	{
		double image_gamma;
		if (png_get_gAMA(png_ptr, info_ptr, &image_gamma))
			png_set_gamma(png_ptr, screen_gamma, image_gamma);
		else
			png_set_gamma(png_ptr, screen_gamma, 0.45455);
	}

	/* Dither RGB files down to 8 bit palette or reduce palettes
	 * to the number of colors available on your screen, if required.
	 */
	reader->state = DITHERING;

	if (reader->required_depth == 8)
	{
		int i, max, num_palette;
		Colour col;
		png_colorp palette;
		png_uint_16p histogram;

		/* This reduces the image to the application supplied palette */
		if (reader->src_pal) /* we have our own palette */
		{
			max = reader->src_pal->size;
			/*
			palette = app_alloc(sizeof(png_color) * max);

			for (i=0; i < max; i++) {
				col = reader->src_pal->element[i];
				palette[i].red   = col.red;
				palette[i].green = col.green;
				palette[i].blue  = col.blue;
			}

			if (! png_get_hIST(png_ptr, info_ptr, &histogram))
				histogram = NULL;

			png_set_dither(png_ptr, palette, max, max, histogram, 0);

			app_free(palette);
			*/
			reader->pal = app_new_palette(max, reader->src_pal->element);
			transform.pal = reader->pal;
			if (color_type == PNG_COLOR_TYPE_RGB)
				transform_data = dither_rgb;
			else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
				transform_data = dither_rgba;
			else if (color_type == PNG_COLOR_TYPE_PALETTE)
				transform_data = dither_rgba;
			else if (color_type == PNG_COLOR_TYPE_GRAY)
				transform_data = dither_g;
			else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
				transform_data = dither_ga;
		}
		/* This reduces the image to the palette supplied in the file */
		else if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette))
		{
			max = reader->max_cmap_size;
			if (max <= 0)
				max = 256;
			if (max > num_palette)
				max = num_palette;

			if (! png_get_hIST(png_ptr, info_ptr, &histogram))
				histogram = NULL;

			png_set_dither(png_ptr, palette, num_palette, max, histogram, 0);

			reader->pal = app_new_palette(max, NULL);
			for (i=0; i < max; i++) {
				col = rgb(palette[i].red, palette[i].green, palette[i].blue);
				reader->pal->element[i] = col;
			}
			transform.pal = reader->pal;
			if (color_type == PNG_COLOR_TYPE_RGB)
				transform_data = dither_rgb;
			else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
				transform_data = dither_rgba;
			else if (color_type == PNG_COLOR_TYPE_PALETTE)
				transform_data = dither_rgba;
			else if (color_type == PNG_COLOR_TYPE_GRAY)
				transform_data = dither_g;
			else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
				transform_data = dither_ga;
		}
		/* Quantize, or use a colour cube */
		else if (color_type == PNG_COLOR_TYPE_RGB)
		{
			/* Because we're reading the image progresively,
			 * quantization is not feasible (we'd have to read
			 * the entire image first, then quantize to produce
			 * a valid palette to use). So, we find the largest
			 * colour cube we can, and use that approximation.
			 */
			max = reader->max_cmap_size;
			if (max <= 0)
				max = 256;

			reader->pal = generate_colour_cube(max, 1, &transform);
			transform_data = transform_rgb_to_cube;
		}
		/* Quantize, or use a colour cube, with transparency */
		else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
		{
			max = reader->max_cmap_size;
			if (max <= 0)
				max = 256;

			reader->pal = generate_colour_cube(max, 1, &transform);
			transform_data = transform_rgba_to_cube;
		}
		/* Handle paletted images as RGBA, map to colour cube */
		else if (color_type == PNG_COLOR_TYPE_PALETTE) {
			max = reader->max_cmap_size;
			if (max <= 0)
				max = 256;

			reader->pal = generate_colour_cube(max, 1, &transform);
			transform_data = transform_rgba_to_cube;
		}
		/* Generate a greyscale palette to use */
		else if (color_type == PNG_COLOR_TYPE_GRAY)
		{
			max = palette_size(bit_depth);
			if (reader->max_cmap_size > 0)
				if (max > reader->max_cmap_size)
					max = reader->max_cmap_size;

			reader->pal = generate_greyscale_palette(max, 0, &transform);
			transform_data = transform_g_to_ramp;
		}
		/* Generate a greyscale palette with one transparent entry */
		else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		{
			max = palette_size(bit_depth);
			if (reader->max_cmap_size > 0)
				if (max > reader->max_cmap_size)
					max = reader->max_cmap_size;

			reader->pal = generate_greyscale_palette(max, 1, &transform);
			transform_data = transform_ga_to_ramp;
		}
	}

	/* call after_dither function */
	if (reader->after_dither_func)
		if (! reader->after_dither_func(reader)) {
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			return IMAGE_ERROR;
		}

	/* Turn on interlace handling.  REQUIRED if you are not using
	 * png_read_image().  To see how to handle interlacing passes,
	 * see the png_read_row() method below:
	 */
	number_passes = png_set_interlace_handling(png_ptr);

	/* Optional call to gamma correct and add the background to the palette
	 * and update info structure.  REQUIRED if you are expecting libpng to
	 * update the palette for you (ie you selected such a transform above).
	 */
	png_read_update_info(png_ptr, info_ptr);

	/* Allocate the memory to hold the image using the fields of info_ptr. */

	rowbytes = png_get_rowbytes(png_ptr, info_ptr);

	if (reader->required_depth == 8)
	{
		reader->data8 = app_alloc(height * sizeof(void *));
		for (row = 0; row < height; row++)
			reader->data8[row] = app_alloc(reader->width);
		temp_data = app_alloc(rowbytes); /* use separate array */
		data_ptr = &temp_data;
	}
	else if (reader->required_depth == 32)
	{
		reader->data32 = app_alloc(height * sizeof(void *));
		for (row = 0; row < height; row++)
			reader->data32[row] = app_alloc(reader->width * sizeof(Colour));
	}

	/* Set up transformation: */
	transform.width = width;
	transform.height = height;
	transform.rowbytes = rowbytes;

	/* Now it's time to read the image. */
	/* Read several rows at a time and deal with interlacing: */

	reader->max_stages = number_passes;
	reader->row_height = 1;

	for (pass = 0; pass < number_passes; pass++)
	{
		reader->stage = pass + 1;
		reader->rows_done = 0;

		for (y = 0; y < height; y++)
		{
			reader->row = y;

			if (reader->required_depth == 8) {
				png_read_rows(png_ptr, NULL, data_ptr, 1);
				transform_data(&transform, temp_data, reader->data8[y]);
			}
			else {
				data_ptr = (byte **) &reader->data32[y];
				png_read_rows(png_ptr, NULL, data_ptr, 1);
			}

			reader->rows_done++;

			if (reader->progress_func)
				if (! reader->progress_func(reader)) {
					png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
					return IMAGE_ERROR;
				}
			if (reader->rendering_func)
				if (! reader->rendering_func(reader)) {
					png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
					return IMAGE_ERROR;
				}
		}
	}

	/* Free temporary array of data */
	if (reader->required_depth == 8)
		app_free(temp_data);

	/* read rest of file, and get additional chunks in info_ptr - REQUIRED */
	png_read_end(png_ptr, info_ptr);

	/* clean up after the read, and free any memory allocated - REQUIRED */
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

	/* success! */
	if (reader->success_func)
		if (! reader->success_func(reader)) {
			return IMAGE_ERROR;
		}

	/* that's it */
	reader->state = STOPPED;
	return STOPPED;
}

