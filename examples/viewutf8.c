/*
 *  View UTF-8 files in a window.
 *
 *  Usage: viewutf8 filename
 *  where filename refers to a UTF-8 encoded file
 *  (ordinary ASCII text files will also work)
 *
 *  This program displays a window which displays the
 *  text of the file graphically. Arrow keys can be
 *  used to scroll the text up or down.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <graphapp.h>

enum {
	MAX_LINES = 200
};

/*
 *  Global variables:
 */

Font * font;
int    top_line = 0;
int    num_lines = 0;
long   line_lengths[MAX_LINES];
char * lines[MAX_LINES];

/*
 *  Functions:
 */

void window_redraw(Window *w, Graphics *g)
{
	Rect r;
	Point p;
	int i, h;

	r = get_window_area(w);

	set_rgb(g, rgb(240,240,240));
	fill_rect(g, r);

	set_rgb(g, rgb(255,0,0));
	set_font(g, font);
	set_text_direction(g, LR_TB);
	p = pt(0,0);
	for (i=top_line; i < num_lines; i++) {

		/*
		h = text_height(font, r.width-p.x,
			lines[i], line_lengths[i]);
		draw_text(g, rect(p.x,p.y,r.width-p.x,h),
			ALIGN_JUSTIFY, lines[i], line_lengths[i]);
		*/

		draw_utf8(g, p, lines[i], line_lengths[i]);
		h = font->height;

		p.y += h;
		if (p.y > r.height)
			break;
	}
}

void scroll_window(Window *w, int dx, int dy)
{
	Graphics *g;
	Rect r;
	Point p;

	g = get_window_graphics(w);
	r = get_window_area(w);
	p = pt(r.x + dx, r.y + dy);
	copy_rect(g, p, g, r);
	if (dy > 0) {
		/* moving window contents downwards */
		redraw_rect(w, rect(0,0,r.width,dy));
	}
	else if (dy < 0) {
		/* moving window contents upwards */
		redraw_rect(w, rect(0,r.height+dy,r.width,0-dy));
	}
	if (dx > 0) {
		/* moving window contents to the right */
		redraw_rect(w, rect(0,0,dx,r.height));
	}
	else if (dx < 0) {
		/* moving window contents to the left */
		redraw_rect(w, rect(r.width+dx,0,0-dx,r.height));
	}
	del_graphics(g);
}

void window_key_action(Window *w, unsigned long key)
{
	Rect r;
	int lines_per_page;
	int prev;

	prev = top_line;
	r = get_window_area(w);
	lines_per_page = r.height / font->height;
	if (lines_per_page < 1)
		lines_per_page = 1;

	if (key == DOWN)
		top_line++;
	if (key == UP)
		top_line--;
	if (key == PGDN)
		top_line += lines_per_page;
	if (key == PGUP)
		top_line -= lines_per_page;

	if (top_line + lines_per_page > num_lines)
		top_line = num_lines - lines_per_page;
	if (top_line < 0)
		top_line = 0;
	if (top_line >= num_lines)
		top_line = num_lines - 1;

	if (prev != top_line) {
		scroll_window(w, 0, (prev - top_line) * font->height);
		/* draw_window(w); */
	}
}

void load_lines(char *name)
{
	int i;
	long nbytes, nchars;
	char *line;
	FILE *f;

	/* load the lines of the file */
	f = open_file(name, "rb");
	if (f == NULL)
		return;
	num_lines = 0;
	for (i=0; i < MAX_LINES; i++) {
		line = read_utf8_line(f, &nbytes, &nchars);
		if (nbytes > 0) {
			lines[i] = line;
			if (line[nbytes-1] == '\n')
				nbytes--;
			if ((nbytes>0) && (line[nbytes-1] == '\r'))
				nbytes--;
			line_lengths[i] = nbytes;
			num_lines++;
		}
		else {
			break;
		}
	}
	close_file(f);
}

int main(int argc, char *argv[])
{
	App *app;
	Window *win;

	app = new_app(argc, argv);

	font = new_font(app, "unifont", PLAIN, 16);

	if (argc < 2) {
		lines[0] = "usage: viewutf8 filename";
		line_lengths[0] = strlen(lines[0]);
		num_lines = 1;
	}
	else {
		load_lines(argv[1]);
	}

	win = new_window(app, rect(50,100,512,324), "View UTF-8",
		STANDARD_WINDOW);
	on_window_redraw(win, window_redraw);
	on_window_key_action(win, window_key_action);
	show_window(win);

	main_loop(app);

	del_app(app);
	return 0;
}

