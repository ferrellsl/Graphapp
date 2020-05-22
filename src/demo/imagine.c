/*
 *  Imagine Viewer
 *  --------------
 *  Imagine Viewer is an image viewer program which allows you to
 *  view the following graphics formats in a window on your PC:
 *
 *  1. Normal and progressive JPEG files
 *  2. Any PNG file
 *  3. Any GIF file
 *
 *  A window palette provides the best mapping of the file
 *  to the current screen resolution and depth, if the screen
 *  depth is <= 8. Otherwise, the images are loaded as 32-bit
 *  direct colour images.
 *
 *  Copyright (c) L.Patrick 2000-2002
 *
 *  This program uses the JPEG library from the Independent JPEG Group,
 *  version 6b. It also uses the excellent LibPNG and ZLIB libraries.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <graphapp.h>

char **app_add_array_element(char **, char *);
int app_get_array_length(char **);

static char *progname = "Imagine Viewer";

App *app;

typedef struct Viewer    Viewer;

struct Viewer {
	App	* app;
	Window *  win;
	Bitmap *  bmp;
	Image *   img;
	Palette	* pal;
	Graphics *gb;
	Graphics *gi;
	Graphics *gw;
	char *    filename;
	int       percent;
	int       count;
	int       halt;
	Folder *  folder;
	int       fileno;
	char **   filepaths;
};

Viewer *new_viewer(App *app, Window *win)
{
	Viewer *v;

	v = zero_alloc(sizeof(struct Viewer));
	v->app = app;
	v->win = win;
	return v;
}

void del_viewer(Viewer *v)
{
	if (v->gw)
		del_graphics(v->gw);
	if (v->gb)
		del_graphics(v->gb);
	if (v->gi)
		del_graphics(v->gi);
	if (v->pal)
		del_palette(v->pal);
	if (v->bmp)
		del_bitmap(v->bmp);
	if (v->img)
		del_image(v->img);
	if (v->win)
		del_window(v->win);
	if (v->app)
		del_app(v->app);
	free(v);
}

int message_func(ImageReader *r, char *msg)
{
	ask_ok(app, "Error Loading Image File", msg);
	return 1;
}

int error_func(ImageReader *r)
{
	Viewer *v = r->user_data;

	if (v->gw) {
		del_graphics(v->gw);
		v->gw = NULL;
	}
	if (v->gb) {
		del_graphics(v->gb);
		v->gb = NULL;
	}
	if (v->gi) {
		del_graphics(v->gi);
		v->gi = NULL;
	}
	if (v->bmp) {
		del_bitmap(v->bmp);
		v->bmp = NULL;
	}
	if (v->img) {
		del_image(v->img);
		v->img = NULL;
	}
	return 1;
}

int startup_func(ImageReader *r)
{
	Viewer *v = r->user_data;

	v->gw = NULL;
	v->gb = NULL;
	v->gi = NULL;
	v->percent = -1;
	v->count = 0;
	size_window(v->win, rect(0, 0, r->width, r->height));
	show_window(v->win);
	return 1;
}

int after_dither_func(ImageReader *r)
{
	Viewer *v = r->user_data;

	/* Set up window palette. */
	if (r->required_depth <= 8) {
		if (v->pal) {
			set_window_palette(v->win, v->pal);
		}
		else if (r->pal) {
			v->pal = new_palette(r->pal->size, r->pal->element);
			set_window_palette(v->win, v->pal);
		}
	}

	/* Resize the window to the size of the image (if possible). */
	size_window(v->win, rect(0, 0, r->width, r->height));

	/* Create bitmap to hold image. */
	if (v->bmp)
		del_bitmap(v->bmp);
	v->bmp = new_bitmap(v->win, r->width, r->height);

	/* Fill bitmap with white */
	v->gb = get_bitmap_graphics(v->bmp);
	set_rgb(v->gb, rgb(255, 255, 255));
	fill_rect(v->gb, rect(0, 0, r->width, r->height));

	/* Create image to hold data. */
	if (v->img)
		del_image(v->img);
	v->img = new_image(r->width, r->height, r->required_depth);
	if (r->pal)
		set_image_cmap(v->img, r->pal->size, r->pal->element);

	/* Fill image with white */
	v->gi = get_image_graphics(v->img);
	set_rgb(v->gi, rgb(255, 255, 255));
	fill_rect(v->gi, rect(0, 0, r->width, r->height));

	v->filename = app_copy_string(r->filename);

	v->gw = get_window_graphics(v->win);
	if (! r->src_pal) {
		/* erase window before displaying new bmp */
		set_rgb(v->gw, rgb(255, 255, 255));
		fill_rect(v->gw, rect(0, 0, r->width, r->height));
	}

	return 1;
}

