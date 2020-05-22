/*
 *  Portable Unicode font loading and caching system.
 *
 *  Platform: Neutral.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Added native font interface.
 *  Version: 3.08  2001/11/11  Expanded native fonts to allow Latin1 text.
 *  Version: 3.10  2001/12/01  Fonts can be in a program's resources.
 *  Version: 3.21  2002/04/04  Fixed some memory leaks.
 *  Version: 3.35  2002/12/23  Renamed fonts_loaded to num_fonts, etc.
 *  Version: 3.45  2003/05/12  Added app_change_default_font function.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced some size_t conversion warnings.
 *  Version: 3.58  2005/09/25  Finds .png then .gif. Anti-aliased fonts.
 *  Version: 3.59  2005/10/10  Supports over-sized glyphs.
 *  Version: 3.60  2005/12/29  Better italics synthesis. New search order.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

/*
 *  Global constants:
 */

enum {
	APP_FONT_MAX_SUBFONTS = 15
};

static const char * app_font_search_var      = "APP_FONT_PATH";
static const char * app_font_home_var        = "HOME";
static const char * app_font_install_dir     = "/graphappfonts/";
static const char * app_font_resource_hdr    = "*fonts/";
static const char * app_font_info_suffix     = ".txt";
static const char * app_font_file_suffix     = ".png"; /* tried first */
static const char * app_font_file_suffix_alt = ".gif"; /* tried second */
static const char * app_font_default         = "unifont";
static int          app_font_default_style   = PLAIN;
static int          app_font_default_size    = 16;

typedef struct FontSearch
{
	char *	short_name;
	int 	flags;
	int 	found_flags;
} FontSearch;

/*
 *  Some of these choices are commented out because synthesising
 *  a bold font from an anti-aliased font generally looks bad
 *  because you get strange over-print effects with anti-aliasing.
 *  Synthetic italic anti-aliased fonts are fine because we
 *  just shift the pixels; there is no over-printing.
 */
static FontSearch app_font_search[] =
{
	{ "bia",    BOLD | ITALIC | ANTI_ALIAS, BOLD | ITALIC | ANTI_ALIAS },
/*	{ "ia",     BOLD | ITALIC | ANTI_ALIAS, ITALIC | ANTI_ALIAS }, */
	{ "ba",     BOLD | ITALIC | ANTI_ALIAS, BOLD | ANTI_ALIAS },
/*	{ "a",      BOLD | ITALIC | ANTI_ALIAS, ANTI_ALIAS }, */
	{ "bi",     BOLD | ITALIC | ANTI_ALIAS, BOLD | ITALIC },
	{ "i",      BOLD | ITALIC | ANTI_ALIAS, ITALIC },
	{ "b",      BOLD | ITALIC | ANTI_ALIAS, BOLD },
	{ "",       BOLD | ITALIC | ANTI_ALIAS, PLAIN },
	{ "bi",     BOLD | ITALIC,              BOLD | ITALIC },
	{ "i",      BOLD | ITALIC,              ITALIC },
	{ "b",      BOLD | ITALIC,              BOLD },
	{ "",       BOLD | ITALIC,              PLAIN },
	{ "ba",     BOLD | ANTI_ALIAS,          BOLD | ANTI_ALIAS },
	{ "b",      BOLD | ANTI_ALIAS,          BOLD },
/*	{ "a",      BOLD | ANTI_ALIAS,          ANTI_ALIAS }, */
	{ "",       BOLD | ANTI_ALIAS,          PLAIN },
	{ "ia",     ITALIC | ANTI_ALIAS,        ITALIC | ANTI_ALIAS },
	{ "i",      ITALIC | ANTI_ALIAS,        ITALIC },
	{ "a",      ITALIC | ANTI_ALIAS,        ANTI_ALIAS },
	{ "",       ITALIC | ANTI_ALIAS,        PLAIN },
	{ "b",      BOLD,                       BOLD },
	{ "",       BOLD,                       PLAIN },
	{ "i",      ITALIC,                     ITALIC },
	{ "",       ITALIC,                     PLAIN },
	{ "a",      ANTI_ALIAS,                 ANTI_ALIAS },
	{ "",       ANTI_ALIAS,                 PLAIN },
	{ "",       PLAIN,                      PLAIN },
};

#ifndef NELEM
#define NELEM(arr) (sizeof((arr))/sizeof((arr)[0]))
#endif

/*
 *  Look for a font info file in the application's resources.
 */
static FILE * app_open_font_info_from_resources(App *app,
	const char *name, int height, int style)
{
	int i;
	char *filepath;
	int length;
	int flags;
	char *short_name;
	FILE *f = NULL;

	if ((app == NULL) || (app->has_resources == 0))
		return NULL; /* no resources, no point looking */

	/* build font resource name */
	filepath = app_zero_alloc((long) strlen(app_font_resource_hdr) +
				(long) strlen(name) + 40);
	if (! filepath)
		return NULL;
	sprintf(filepath, "%s%s/", app_font_resource_hdr, name);
	length = (int) strlen(filepath);

	for (i=0; i < NELEM(app_font_search); i++)
	{
		flags = app_font_search[i].flags;
		short_name = app_font_search[i].short_name;

		/* only use the part of the search table which applies */
		if ((style & flags) != flags)
			continue;

		/* try loading the most specific remaining info file */
		sprintf(filepath + length, "%d%s%s",
				height, short_name,
				app_font_info_suffix);

		/* load subfont info */
		f = app_open_resource(app->program_name,
				filepath, NULL);
		if (f)
			break;
		/* failed: continue to try a less specific search case */
	}
	app_free(filepath);
	return f;
}

