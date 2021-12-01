/*
 * Monty - a simple project editor.
 *
 * File: hexdump.c -- a GraphApp hexdump utility.
 * Platform: Neutral  Version: 2.00  Date: 1998/02/02
 *
 * Version: 1.00  Changes: Original version by L. Patrick.
 */

/* Copyright (C) 1998 L. Patrick

   This file is part of Monty, a simple project editor.
*/


/*
 *  Some local definitions:
 */
	#define LINE_SIZE	16
	#define LINE_WIDTH	80

	typedef struct FileInfoStruct {
		char *    file;
		Window *  w;
		Control * t;
		Control * s; /* scroll bar */
		FILE *    f;
		long      size;
		int       value;
	} FileInfoStruct;

/*
 *  The hexdump functions:
 */

static int  char_to_printable(int ch)
{
	int out_ch;

	if (ch >= 0x80)
		ch = ch - 0x80;

	if (isprint(ch))
		out_ch = ch;
	else
		out_ch = '.';

	if ((ch >= '\t') && (ch <= '\r'))
		out_ch = ' ';

	return out_ch;
}

static char * gen_hex_line(int line[], unsigned long position, int counter)
{
	static char outline[LINE_WIDTH];
	char tmp[10];
	int i;

	sprintf(outline, "%08.8lX: ", position);
	for (i = 0; i < LINE_SIZE; i = i + 1) /* print hex */
	{
		if (i % 4 == 0)
			strcat(outline, " ");
		if (i < counter) {
			sprintf(tmp, "%02.2X", line[i]);
			strcat(outline, tmp);
		}
		else
			strcat(outline, "  ");
	}
	strcat(outline, "  ");
	for (i = 0; i < LINE_SIZE; i = i + 1) /* print ascii */
	{
		if (i < counter) {
			sprintf(tmp, "%c", char_to_printable(line[i]));
			strcat(outline, tmp);
		}
		else
			strcat(outline, " ");
	}
	return outline;
}

static void reset_hex_line(int line[])
{
	int i;

	for (i = 0; i < LINE_SIZE; i = i + 1)
		line[i] = 0;
}

static void perform_hexdump(Graphics *g, FILE *f, Control *t,
				long start, long end, int h)
{
	int line[LINE_SIZE];
	unsigned long position = start;
	int counter = 0;
	int ch;
	char *outline;
	Rect r;
	Point p;
	int remains;

	r = app_get_control_area(t);
	r.width += 1;
	p = pt(r.x+2,r.y+2);

	app_set_colour(g, WHITE);
	remains = (r.height - 4) % h;
	app_fill_rect(g, rect(2,r.y+r.height-remains-2,r.width-4,remains));
	app_set_colour(g, BLACK);

	rewind(f);
	fseek(f, start, 0);
	do {
		ch = fgetc(f);
		if ((counter == LINE_SIZE) || (ch == EOF) || (start == end))
		{	/* print previous line of chars first */
			outline = gen_hex_line(line, position, counter);
			reset_hex_line(line);
			position = position + LINE_SIZE;
			counter = 0;

			app_set_colour(g, WHITE);
			app_fill_rect(g, rect(p.x,p.y,r.width-4,h));
			app_set_colour(g, BLACK);
			app_draw_utf8(g, p, outline, strlen(outline));
			p.y += h;
		}
		/* store char into line */
		line[counter] = ch;
		counter += 1;
		start += 1;
	} while ((ch != EOF) && (start <= end));

	app_set_colour(g, WHITE);
	app_draw_rect(g, app_inset_rect(r,1));
	app_set_colour(g, BLACK);
	app_draw_rect(g, r);
}

static void redraw_hexwin(Control *t, Graphics *g)
{
	Window *w     = app_parent_window(t);
	Rect r        = app_get_control_area(t);
	FileInfoStruct *fi  = app_get_window_data(w);
	Font *fnt     = app_find_default_font(w->app);
	long h        = app_font_height(fnt);
	long pagesize = ((r.height - 4) / h);
	long value    = fi->value;
	long start    = value * LINE_SIZE;
	long end      = start + (pagesize+1) * LINE_SIZE;

	perform_hexdump(g, fi->f, fi->t, start, end, h);
}

static void scroll_hexwin(Control *s)
{
	Window *w    = app_parent_window(s);
	FileInfoStruct *fi = app_get_window_data(w);
	int value = app_get_control_value(s);

	if (value != fi->value) {
		fi->value = value;
		app_draw_control(fi->t);
	}
}

static void resize_hexwin(Window *w)
{
	FileInfoStruct *fi = (FileInfoStruct *) app_get_window_data(w);
	long numlines, pagesize, h, max, value;
	Font *fnt;
	Rect r;

	r = app_get_window_area(w);

	app_set_control_area(fi->t, rect(10,10,r.width-20-16,r.height-20));
	app_set_control_area(fi->s, rect(r.width-10-16,10,16,r.height-20));

	fnt = app_find_default_font(w->app);

	h = app_font_height(fnt);
	r = app_get_control_area(fi->t);
	pagesize = ((r.height - 4) / h);
	numlines = (fi->size + LINE_SIZE - 1) / LINE_SIZE;
	max = numlines - pagesize;
	if (max < 0)
		max = 0;
	value = app_get_control_value(fi->s);

	if (value > max) {
		app_change_scroll_bar(fi->s, max, max, pagesize);
		fi->value = max;
		app_draw_control(fi->t);
	} else {
		app_change_scroll_bar(fi->s, value, max, pagesize);
	}
}

static void close_hexwin(Window *w)
{
	FileInfoStruct *fi = (FileInfoStruct *) app_get_window_data(w);

	if (app_close_file(fi->f) == 0) {
		ask_ok_str(w->app, "Error",
			"Could not close file \"%s\"\n", fi->file);
		return;
	}
	discard(fi->file);
	discard(fi);
	app_del_window(w);
}

static void create_hexwin(App *app, char *file)
{
	Rect r       = rect(10,10,620,400);
	Font *fnt    = app_find_default_font(app);
	FileInfoStruct *fi = create(FileInfoStruct);
	char *name;

	if ((fi->f = app_open_file(file, "rb")) == NULL) {
		ask_ok_str(app, "Error",
			"Could not hexdump \"%s\"\n", file);
		return;
	}

	fi->size = app_file_size(file);
	fi->file = app_copy_string(file);

	name = add_strings(NULL, "Hexdump: ");
	name = add_strings(name, file);
	fi->w = app_new_window(app, r, name, STANDARD_WINDOW);
	app_del_string(name);
	app_set_window_data(fi->w, fi);
	app_set_window_background(fi->w, LIGHT_GREY);

	r = rect(10,10,r.width-20-16,r.height-20);
	fi->t = app_new_control(fi->w, r);
	/* app_set_control_textfont(fi->t, fnt); */
	app_on_control_redraw(fi->t, redraw_hexwin);

	fi->s = app_new_scroll_bar(fi->w,
			rect(r.x+r.width,r.y,16,r.height),
			100, 25, scroll_hexwin);

	app_on_window_resize(fi->w, resize_hexwin);
	app_on_window_close(fi->w, close_hexwin);
	app_show_window(fi->w);
}

void hexdump(int num_files, char *files[])
{
	int i;
	App *app;

	app = app_new_app(0,0);
	for (i=0; i<num_files; i++) {
		if ((files[i] != NULL) && (files[i][0] != '-'))
			create_hexwin(app, files[i]);
	}
	app_main_loop(app);
	app_del_app(app);
}