int progress_func(ImageReader *r)
{
	Viewer *v;
	int percent;
	int length;
	char *buffer;

	v = r->user_data;
	percent = (r->rows_done * 100L / r->height);

	if (percent != v->percent) {
		v->percent = percent;
		length = strlen(r->filename) + 80;
		buffer = alloc(length);
		if (r->max_stages > 1) {
			sprintf(buffer, "%s %d/%d: %3d%%",
				r->filename,
				r->stage,
				r->max_stages,
				percent);
		} else {
			sprintf(buffer, "%s %3d%%",
				r->filename, percent);
		}
		set_window_title(v->win, buffer);
		free(buffer);
	}
	while (peek_event(v->app))
		if (! do_event(v->app))
			break;
	if (v->halt)
		return 0;
	return 1;
}

int rendering_func(ImageReader *r)
{
	Viewer *v;
	int y;
	Rect dr;
	Point dp;

	v = r->user_data;
	y = r->row;
	dp = pt(0,y);
	dr = rect(0,y,r->width,1);

	if (r->required_depth <= 8) {
		copy_bits(v->gb, dr, v->pal, &r->data8[y]);
		copy_bits(v->gi, dr, v->pal, &r->data8[y]);
	}
	else {
		copy_rgbs(v->gb, dr, &r->data32[y]);
		copy_rgbs(v->gi, dr, &r->data32[y]);
	}

	copy_rect(v->gw, dp, v->gb, dr);

	return 1;
}

int success_func(ImageReader *r)
{
	Viewer *v;

	v = r->user_data;
	set_window_title(v->win, r->filename);

	/* Release graphics objects used in drawing. */
	del_graphics(v->gw);
	del_graphics(v->gb);
	del_graphics(v->gi);
	v->gw = NULL;
	v->gb = NULL;
	v->gi = NULL;

	return 1;
}

/*
 *  Special effects:
 */
Rect display_area(Window *w, Rect ir)
{
	double hscale, vscale;
	Rect r;

	r = get_window_area(w);
	hscale = r.width * 1.0 / ir.width;
	vscale = r.height * 1.0 / ir.height;
	if ((hscale <= 0.2) || (vscale <= 0.2))
		hscale = vscale = 0.2;
	if (hscale < vscale)
		vscale = hscale;
	else
		hscale = vscale;
	r = rect(0, 0, ir.width * hscale, ir.height * vscale);
	return r;
}

void lighten_image(Viewer *v)
{
	int x, y;
	Rect r;
	Colour c;
	Image *i;
	Graphics *gw, *gb;
	double factor = 1.1;
	long max = 255 / factor;

	if (v->img) {
		/* modify the image */
		i = v->img;
		if (i->depth == 8) {
			for (y=0; y < i->cmap_size; y++) {
				c = i->cmap[y];
				if (c.red >= max)
					c.red = 255;
				else
					c.red *= factor;
				if (c.green >= max)
					c.green = 255;
				else
					c.green *= factor;
				if (c.blue >= max)
					c.blue = 255;
				else
					c.blue *= factor;
				i->cmap[y] = c;
			}
		}
		else for (y=0; y < i->height; y++)
			for (x=0; x <= i->width; x++) {
				c = i->data32[y][x];
				if (c.red >= max)
					c.red = 255;
				else
					c.red *= factor;
				if (c.green >= max)
					c.green = 255;
				else
					c.green *= factor;
				if (c.blue >= max)
					c.blue = 255;
				else
					c.blue *= factor;
				i->data32[y][x] = c;
			}

		/* update the bitmap and window */
		r = get_image_area(i);
		r = display_area(v->win, r);
		if (v->bmp)
			del_bitmap(v->bmp);
		v->bmp = new_bitmap(v->win, r.width, r.height);
		gb = get_bitmap_graphics(v->bmp);
		draw_image(gb, r, i, get_image_area(i));

		gw = get_window_graphics(v->win);
		copy_rect(gw, pt(0,0), gb, get_bitmap_area(v->bmp));
		del_graphics(gw);
		del_graphics(gb);
	}
}