/*
 *  Open a font info file by searching within path + dir.
 */
static FILE * app_open_font_info_by_path(App *app,
		const char *path, const char *dir,
		const char *name, int height, int style)
{
	int i;
	char *filepath;
	int length;
	int flags;
	char *short_name;
	FILE *f = NULL;

	if ((! path) || (! dir))
		return NULL;
	if ((path[0] == '\0') && (dir[0] == '\0'))
		return NULL;

	filepath = app_zero_alloc((long) strlen(path) + (long) strlen(dir) +
				(long) strlen(name) + 40);
	if (filepath == NULL)
		return NULL;

	sprintf(filepath, "%s%s%s/", path, dir, name);
	length = (int) strlen(filepath);

	for (i=0; i < NELEM(app_font_search); i++)
	{
		flags = app_font_search[i].flags;
		short_name = app_font_search[i].short_name;

		/* only use the part of the search table which applies */
		if ((style & flags) != flags)
			continue;

		/* try loading the most specific info file left */
		sprintf(filepath + length, "%d%s%s",
				height, short_name,
				app_font_info_suffix);

		/* load subfont info */
		f = app_open_file(filepath, "rb");
		if (f)
			break;
		/* failed: continue to try a less specific search case */
	}

	app_free(filepath);
	return f;
}

/*
 *  Open a font's information file. Return NULL if it's not there.
 *  First the APP_FONT_PATH environment variable is tried,
 *  then the HOME environment variable is prepended to font_install_dir
 *  then the APP_FONT_PATH environment variable is prepended instead.
 *  If all of those fail, the font_install_dir is tried on its own.
 *  If that fails, try the application's resources.
 *  If that fails, return NULL.
 */
static FILE * app_open_font_info(App *app,
	const char *name, int height, int style)
{
	FILE *f;

	/* try env[APP_FONT_PATH]/ alone */
	f = app_open_font_info_by_path(app,
					getenv(app_font_search_var), "/",
					name, height, style);
	if (f != NULL)
		return f;

	/* try env[HOME] + font_install_dir */
	f = app_open_font_info_by_path(app, getenv(app_font_home_var),
					app_font_install_dir,
					name, height, style);
	if (f != NULL)
		return f;

	/* try env[APP_FONT_PATH] + font_install_dir */
	f = app_open_font_info_by_path(app, getenv(app_font_search_var),
					app_font_install_dir,
					name, height, style);
	if (f != NULL)
		return f;

	/* try font_install_dir alone */
	f = app_open_font_info_by_path(app, "", app_font_install_dir,
					name, height, style);
	if (f != NULL)
		return f;

	/* try resources last */
	f = app_open_font_info_from_resources(app, name, height, style);
	if (f != NULL)
		return f;

	return f;
}

/*
 *  Look for a sub-font image file in the application's resources.
 */
static Image * app_get_subfont_image_from_resources(App *app,
		const char *name, int height, unsigned long base, int style,
		int *style_found)
{
	int i;
	char *filepath;
	int length;
	int flags;
	char *short_name;
	FILE *f;
	Image *img = NULL;

	if ((app == NULL) || (app->has_resources == 0))
		return NULL; /* no resources, no point looking */

	/* build resource font name */
	filepath = app_zero_alloc((int) strlen(app_font_resource_hdr) +
				(int) strlen(name) + 40);
	if (! filepath)
		return NULL;
	sprintf(filepath, "%s%s/", app_font_resource_hdr, name);
	length = (int) strlen(filepath);

	for (i=0; i < NELEM(app_font_search); i++)
	{
		flags = app_font_search[i].flags;
		short_name = app_font_search[i].short_name;

		/* only use the part of the search table which applies */
		if ((style & flags) != flags)
			continue;

		/* try loading the most specific subfont left */
		sprintf(filepath + length, "%d%s/%08lx%s",
					height, short_name,
					base, app_font_file_suffix);

		/* load subfont image */
		f = app_open_resource(app->program_name,
					filepath, NULL);
		if (! f) {
			/* try the alternative file suffix */
			sprintf(filepath + length, "%d%s/%08lx%s",
					height, short_name,
					base, app_font_file_suffix_alt);

			/* load alternative subfont image */
			f = app_open_resource(app->program_name,
					filepath, NULL);
		}

		if (f) {
			img = app_read_image_file(f, 8);
			app_close_file(f);
		}
		if (img) {
			*style_found = app_font_search[i].found_flags;
			break;
		}
		/* failed: continue to try a less specific subfont */
	}

	app_free(filepath);
	return img;
}

/*
 *  Look for a sub-font image file by searching within path + dir.
 */
