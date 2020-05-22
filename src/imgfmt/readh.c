/*
 *  Load images from GraphApp header file format.
 *
 *  Platform: Neutral.
 *
 *  Version: 3.22  2002/04/10  First release.
 *  Version: 3.43  2003/04/25  Now reads hex or decimal pixel values.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "app.h" 

static const char * header_comment    = "/* GraphApp image type 2 */\n";
static const char * depth_comment     = "/* depth  = %d */\n";
static const char * width_comment     = "/* width  = %d */\n";
static const char * height_comment    = "/* height = %d */\n";
static const char * cmap_size_comment = "/* cmap_size = %d */\n";

static unsigned int read_number(FILE *file)
{
	int ch;
	unsigned int value;
	unsigned int base = 10;
	unsigned int result = 0;

	/* consume non-digits */
	while (((ch = getc(file)) != EOF) && (!isdigit(ch)))
		continue;
	/* ch is now either EOF or a digit */
	if (ch == '0') {
		ch = getc(file);
		if ((ch == 'x') || (ch == 'X')) {
			/* found a hex number starting as "0x" or "0X" */
			base = 16;
			ch = getc(file); /* obtain first hex numeral */
		}
		/* else already have the first decimal numeral after 0 */
		/* or EOF, or a non-decimal such as , or A */
	}

	while (ch != EOF) {
		if (isdigit(ch))
			value = ch - '0'; /* always valid */
		else if ((base > 10) && (isalpha(ch)))
			value = tolower(ch) - 'a' + 10; /* maybe > 'f' */
		else
			value = base; /* invalid input */

		if (value >= base) { /* invalid input */
			ungetc(ch, file);
			break;
		}
		result = (result * base) + value;
		ch = getc(file);
	}
	return result;
}

static Colour read_colour(FILE *file)
{
	Colour c;

	c.alpha = read_number(file);
	c.red   = read_number(file);
	c.green = read_number(file);
	c.blue  = read_number(file);

	return c;
}

Image * app_read_header_image_file(FILE *file)
{
	char	line[100];
	long	i, x, y;
	int 	width = 0, height = 0, depth = 8;
	int 	cmap_size = 0;
	Colour *cmap;
	Colour **data32;
	byte **	data8;
	Image *	img = NULL;

	if (file == NULL)
		return NULL;

	if (fgets(line, sizeof(line)-2, file) == NULL)
		return NULL;
	if (strcmp(line, header_comment))
		return NULL;

	for (i=0; i<4; i++) {
		if (fgets(line, sizeof(line)-2, file) == NULL)
			return NULL;
		if (! strncmp(line, depth_comment, 12))
			depth = atoi(line+12);
		if (! strncmp(line, width_comment, 12))
			width = atoi(line+12);
		if (! strncmp(line, height_comment, 12))
			height = atoi(line+12);
		if (! strncmp(line, cmap_size_comment, 15))
			cmap_size = atoi(line+14);
	}

	img = app_new_image(width, height, depth);
	if (img == NULL)
		return NULL;

	if (depth <= 8) {
		if (fgets(line, sizeof(line)-2, file) == NULL) {
			app_del_image(img);
			return NULL;
		}
		if ((strncmp(line, "Colour ", 7) != 0) &&
		    (strncmp(line, "static Colour ", 14) != 0))
		{
			app_del_image(img);
			return NULL;
		}
		cmap = app_alloc(sizeof(Colour) * cmap_size);
		for (i=0; i < cmap_size; i++) {
			cmap[i].alpha = read_number(file);
			cmap[i].red   = read_number(file);
			cmap[i].green = read_number(file);
			cmap[i].blue  = read_number(file);
		}
		if (fgets(line, sizeof(line)-2, file) == NULL) {
			app_del_image(img);
			return NULL;
		}
		if (fgets(line, sizeof(line)-2, file) == NULL) {
			app_del_image(img);
			return NULL;
		}

		img->cmap_size = cmap_size;
		img->cmap = cmap;
	}

	data32 = img->data32;
	data8  = img->data8;

	if (fgets(line, sizeof(line)-2, file) == NULL)
		return NULL;

	if (depth <= 8) {
		for (y=0; y < height; y++)
			for (x=0; x < width; x++)
				data8[y][x] = read_number(file);
	}
	else {
		for (y=0; y < height; y++)
			for (x=0; x < width; x++)
				data32[y][x] = read_colour(file);
	}

	return img;
}

Image * app_read_header_image(const char *filename)
{
	FILE *	file;
	Image *	img;

	file = fopen(filename, "r");
	img = app_read_header_image_file(file);
	fclose(file);
	return img;
}