void darken_image(Viewer *v)
{
	int x, y;
	Rect r;
	Colour c;
	Image *i;
	Graphics *gw, *gb;
	double factor = 1.1;

	if (v->img) {
		/* modify the image */
		i = v->img;
		if (i->depth == 8) {
			for (y=0; y < i->cmap_size; y++) {
				c = i->cmap[y];
				c.red /= factor;
				c.green /= factor;
				c.blue /= factor;
				i->cmap[y] = c;
			}
		}
		else for (y=0; y < i->height; y++)
			for (x=0; x <= i->width; x++) {
				c = i->data32[y][x];
				c.red /= factor;
				c.green /= factor;
				c.blue /= factor;
				i->data32[y][x] = c;
			}

		/* update the bitmap and window */
		r = get_image_area(i);
		r = display_area(v->win, r);
		if (v->bmp)
			del_bitmap(v->bmp);
		v->bmp = new_bitmap(v->win, r.width, r.height);
		gb = get_bitmap_graphics(v->bmp);
		draw_image(gb, r, i, get_image_area(i));

		gw = get_window_graphics(v->win);
		copy_rect(gw, pt(0,0), gb, get_bitmap_area(v->bmp));
		del_graphics(gw);
		del_graphics(gb);
	}
}

/*
 *  Utility functions:
 */
int correct_aspect_ratio(Window *w)
{
	long win_aspect, img_aspect;
	Rect area;
	int width, height;
	ImageReader *r = get_window_data(w);

	area = get_window_area(w);
	win_aspect = (area.height * 1024L / area.width);
	img_aspect = (r->height * 1024L / r->width);

	if (win_aspect > img_aspect) {
		/* window width was reduced to fit screen */
		height = ((long) area.width * r->height) / r->width;
		width = area.width; 
		size_window(w, rect(0,0,width,height));
		return 0;
	}
	else if (win_aspect < img_aspect) {
		/* window height was reduced to fit screen */
		width = ((long) area.height * r->width) / r->height;
		height = area.height;
		size_window(w, rect(0,0,width,height));
		return 0;
	}
	return 1;
}

int string_ends_with(char *s, char *ending)
{
	int len1 = strlen(s);
	int len2 = strlen(ending);

	if (len1 < len2)
		return 0;
	if (!strcmp(s + len1 - len2, ending))
		return 1;
	return 0;
}

int sort_alphabetically(const void *a, const void *b)
{
	return strcmp(*(char **)a, *(char **)b);
}

void init_folder_view(Viewer *v)
{
	char *foldername;
	int i, len1, len2;
	char *filename;

	if (v->folder)
		app_close_folder(v->folder);
	if (v->filepaths) {
		for (i=0; v->filepaths[i]; i++)
			app_del_string(v->filepaths[i]);
		v->filepaths = NULL;
	}

	if (v->filename)
		foldername = app_copy_string(v->filename);
	else
		foldername = app_copy_string("./");

	for (i=0; foldername[i] != '\0'; i++)
		continue;
	for (; i >= 0; i--) {
		if (foldername[i] == '/') {
			i++;
			foldername[i] = '\0';
			break;
		}
	}
	if (i < 0) {
		app_free(foldername);
		foldername = app_copy_string("./");
		i = 0;
	}
	len1 = strlen(foldername);
	v->folder = app_open_folder(foldername);
	v->fileno = 0;

	for (i=0; v->folder != NULL; i++) {
		filename = app_read_folder(v->folder);
		if (! filename) {
			app_close_folder(v->folder);
			v->folder = NULL;
			break;
		}
		len2 = strlen(filename);
		if (string_ends_with(filename, ".gif")
		 || string_ends_with(filename, ".png")
		 || string_ends_with(filename, ".jpg"))
		{
			char *p = app_zero_alloc(len1 + len2 + 1);
			strcpy(p, foldername);
			strcat(p + len1, filename);
			v->filepaths = app_add_array_element(v->filepaths, p);
		}
	}
	if (v->filepaths)
	{
		int max = app_get_array_length(v->filepaths);
		qsort(v->filepaths, max, sizeof(char *), sort_alphabetically);
		for (i = 0; v->filepaths[i]; i++)
		{
			if (! v->filename)
				break;
			if (string_ends_with(v->filepaths[i], v->filename))
			{
				v->fileno = i;
				break;
			}
		}
	}
}