static Image * app_get_subfont_image_by_path(App *app,
		const char *path, const char *dir,
		const char *name, int height, unsigned long base, int style,
		int *style_found)
{
	int i;
	char *filepath;
	int length;
	int flags;
	char *short_name;
	Image *img = NULL;

	if ((! path) || (! dir))
		return NULL;
	if ((path[0] == '\0') && (dir[0] == '\0'))
		return NULL;

	/* build font name */
	filepath = app_zero_alloc((long) strlen(path) + (long) strlen(dir) +
				(long) strlen(name) + 40);
	if (filepath == NULL)
		return NULL;

	sprintf(filepath, "%s%s%s/", path, dir, name);
	length = (int) strlen(filepath);

	for (i=0; i < NELEM(app_font_search); i++)
	{
		flags = app_font_search[i].flags;
		short_name = app_font_search[i].short_name;

		/* only use the part of the search table which applies */
		if ((style & flags) != flags)
			continue;

		/* try loading the most specific subfont remaining */
		sprintf(filepath + length, "%d%s/%08lx%s",
					height, short_name,
					base, app_font_file_suffix);

		/* load subfont image */
		img = app_read_image(filepath, 8);

		if (! img) {
			/* try the alternative file suffix */
			sprintf(filepath + length, "%d%s/%08lx%s",
					height, short_name,
					base, app_font_file_suffix_alt);

			img = app_read_image(filepath, 8);
		}

		if (img) {
			*style_found = app_font_search[i].found_flags;
			break;
		}
		/* failed: continue to try a less specific subfont */
	}

	app_free(filepath);
	return img;
}

/*
 *  Returns a subfont's image file or NULL if it's not there.
 *  First the APP_FONT_PATH environment variable is tried,
 *  then the HOME environment variable is prepended to font_install_dir
 *  then the APP_FONT_PATH environment variable is prepended instead.
 *  If all of those fail, the font_install_dir is tried on its own.
 *  If that fails, try the application's resources.
 *  If that fails, return NULL.
 */
static Image * app_get_subfont_image(App *app, const char *name,
	int height, unsigned long base, int style, int *style_found)
{
	Image *img;

	*style_found = PLAIN;

	/* try env[APP_FONT_PATH]/ alone */
	img = app_get_subfont_image_by_path(app,
					getenv(app_font_search_var), "/",
					name, height, base, style,
					style_found);
	if (img != NULL)
		return img;

	/* try env[HOME] + font_install_dir */
	img = app_get_subfont_image_by_path(app, getenv(app_font_home_var),
					app_font_install_dir,
					name, height, base, style,
					style_found);
	if (img != NULL)
		return img;

	/* try env[APP_FONT_PATH] + font_install_dir */
	img = app_get_subfont_image_by_path(app, getenv(app_font_search_var),
					app_font_install_dir,
					name, height, base, style,
					style_found);
	if (img != NULL)
		return img;

	/* try font_install_dir alone */
	img = app_get_subfont_image_by_path(app, "", app_font_install_dir,
					name, height, base, style,
					style_found);
	if (img != NULL)
		return img;

	/* try resources last */
	img = app_get_subfont_image_from_resources(app, name, height,
			base, style, style_found);
	if (img != NULL)
		return img;

	return img;
}

/*
 *  This ugly parser function loads the font information file
 *  and skips through it until it finds an entry for the
 *  required subfont. It then proceeds to load just the
 *  information for that subfont.
 *
 *  This means that loading each subfont may parse the whole file.
 *  This may seem wasteful, but consider the alternative.
 *  We could read the file once and store the entire font's
 *  character-width information in memory all the time, saving
 *  parsing time, but using up memory (a lot of memory if the
 *  font is large and proportionally spaced).
 *
 *  So, we slightly wastefully read the file for each subfont.
 *  This reduces the memory needed, and allows us to control
 *  how many subfonts we wish to have in memory at the one time
 *  in a fairly straightforward manner: it's just a list of
 *  subfonts up to a certain maximum number.
 */
