/*
 *  Photo Viz
 *  ---------
 *  Photo Viz is a program designed to show photos in a window.
 *  It allows browsing of the photos.
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

/*
 *  Globals:
 */
static char *progname = "Photos Viz";

App *app;
Window *win;

enum {
	NUM_THUMBS_TO_SHOW = 5,
	FULL_SIZE  = 540,
	THUMB_SIZE = 64
};

Image *	prev_image = NULL;	/* image just above this one, for speed */
Image *	this_image = NULL;	/* this image being shown at photo size */
Image *	next_image = NULL;	/* image just below this one, for speed */

int	total = 0;		/* number of images to show */
char **	names;			/* all filenames of those images */
Image **thumbs;			/* stores all thumbs (they're small) */

int	shown = 0;		/* images[1] shows names[shown] */

Rect scale_rect(Rect dr, Rect sr)
{
	double hscale, vscale;

	hscale = dr.width * 1.0 / sr.width;
	vscale = dr.height * 1.0 / sr.height;
	if (hscale < vscale)
		vscale = hscale;
	else
		hscale = vscale;
	dr = rect(0, 0, sr.width * hscale, sr.height * vscale);
	return dr;
}

int load_image(int i)
{
	Image *img = NULL;
	int temp = 0;

	if (i < 0)
		return 0;
	if (i >= total)
		return 0;

	if ((i == shown) && (this_image))
		img = this_image;
	if ((i == shown - 1) && (prev_image))
		img = prev_image;
	if ((i == shown + 1) && (next_image))
		img = next_image;

	if (img == NULL) {
		img = read_image(names[i], 32);
		temp = 1;
	}
	if (img == NULL)
		return 0;

	if ((i == shown) && (this_image == NULL)) {
		this_image = img;
		temp = 0;
	}
	if ((i == shown - 1) && (prev_image == NULL)) {
		prev_image = img;
		temp = 0;
	}
	if ((i == shown + 1) && (next_image == NULL)) {
		next_image = img;
		temp = 0;
	}

	if (thumbs[i] == NULL)
		thumbs[i] = scale_image(img,
			scale_rect(rect(0, 0, THUMB_SIZE, THUMB_SIZE),
					get_image_area(img)),
			get_image_area(img));

	if (temp)
		del_image(img);
	return 1;
}

int load_thumb(int i)
{
	if (i < 0)
		return 0;
	if (i >= total)
		return 0;

	if (thumbs[i] != NULL)
		return 1;

	return load_image(i);
}

void go_to_next()
{
	if (shown < total - 1) {
		shown++;

		if (prev_image)
			del_image(prev_image);
		prev_image = this_image;
		this_image = next_image;
		next_image = NULL;
	}
}

void go_to_prev()
{
	if (shown > 0) {
		shown--;

		if (next_image)
			del_image(next_image);
		next_image = this_image;
		this_image = prev_image;
		prev_image = NULL;
	}
}

/*
 *  Responding to user actions:
 */
void window_mouse_down(Window *w, int buttons, Point p)
{
}

void window_mouse_up(Window *w, int buttons, Point p)
{
}

void window_key_down(Window *w, unsigned long key)
{
	int i;
	int was_shown = shown;

	switch (key) {
		case 'q':
		case 'Q':
		case ESC:
			hide_window(w);
			exit(0);
			break;
		case 'n':
		case 'N':
		case 'd':
		case 'D':
		case RIGHT:
		case DOWN:
			go_to_next();
			break;
		case 'p':
		case 'P':
		case 'u':
		case 'U':
		case LEFT:
		case UP:
			go_to_prev();
			break;
		case PGDN:
			for (i = 0; i < NUM_THUMBS_TO_SHOW; i++)
				go_to_next();
			break;
		case PGUP:
			for (i = 0; i < NUM_THUMBS_TO_SHOW; i++)
				go_to_prev();
			break;
		case END:
			while (shown < total - 1)
				go_to_next();
			break;
		case HOME:
			while (shown > 0)
				go_to_prev();
			break;
		default:
			break;
	}
	if (shown != was_shown) {
		set_window_title(w, get_file_name(names[shown]));
		redraw_window(w);
	}
}

