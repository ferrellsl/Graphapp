/*
 * Load and display a JPEG file.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/07/25  First release.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>
#include <string.h>
#include <setjmp.h>

#include "app.h"
#include "readjpg.h"
#include <jpeglib.h>

/*
 * IMAGE DATA FORMATS:
 *
 * The standard input image format is a rectangular array of pixels, with
 * each pixel having the same number of "component" values (color channels).
 * Each pixel row is an array of JSAMPLEs (which typically are unsigned chars).
 * If you are working with color data, then the color values for each pixel
 * must be adjacent in the row; for example, R,G,B,R,G,B,R,G,B,... for 24-bit
 * RGB color.
 *
 */

/*
 * ERROR HANDLING:
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Extended error handler struct:
 */

struct my_error_mgr {
	struct jpeg_error_mgr pub; /* "public" fields */

	jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 *  We also extend the progress manager structure to include a
 *  pointer to the ImageReader we are using.
 */

struct my_progress_mgr {
	struct jpeg_progress_mgr pub;	/* JPEG library fields */
	ImageReader *reader;
};

typedef struct my_progress_mgr * my_progress_ptr;

/*
 * Here's the routine that replaces the standard error_exit method:
 */

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr) cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

/*
 * This routine overrides the normal message display routine:
 */

METHODDEF(void)
my_output_message (j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];
	my_progress_ptr progress;
	ImageReader *reader;

	progress = (my_progress_ptr) cinfo->progress;
	reader = progress->reader;

	/* Create the message */
	(*cinfo->err->format_message) (cinfo, buffer);

	/* Display it using the reader's message function */
	if (reader->message_func)
		reader->message_func(reader, buffer);
}


/*
 *  Progress monitor:
 */


/* Return control to the setjmp point */

static void stop_loading_file(j_common_ptr cinfo)
{
	my_error_ptr myerr = (my_error_ptr) cinfo->err;
	longjmp(myerr->setjmp_buffer, 1);
}


METHODDEF(void)
my_progress_monitor (j_common_ptr cinfo)
{
	my_progress_ptr progress;
	ImageReader *reader;

	progress = (my_progress_ptr) cinfo->progress;
	reader = progress->reader;

	reader->stage = progress->pub.completed_passes+1;
	reader->max_stages = progress->pub.total_passes;

	if (reader->progress_func)
		if (! reader->progress_func(reader))
			stop_loading_file(cinfo);
}


static void start_progress_monitor (j_common_ptr cinfo,
	my_progress_ptr progress, ImageReader *reader)
{
	/* Enable progress display, unless trace output is on */
	if (cinfo->err->trace_level == 0) {
		progress->pub.progress_monitor = my_progress_monitor;
		progress->reader = reader;
		cinfo->progress = &progress->pub;
	}
}


/*
 *  Create a palette based on the JPEG file's quantized palette.
 */

#define EXTRAS 2

static Palette *create_cmap(struct jpeg_decompress_struct * cinfo)
{
	int i, size;
	Colour elem[256];
	Colour additional[EXTRAS];
	JSAMPARRAY cmap;

	additional[0] = rgb(255, 255, 255);
	additional[1] = rgb(0, 0, 0);

	size = cinfo->actual_number_of_colors;
	cmap = cinfo->colormap;

	if (cinfo->out_color_components == 3) {
		/* RGB data */
		for (i=0; i < size; i++) {
			/* copy palette from JPEG colormap */
			elem[i] = rgb(cmap[0][i], cmap[1][i], cmap[2][i]);
		}
	}
	else if (cinfo->out_color_components == 1) {
		/* Greyscale */
		for (i=0; i < size; i++) {
			/* copy palette from JPEG colormap */
			elem[i] = rgb(cmap[0][i], cmap[0][i], cmap[0][i]);
		}
	}

	/* add some additional colours to the palette */
	for (i = 0; (i < EXTRAS) && (i + size < 256); i++) {
		elem[i + size] = additional[i];
	}

	return app_new_palette(size + EXTRAS, elem);
}