static int app_load_subfont_info(App *app, Font *fnt, Subfont *sub,
	const char *name, int height, int style)
{
	FILE *f;
	int i, r, ch, width, start, finish;
	char buf[10];
	unsigned long base;
	FontWidth *fw;

	/* load the font information file */
	f = app_open_font_info(app, name, height, style);
	if (! f)
		return 0;

	/* create the subfont glyph width array, if necessary */
	if (sub->width == NULL) {
		sub->width = app_alloc(sizeof(sub->width[0]) * 256);
		if (sub->width) {
			for (i=0; i < 256; i++)
				sub->width[i] = -2;
		}
	}

	base = 0UL;
	while ((ch = getc(f)) != EOF)
	{
		if ((ch == '\r') || (ch == '\n') || (ch == ' '))
			continue;
		else if (ch == '\t') {
			/* font width descriptor line */
			/* do nothing yet */
		}
		else {
			/* subfont base number */
			i = 0;
			while ((ch = buf[i++] = getc(f)) != EOF) {
				if (i == sizeof(buf)-1)
					break;
				if ((ch == '\r') || (ch == '\n'))
					break;
			}
			buf[i] = '\0';
			base = strtoul(buf, NULL, 16);
			if (base > sub->base)
				break; /* gone too far */
			continue; /* keep looking for the subfont */
		}

		/* if we are here, must be a font width descriptor line */
		if (base != sub->base) {
			/* not the subfont we were looking for */
			while ((ch = getc(f)) != EOF)
				/* discard the line */
				if ((ch == '\r') || (ch == '\n'))
					break;
			continue;
		}

		/* determine the width described by this line */
		i = 0;
		while ((ch = buf[i++] = getc(f)) != EOF) {
			if (i == sizeof(buf)-1)
				break;
			if (ch == ' ')
				break;
		}
		buf[i] = '\0';
		width = strtol(buf, NULL, 10); /* decimal integer */
		if (width > fnt->maximum_width)
			fnt->maximum_width = width;
		start = -1;
		finish = -2;

		/* allocate FontWidth structure */
		fw = app_zero_alloc(sizeof(FontWidth));
		fw->width = width;
		fw->range_list = app_zero_alloc(256);
		r = 0;

		/* read the width ranges */
		i = 0;
		while ((ch = getc(f)) != EOF) {
			buf[i++] = ch;
			if (i == sizeof(buf)-1)
				break;
			if (ch == '-') {
				buf[i] = '\0';
				if (start >= 0) {
					fw->range_list[r++] = start;
					fw->range_list[r++] = start;
				}
				start = strtol(buf, NULL, 16);
				finish = start;
				i = 0;
			}
			if (ch == ',') {
				buf[i] = '\0';
				if (finish == start) {
					finish = strtol(buf, NULL, 16);
					fw->range_list[r++] = start;
					fw->range_list[r++] = finish;
					start = -1;
				}
				else if (start >= 0) {
					fw->range_list[r++] = start;
					fw->range_list[r++] = start;
					start = strtol(buf, NULL, 16);
				}
				else {
					start = strtol(buf, NULL, 16);
					finish = start - 1;
				}
				i = 0;
			}
			if ((ch == '\r') || (ch == '\n')) {
				buf[i] = '\0';
				if (finish == start) {
					finish = strtol(buf, NULL, 16);
					fw->range_list[r++] = start;
					fw->range_list[r++] = finish;
				}
				else if (start >= 0) {
					fw->range_list[r++] = start;
					fw->range_list[r++] = start;
					start = strtol(buf, NULL, 16);
					fw->range_list[r++] = start;
					fw->range_list[r++] = start;
				}
				else {
					start = strtol(buf, NULL, 16);
					fw->range_list[r++] = start;
					fw->range_list[r++] = start;
				}
				break;
			}
		}

		/* add this font width descriptor line to the subfont */
		fw->range_list = app_realloc(fw->range_list, r);
		fw->num_ranges = r/2;
		sub->widths = app_realloc(sub->widths,
			(sub->num_widths+1) * sizeof(FontWidth *));
		sub->widths[sub->num_widths] = fw;
		sub->num_widths++;

		/* copy width to the glyph width array for fast lookup */
		if (sub->width) {
			for (i=0; i < 256; i++) {
				if (sub->width[i] != -2)
					continue;
				for (r=0; r < fw->num_ranges*2; r+=2) {
					if ((i >= fw->range_list[r]) &&
					    (i <= fw->range_list[r+1]))
						sub->width[i] = width;
				}
			}
		}
	}
	if (sub->width) {
		for (i=0; i < 256; i++)
			if (sub->width[i] == -2)
				sub->width[i] = -1;
	}

	app_close_file(f);
	return 1;
}

void app_print_subfont(Subfont *sub)
{
	int i,j;
	printf(" subfont\n");
	printf("  base=%08lx\n", sub->base);
	printf("  number of widths=%d\n", sub->num_widths);
	for (i=0; i < sub->num_widths; i++) {
		printf("   width=%d\n",
				sub->widths[i]->width);
		printf("    number of ranges=%d\n     ",
				sub->widths[i]->num_ranges);
		for (j=0; j < sub->widths[i]->num_ranges*2; j+=2) {
			printf(" %02x to %02x,",
				sub->widths[i]->range_list[j],
				sub->widths[i]->range_list[j+1]);
		}
		printf("\n");
	}
	if (sub->width == NULL) {
		printf("  width array=NULL\n");
		return;
	}
	printf("  width array=[\n");
	for (i=0; i < 256; i++) {
		if ((i % 32) == 0)
			printf("    ");
		printf("%d", (int)sub->width[i]);
		if (i < 255)
			printf(",");
		if ((i % 32) == 31)
			printf("\n");
	}
	printf("  ]\n");
}

/*
 *  Delete a subfont and its character width ranges.
 */
void app_del_subfont(Font *f, Subfont *sub)
{
	int i;

	if (sub->width)
		app_free(sub->width);
	for (i=0; i < sub->num_widths; i++) {
		app_free(sub->widths[i]->range_list);
		app_free(sub->widths[i]);
	}
	app_free(sub->widths);
	app_release_native_subfont(f, sub);
	if (sub->img)
		app_del_image(sub->img);
	app_free(subfont_extra(sub));
	app_free(sub);
}

/*
 *  Create a transparent image based on the given image.
 */
static Image * app_new_transparent_image_8(int width, int height,
			int cmap_size, Colour *cmap)
{
	Image *img;
	int i, y, transparent;

	img = app_new_image(width, height, 8);
	app_set_image_cmap(img, cmap_size, cmap);

	transparent = 0;
	for (i=0; i < cmap_size; i++) {
		if (cmap[i].alpha == 255) {
			transparent = i;
			break;
		}
	}
	for (y=0; y < height; y++)
		memset(img->data8[y], transparent, width);

	return img;
}

