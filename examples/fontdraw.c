/*
 *  Font Draw
 *
 *  A program to draw a font bitmap from another font,
 *  perhaps a native font. This allows the bitmap to
 *  be manipulated and used as a pixel source.
 *
 *  The font bitmap is saved to
 *  'fontsave/font_name/height/00000000.png'
 *  and the font metrics are saved to
 *  'fontsave/font_name/heightSTYLE.txt',
 *  where font_name is the font's name,
 *  height is a 2-digit decimal pixel height,
 *  and STYLE is "" or "b" or "bi" or "i"
 *  depending if the style is bold and/or italic.
 *  It can also synthesise anti-aliased fonts
 *  from a font twice the size; such fonts have
 *  the letter "a" appended to their style.
 *  If you move the folder 'font_name/' into
 *  the App font folder you should have a working
 *  portable font. Obviously you should only do
 *  that if you own or license the rights to do so.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <app.h>

App *app;
Window *w;
Font *f;
Bitmap *b = NULL;
int max_width;
int line_height;
int widths[256];

void draw_font(Graphics *g, Font *f)
{
	int i, x, y, len;
	char ch[3];

	ch[1] = '\0';
	ch[2] = '\0';

	app_set_font(g, f);

	/* Draw the ISO Latin 1 set. */
	for (i = 0; i < 256; i++)
	{
		if (i < 128) {
			ch[0] = i;
			len = 1;
		}
		else {
			ch[0] = 0xC0 | ((i >> 6) & 0x03); /* 0b110zzzxx */
			ch[1] = 0x80 | (i & 0x3F);        /* 0b10xxxxxx */
			len = 2;
		}

		x = (i % 32) * max_width;
		y = (i / 32) * (line_height *5/4);

		widths[i] = app_font_width(f, ch, len);
		app_draw_utf8(g, pt(x + (max_width - widths[i]) / 2,
				    y + (line_height / 8)),
				ch, len);
		if (widths[i] == 0)
			widths[i] = -1;
	}
}

int save_subfont_widths(FILE *f, int *widths, int max_width, unsigned long sub_base)
{
	int i, width, start, finish, printed;

	fprintf(f, "%08lX\n", sub_base);
	for (width=-1; width <= max_width; width++) {
		printed = 0;
		start = -1;
		finish = -2;
		for (i=0; i < 256; i++) {
			if (widths[i] == width) {
				if (! printed) {
					fprintf(f, "\t%02d ", width);
					printed = 1;
				}
				if (start < 0) {
					start = finish = i;
				}
				else {
					finish++;
				}
			}
			else if (start >= 0) {
				if (printed == 2)
					fprintf(f, ",");
				if (finish > start)
					fprintf(f, "%02X-%02X", start,
						finish);
				else
					fprintf(f, "%02X", start);
				printed = 2;
				start = -1;
			}
		}
		if (start >= 0) {
			if (printed == 2)
				fprintf(f, ",");
			if (finish > start)
				fprintf(f, "%02X-%02X", start, finish);
			else
				fprintf(f, "%02X", start);
		}
		if (printed)
			fprintf(f, "\n");
	}

	return 1;
}

/*
 *  Windows objects to certain characters within folder names.
 *  So this function forms a name which doesn't have those characters.
 *  The caller must later app_free the returned string.
 */
char *to_safe_folder_name(char *name)
{
	int i, ch;
	char *safe = app_alloc((int) strlen(name) + 1);

	for (i=0; (ch = name[i]) != '\0'; i++)
	{
		if ((ch == ':') || (ch == '?') || (ch == '\\') || (ch == '/')
		 || (ch == '"') || (ch == '<') || (ch == '>'))
			ch = '_';
		safe[i] = ch;
	}
	safe[i] = '\0';
	return safe;
}

/*
 *  Convert a black and white 32-bit image into
 *  a paletted black and white image.
 *  Delete the 32-bit image, too.
 */
Image * to_bw_paletted_image(Image *img)
{
	int x, y;
	Colour c;
	Image *bw;

	bw = app_new_image(img->width, img->height, 8);
	bw->cmap = app_alloc(sizeof(Colour) * 5);
	bw->cmap_size = 5;
	bw->cmap[0] = BLACK;
	bw->cmap[1] = DARK_GREY;
	bw->cmap[2] = GREY;
	bw->cmap[3] = LIGHT_GREY;
	bw->cmap[4] = WHITE;

	for (y = 0; y < img->height; y++)
	{
		for (x = 0; x < img->width; x++)
		{
			c = img->data32[y][x];
			bw->data8[y][x] = (c.red > 0x7F) ? 4 : 0;
		}
	}
	app_del_image(img);
	return bw;
}

