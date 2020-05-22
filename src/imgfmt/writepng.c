/*
 *  Saving PNG files.
 *
 *  Platform: Neutral
 *
 *  Version: 3.57  2002/08/09  Added saving of PNG file format.
 *  Version: 3.60  2007/06/06  Can now save DPI.
 */

/* Copyright (c) L. Patrick and the LibPNG group.

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/


#include <stdio.h>
#include <png.h>
#include "app.h"

/*
 *  Write a PNG file, either as an 8bpp paletted image, or a 32bpp image.
 */
int app_save_png(Image *img, const char *filename, int dpi, int interlace)
{
	FILE *fp;
	png_structp png_ptr;
	png_infop info_ptr;
	png_colorp palette;
	png_color_8 sig_bit;
	int pass, number_passes;

	/* open the file */
	fp = fopen(filename, "wb");
	if (fp == NULL)
		return 0;

	/* Create and initialize the png_struct with the desired error
	 * handler functions.
	 * If you want to use the default stderr and longjump method,
	 * you can supply NULL for the last three parameters (which we do).
	 * We also check that the library version is compatible with
	 * the one used at compile time, in case we are using dynamically
	 * linked libraries.  REQUIRED.
	 */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
					NULL, NULL, NULL);

	if (png_ptr == NULL)
	{
		fclose(fp);
		return 0;
	}

	/* Allocate/initialize the image information data.  REQUIRED */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		fclose(fp);
		png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
		return 0;
	}

	/* Set error handling.  REQUIRED if you aren't supplying your own
	 * error handling functions in the png_create_write_struct() call.
	 */
	if (setjmp(png_ptr->jmpbuf))
	{
		/* If we get here, we had a problem reading the file */
		fclose(fp);
		png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
		return 0;
	}

	/* One of the following I/O initialization functions is REQUIRED */
	/* set up the output control if you are using standard C streams */
	png_init_io(png_ptr, fp);

	/* Set the image information here.  Width and height are up to 2^31,
	 * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also
	 * depend on the color_type selected.
	 * color_type is one of PNG_COLOR_TYPE_GRAY,
	 * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE,
	 * PNG_COLOR_TYPE_RGB, or PNG_COLOR_TYPE_RGB_ALPHA.
	 * Interlacing is either PNG_INTERLACE_NONE or PNG_INTERLACE_ADAM7,
	 * and the compression_type and filter_type MUST currently be
	 * PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
	 */
	png_set_IHDR(png_ptr, info_ptr, img->width, img->height,
		8,
		(img->depth <= 8) ?
			PNG_COLOR_TYPE_PALETTE : 
			PNG_COLOR_TYPE_RGB_ALPHA,
		(interlace) ?
			PNG_INTERLACE_ADAM7 :
			PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	/* Set the physical pixel size (resolution, given in DPI). */
	if (dpi > 0)
	{
		/* one metre = 100 centimetre (cm), and 2.54 cm = 1 inch */
		/* 1 metre is about 40 inches (well, 100/2.54 or 39.37) */
		/* so the number of dots per metre is about 40 times */
		/* larger than the number of dots per inch */
		/* thus DPM = DPI * 100 / 2.54 = DPI * 10000 / 254 */ 
		int ppm_x, ppm_y; /* pixels per metre */
		ppm_x = (dpi * 10000 + 127) / 254; /* round to nearest */
		ppm_y = ppm_x;
		png_set_pHYs(png_ptr, info_ptr, ppm_x, ppm_y,
			PNG_RESOLUTION_METER);
	}

	/* Set the palette if there is one.
	 * REQUIRED for indexed-color images. */
	if (img->depth <= 8)
	{
		int i;
		palette = (png_colorp) png_malloc(png_ptr,
						256 * sizeof (png_color));
		/* Set palette colors. */
		for (i=0; i < img->cmap_size; ++i)
		{
			palette[i].red   = img->cmap[i].red;
			palette[i].green = img->cmap[i].green;
			palette[i].blue  = img->cmap[i].blue;
			/* palette[i].alpha = 256 - img->cmap[i].alpha; */
		}
		png_set_PLTE(png_ptr, info_ptr, palette, img->cmap_size);
	}

	/* optional significant bit chunk */
	/* if we are dealing with a grayscale image then */
	sig_bit.gray = 0;
	/* otherwise, if we are dealing with a color image then */
	sig_bit.red = 8;
	sig_bit.green = 8;
	sig_bit.blue = 8;
	sig_bit.alpha = 8;
	png_set_sBIT(png_ptr, info_ptr, &sig_bit);

	/* Optional gamma chunk is strongly suggested if you have any guess
	 * as to the correct gamma of the image.
	 */
	/* png_set_gAMA(png_ptr, info_ptr, 2.2); */ /* Guess a PC in a dim room. */

	/* Optionally write comments into the image */
	/*
	text_ptr[0].key = "Title";
	text_ptr[0].text = "Mona Lisa";
	text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
	text_ptr[1].key = "Author";
	text_ptr[1].text = "Leonardo DaVinci";
	text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
	text_ptr[2].key = "Description";
	text_ptr[2].text = "<long text>";
	text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;
	png_set_text(png_ptr, info_ptr, text_ptr, 3);
	*/

	/* other optional chunks like cHRM, bKGD, tRNS, tIME, oFFs, pHYs, */
	/* note that if sRGB is present the cHRM chunk must be ignored
	 * on read and must be written in accordance with the sRGB profile */

	png_set_invert_alpha(png_ptr); /* needed? */

	/* Write the file header information.  REQUIRED */
	png_write_info(png_ptr, info_ptr);

	/* Once we write out the header, the compression type on the text
	 * chunks gets changed to PNG_TEXT_COMPRESSION_NONE_WR or
	 * PNG_TEXT_COMPRESSION_zTXt_WR, so it doesn't get written out again
	 * at the end.
	 */

	/* set up the transformations you want.  Note that these are
	 * all optional.  Only call them if you want them.
	 */

	/* swap location of alpha bytes from ARGB to RGBA */
	png_set_swap_alpha(png_ptr); /* needed? */

	/* turn on interlace handling if you are not using png_write_image() */
	if (interlace)
		number_passes = png_set_interlace_handling(png_ptr);
	else
		number_passes = 1;

	/* The easiest way to write the image is to write it in one go. */
	/* But we don't do that because we might be interlacing. */
#ifdef DO_NOT_COMPILE
	if (img->depth <= 8)
	{
		/*
		for (h = 0; h < height; h++)
			row_pointers[h] = img->data8[h];
		*/
		png_write_image(png_ptr, (png_bytepp) img->data8);
	}
	else
	{
		/*
		for (h = 0; h < height; h++)
			row_pointers[h] = img->data32[h];
		*/
		png_write_image(png_ptr, (png_bytepp) img->data32);
	}
#endif

	if (img->depth <= 8)
	{
		/* The number of passes is either
		 * 1 for non-interlaced images,
		 * or 7 for interlaced images.
		 */
		for (pass = 0; pass < number_passes; pass++)
		{
			int y;
	
			/* Write one row at a time. */
			for (y = 0; y < img->height; y++)
			{
				png_write_rows(png_ptr, &img->data8[y], 1);
			}
		}
	}
	else
	{
		/* The number of passes is either
		 * 1 for non-interlaced images,
		 * or 7 for interlaced images.
		 */
		for (pass = 0; pass < number_passes; pass++)
		{
			int y;
	
			/* Write one row at a time. */
			for (y = 0; y < img->height; y++)
			{
				png_write_rows(png_ptr, (png_bytepp) &img->data32[y], 1);
			}
		}
	}

	/* You can write optional chunks like tEXt, zTXt, and tIME at the end
	 * as well.
	 */

	/* It is REQUIRED to call this to finish writing the rest of the file */
	png_write_end(png_ptr, info_ptr);

	/* if you malloced the palette, free it here */
	if (img->depth <= 8)
		png_free(png_ptr, info_ptr->palette);

	/* if you allocated any text comments, free them here */

	/* clean up after the write, and free any memory allocated */
	png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

	/* close the file */
	fclose(fp);

	/* that's it */
	return 1;
}