/*
 *  Create a "bold" version of a subfont image.
 *  Assume the image is in 8-bit indexed format.
 *  A bold version of a subfont is formed by drawing
 *  the glyphs into a new image, and then drawing them
 *  again, shifted right by one pixel.
 */
static Image * app_subfont_image_to_bold(Font *fnt, Subfont *sub, Image *img)
{
	Image *img2;
	int x, y, old_width, glyph_width, height, shift, dx;
	Graphics *src, *dst;
	Rect r;

	old_width = img->width / 32;

	/* if old_width is even, we add two, otherwise add just one */
	glyph_width = old_width + 2 - (old_width % 2);

	height = img->height / 8;

	img2 = app_new_transparent_image_8(glyph_width * 32,
				img->height, img->cmap_size, img->cmap);

	src = app_get_image_graphics(img);
	dst = app_get_image_graphics(img2);

	for (y=0; y < 8; y++)
	{
	  for (x=0; x < 32; x++)
	  {
		dx = 0;
		shift = 1; /* right */
		if (fnt->height < (img->height / 8)) {
			/* the glyph is centered in the box */
			/* thus, we must take care to shift evens left */
			if (sub->width != NULL) {
				if (sub->width[y*32+x] > 0) {
					if ((sub->width[y*32+x] % 2) == 0)
						shift = -1; /* left */
				}
				dx = 1; /* centered in box, move right */
			}
		}

		r = rect(x*old_width, y*height, old_width, height);

		/* copy original glyph */
		app_copy_rect(dst, pt(x*glyph_width+dx,y*height), src, r);

		/* overstrike to provide greater width */
		app_copy_rect(dst, pt(x*glyph_width+shift+dx,y*height), src, r);
	  }
	}

	app_del_graphics(dst);
	app_del_graphics(src);

	return img2;
}

/*
 *  To make a subfont bold, increase the widths of all glyphs.
 */
static void app_make_subfont_info_bold(Font *fnt, Subfont *sub)
{
	int i, extra;

	extra = 1; /* 1 pixel overstrike for bold */

	for (i=0; i < sub->num_widths; i++) {
		if (sub->widths[i]->width > 0)
			sub->widths[i]->width += extra;
		if (fnt->maximum_width < sub->widths[i]->width)
			fnt->maximum_width = sub->widths[i]->width;
	}
	if (sub->width != NULL) {
		for (i=0; i < 256; i++) {
			if (sub->width[i] > 0)
				sub->width[i] += extra;
			if (fnt->maximum_width < sub->width[i])
				fnt->maximum_width = sub->width[i];
		}
	}
}

static int app_subfont_char_width(Subfont *sub, unsigned long ch);

/*
 *  Create an "italic" version of a subfont image.
 *  Assume the image is in 8-bit indexed format.
 *  An italic version of a subfont is formed by drawing
 *  each glyph in pieces. The bottom-most 6 pixels of
 *  the glyph is drawn, then the next 6 pixels upwards
 *  are drawn shifted 1 pixel to the right, then the next
 *  6 pixels are drawn shifted 2 pixels to the right and
 *  so on. This gives an angle of about 10 degrees.
 *
 *  This function also draws each new glyph centered within
 *  its new glyph box, so that overhanging parts of the
 *  glyph may be outside the glyph box, but will still be
 *  drawn. The signal that a subfont image uses centered
 *  glyphs is that each glyphs height is greater than the
 *  the font height. This also implies that each glyph is
 *  centered horizontally too (slightly to the left of
 *  centre if the glyph has an odd width within an even
 *  width glyph box).
 */
static Image * app_subfont_image_to_italic(Font *fnt, Subfont *sub, Image *img)
{
	Image *img2;
	int i, x, y, extra, dx, dy, glyph_width;
	int old_width, old_height;
	int new_width, new_height;
	Graphics *src, *dst;
	Point p;
	Rect r;

	old_height = img->height / 8;
	old_width = img->width / 32;
	new_height = old_height;
	if (new_height < fnt->height + 2)
		new_height = fnt->height + 2;
	extra = (fnt->height +5) /6 -1; // + (old_height / 4);
	new_width = old_width + extra;
	dy = (new_height - old_height) / 2;

	img2 = app_new_transparent_image_8(new_width * 32, new_height * 8,
				img->cmap_size, img->cmap);

	src = app_get_image_graphics(img);
	dst = app_get_image_graphics(img2);

	r.width = old_width;

	for (y=0; y < 8; y++)
	{
		for (x=0; x < 32; x++)
		{
			r.x = x * old_width;
			r.height = 6;

			if (sub->width)
				glyph_width = sub->width[y*8+x];
			else
				glyph_width = app_subfont_char_width(sub, y*8+x);

			if (dy) /* must center the glyph */
				//dx = (new_width - glyph_width - old_height / 6) / 2;
				dx = old_height / 8;
			else
				dx = 0;

			for (i=0; i < old_height; i+=r.height)
			{
				p.x = x * new_width;
				p.x += dx;
				p.x += (old_height-i +5) /6 -1;
				if (i + r.height > new_height)
					r.height = new_height - i;
				p.y = i + y * new_height + dy;
				r.y = i + y * old_height;

				//printf("img=%d,%d p=%d,%d r=%d,%d,%d,%d dy=%d dx=%d i=%d, x=%d y=%d gw=%d ow=%d oh=%d nw=%d nh=%d\n", img2->width, img2->height, p.x, p.y, r.x, r.y, r.width, r.height, dy, dx, i, x, y, glyph_width, old_width, old_height, new_width, new_height);
				app_copy_rect(dst, p, src, r);
			}
		}
	}

	app_del_graphics(dst);
	app_del_graphics(src);

	return img2;
}

