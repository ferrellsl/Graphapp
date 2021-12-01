/*
 *  A program to test window creation and event handling.
 *
 *  This program tests various simple activities:
 *   - creating two windows
 *   - receiving events for each separately
 *   - creating controls on one of the windows
 *   - drawing lines and small circles by mouse clicking
 *   - redrawing, using different colours each time to
 *     demonstrate the way redrawing occurs
 *   - drawing text
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <graphapp.h>

/*
 *  Some global variables:
 */

Colour colours[] = {
	{0x00, 0xFF, 0x00, 0x00},
	{0x00, 0x00, 0xFF, 0x00},
	{0x00, 0x00, 0x00, 0xFF},
	{0x00, 0x00, 0xFF, 0xFF},
	{0x00, 0xFF, 0x00, 0xFF},
	{0x00, 0xFF, 0xFF, 0x00},
	{0x00, 0xBF, 0xBF, 0xBF},
	{0x00, 0x7F, 0x7F, 0x7F}
};

int which = 0;

Point p1, p2;

Window *third = NULL;

/*
 *  Print some formatted text to a window:
 */

void say(Window *w, char *fmt, ...)
{
	va_list args;
	int i;
	Graphics *g;
	Rect r;
	char s[80];

	va_start(args, fmt);
	i = vsprintf(s, fmt, args);
	va_end(args);

	if (i >= 0) {
		g = get_window_graphics(w);
		set_rgb(g, rgb(255,255,255));
		r = get_window_area(w);
		fill_rect(g, rect(0,0,r.width,16));
		set_rgb(g, rgb(0,0,0));
		draw_utf8(g, pt(0,0), s, i);
		del_graphics(g);
	}
}

/*
 *  Make a third window:
 */

void create_new_window(Control *btn)
{
	Window *w = parent_window(btn);
	App *app = w->app;

	if (third == NULL)
		third = new_window(app, rect(500,100,200,160), "Tester 3",
			STANDARD_WINDOW);
	show_window(third);
}

void destroy_new_window(Control *btn)
{
	if (third)
		del_window(third);
	third = NULL;
}

/*
 *  First window's call-back functions:
 */

void window1_key_down(Window *w, unsigned long key)
{
	if ((key < 256) && (isprint(key)))
		say(w, "key_down   (key='%c')", (int)key);
	else
		say(w, "key_down   (key=%ld)", key);
}

void window1_key_action(Window *w, unsigned long key)
{
	say(w, "key_action (key=%ld)", key);
}

void window1_mouse_down(Window *w, int buttons, Point p)
{
	say(w, "mouse_down (buttons=%d, xy=%d,%d)", buttons, p.x, p.y);
	p1 = p;
}

void window1_mouse_up(Window *w, int buttons, Point p)
{
	Graphics *g;

	say(w, "mouse_up   (buttons=%d, xy=%d,%d)", buttons, p.x, p.y);
	p2 = p;

	g = get_window_graphics(w);
	draw_line(g, p1, p2);
	del_graphics(g);
}

void window1_mouse_drag(Window *w, int buttons, Point p)
{
	say(w, "mouse_drag (buttons=%d, xy=%d,%d)", buttons, p.x, p.y);
}

void window1_mouse_move(Window *w, int buttons, Point p)
{
	say(w, "mouse_move (buttons=%d, xy=%d,%d)", buttons, p.x, p.y);
}

void window1_resize(Window *w)
{
	say(w, "resize");
}

void window1_redraw(Window *w, Graphics *g)
{
	Rect r;

	r = get_window_area(w);
	which++;
	which %= 8;
	set_rgb(g, colours[which]);
	fill_rect(g, r);

	say(w, "redraw");
}

void window1_close(Window *w)
{
	say(w, "close");
	hide_window(w);
}

/*
 *  Second window's call-back functions:
 */

void window2_redraw(Window *w, Graphics *g)
{
	Rect r;

	r = get_window_area(w);
	set_rgb(g, rgb(240,240,240)); /* pale grey */
	fill_rect(g, r);

	set_rgb(g, rgb(0,0,128)); /* dark blue */
	fill_rect(g, rect(20,5,3,100));
}

void window2_mouse_down(Window *w, int buttons, Point p)
{
	Graphics *g;

	g = get_window_graphics(w);
	which++;
	which %= 8;
	set_colour(g, colours[which]);
	fill_ellipse(g, rect(p.x-which/2,p.y-which/2,which,which));
	del_graphics(g);

	/*printf("window  mouse_down (buttons=%d, xy=%d,%d)\n", buttons, p.x, p.y);*/
}

