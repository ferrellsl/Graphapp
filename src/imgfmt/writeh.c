/*
 *  Saving images to GraphApp header image file format.
 *
 *  Platform: Neutral.
 *
 *  Version: 3.22  2002/04/10  First release.
 *  Version: 3.42  2003/03/28  Fixed some serious bugs.
 *  Version: 3.43  2003/04/25  Decimal pixels, fewer exported symbols.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced a size_t conversion warning.
 *  Version: 3.57  2005/08/16  Now returns a success indicator.
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

static const char * header_comment    = "/* GraphApp image type 2 */\n";
static const char * depth_comment     = "/* depth  = %d */\n";
static const char * width_comment     = "/* width  = %d */\n";
static const char * height_comment    = "/* height = %d */\n";
static const char * cmap_size_comment = "/* cmap_size = %d */\n";

static int save_header_image_file(FILE *file, const char *name, Image *img)
{
	long     i, x, y, area;
	int      columns;
	Colour **data32;
	byte **	 data8;
	char *   as_hex     = "0x%-2.2X";
	char *   as_decimal = "%d";
	char *   fmt        = as_hex;
	char *   comma      = ", ";

	if (file == NULL)
		return 0;
	if (img == NULL)
		return 0;

	fprintf(file, header_comment);
	fprintf(file, depth_comment,  img->depth);
	fprintf(file, width_comment,  img->width);
	fprintf(file, height_comment, img->height);
	fprintf(file, cmap_size_comment, img->cmap_size);

	if (img->depth <= 8) {
		fprintf(file, "static Colour %s_cmap [] = {\n", name);
		for (i=0; i < img->cmap_size; i++) {
			fprintf(file, "\t{ ");
			fprintf(file, as_hex, img->cmap[i].alpha);
			fprintf(file, ", ");
			fprintf(file, as_hex, img->cmap[i].red);
			fprintf(file, ", ");
			fprintf(file, as_hex, img->cmap[i].green);
			fprintf(file, ", ");
			fprintf(file, as_hex, img->cmap[i].blue);
			fprintf(file, "}");
			if (i < img->cmap_size-1)
				fprintf(file, ",");
			fprintf(file, "\n");
		}
		fprintf(file, "};\n");
	}

	data32	= img->data32;
	data8	= img->data8;
	area	= img->width * img->height;
	columns	= img->width;

	if (img->depth <= 8) {
		fprintf(file, "static byte %s_pixels [] = {", name);
		if (img->cmap_size <= 10) {
			if (columns > 32)
				columns = 32;
			fmt = as_decimal;
			comma = ",";
		}
		else {
			if (columns > 10)
				columns = 10;
			fmt = as_hex;
			comma = ", ";
		}
		i = 0;
		for (y=0; y < img->height; y++) {
		  for (x=0; x < img->width; x++) {
			if ((i % columns) == 0)
				fprintf(file, "\n\t");
			fprintf(file, fmt, data8[y][x]);
			if (++i < area)
				fprintf(file, comma);
		  }
		}
		fprintf(file, "\n};\n");

		fprintf(file, "static byte * %s_data8 [] = {", name);
		for (y=0; y < img->height; ) {
			fprintf(file, "\n\t");
			fprintf(file, "&%s_pixels[%d*%ld]",
				name, img->width, y);
			if (++y < img->height)
				fprintf(file, ",");
		}
		fprintf(file, "\n};\n");
	}
	else {
		fprintf(file, "static Colour %s_pixels [] = {", name);
		if (columns > 2) columns = 2;
		i = 0;
		for (y=0; y < img->height; y++) {
		  for (x=0; x < img->width; x++) {
			if ((i % columns) == 0)
				fprintf(file, "\n  ");
			fprintf(file, "{ ");
			fprintf(file, as_hex, data32[y][x].alpha);
			fprintf(file, ", ");
			fprintf(file, as_hex, data32[y][x].red);
			fprintf(file, ", ");
			fprintf(file, as_hex, data32[y][x].green);
			fprintf(file, ", ");
			fprintf(file, as_hex, data32[y][x].blue);
			fprintf(file, "}");
			if (++i < area)
				fprintf(file, ", ");
		  }
		}
		fprintf(file, "\n};\n");

		fprintf(file, "static Colour * %s_data32 [] = {", name);
		for (y=0; y < img->height; ) {
			fprintf(file, "\n  ");
			fprintf(file, "&%s_pixels[%d*%ld]",
				name, img->width, y);
			if (++y < img->height)
				fprintf(file, ",");
		}
		fprintf(file, "\n};\n");
	}

	fprintf(file, "static Image %s_imagedata = {\n", name);
	fprintf(file, "\t%d,\t/* depth */\n",  img->depth);
	fprintf(file, "\t%d,\t/* width */\n",  img->width);
	fprintf(file, "\t%d,\t/* height */\n", img->height);
	fprintf(file, "\t%d,\t/* cmap_size */\n", img->cmap_size);
	if (img->depth <= 8) {
		fprintf(file, "\t%s_cmap,\n", name);
		fprintf(file, "\t%s_data8,\n", name);
		fprintf(file, "\t(Colour **) 0\n");
	}
	else {
		fprintf(file, "\t(Colour *) 0,\n");
		fprintf(file, "\t(byte **) 0,\n");
		fprintf(file, "\t%s_data32\n", name);
	}
	fprintf(file, "};\n");
	fprintf(file, "Image * %s_image = & %s_imagedata;\n",
			name, name);
	fprintf(file, "\n");

	return 1;
}

static char *base_file_name(const char *filename)
{
	char *name;
	size_t start, end;

	end = strlen(filename);
	while ((end > 0) && (filename[end] != '.'))
		end--;
	for (start=end; start > 0; start--) {
		if ((filename[start] == '\\')
		 || (filename[start] == '/')) {
			start ++;
			break;
		}
	}
	name = app_alloc((long) (end-start)+1);
	if (! name)
		return NULL;
	strncpy(name, filename+start, end-start);
	name[end-start] = '\0';
	return name;
}

int app_save_header_image(Image *img, const char *filename)
{
	FILE *file;
	char *name;
	int result;

	name = base_file_name(filename);
	if (! name)
		return 0;
	file = fopen(filename, "w");
	if (! file) {
		app_free(name);
		return 0;
	}
	result = save_header_image_file(file, name, img);
	app_free(name);
	fclose(file);
	return result;
}