void window_redraw(Window *w, Graphics *g)
{
	int i, pos;
	int len;
	Rect dr;
	char info[80];
	int y = 10 + FULL_SIZE + 2;
	Rect ir = rect(10, 10, FULL_SIZE, FULL_SIZE);
	Rect tr = rect(20+FULL_SIZE, 10, THUMB_SIZE+120, FULL_SIZE);

	tr.width = get_window_area(w).width - tr.x - 10;

	set_rgb(g, BLACK);
	draw_rect(g, inset_rect(ir, -1));
	draw_rect(g, inset_rect(tr, -1));

	/* Draw the filename. */
	set_rgb(g, BLUE);
	if (names[shown]) {
		len = strlen(get_file_name(names[shown]));
		draw_utf8(g, pt(ir.x,y), get_file_name(names[shown]), len);
		draw_all(app);
	}

	/* Then load the image if necessary. */
	if (this_image == NULL)
		load_image(shown);

	/* Then draw the image and some statistics about it. */
	if (this_image) {
		len = sprintf(info, "Width: %d, Height: %d",
			this_image->width, this_image->height);
		draw_utf8(g, pt(ir.x,y+17), info, len);

		dr = scale_rect(ir, get_image_area(this_image));

		len = sprintf(info, "Displayed at: %d x %d",
			dr.width, dr.height);
		draw_utf8(g, pt(ir.x,y+34), info, len);

		draw_image(g, center_rect(dr, ir),
			this_image, get_image_area(this_image));
	}

	/* Show the thumbnail images. */
	set_clip_rect(g, inset_rect(tr, 2));
	i = shown;
	i -= NUM_THUMBS_TO_SHOW / 2;
	if (i + NUM_THUMBS_TO_SHOW > total)
		i = total - NUM_THUMBS_TO_SHOW;
	if (i < 0)
		i = 0;
	for (pos = 0; i < total; i++, pos++)
	{
		Rect r = rect(tr.x,
				tr.y + 5 + pos * (THUMB_SIZE + 25),
				tr.width, THUMB_SIZE);
		if (thumbs[i] == NULL)
			load_thumb(i);
		if (r.y + r.height + 18 >= tr.y + tr.height - 5)
			break;
		if (thumbs[i] == NULL)
			continue;
		if (i == shown) {
			set_rgb(g, BLACK);
			draw_rect(g, inset_rect(center_rect(rect(0, 0, THUMB_SIZE, THUMB_SIZE), r), -1));
		}
		draw_image(g, center_rect(get_image_area(thumbs[i]), r),
			thumbs[i], get_image_area(thumbs[i]));
		if (names[i]) {
			set_rgb(g, BLUE);
			len = strlen(get_file_name(names[i]));
			draw_utf8(g, pt(tr.x + 5, r.y + r.height + 1),
				get_file_name(names[i]), len);
		}
		draw_all(app);
	}

	/* Load the next or previous images now, for speed. */
	if (prev_image == NULL)
		load_image(shown - 1);
	if (next_image == NULL)
		load_image(shown + 1);
}

void window_resize(Window *w)
{
}

void window_close(Window *w)
{
	hide_window(w);
}

int main(int argc, char *argv[])
{
	int arg;

	debug_memory(1);
	app = new_app(argc, argv);

	win = new_window(app, rect(0,0,700,560), progname,
		TITLEBAR | RESIZE | CLOSEBOX | MINIMIZE);

	on_window_resize(win, window_resize);
	on_window_redraw(win, window_redraw);
	on_window_close(win, window_close);
	on_window_mouse_down(win, window_mouse_down);
	on_window_mouse_up(win, window_mouse_up);
	on_window_key_down(win, window_key_down);
	on_window_key_action(win, window_key_down);

	if ((names = zero_alloc(argc * sizeof(char *))) == NULL)
		return 1;
	for (arg=1; argv[arg] != NULL; arg++)
		names[arg-1] = argv[arg];
	total = arg - 1;

	if ((thumbs = zero_alloc(total * sizeof(Image *))) == NULL)
		return 1;

	show_window(win);
	set_window_title(win, get_file_name(names[shown]));
	main_loop(app);
	del_app(app);

	return 0;
}