void load_a_specific_image(Window *w, Viewer *v, ImageReader *r, char *name)
{
	if (name == NULL)
		return; /* cancelled */
	if (v->filename)
		del_string(v->filename);
	v->filename = name;
	if (v->img)
		del_image(v->img);
	v->img = read_image(name, r->required_depth);
	if (v->bmp)
		del_bitmap(v->bmp);
	if (v->img)
		v->bmp = image_to_bitmap(w, v->img);
	r->width = v->img->width;
	r->height = v->img->height;
	set_window_title(w, v->filename);
	redraw_window(w);
}

void load_a_new_image(Window *w, Viewer *v, ImageReader *r)
{
	char *name;

	name = ask_file_open(w->app, "Open Image", "Open", v->filename);
	if (name == NULL)
		return; /* cancelled */
	if (v->filename)
		del_string(v->filename);
	v->filename = name;
	if (v->img)
		del_image(v->img);
	v->img = read_image(name, r->required_depth);
	if (v->bmp)
		del_bitmap(v->bmp);
	if (v->img)
		v->bmp = image_to_bitmap(w, v->img);
	r->width = v->img->width;
	r->height = v->img->height;
	set_window_title(w, v->filename);
	redraw_window(w);

	init_folder_view(v);
}

/*
 *  Responding to user actions:
 */
void window_mouse_down(Window *w, int buttons, Point p)
{
	ImageReader *r = get_window_data(w);
	Viewer *v = r->user_data;
	int i;
	char *lines[] = { "Open...", "-",
		"Lighten", "Darken",
		"-", "About...",
		"-", "Quit", NULL };

	v->halt = 1;

	if (buttons & RIGHT_BUTTON) {
		i = pop_up_list(w, NULL, lines, RIGHT_BUTTON, p);
		if (i == 0) {
			load_a_new_image(w, v, r);
		}
		else if (i == 2) {
			lighten_image(v);
		}
		else if (i == 3) {
			darken_image(v);
		}
		else if (i == 5) {
			ask_ok(w->app, "About Imagine",
			"Imagine image viewer\nCopyright 2000-2001 L. Patrick.\nAll rights reserved.");
		}
		else if (i == 7) {
			exit(0);
		}
	}
}

void window_mouse_up(Window *w, int buttons, Point p)
{
}

void window_key_down(Window *w, unsigned long key)
{
	ImageReader *r = get_window_data(w);
	Viewer *v = r->user_data;
	int move = 0;

	if ((key == 'q') || (key == 'Q') || (key == ESC)) {
		hide_window(w);
		exit(0);
	}

	if (key == DOWN)
		move = 1;
	else if (key == UP)
		move = -1;
	else if (key == PGDN)
		move = 10;
	else if (key == PGUP)
		move = -10;

	if (move) {
		int max = app_get_array_length(v->filepaths);
		v->fileno += move;
		while (v->fileno < 0)
			v->fileno += max;
		while (v->fileno > max)
			v->fileno -= max;
	}

	if (move && v->filename && v->filepaths)
	{
		load_a_specific_image(w, v, r,
				app_copy_string(v->filepaths[v->fileno]));
	}
}

void window_redraw(Window *w, Graphics *g)
{
	Graphics *bg;
	Rect br;
	ImageReader *r;
	Viewer *v;
	int len;
	Rect ir;
	char info[80];

	r = get_window_data(w);
	v = r->user_data;

	if (v->bmp) {
		bg = get_bitmap_graphics(v->bmp);
		br = get_bitmap_area(v->bmp);

		copy_rect(g, pt(0,0), bg, br);
		del_graphics(bg);
	}
	else {
		br = get_window_area(w);
	}
	if (v->filename) {
		set_rgb(g, BLUE);
		len = strlen(v->filename);
		draw_utf8(g, pt(0,br.height), v->filename, len);
	}

	if (v->img == NULL)
		return;

	len = sprintf(info, "Width: %d, Height: %d", r->width, r->height);
	draw_utf8(g, pt(0,br.height+17), info, len);

	ir = display_area(w, get_image_area(v->img));

	len = sprintf(info, "Displayed at: %d x %d", ir.width, ir.height);
	draw_utf8(g, pt(0,br.height+34), info, len);
}