/*
 *  To make a subfont italic, increase the widths of all glyphs.
 */
static void app_make_subfont_info_italic(Font *fnt, Subfont *sub)
{
	int i, extra;

	return;

	extra = (sub->img->height /8 +5) /6 -1;

	for (i=0; i < sub->num_widths; i++) {
		if (sub->widths[i]->width > 0)
			sub->widths[i]->width += extra;
		if (fnt->maximum_width < sub->widths[i]->width)
			fnt->maximum_width = sub->widths[i]->width;
	}
	if (sub->width != NULL) {
		for (i=0; i < 256; i++) {
			if (sub->width[i] > 0)
				sub->width[i] += extra;
			if (fnt->maximum_width < sub->width[i])
				fnt->maximum_width = sub->width[i];
		}
	}
}

/*
 *  Create an "italic" version of a subfont image.
 *  Assume the image is in 8-bit indexed format.
 *  An italic version of a subfont is formed by drawing
 *  each glyph in pieces. The bottom-most 6 pixels of
 *  the glyph is drawn, then the next 6 pixels upwards
 *  are drawn shifted 1 pixel to the right, then the next
 *  6 pixels are drawn shifted 2 pixels to the right and
 *  so on. This gives an angle of about 10 degrees.
 */
static Image * app_subfont_image_to_italic_old(Font *fnt, Subfont *sub, Image *img)
{
	Image *img2;
	int i, x, y, extra, old_width, glyph_width, glyph_height;
	Graphics *src, *dst;
	Point p;
	Rect r;

	glyph_height = img->height / 8;
	old_width = img->width / 32;
	extra = (glyph_height +5) /6 -1;
	glyph_width = old_width + extra;

	img2 = app_new_transparent_image_8(glyph_width * 32,
				img->height, img->cmap_size, img->cmap);

	src = app_get_image_graphics(img);
	dst = app_get_image_graphics(img2);

	r.width = old_width;

	for (y=0; y < 8; y++)
	{
		for (x=0; x < 32; x++)
		{
			r.x = x * old_width;
			r.height = 6;

			for (i=0; i < glyph_height; i+=r.height)
			{
				p.x = x * glyph_width;
				p.x += (glyph_height-i +5) /6 -1;
				if (i + r.height > glyph_height)
					r.height = glyph_height - i;
				p.y = i + y * glyph_height;
				r.y = p.y;

				app_copy_rect(dst, p, src, r);
			}
		}
	}

	app_del_graphics(dst);
	app_del_graphics(src);

	return img2;
}

/*
 *  To make a subfont italic, increase the widths of all glyphs.
 */
static void app_make_subfont_info_italic_old(Font *fnt, Subfont *sub)
{
	int i, extra;

	extra = (sub->img->height /8 +5) /6 -1;

	for (i=0; i < sub->num_widths; i++) {
		if (sub->widths[i]->width > 0)
			sub->widths[i]->width += extra;
		if (fnt->maximum_width < sub->widths[i]->width)
			fnt->maximum_width = sub->widths[i]->width;
	}
	if (sub->width != NULL) {
		for (i=0; i < 256; i++) {
			if (sub->width[i] > 0)
				sub->width[i] += extra;
			if (fnt->maximum_width < sub->width[i])
				fnt->maximum_width = sub->width[i];
		}
	}
}

/*
 *  Load a subfont into the given font.
 *  If there isn't much space, evict a subfont from the bottom
 *  of the list.
 */