/*
 *  Convert a black and white 32-bit image into
 *  a paletted 5-grey greyscale image.
 *  Delete the 32-bit image, too.
 */
Image * to_half_size_greyscale_image(Image *img)
{
	int x, y, sw, sh, dw, dh, row, col;
	Colour c1, c2, c3, c4;
	int value;
	Image *bw;

	sw = img->width / 32;
	sh = img->height / 8;
	dw = (sw + 1) / 2;
	dh = sh / 2;

	bw = app_new_image(32 * dw, 8 * dh, 8);
	bw->cmap = app_alloc(sizeof(Colour) * 5);
	bw->cmap_size = 5;
	bw->cmap[0] = BLACK;
	bw->cmap[1] = DARK_GREY;
	bw->cmap[2] = GREY;
	bw->cmap[3] = LIGHT_GREY;
	bw->cmap[4] = WHITE;

	for (row = 0; row < 8; row++)
	{
	  for (col = 0; col < 32; col++)
	  {
	    for (y = 0; y < dh; y++)
	    {
		for (x = 0; x < dw; x++)
		{
			c1 = img->data32[row*sh + y*2][col*sw + x*2];
			if (x*2+1 < sw)
				c2 = img->data32[row*sh + y*2][col*sw + x*2+1];
			else
				c2 = WHITE;
			if (y*2+1 < sh)
				c3 = img->data32[row*sh + y*2+1][col*sw + x*2];
			else
				c3 = WHITE;
			if ((x*2+1 < sw) && (y*2+1 < sh))
				c4 = img->data32[row*sh + y*2+1][col*sw + x*2+1];
			else
				c4 = WHITE;
			value = c1.red;
			value += c1.green;
			value += c1.blue;
			value += c2.red;
			value += c2.green;
			value += c2.blue;
			value += c3.red;
			value += c3.green;
			value += c3.blue;
			value += c4.red;
			value += c4.green;
			value += c4.blue;
			value /= 12;
			if (value >= 0x7F)
			{
				if (value >= 0xC0)
					value = 4;
				else if (value >= 0x90)
					value = 3;
				else
					value = 2;
			}
			else
			{
				if (value >= 0x60)
					value = 2;
				else if (value >= 0x30)
					value = 1;
				else
					value = 0;
			}
			bw->data8[row*dh + y][col*dw + x] = value;
		}
	    }
	  }
	}
	app_del_image(img);
	return bw;
}

/*
 *  Halve the widths of each glyph, rounding up.
 *  This function and the one above are only used
 *  when synthesising an anti-aliased font from
 *  a font twice the size.
 */
void halve_glyph_widths(int *widths)
{
	int i;

	for (i=0; i < 256; i++)
		widths[i] = (widths[i] + 1) / 2;
}

/*
 *  Create a folder structure to contain the saved image.
 *  Then, save the image to a file in the appropriate folder.
 *  Also save the font information into a file too.
 */
int save_font_image(Image *img, char *font_name, int style, int height)
{
	int result;
	FILE *fm;
	char path[256];

	if (! img)
		return 0;

	/* Create the folders needed. */
	font_name = to_safe_folder_name(font_name);

	sprintf(path, "fontsave/");
	app_make_folder(path, 0755);

	sprintf(path, "fontsave/%s/", font_name);
	app_make_folder(path, 0755);

	sprintf(path, "fontsave/%s/%02d%s%s%s/",
			font_name, height,
			style & BOLD ? "b" : "",
			style & ITALIC ? "i" : "",
			style & ANTI_ALIAS ? "a" : "");
	app_make_folder(path, 0755);

	/* Save the sub-font image file. */

	strcat(path, "00000000.png");
	fprintf(stderr, "Saving font image file %s\n", path);

	result = app_write_image(img, path);
	fprintf(stderr, "Created font image file.\n");

	/* Save the font metrics information. */

	sprintf(path, "fontsave/%s/%02d%s%s%s.txt",
			font_name, height,
			style & BOLD ? "b" : "",
			style & ITALIC ? "i" : "",
			style & ANTI_ALIAS ? "a" : "");

	fprintf(stderr, "Creating metrics file %s\n", path);
	fm = fopen(path, "w");
	if (! fm) {
		app_free(font_name);
		return 0;
	}
	result &= save_subfont_widths(fm, widths, max_width, 0x00000000UL);
	fclose(fm);
	fprintf(stderr, "Created metrics file.\n");

	app_free(font_name);
	return result;
}