/*
 *  Create a colormap based on a palette, and add it to the JPEG structure.
 *  Sets the actual_number_of_colors field, and the colormap field.
 *  Flags quantize_colors as true to yield colormapped output.
 */
static void create_colormap(struct jpeg_decompress_struct * cinfo,
		Palette *pal)
{
	int i;
	JSAMPARRAY cmap;

	cmap = (*cinfo->mem->alloc_sarray)
		((j_common_ptr) cinfo, JPOOL_IMAGE, pal->size, 3);

	/* RGB data */
	for (i=0; i < pal->size; i++) {
		/* copy palette into JPEG colormap */
		cmap[0][i] = pal->element[i].red;
		cmap[1][i] = pal->element[i].green;
		cmap[2][i] = pal->element[i].blue;
	}

	cinfo->quantize_colors = 1;
	cinfo->desired_number_of_colors = pal->size;
	cinfo->actual_number_of_colors = pal->size;
	cinfo->colormap = cmap;
}

/*
 *  Read a JPEG image from an open file.
 *  Return IMAGE_ERROR is there is any error.
 */

int app_read_jpeg (ImageReader *reader)
{
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	 * Note that this struct must live as long as the main JPEG
	 * parameter struct, to avoid dangling-pointer problems.
	 */
	struct my_error_mgr	jerr;
	struct my_progress_mgr	progress;
	JSAMPARRAY buffer;	/* Output row buffer */
	int rowbytes;		/* byte row width in output buffer */
	int row;

	/* The file should already be open. */

	reader->state = STARTING;
	if (reader->file == NULL) {
		return IMAGE_ERROR;
	}

	/* Step 1: initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then
	 * override error_exit and output_message. */

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	jerr.pub.output_message = my_output_message;

	/* Establish the setjmp return context for
	 * my_error_exit to use. */

	if (setjmp(jerr.setjmp_buffer))
	{
		/* If we get here, the JPEG code has found an error.
		 * We need to clean up the JPEG object, and return.
		 */

		jpeg_destroy_decompress(&cinfo);
		if (reader->error_func)
			reader->error_func(reader);
		return IMAGE_ERROR;
	}

	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Initialize the progress monitor. */
	start_progress_monitor((j_common_ptr) &cinfo, &progress, reader);

	/* Step 2: specify data source (eg, a file). */

	jpeg_stdio_src(&cinfo, reader->file);

	/* Step 3: read file parameters with jpeg_read_header() */

	jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */

	/* Step 4: set parameters for decompression */

	cinfo.scale_denom = 1; /* scale = 1:1 */
	/* cinfo.dct_method = JDCT_FLOAT; */

	/* Determine final width and height. */
	reader->width = cinfo.image_width;
	reader->height = cinfo.image_height;

	reader->max_stages = 1;
	reader->row = 0;
	reader->rows_done = 0;
	reader->row_height = 1;

	if (reader->src_pal) {
		create_colormap(&cinfo, reader->src_pal);
	}
	else if (reader->required_depth == 8) {
		cinfo.quantize_colors = 1;
		cinfo.desired_number_of_colors = reader->max_cmap_size;
	}

	/* Call startup function. */

	if (reader->startup_func)
		if (! reader->startup_func(reader)) {
			jpeg_destroy_decompress(&cinfo);
			return IMAGE_ERROR;
		}

	/* Step 5: Start decompressor */

	reader->state = DITHERING;
	jpeg_start_decompress(&cinfo);

	/* We can ignore the return value since suspension is
	 * not possible with the stdio data source.
	 */

	if (reader->required_depth == 8)
		reader->pal = create_cmap(&cinfo);

	if (reader->after_dither_func)
		if (! reader->after_dither_func(reader)) {
			jpeg_destroy_decompress(&cinfo);
			return IMAGE_ERROR;
	}
 
	/* JSAMPLEs per row in output buffer */
	rowbytes = cinfo.output_width * cinfo.output_components;

	/* Make a one-row-high sample array that will go away when
	 * done with image. */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, rowbytes, 1);

	/* Allocate the ImageReader data pointers. */

	if (reader->required_depth == 8)
	{
		reader->data8 = app_alloc(reader->height * sizeof(void *));
		for (row = 0; row < reader->height; row++)
			reader->data8[row] = app_alloc(rowbytes);
	}
	else if (reader->required_depth == 32)
	{
		reader->data32 = app_alloc(reader->height * sizeof(void *));
		for (row = 0; row < reader->height; row++)
			reader->data32[row] = app_alloc(reader->width * sizeof(Colour));
	}

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline
	 * as the loop counter, so we don't have to keep track ourselves.
	 */

	reader->state = RENDERING;
	reader->rows_done = 0;
	reader->row_height = 1;
	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers
		 * to scanlines. Here the array is only one element long,
		 * but you could ask for more than one scanline at a
		 * time if that's more convenient.
		 */
		row = reader->row = cinfo.output_scanline;
		jpeg_read_scanlines(&cinfo, buffer, 1);

		if (reader->required_depth == 8)
			memcpy(reader->data8[row], buffer[0], rowbytes);
		else if (cinfo.output_components == 1) {
			int i,g;
			for (i=0; i < reader->width; i++) {
				g = buffer[0][i];
				reader->data32[row][i] = rgb(g,g,g);
			}
		}
		else {
			int i,r,g,b;
			for (i=0; i < reader->width; i++) {
				r = buffer[0][i*3];
				g = buffer[0][i*3+1];
				b = buffer[0][i*3+2];
				reader->data32[row][i] = rgb(r,g,b);
			}
		}
		reader->rows_done++;

		if (reader->progress_func)
			if (! reader->progress_func(reader)) {
				jpeg_destroy_decompress(&cinfo);
				return IMAGE_ERROR;
			}

		if (reader->rendering_func)
			if (! reader->rendering_func(reader)) {
				jpeg_destroy_decompress(&cinfo);
				return IMAGE_ERROR;
			}
	}

	/* Step 7: Finish decompression */

	/* Release JPEG decompressor. */
	jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* Step 8: Release JPEG decompression object */

	jpeg_destroy_decompress(&cinfo);

	/* At this point you may want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */

	/* success! */
	if (reader->success_func)
		if (! reader->success_func(reader)) {
			return IMAGE_ERROR;
		}

	/* that's it */
	reader->state = STOPPED;
	return STOPPED;
}


/*
 * SOME FINE POINTS:
 *
 * In the above code, we ignored the return value of jpeg_read_scanlines,
 * which is the number of scanlines actually read.  We could get away with
 * this because we asked for only one line at a time and we weren't using
 * a suspending data source.  See libjpeg.doc for more info.
 *
 * We cheated a bit by calling alloc_sarray() after jpeg_start_decompress();
 * we should have done it beforehand to ensure that the space would be
 * counted against the JPEG max_memory setting.  In some systems the above
 * code would risk an out-of-memory error.  However, in general we don't
 * know the output image dimensions before jpeg_start_decompress(), unless we
 * call jpeg_calc_output_dimensions().  See libjpeg.doc for more about this.
 *
 * Scanlines are returned in the same order as they appear in the JPEG file,
 * which is standardly top-to-bottom.  If you must emit data bottom-to-top,
 * you can use one of the virtual arrays provided by the JPEG memory manager
 * to invert the data.  See wrbmp.c for an example.
 *
 * As with compression, some operating modes may require temporary files.
 * On some systems you may need to set up a signal handler to ensure that
 * temporary files are deleted if the program is interrupted.  See libjpeg.doc.
 */
