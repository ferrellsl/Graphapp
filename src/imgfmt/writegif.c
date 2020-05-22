/*
 *  Saving GIF files.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.10  2001/12/01  Changed transparency bitfield from 1 to 5.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.57  2005/08/16  Now returns a success indicator.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>

#include "app.h"
#include "writegif.h"
#include <gif.h>

static int bitsize(int cmap_length)
{
	int depth;

	for (depth = 1; depth <= 8; depth++)
		if ((1 << depth) >= cmap_length)
			break;
	return depth;
}

int app_save_gif(Image *img, const char *filename, int interlace)
{
	Gif *gif;
	GifPalette *cmap;
	GifPicture *pic;
	GifExtension *ext = NULL;
	unsigned char *data;
	GifBlock *block;
	int i, depth, size;
	Image *img8 = NULL;

	/* Create a blank Gif: */
	gif = new_gif();
	if (gif == NULL)
		return 0;

	if (img->depth > 8)
		img = img8 = app_image_convert_32_to_8(img);

	/* Set the screen information: */
	depth = bitsize(img->cmap_size);

	gif->screen->width      = img->width;
	gif->screen->height     = img->height;
	gif->screen->has_cmap   = 1;
	gif->screen->color_res  = 8;
	gif->screen->cmap_depth = depth;

	/* Fill the colour map: */
	cmap = gif->screen->cmap;
	cmap->length = 1 << depth;
	cmap->colours = app_alloc(cmap->length * sizeof(Colour));

	for (i=0; i < img->cmap_size; i++) {
		/* Append the colour: */
		cmap->colours[i] = img->cmap[i];

		if (img->cmap[i].alpha <= 0x7F)
			continue;	/* not transparent */

		/* Create transparent colour block: */
		if (ext)
			continue;	/* already made one */

		ext = new_gif_extension();
		ext->marker = 0xF9;
		ext->data = app_alloc(sizeof(GifData *));
		ext->data[0] = new_gif_data(4);
		data = ext->data[0]->bytes;
		data[0] = 0x05;	/* was 0x01 (no disposal method) */
		data[1] = 0x00;
		data[2] = 0x00;
		data[3] = i;
		ext->data_count++;

		/* Link the transparency block to the Gif: */
		block = new_gif_block();
		block->intro = 0x21;
		block->ext = ext;
		/* Append the block: */
		size = ++gif->block_count;
		gif->blocks = app_realloc(gif->blocks, size * sizeof(GifBlock *));
		gif->blocks[size-1] = block;
	}

	/* Fill rest of colour map with zeros: */
	for ( ; i < cmap->length; i++)
		cmap->colours[i] = rgb(0,0,0);

	/* Create a new GifPicture: */
	pic = new_gif_picture();
	pic->width      = img->width;
	pic->height     = img->height;
	if (interlace)
		pic->interlace  = 1; /* 0=no interlace, 1=interlace */
	else
		pic->interlace  = 0; /* 0=no interlace, 1=interlace */
	pic->has_cmap   = 0;
	pic->cmap_depth = depth; /* must be depth, despite not writing it */

	/* Link GifPicture data to supplied pixels: */
	pic->data = img->data8;

	/* Link the GifPicture to the Gif: */
	block = new_gif_block();
	block->intro = 0x2C;
	block->pic   = pic;

	/* Append the block: */
	size = ++gif->block_count;
	gif->blocks = app_realloc(gif->blocks, size * sizeof(GifBlock *));
	gif->blocks[size-1] = block;

	/* Write the Gif file: */
	write_gif_file(filename, gif);

	/* Unlink the pixel data and clean up: */
	pic->data = NULL;
	del_gif(gif);

	if (img8)
		app_del_image(img8);

	return 1;
}
