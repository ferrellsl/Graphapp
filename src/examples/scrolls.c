/*
 *  Scrolls
 *  -------
 *
 *  A program to display a number of scrollbars,
 *  and test user-interactions.
 */

#include <stdio.h>
#include <string.h>
#include <graphapp.h>

enum {
	MAX_SCROLLBARS = 10
};

void scroll_bar_fn(Control *c)
{
	Window *w;
	Graphics *g;
	char *name;
	char info[160];

	w = parent_window(c);
	g = get_window_graphics(w);
	set_rgb(g, WHITE);
	fill_rect(g, rect(0,0,200,200));
	set_rgb(g, BLACK);
	name = get_control_text(c);
	draw_utf8(g, pt(5,5), name, strlen(name));
	sprintf(info, "Current value %ld", get_control_value(c));
	draw_utf8(g, pt(5,24), info, strlen(info));
	del_graphics(g);
}

void add_scroll_bars(Window *w)
{
	Control *c;
	int i;
	Rect r;
	char name[80];

	for (i=1; i <= MAX_SCROLLBARS; i++) {
		r = rect(200+18*i, i, 17, 30*i);
		c = new_scroll_bar(w, r, 100-5*i, 10+i, scroll_bar_fn);
		sprintf(name, "Vertical scroll bar %d", i);
		set_control_text(c, name);
	}
	for (i=1; i <= MAX_SCROLLBARS; i++) {
		r = rect(i, 200+18*i, 30*i, 17);
		c = new_scroll_bar(w, r, 100-5*i, 10+i, scroll_bar_fn);
		sprintf(name, "Horizontal scroll bar %d", i);
		set_control_text(c, name);
	}
}

int main(int argc, char *argv[])
{
	App *app;
	Window *w;

	app = new_app(argc, argv);
	w = new_window(app, rect(10,10,400,400), "Scrollbar Test",
			STANDARD_WINDOW);

	add_scroll_bars(w);

	show_window(w);
	main_loop(app);
	del_app(app);
	return 0;
}
