/*
 *  Photo Viz 2
 *  -----------
 *  This is a program designed to show photos in a window.
 *  It allows browsing of the photos, and selection of photos
 *  for printing. Useful for sorting out your holiday snaps.
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

Control *print;
Control *view;
Control *background;
int *print_list;
int *view_list;
int *background_list;

enum {
	NUM_THUMBS_TO_SHOW = 5,
	FULL_SIZE  = 480,
	THUMB_SIZE = 64
};

Image *	prev_image = NULL;	/* image just above this one, for speed */
Image *	this_image = NULL;	/* this image being shown at photo size */
Image *	next_image = NULL;	/* image just below this one, for speed */

int	total = 0;		/* number of images to show */
char **	names;			/* all filenames of those images */
Image **thumbs;			/* stores all thumbs (they're small) */
Image **cache = NULL;		/* stores all cached scaled images */

int	shown = 0;		/* images[1] shows names[shown] */

void save_data()
{
	/* Save user data on what image files to print, etc. */

	FILE *f;
	int i, c, len, oldlen = -1;
	char *filepath;
	char *filename;
	char *foldername;

	f = open_file("photos.txt", "w");
	if (! f)
		return;

	for (i=0; i < total; i++) {
		filepath = names[i];
		filename = get_file_name(filepath);
		len = filename - filepath;
		if ((oldlen != len) || strncmp(foldername, filepath, len))
		{
			foldername = filepath;
			oldlen = len;
			//fprintf(f, "\n%*s\n", len, foldername);
			fputc('\n', f);
			for (c = 0; c < len; c++)
				fputc(foldername[c], f);
			fputc('\n', f);
		}

		if (print_list[i])
			fprintf(f, "P");
		else
			fprintf(f, " ");
		if (view_list[i])
			fprintf(f, "S");
		else
			fprintf(f, " ");
		if (background_list[i])
			fprintf(f, "B");
		else
			fprintf(f, " ");
		fprintf(f, "\t%s\n", filename);
	}

	fclose(f);
}

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
		if ((cache != NULL) && (cache[i] != NULL)) {
			img = cache[i];
		}
		else {
			img = read_image(names[i], 32);
			temp = 1;
			if (cache != NULL) {
				Image *scaled = scale_image(img,
					scale_rect(
					 rect(0, 0, FULL_SIZE, FULL_SIZE),
					 get_image_area(img)),
					get_image_area(img));
				if (scaled != NULL) {
					cache[i] = scaled;
					del_image(img);
					img = scaled;
					temp = 0;
				}
			}
		}
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

	if (thumbs[i] == NULL) {
		thumbs[i] = scale_image(img,
			scale_rect(rect(0, 0, THUMB_SIZE, THUMB_SIZE),
					get_image_area(img)),
			get_image_area(img));
	}

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

void update_checkboxes()
{
	if (print_list[shown])
		check(print);
	else
		uncheck(print);

	if (view_list[shown])
		check(view);
	else
		uncheck(view);

	if (background_list[shown])
		check(background);
	else
		uncheck(background);
}

void go_to_next()
{
	if (shown < total - 1) {
		shown++;

		if (prev_image && ((cache == NULL) || (cache[shown-2] != prev_image)))
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

		if (next_image && ((cache == NULL) || (cache[shown+2] != next_image)))
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
			save_data();
			break;
		case DOWN:
		case RIGHT:
			go_to_next();
			break;
		case UP:
		case LEFT:
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
		case 'p':
		case 'P':
			if (is_checked(print))
				uncheck(print);
			else
				check(print);
			activate_control(print);
			break;
		case 's':
		case 'S':
			if (is_checked(view))
				uncheck(view);
			else
				check(view);
			activate_control(view);
			break;
		case 'b':
		case 'B':
		case 'u':
		case 'U':
			if (is_checked(background))
				uncheck(background);
			else
				check(background);
			activate_control(background);
			break;
		case DEL:
			printf("rm %s\n", names[shown]);
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

	/* Update the status of the checkboxes to reflect user choices. */
	update_checkboxes();

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

void check_it(Control *c)
{
	int * list;

	if (c == print)
		list = print_list;
	else if (c == view)
		list = view_list;
	else if (c == background)
		list = background_list;
	else
		return;
	list[shown] = is_checked(c);
}

void window_close(Window *w)
{
	hide_window(w);
	save_data();
}

int main(int argc, char *argv[])
{
	int arg;

	debug_memory(1);
	app = new_app(argc, argv);

	win = new_window(app, rect(0,0,700,560), progname,
		TITLEBAR | RESIZE | CLOSEBOX | MINIMIZE);

	print = new_check_box(win, rect(350, FULL_SIZE + 12, 320, 18),
				"Print this photo later?", check_it);
	view = new_check_box(win, rect(350, FULL_SIZE + 32, 320, 18),
				"Show to friends but don't print?", check_it);
	background = new_check_box(win, rect(350, FULL_SIZE + 52, 320, 18),
				"Use as a PC background image?", check_it);

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

	if ((cache = zero_alloc(total * sizeof(Image *))) == NULL)
		return 1;

	if ((print_list = zero_alloc(total * sizeof(int))) == NULL)
		return 1;
	if ((view_list = zero_alloc(total * sizeof(int))) == NULL)
		return 1;
	if ((background_list = zero_alloc(total * sizeof(int))) == NULL)
		return 1;

	show_window(win);
	set_window_title(win, get_file_name(names[shown]));
	main_loop(app);
	del_app(app);

	return 0;
}