Bitmap *make_font_bitmap(Window *w, Font *f)
{
	Bitmap *b;
	Graphics *g;

	b = app_new_bitmap(w, 32 * max_width, 8 * (line_height *5/4));
	if (! b)
		return NULL;

	g = app_get_bitmap_graphics(b);
	app_set_colour(g, WHITE);
	app_fill_rect(g, app_get_bitmap_area(b));
	app_set_colour(g, BLACK);
	draw_font(g, f);
	app_del_graphics(g);

	return b;
}

void show_font_bitmap(Graphics *dst)
{
	Graphics *src = app_get_bitmap_graphics(b);
	app_copy_rect(dst, pt(0,0), src, app_get_bitmap_area(b));
	app_del_graphics(src);
}

void draw_font_window(Window *w, Graphics *wg)
{
	if (b)
		show_font_bitmap(wg);
	else
		draw_font(wg, f);
}

void make_font_window(char *font_name, int style, int pixel_size,
	int show, int save)
{
	/* special code for anti-aliasing */
	if (style & ANTI_ALIAS)
		pixel_size *= 2; /* synthesise from twice-size font */

	/* Load the font. */
	f = app_new_font(app, font_name, style & (~ANTI_ALIAS), pixel_size);
	app_font_char_info(f, 0, NULL); /* force loading of subfont 0 */

	/* Now the font characteristics will be valid. */
	max_width = f->maximum_width * 5/4;
	line_height = app_font_height(f);

	fprintf(stderr, "Font loaded. Pixel height is %d, max width is %d\n",
			line_height, max_width);

	w = app_new_window(app,
			rect(50, 50, 32 * max_width, 8 * (line_height*5/4)),
			font_name,
			STANDARD_WINDOW);

	b = make_font_bitmap(w, f);
	if (save) {
		Image *img = app_bitmap_to_image(b);
		if ((img != NULL) && (style & ANTI_ALIAS)) {
			img = to_half_size_greyscale_image(img);
			halve_glyph_widths(widths);
			save_font_image(img, font_name, style, line_height / 2);
			app_del_image(img);
		}
		else if (img != NULL) {
			img = to_bw_paletted_image(img);
			save_font_image(img, font_name, style, line_height);
			app_del_image(img);
		}
	}

	if (show)
	{
		app_on_window_redraw(w, draw_font_window);
		app_show_window(w);
		app_main_loop(app);
	}
	app_del_bitmap(b);
	app_del_window(w);
	app_del_font(f);
}

int main(int argc, char *argv[])
{
	char *font_name = "Times";
	int style = PLAIN;
	int height = 16;
	int show = 0;
	int save = 1;
	int h, i;

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "bold") == 0)
			style |= BOLD;
		else if (strcmp(argv[i], "b") == 0)
			style |= BOLD;
		else if (strcmp(argv[i], "bi") == 0)
			style |= (BOLD | ITALIC);
		else if (strcmp(argv[i], "ib") == 0)
			style |= (BOLD | ITALIC);
		else if (strcmp(argv[i], "italic") == 0)
			style |= ITALIC;
		else if (strcmp(argv[i], "i") == 0)
			style |= ITALIC;
		else if (strcmp(argv[i], "antialias") == 0)
			style |= ANTI_ALIAS;
		else if (strcmp(argv[i], "anti-alias") == 0)
			style |= ANTI_ALIAS;
		else if (strcmp(argv[i], "a") == 0)
			style |= ANTI_ALIAS;
		else if (strcmp(argv[i], "plain") == 0)
			style = PLAIN;
		else if (strcmp(argv[i], "show") == 0)
			show = 1;
		else if (strcmp(argv[i], "noshow") == 0)
			show = 0;
		else if (strcmp(argv[i], "save") == 0)
			save = 1;
		else if (strcmp(argv[i], "nosave") == 0)
			save = 0;
		else if ((argv[i][0] >= '0') && (argv[i][0] <= '9'))
			height = atoi(argv[i]);
		else
			font_name = argv[i];
	}

	app = app_new_app(argc, argv);
	for (h = 16; h <= height; h++)
	{
		make_font_window(font_name, PLAIN,               h, show, save);
		make_font_window(font_name, BOLD,                h, show, save);
		make_font_window(font_name, BOLD | ITALIC,       h, show, save);
		make_font_window(font_name, ITALIC,              h, show, save);
		make_font_window(font_name, ANTI_ALIAS,          h, show, save);
		make_font_window(font_name, BOLD | ANTI_ALIAS,   h, show, save);
		make_font_window(font_name, ITALIC | ANTI_ALIAS, h, show, save);
		make_font_window(font_name, BOLD|ITALIC|ANTI_ALIAS,h,show,save);
	}
	app_del_app(app);

	return 0;
}