static Subfont * app_load_subfont(Font *f, unsigned long base)
{
	int i, style_found;
	int all_greyscale;
	Colour c;
	Image *img, *img2;
	Subfont *sub;
	Subfont **list;

	/* check if we already have this subfont loaded */
	for (i=0; i < f->num_subfonts; i++)
		if (f->subfonts[i]->base == base) {
			/* found it, move it to the top by swapping */
			if (i > 0) {
				sub = f->subfonts[i];
				f->subfonts[i] = f->subfonts[0];
				f->subfonts[0] = sub;
			} else
				sub = f->subfonts[0];
			return sub;

		}

	/* load subfont image */
	img = app_get_subfont_image(f->app, f->name, f->height, base,
			f->style, &style_found);
	if (! img)
		return NULL;

	/* ensure the subfont is an indexed image */
	if (img->depth == 32) {
		/* shouldn't allow this, because it's very slow; */
		/* for best results subfonts should be indexed images */
		img2 = app_image_convert_32_to_8(img);
		app_del_image(img);
		img = img2;
	}

	/* make all white entries in the palette transparent */
	all_greyscale = 1;
	c = img->cmap[img->data8[0][0]]; /* check top-left pixel */
	if (c.alpha > 0)
		all_greyscale = 0; /* already using alpha */
	else for (i=0; i < img->cmap_size; i++) {
		c = img->cmap[i];
		if ((c.red == 255) && (c.green == 255) && (c.blue == 255))
			img->cmap[i].alpha = 255;
		if ((c.red != c.green) || (c.green != c.blue))
			all_greyscale = 0;
	}

	/* if all entries are greyscale/black/white, apply alpha to all */
	if (all_greyscale) {
		for (i=0; i < img->cmap_size; i++) {
			/* Invariant: red == green == blue for all. */
			/* So, the following makes black opaque, */
			/* white transparent, grey half-opaque black, */
			/* light-grey mostly transparent black, etc. */
			/* Greyscale fonts thus simulate anti-aliasing. */
			img->cmap[i].alpha = img->cmap[i].red;
			img->cmap[i].red   = 0;
			img->cmap[i].green = 0;
			img->cmap[i].blue  = 0;
		}
	}

	/* create the subfont structure */
	sub = app_zero_alloc(sizeof(Subfont));
	if (sub == NULL) {
		app_del_image(img);
		return NULL;
	}
	sub->extra = app_zero_alloc(sizeof(struct SubfontExtra));
	if (sub->extra == NULL) {
		app_free(sub);
		app_del_image(img);
		return NULL;
	}

	sub->img = img;
	sub->base = base;
	sub->anti_alias = all_greyscale;

	/* load the glyph width information */
	if (! app_load_subfont_info(f->app, f, sub, f->name, f->height, style_found))
	{
		app_del_subfont(f, sub);
		return NULL;
	}

	/* if the font is bold, create a new bold subfont image */
	if ((f->style & BOLD) && (! (style_found & BOLD))) {
		img2 = app_subfont_image_to_bold(f, sub, img);
		app_del_image(img);
		sub->img = img = img2;
		app_make_subfont_info_bold(f, sub);
	}

	/* if the font is italic, create a new italic subfont image */
	if ((f->style & ITALIC) && (! (style_found & ITALIC))) {
		img2 = app_subfont_image_to_italic(f, sub, img);
		app_del_image(img);
		sub->img = img = img2;
		app_make_subfont_info_italic(f, sub);
	}

	/* convert the image into a clipmask */
	if (f->app) {
		subfont_extra(sub)->clipmask = app_image_to_clipmask(f->app, img);
		if (! subfont_extra(sub)->clipmask) {
			/*app_del_subfont(f, sub);
			return NULL;*/
		}
	}

	/* add the subfont to the subfont list, evict one if needed */
	i = f->num_subfonts;
	list = NULL;

	if (i == 0) {
		list = app_alloc(sizeof(Subfont*));
		if (list == NULL) {
			app_del_subfont(f, sub);
			return NULL;	/* failed: no memory */
		}
	}
	else if (i < APP_FONT_MAX_SUBFONTS)
		list = app_realloc(f->subfonts, (i+1) * sizeof(Subfont*));

	if (list) {
		i++;
		f->num_subfonts++;
		f->subfonts = list;
	} else {
		list = f->subfonts;
		app_del_subfont(f, f->subfonts[i-1]);
	}

	if (i > 1)
		memmove(&list[1], list, (i-1)*sizeof(Subfont*));
	f->subfonts[0] = sub;

	return sub;
}

/*
 *  Public function for loading a Font object.
 */
Font * app_new_font(App *app, const char *name, int style, int size)
{
	Font *f;
	Font **list;
	FILE *fi;
	int i, height;

	/* No name given implies the default font. */
	if (! name)
		name = app_font_default;
	if ((style & (NATIVE_FONT | PORTABLE_FONT)) == 0)
		style |= (NATIVE_FONT | PORTABLE_FONT);

	/* Convert point size to pixel size. */
	if (size < 0) {
		if (app && app->screen_mm.height) {
			height = -size * 254L *
					app->screen_area.height /
					app->screen_mm.height / 720;
		} else {
			height = -size * 100L / 72;
		}
	} else {
		height = size;
	}

	/* Has the same font already been loaded? */
	if (app) {
		for (i=0; i < app->num_fonts; i++) {
			if (strcmp(app->fonts[i]->name, name) != 0)
				continue;
			if (app->fonts[i]->height != height)
				continue;
			if ((style & (BOLD | ITALIC)) !=
			    (app->fonts[i]->style & (BOLD | ITALIC)))
				continue;
			if (style & (NATIVE_FONT | PORTABLE_FONT) &
			    app->fonts[i]->style)
			{
				f = app->fonts[i];
				f->refcount++;
				return f;
			}
		}
	}

	/* Create the font structure. */
	f = app_zero_alloc(sizeof(Font));
	if (! f)
		return NULL;
	f->extra = app_zero_alloc(sizeof(FontExtra));
	if (! f->extra) {
		app_free(f);
		return NULL;
	}
	f->app = app;

	/* Check if this is a portable font. */
	if (style & PORTABLE_FONT) {
		fi = app_open_font_info(app, name, height, style);
		if (fi) {
			style &= ~NATIVE_FONT;
			app_close_file(fi);
		} else
			style &= ~PORTABLE_FONT;
	}

	/* Check if this is a native font. */
	if (style & NATIVE_FONT) {
		if (app_load_native_font(f, name, size, height, style)) {
			style &= ~PORTABLE_FONT;
			/*height = app_native_font_height(f);*/
			f->maximum_width = app_native_font_width(f);
		} else
			style &= ~NATIVE_FONT;
	}

	/* Check for failure of both methods. */
	if ((style & (NATIVE_FONT | PORTABLE_FONT)) == 0) {
		app_del_font(f);
		return NULL;
	}

	/* Copy information into the structure. */
	f->height = height;
	f->style = style;
	f->name = app_alloc((int) strlen(name)+1);
	if (! f->name) {
		app_del_font(f);
		return NULL;
	}
	strcpy(f->name, name);
	f->refcount = 1;

	/* Store the font with the app. */
	if (app) {
		i = app->num_fonts;
		list = app_realloc(app->fonts, (i+1)*sizeof(Font*));
		if (! list) {
			app_del_font(f);
			return NULL;	/* failed: no memory */
		}
		list[i] = f;
		app->num_fonts++;
		app->fonts = list;
	}

	return f;
}