/*
 *  Controls 1 and 2 call-back functions:
 */
void controls_mouse_down(Control *c, int buttons, Point p)
{
	/*printf("control mouse_down (buttons=%d, xy=%d,%d)\n", buttons, p.x, p.y);*/
	pass_event(c);
}

void controls_redraw(Control *c, Graphics *g)
{
	Rect r;

	r = get_control_area(c);
	which++;
	which %= 8;
	set_rgb(g, colours[which]);
	fill_rect(g, r);
}

/*
 *  Control 3's redraw call-back function:
 */
void control3_redraw(Control *c, Graphics *g)
{
	Rect r;
	Colour col;

	r = get_control_area(c);
	set_rgb(g, rgb(0,0,0));
	draw_utf8(g, pt(4,4), "Control 3", 9);

	col = get_control_background(c);
	col.alpha = 255 - col.alpha;	/* change transparency */
	/*set_control_background(c, col);*/
}

void focus(Control *c)
{
	Control *other;

	other = get_control_data(c);
	set_focus(other);
}

void check_me(Control *c)
{
	Window *w;
	Graphics *g;

	w = parent_window(c);
	g = get_window_graphics(w);
	set_colour(g, colours[which]);
	draw_line(g, pt(1,1), pt(100,100));
	del_graphics(g);

	c = c->data;
	if (c)
		set_control_text(c, "");
}

void quit(Control *c)
{
	exit(0);
}

/*
 *  Program start-up:
 */

int main(int argc, char *argv[])
{
	App *app;
	Window *win1;
	Window *win2;
	Control *c1, *c2, *c3, *c4, *c5, *c6, *c7, *c8, *c9;
	Control *c10, *c11, *c12, *c13, *c14;

	app = new_app(argc, argv);

	/* create the first window */
	win1 = new_window(app, rect(50,100,200,160), "Tester 1",
		STANDARD_WINDOW);
	on_window_resize(win1, window1_resize);
	on_window_redraw(win1, window1_redraw);
	on_window_close(win1, window1_close);
	on_window_mouse_down(win1, window1_mouse_down);
	on_window_mouse_up(win1, window1_mouse_up);
	on_window_mouse_drag(win1, window1_mouse_drag);
	on_window_mouse_move(win1, window1_mouse_move);
	on_window_key_down(win1, window1_key_down);
	on_window_key_action(win1, window1_key_action);
	show_window(win1);

	/* create the second window */
	win2 = new_window(app, rect(150,200,330,300), "Tester 2",
		STANDARD_WINDOW);
	on_window_redraw(win2, window2_redraw);
	on_window_mouse_down(win2, window2_mouse_down);

	c1 = new_control(win2, rect(10,15,80,30));
	on_control_redraw(c1, controls_redraw);
	on_control_mouse_down(c1, controls_mouse_down);

	c2 = add_control(c1, rect(50,5,10,70));
	on_control_redraw(c2, controls_redraw);
	on_control_mouse_down(c2, controls_mouse_down);

	c3 = new_control(win2, rect(100,100,80,80));
	on_control_redraw(c3, control3_redraw);

	c4 = new_check_box(win2, rect(10,120,90,30), "Check me", check_me);

	c5 = new_radio_button(win2, rect(10,160,90,30), "Radio", NULL);
	c6 = new_radio_button(win2, rect(10,200,90,30), "Radio 2", NULL);

	new_radio_group(win2);

	c7 = new_radio_button(win2, rect(10,240,90,30), "Radio 3", NULL);

	c7 = new_radio_button(win2, rect(10,280,90,30), "Radio 4", NULL);

	c8 = new_button(win2, rect(110,10,80,30), "Quit", quit);
	set_control_background(c8, rgb(200,240,200));

	c9 = new_button(win2, rect(110,50,80,30), "Set Focus", focus);

	c10 = new_label(win2, rect(80,10,80,30), "Hi", ALIGN_LEFT);

	c11 = new_scroll_bar(win2, rect(200,10,16,100), 100, 24, NULL);
	c12 = new_scroll_bar(win2, rect(217,111,100,16), 10, 5, NULL);
	c13 = new_field(win2, rect(217,150,100,30), "Field");
	c14 = new_text_box(win2, rect(217,200,100,100), "Text box");

	set_control_data(c9, c11); /* which control to give focus */
	set_control_data(c4, c14); /* which control to clear */

	new_button(win2, rect(110,200,80,40), "New Window", create_new_window);
	new_button(win2, rect(110,250,80,40), "Destroy Window", destroy_new_window);

	show_window(win2);

	/* handle all events until the end */
	main_loop(app);

	del_app(app);
	return 0;
}

