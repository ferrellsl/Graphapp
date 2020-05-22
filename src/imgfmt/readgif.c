/*
 *  Loading GIF files.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>
#include <string.h>

#include "app.h"
#include "readgif.h"
#include <gif.h>

static unsigned char read_byte(FILE *file)
{
	int ch = getc(file);
	if (ch == EOF)
		ch = 0;
	return ch;
}

static Image * gif_to_image(GifPicture *pic, GifExtension *ext, GifScreen *screen)
{
	Image * img = NULL;
	int trans_value;
	GifPalette *cmap;

	if (pic == NULL)
		return NULL;

	/* Find a colour map: */
	cmap = pic->cmap;
	if ((cmap == NULL) || (cmap->length == 0))
		cmap = screen->cmap;
	if (cmap == NULL)
		return NULL;

	/* Check for a transparent colour: */
	if ((ext) && (ext->marker == 0xF9))	/* graphic control block */
	{
		if ((ext->data) && (ext->data[0]->byte_count == 4)
			&& (ext->data[0]->bytes[0] & 0x01)) /* transparent flag */
		{
			trans_value = ext->data[0]->bytes[3];

			if (cmap->length > trans_value)
				cmap->colours[trans_value].alpha = 0xFF;
		}
	}

	/* Create an image: */
	img = gif_alloc(sizeof(Image));
	if (img == NULL)
		return NULL;

	/* Fill the image with information: */
	img->depth      = 8;
	img->width      = pic->width;
	img->height     = pic->height;
	img->cmap_size  = cmap->length;
	img->data8      = pic->data;
	img->cmap       = app_alloc(cmap->length * sizeof(Colour));
	memcpy(img->cmap, cmap->colours, cmap->length * sizeof(Colour));

	/* Unlink the pointers from their original locations: */
	/* ... not needed for cmap, since we copy this info above */
	/* cmap->colours = NULL; */
	pic->data = NULL;

	return img;
}

Image * app_load_gif(const char *filename)
{
	Image *img = NULL;
	Gif *gif;
	GifPicture *pic = NULL;
	GifExtension *ext = NULL;
	int i;

	/* Read the Gif file into memory: */
	gif = read_gif_file(filename);
	if (gif == NULL)
		return NULL;

	/* Find an image block and the extension just before it: */
	for (i=0; i < gif->block_count; i++) {
		if (gif->blocks[i]->ext) {
			ext = gif->blocks[i]->ext;
		}
		if (gif->blocks[i]->pic) {
			pic = gif->blocks[i]->pic;
			img = gif_to_image(pic, ext, gif->screen);
			break;
		}
	}
	/* Clean up and return the image *: */
	del_gif(gif);
	return img;
}

ImageList * app_load_gif_completely(const char *filename)
{
	Image *img = NULL;
	ImageList *imglist;
	Gif *gif;
	GifPicture *pic = NULL;
	GifExtension *ext = NULL;
	int i;

	/* Read the Gif file into memory: */
	gif = read_gif_file(filename);
	if (gif == NULL)
		return NULL;

	/* Create an ImageList: */
	imglist = app_new_image_list();

	/* Find an image block and the extension just before it: */
	for (i=0; i < gif->block_count; i++) {
		if (gif->blocks[i]->ext) {
			ext = gif->blocks[i]->ext;
		}
		if (gif->blocks[i]->pic) {
			pic = gif->blocks[i]->pic;
			img = gif_to_image(pic, ext, gif->screen);
			/* Append the image: */
			app_append_to_image_list(imglist, img);
		}
	}

	/* Clean up and return the image list: */
	del_gif(gif);
	return imglist;
}

static int read_gif_error(ImageReader *reader, Gif *gif, char *msg)
{
	if (gif)
		del_gif(gif);
	reader->state = IMAGE_ERROR;
	if (msg)
		if (reader->message_func)
			reader->message_func(reader, msg);
	if (reader->error_func)
		reader->error_func(reader);
	return IMAGE_ERROR;
}

