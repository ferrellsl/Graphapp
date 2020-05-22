/*
 *  Image reading.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.10  2001/12/01  Fixed some bugs; added app_read_image_file.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced some size_t conversion warnings.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h" 
#include "readgif.h"
#include "readjpg.h"
#include "readpng.h"
#include "readh.h"

ImageReader *app_new_image_reader(void)
{
	ImageReader * reader;

	reader = app_zero_alloc(sizeof(struct ImageReader));
	return reader;
}

void app_del_image_reader(ImageReader *reader)
{
	int row;

	if (reader->data8) {
		for (row=0; row < reader->height; row++)
			app_free(reader->data8[row]);
	}
	app_free(reader->data8);

	if (reader->data32) {
		for (row=0; row < reader->height; row++)
			app_free(reader->data32[row]);
	}
	app_free(reader->data32);

	if (reader->pal)
		app_del_palette(reader->pal);

	app_free(reader);
}

Image *app_read_image(const char *filename, int required_depth)
{
	ImageReader * reader;
	Image * img;

	reader = app_new_image_reader();
	reader->filename = app_copy_string(filename);
	reader->required_depth = required_depth;
	img = app_read_image_progressively(reader);
	app_del_string(reader->filename);
	app_del_image_reader(reader);
	return img;
}

Image *app_read_image_file(FILE *file, int required_depth)
{
	ImageReader * reader;
	Image * img;

	reader = app_new_image_reader();
	reader->file = file;
	reader->required_depth = required_depth;
	img = app_read_image_progressively(reader);
	app_del_image_reader(reader);
	return img;
}

Image *app_read_image_memory(const byte *memsrc, int memsize, int required_depth)
{
	ImageReader * reader;
	Image * img;

	reader = app_new_image_reader();
	reader->memsrc = (byte *) memsrc;
	reader->memsize = memsize;
	reader->required_depth = required_depth;
	img = app_read_image_progressively(reader);
	app_del_image_reader(reader);
	return img;
}

Image *app_read_image_progressively(ImageReader *reader)
{
	int depth;
	int format = UNKNOWN_FORMAT;
	int result = IMAGE_ERROR;
	Image * img = NULL;

	depth = reader->required_depth;
	if (depth <= 8)
		depth = 8;
	else
		depth = 32;
	reader->required_depth = depth;

	if (reader->filename) {
		reader->file = app_open_file(reader->filename, "rb");
		if (! reader->file)
			return NULL;
	}
	
	if (reader->file) {
		format = app_find_image_format(reader->file);
	}
	else if (reader->memsrc) {
		format = app_find_image_format_in_memory(reader->memsrc, reader->memsize);
	}
	reader->bytes_read = 0;

	switch (format) {
	  case PNG_FORMAT:
		result = app_read_png(reader);
		break;
	  case JPEG_FORMAT:
		result = app_read_jpeg(reader);
		break;
	  case GIF_FORMAT:
		result = app_read_gif(reader);
		break;
	  case GA_H_FORMAT:
		/* cannot read this format interactively yet */
		img = app_read_header_image_file(reader->file);
		break;
	  default:
		break;
	}

	if (reader->filename)
		app_close_file(reader->file);
	reader->file = NULL;

	if (result != IMAGE_ERROR) {
		/* move data over into a new image structure */
		img = app_zero_alloc(sizeof(Image));
		img->depth = depth;
		img->width = reader->width;
		img->height = reader->height;
		img->data8 = reader->data8;
		img->data32 = reader->data32;
		if (reader->pal) {
			img->cmap = reader->pal->element;
			img->cmap_size = reader->pal->size;
		}
		else {
			img->cmap = NULL;
			img->cmap_size = 0;
		}

		/* unlink data from old structure */
		reader->data8 = NULL;
		reader->data32 = NULL;
		app_free(reader->pal);
		reader->pal = NULL;
	}

	return img;
}

int app_find_image_format(FILE *f)
{
	int i, pos, format;
	byte ch;
	byte buffer[8];
	long formats[] = {
		0x89504EL, PNG_FORMAT,
		0xFFD8FFL, JPEG_FORMAT,
		0x474946L, GIF_FORMAT,
		0x2F2A20L, GA_H_FORMAT
		};

	if (f == NULL)
		return UNKNOWN_FORMAT;

	i = (int) fread(buffer, 1, 3, f);
	fseek(f, -3, SEEK_CUR);
	if (i < 3)
		return UNKNOWN_FORMAT;

	for (i=0; i < sizeof(formats) / sizeof(formats[0]); i += 2) {
		format = formats[i+1];
		for (pos=0; pos < 3; pos++) {
			ch = (char) ((formats[i] >> ((2-pos)*8)) & 0x00FF);
			if (ch != buffer[pos]) {
				format = UNKNOWN_FORMAT;
				break;
			}
		}
		if (format != UNKNOWN_FORMAT)
			break;
	}
	return format;
}

int app_find_image_format_in_memory(byte *memsrc, int memsize)
{
	int i, pos, format;
	byte ch;
	long formats[] = {
		0x89504EL, PNG_FORMAT,
		0xFFD8FFL, JPEG_FORMAT,
		0x474946L, GIF_FORMAT,
		0x2F2A20L, GA_H_FORMAT
		};

	if (memsrc == NULL)
		return UNKNOWN_FORMAT;

	if (memsize < 3)
		return UNKNOWN_FORMAT;

	for (i=0; i < sizeof(formats) / sizeof(formats[0]); i += 2) {
		format = formats[i+1];
		for (pos=0; pos < 3; pos++) {
			ch = (char) ((formats[i] >> ((2-pos)*8)) & 0x00FF);
			if (ch != memsrc[pos]) {
				format = UNKNOWN_FORMAT;
				break;
			}
		}
		if (format != UNKNOWN_FORMAT)
			break;
	}
	return format;
}
