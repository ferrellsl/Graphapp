/*
 *  Mouse wheel demo
 *  ----------------
 *
 *  Turn the mouse wheel over each part of the window:
 *    the list (300 items) and the text (200 lines) scroll 3 lines a
 *    notch (or as many as Windows is set to); the list also scrolls
 *    sideways with Shift + wheel or a tilting wheel (the text box
 *    wraps its lines, so it has no sideways scrolling);
 *    the scroll bars scroll themselves;
 *    the circle grows and shrinks, using on_control_mouse_wheel(),
 *    which gives how many lines the wheel turned, all at once;
 *    the counter below it uses the older on_window_mouse_wheel_up()
 *    and _down() handlers (GraphApp 3.70's), one call per notch.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graphapp.h"

App *app;
Window *w;
Control *canvas, *zoom_label, *notch_label;
int radius = 40;
int notches_up = 0, notches_down = 0;

static void draw_canvas(Control *c, Graphics *g)
{
	Rect r = get_control_area(c);
	int cx = r.x + r.width / 2, cy = r.y + r.height / 2;

	set_colour(g, WHITE);
	fill_rect(g, r);
	set_colour(g, rgb(0x4F, 0x3C, 0xD6));
	fill_ellipse(g, rect(cx - radius, cy - radius, 2 * radius, 2 * radius));
	set_colour(g, BLACK);
	draw_rect(g, r);
}

/* the new handler: lines turned, dx and dy, in one call */
static void wheel_canvas(Control *c, int buttons, Point p, int dx, int dy)
{
	char text[80];

	radius -= dy * 2;	/* away from you = bigger */
	if (radius < 4) radius = 4;
	if (radius > 110) radius = 110;
	sprintf(text, "Circle: wheel %+d lines, radius %d", dy, radius);
	set_control_text(zoom_label, text);
	draw_control(c);
}

/* the 3.70 handlers: one call a notch */
static void show_notches(void)
{
	char text[80];
	sprintf(text, "Window: %d notches up, %d down", notches_up, notches_down);
	set_control_text(notch_label, text);
}
static void notch_up(Window *win, int buttons, Point p)   { notches_up++;   show_notches(); }
static void notch_down(Window *win, int buttons, Point p) { notches_down++; show_notches(); }

int main(int argc, char *argv[])
{
	static char *items[301];
	static char text[200 * 100];
	char line[100];
	int i;

	for (i = 0; i < 300; i++) {
		sprintf(line, (i % 3) ? "List item %d" : "List item %d, a long one, wider than the box: Shift + wheel scrolls it sideways", i + 1);
		items[i] = strdup(line);
	}
	items[300] = NULL;
	text[0] = '\0';
	for (i = 0; i < 200; i++) {
		sprintf(line, "Line %d of the text box, a little longer than the box is wide.\n", i + 1);
		strcat(text, line);
	}

	app = new_app(argc, argv);
	w = new_window(app, rect(50, 30, 760, 420), "Mouse Wheel Demo", STANDARD_WINDOW);

	new_label(w, rect(10, 8, 220, 20), "List box:", ALIGN_LEFT);
	new_list_box(w, rect(10, 30, 220, 370), items, NULL);

	new_label(w, rect(245, 8, 250, 20), "Text box:", ALIGN_LEFT);
	new_text_box(w, rect(245, 30, 250, 370), text);

	new_label(w, rect(510, 8, 240, 20), "Canvas (window-level below):", ALIGN_LEFT);
	canvas = new_control(w, rect(510, 30, 240, 240));
	on_control_redraw(canvas, draw_canvas);
	on_control_mouse_wheel(canvas, wheel_canvas);
	zoom_label = new_label(w, rect(510, 280, 240, 20), "Circle: turn the wheel over it", ALIGN_LEFT);
	notch_label = new_label(w, rect(510, 310, 240, 40), "Window: turn the wheel here", ALIGN_LEFT);

	on_window_mouse_wheel_up(w, notch_up);
	on_window_mouse_wheel_down(w, notch_down);

	show_window(w);
	main_loop(app);
	del_app(app);
	return 0;
}