void window_resize(Window *w)
{
	Graphics *gb;
	Rect wr, ir;
	ImageReader *r;
	Viewer *v;

	r = get_window_data(w);
	v = r->user_data;

	if (v->img == NULL)
		return;
	if (r->state != STOPPED)
		return;
	if (v->bmp)
		del_bitmap(v->bmp);
	v->bmp = NULL;

	ir = get_image_area(v->img);
	wr = display_area(w, ir);

	v->bmp = new_bitmap(w, wr.width, wr.height);
	if (v->bmp == NULL)
		return;

	gb = get_bitmap_graphics(v->bmp);
	draw_image(gb, wr, v->img, ir);
	del_graphics(gb);
}

void window_close(Window *w)
{
	ImageReader *r = get_window_data(w);
	Viewer *v = r->user_data;

	v->halt = 2;
	hide_window(w);
}

Palette *create_standard_palette(void)
{
	int i, red, green, blue, grey;
	Colour elem[256];
	Palette *pal;

	i = 0;

	/* assign 216 colours in a 6x6x6 colour cube */
	for (red=0; red < 256; red += 51)
	  for (green=0; green < 256; green += 51)
	    for (blue=0; blue < 256; blue += 51) {
		elem[i] = rgb(red, green, blue);
		i++;
	    }

	/* add 15 greyscales */
	for (grey=16; grey < 255; grey += 16) {
		elem[i] = rgb(grey, grey, grey);
		i++;
	}

	pal = new_palette(i, elem);
	return pal;
}

/*
 *  Initialise the program:
 */
ImageReader *create_image_reader(Viewer *v, Palette *pal, int depth)
{
	ImageReader *r;

	r = new_image_reader();

	if (pal) {
		r->src_pal = pal;
		r->required_depth = 8;
		r->max_cmap_size = pal->size;
	}
	else if (depth <= 8) {
		r->required_depth = 8;
		r->max_cmap_size = 256;
	}
	else {
		r->required_depth = 32;
	}

	r->width = 300;
	r->height = 200;
	r->user_data = v;

	r->message_func = message_func;
	r->error_func = error_func;
	r->startup_func = startup_func;
	r->after_dither_func = after_dither_func;
	r->progress_func = progress_func;
	r->rendering_func = rendering_func;
	r->success_func = success_func;

	return r;
}

int main(int argc, char *argv[])
{
	Window *win;
	Palette *pal;
	ImageReader *r;
	Viewer *v;
	Image *img = NULL;
	int arg;

	debug_memory(1);
	app = new_app(argc, argv);

	win = new_window(app, rect(0,0,300,200), progname,
		TITLEBAR | RESIZE | CLOSEBOX | MINIMIZE);

	pal = NULL;
	/*pal = create_standard_palette();*/

	v = new_viewer(app, win);
	r = create_image_reader(v, pal, 32);
	set_window_data(win, r);

	on_window_resize(win, window_resize);
	on_window_redraw(win, window_redraw);
	on_window_close(win, window_close);
	on_window_mouse_down(win, window_mouse_down);
	on_window_mouse_up(win, window_mouse_up);
	on_window_key_down(win, window_key_down);
	on_window_key_action(win, window_key_down);

	if (argc < 2)
		show_window(win);

	v->filename = NULL;

	if (argc == 1) {
		r->filename = "- standard input -";
		r->file = stdin;
		img = read_image_progressively(r);
		set_window_title(win, r->filename);
	}

	for (arg=1; argv[arg] != NULL; arg++) {
		if (v->halt == 2)
			break;
		if (v->halt) {
			if (ask_yes_no(app, "Continue?",
				"Continue displaying images?") == YES)
				v->halt = 0;
			else
				break;
		}

		if (r->filename)
			del_string(r->filename);
		r->filename = copy_string(argv[arg]);

		if (img)
			del_image(img);
		img = read_image_progressively(r);
		if (img == NULL)
			img = read_image(r->filename, 32);
		if (img == NULL)
			v->halt = 1;

		if (v->filename)
			set_window_title(win, v->filename);
		init_folder_view(v);
		/*
		if (! correct_aspect_ratio(win))
			draw_window(win);
		draw_all(app);
		*/
		while (peek_event(app))
			do_event(app);
		if ((! v->halt) && (argv[arg+1] != NULL))
			app_delay(app, 1000);
		while (peek_event(app))
			do_event(app);
	}

	if (! v->filename)
		set_window_title(win, progname);

	draw_window(win);

	main_loop(app);

	if (v->bmp)
		del_bitmap(v->bmp);
	del_window(win);
	del_app(app);

	return 0;
}