/*
 *  Delete a font and all its subfonts.
 */
void app_del_font(Font *f)
{
	int i;
	App *app;

	f->refcount--;
	if (f->refcount > 0)
		return; /* not yet time to delete */

	/* Remove it from the app's list. */
	app = f->app;
	if (app) {
		for (i=0; i < app->num_fonts; i++) {
			if (app->fonts[i] != f)
				continue;
			if (i < app->num_fonts-1) {
				memmove(&app->fonts[i], &app->fonts[i+1],
					(app->num_fonts-1-i)
					* sizeof(Font*));
			}
			app->num_fonts--;
			break;
		}
		app_release_native_font(f);
	}

	/* Destroy the font structure. */
	for (i=f->num_subfonts-1; i >= 0; i--)
		app_del_subfont(f, f->subfonts[i]);
	app_free(f->subfonts);
	app_free(f->name);
	app_free(f->extra);
	app_free(f);
}

/*
 *  Load the default font into the app's list.
 *  If it was already there, just return it.
 */
Font *app_find_default_font(App *app)
{
	Font *f;

	f = app_new_font(app, app_font_default,
			app_font_default_style, app_font_default_size);
	if (f == NULL)
		f = app_new_font(app, "helvetica",
			NATIVE_FONT | BOLD, app_font_default_size);
	return f;
}

/*
 *  The basic function which tells the system the width of
 *  a given character. Its height is fixed by the font.
 *  The subfont containing this character must already have
 *  been loaded.
 */
static int app_subfont_char_width(Subfont *sub, unsigned long ch)
{
	int i, r;
	byte c;
	FontWidth *fw;
	byte *list;

	c = (byte) (ch & 0x00FF);
	if (sub->width != NULL)
		return sub->width[c];
	else {
		for (i=0; i < sub->num_widths; i++) {
			fw = sub->widths[i];
			list = fw->range_list;
			for (r=0; r < fw->num_ranges*2; r+=2) {
				if ((c >= list[r]) && (c <= list[r+1]))
					return fw->width;
			}
		}
	}
	return -1;
}

/*
 *  This function finds the subfont which contains the required
 *  character, and determines its width. If the given font does
 *  not contain the character, a second search looks for the
 *  character in the default font. If that also fails, width will
 *  be set to -1.
 */
Subfont *app_font_char_info(Font *f, unsigned long ch, int *width)
{
	Font *df;
	Subfont *sub;
	unsigned long base;
	int w = -1;

	base = ch & 0xFFFFFF00UL;
	sub = app_load_subfont(f, base);
	if (sub)
		w = app_subfont_char_width(sub, ch);
	if (w == -1) {
		/* character not found on this font */
		/* so look at the default font */
		df = app_find_default_font(f->app);
		if ((df) && (df != f)) {
			sub = app_load_subfont(df, base);
			if (sub)
				w = app_subfont_char_width(sub, ch);
		}
	}

	if (width != NULL)
		*width = w;
	return sub;
}

/*
 *  Return the height of a font, in pixels.
 */
int app_font_height(Font *f)
{
	return f->height;
}

/*
 *  Return the width of a UTF-8 string, in pixels.
 */
int app_font_width(Font *f, const char *s, int nbytes)
{
	int w, total;
	const char *sp;
	const char *src_end;
	unsigned long ch;
	unsigned long *cp;
	Subfont *sub;

	if (f->style & NATIVE_FONT)
		return app_native_font_string_width(f, s, nbytes);

	total = 0;
	sp = s;
	src_end = s + nbytes;

	while (nbytes > 0) {
		cp = &ch;
		if (app_utf8_to_unicode(&sp, src_end, &cp, cp+1)
		    & SourceExhausted)
			break;
		nbytes -= (int) (sp - s);
		s = sp;

		sub = app_font_char_info(f, ch, &w);

		if (w < 0) {
			/* character glyph not found, reserve 6 pixels */
			total += 6;
			continue; /* go to next character */
		}
		/* else, character glyph exists */
		total += w;
	}
	return total;
}

/*
 *  Load the default font into a Graphics object.
 */
void app_set_default_font(Graphics *g)
{
	app_set_font(g, app_find_default_font(g->app));
}

/*
 *  Change the default font, rather than use the portable Unicode font.
 *  This function is temporary and may be replaced in the future.
 */
void app_change_default_font(const char *name)
{
        if (name != NULL)
		app_font_default = name;
}