int app_read_gif (ImageReader *reader)
{
	int i;
	FILE *file;
	Gif *gif;
	GifBlock *block;
	GifExtension *ext;
	GifPicture *pic;
	unsigned char info;
	byte *translation;
	int trans_value = -1; /* no transparency yet */
	Colour *target_cmap = NULL;
	unsigned char *data = NULL;
	GifDecoder *decoder;
	long w, h;
	int interlace_start[]  = {0, 4, 2, 1};
	int interlace_step[]   = {8, 8, 4, 2};
	int interlace_height[] = {8, 4, 2, 1};
	int scan_pass, row;

	/* The file should already be open. */

	reader->state = STARTING;
	file = reader->file;
	if (file == NULL)
		return read_gif_error(reader, NULL, "The file is not open.");

	/* Initialize GIF object, check header. */

	gif = new_gif();
	if (gif == NULL)
		return read_gif_error(reader, NULL, "Ran out of memory.");
	if ((reader->bytes_read > 0) && (reader->bytes_read <= 6))
		strncpy(gif->header, "GIF89a", reader->bytes_read);
	for (i=reader->bytes_read; i<6; i++)
		gif->header[i] = read_byte(file);
	if (strncmp(gif->header, "GIF", 3) != 0)
		return read_gif_error(reader, gif, "The file is not GIF.");

	/* Find image height and width. */

	read_gif_screen(file, gif->screen);

	block = new_gif_block();

	while (1)
	{
		block->intro = read_byte(file);

		if (block->intro == 0x2C) {	/* image */
			/* Don't read the image just yet: */
			block->pic = new_gif_picture();

			/* Append the image block and leave the loop: */
			i = ++gif->block_count;
			gif->blocks = app_realloc(gif->blocks, i * sizeof(GifBlock *));
			gif->blocks[i-1] = block;
			break;
		}
		else if (block->intro == 0x21) {	/* extension */
			/* Read the extension: */
			ext = new_gif_extension();
			read_gif_extension(file, ext);

			/* Is it a transparency extension? */
			if ((ext) && (ext->marker == 0xF9))
			{
				if ((ext->data) &&
				    (ext->data[0]->byte_count == 4) &&
				    (ext->data[0]->bytes[0] & 0x01))
				{
					trans_value = ext->data[0]->bytes[3];
				}
			}

			/* Discard it and continue looking for an image: */
			del_gif_extension(ext);
			continue;
		}
		else if (block->intro == 0x3B) {	/* terminator */
			del_gif_block(block);
			block = NULL;
			break;
		}
		else {	/* error! */
			del_gif_block(block);
			block = NULL;
			break;
		}
	}

	if (block == NULL)
		return read_gif_error(reader, gif, "No GIF image block found.");

	/* Read some preliminary information about the picture: */

	pic = block->pic;

	pic->left   = read_gif_int(file);
	pic->top    = read_gif_int(file);
	pic->width  = read_gif_int(file);
	pic->height = read_gif_int(file);

	info = read_byte(file);
	pic->has_cmap    = (info & 0x80) >> 7;
	pic->interlace   = (info & 0x40) >> 6;
	pic->sorted      = (info & 0x20) >> 5;
	pic->reserved    = (info & 0x18) >> 3;
	if (pic->has_cmap)
		pic->cmap_depth  = (info & 0x07) + 1;

	/* Set reader's width and height values. */

	reader->width = pic->width;
	reader->height = pic->height;

	reader->stage = 1;
	if (pic->interlace)
		reader->max_stages = 4;
	else
		reader->max_stages = 1;
	reader->row = 0;
	reader->rows_done = 0;
	reader->row_height = 1;

	/* Call startup function. */

	if (reader->startup_func)
		if (! reader->startup_func(reader)) {
			/* startup function told us to stop */
			return read_gif_error(reader, gif, NULL);
		}

	/* Read palette information. */

	reader->state = DITHERING;

	if (pic->has_cmap) {
		pic->cmap->length = 1 << pic->cmap_depth;
		read_gif_palette(file, pic->cmap);
	}

	if (trans_value >= 0) { /* handle transparency */
		if ((pic->has_cmap) && (trans_value < pic->cmap->length))
			pic->cmap->colours[trans_value].alpha = 0xFF;
		else if (trans_value < gif->screen->cmap->length)
			gif->screen->cmap->colours[trans_value].alpha = 0xFF;
	}

	/* Set target colour map. */

	if (reader->src_pal) {
		if (pic->cmap->length > 0) {
			translation = app_alloc(pic->cmap->length);
			app_palette_translation(reader->src_pal, translation,
				pic->cmap->length, pic->cmap->colours);
		}
		else {
			translation = app_alloc(gif->screen->cmap->length);
			app_palette_translation(reader->src_pal, translation,
				gif->screen->cmap->length, gif->screen->cmap->colours);
		}
		reader->pal = app_new_palette(reader->src_pal->size,
			reader->src_pal->element);
	}
	else if (reader->required_depth == 8) {
		translation = NULL;
		if (pic->cmap->length > 0)
			reader->pal = app_new_palette(pic->cmap->length,
				pic->cmap->colours);
		else
			reader->pal = app_new_palette(gif->screen->cmap->length,
				gif->screen->cmap->colours);
	}
	else /* required_depth == 32 */ {
		translation = NULL;
		if (pic->cmap->length > 0)
			target_cmap = pic->cmap->colours;
		else
			target_cmap = gif->screen->cmap->colours;
	}

	/* Finished dithering, so call the after_dither function. */

	if (reader->after_dither_func)
		if (! reader->after_dither_func(reader)) {
			/* after_dither function told us to stop */
			return read_gif_error(reader, gif, NULL);
		}
 
	/* Allocate the ImageReader data pointers. */

	if (reader->required_depth == 8)
	{
		reader->data8 = app_alloc(reader->height * sizeof(void *));
		for (row = 0; row < reader->height; row++)
			reader->data8[row] = app_zero_alloc(reader->width);
	}
	else if (reader->required_depth == 32)
	{
		data = app_alloc(reader->width); /* an input buffer */
		reader->data32 = app_alloc(reader->height * sizeof(void *));
		for (row = 0; row < reader->height; row++)
			reader->data32[row] = app_zero_alloc(reader->width * sizeof(Colour));
	}

	reader->state = RENDERING;

	w = pic->width;
	h = pic->height;

	decoder = new_gif_decoder();
	init_gif_decoder(file, decoder);

	if (pic->interlace) {
	  for (scan_pass = 0; scan_pass < 4; scan_pass++) {
		reader->stage = scan_pass+1;
		reader->row_height = interlace_height[scan_pass];
		row = interlace_start[scan_pass];
		reader->rows_done = row;
		while (row < h) {
			reader->row = row;
			if (reader->required_depth == 8)
				data = reader->data8[row];

			read_gif_line(file, decoder, data, w);

			if (translation) {
				for (i=0; i < reader->width; i++)
					data[i] = translation[data[i]];
			}
			if (reader->required_depth == 32) {
				for (i=0; i < reader->width; i++)
					reader->data32[row][i] = target_cmap[data[i]];
			}
			reader->rows_done = row + reader->row_height;

			if (reader->progress_func)
				if (! reader->progress_func(reader)) {
					/* progress function told us to stop */
					return read_gif_error(reader, gif, NULL);
				}

			if (reader->rendering_func)
				if (! reader->rendering_func(reader)) {
					/* rendering function told us to stop */
					return read_gif_error(reader, gif, NULL);
				}

			row += interlace_step[scan_pass];
		}
	  }
	}
	else {
	  reader->stage = 1;
	  reader->row_height = 1;
	  row = 0;
	  while (row < h) {
		reader->row = row;
		if (reader->required_depth == 8)
			data = reader->data8[row];

		read_gif_line(file, decoder, data, w);

		if (translation) {
			for (i=0; i < reader->width; i++)
				data[i] = translation[data[i]];
		}
		if (reader->required_depth == 32) {
			for (i=0; i < reader->width; i++)
				reader->data32[row][i] = target_cmap[data[i]];
		}
		reader->rows_done++;

		if (reader->progress_func)
			if (! reader->progress_func(reader)) {
				/* progress function told us to stop */
				return read_gif_error(reader, gif, NULL);
			}

		if (reader->rendering_func)
			if (! reader->rendering_func(reader)) {
				/* rendering function told us to stop */
				return read_gif_error(reader, gif, NULL);
			}

		row += 1;
	  }
	}
	/*finish_gif_picture(file, decoder);*/

	/* Release temporary structures. */

	del_gif_decoder(decoder);
	del_gif(gif);
	if (translation)
		app_free(translation);
	if (reader->required_depth == 32)
		app_free(data);

	/* Success! */
	if (reader->success_func)
		if (! reader->success_func(reader)) {
			/* success function told us to stop */
			return read_gif_error(reader, gif, NULL);
		}

	/* That's it. */
	reader->state = STOPPED;
	return STOPPED;
}
