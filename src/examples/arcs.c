/*
 *  Arcs
 *  ----
 *
 *  A program to test the drawing of arcs.
 *  It draws several elliptical arcs of several sizes and
 *  line thicknesses across the window.
 *  Use the left and right mouse buttons to change the
 *  start and end angles of the arcs, respectively.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <graphapp.h>

int start_angle = 45;
int end_angle = 270;
int changed = 0;

#define PI (3.14159265359)
#define RADIANS_TO_DEGREES(rads) ((rads)*180/PI)

void draw_arcs(Window *w, Graphics *g)
{
	int h, t, i;
	Rect r;
	char buffer[120];

	r = get_window_area(w);
	set_colour(g, WHITE);
	fill_rect(g, r);
	set_colour(g, BLACK);

	sprintf(buffer, "start_angle = %d, end_angle = %d",
		start_angle, end_angle);
	draw_utf8(g, pt(3,3), buffer, strlen(buffer));

	r = rect(1,25,1,1);

	for (h=1; h <= 26; h += 5)	/* try several heights */
	{
		r.height = h;

		for (t=1; t <= h/2+1; t += 3)	/* line widths */
		{
			set_line_width(g, t);

			for (i=1; i <= 25; i += 1) { /* widths */
				r.width = i;
				draw_arc(g, r, start_angle, end_angle);
				r.x += r.width + 5;
			}

			r.y += r.height + 2; /* move down one line */
			r.x = 1;
		}

		r.y += 1; /* small space down to next ellipse height*/
	}
}

void handle_mouse(Window *w, int buttons, Point p)
{
	Rect r = get_window_area(w);
	int rise, run, angle;

	run  = p.x - (r.x+r.width/2);
	rise = p.y - (r.y+r.height/2);

	if (run == 0) {
		if (rise < 0)
			angle = 90;
		else
			angle = 270;
	}
	else
		angle = -RADIANS_TO_DEGREES(atan(rise/(run+0.0)));
	if (run < 0)
		angle += 180;
	if (angle < 0)
		angle += 360;

	/* set the start and end angles of the arcs from the mouse */
	if (buttons & LEFT_BUTTON) {
		/* set start angle using angle of mouse from centre */
		start_angle = angle;
		changed = 1;
	}
	if (buttons & RIGHT_BUTTON) {
		/* set end angle using angle of mouse from centre */
		end_angle = angle;
		changed = 1;
	}
	if (changed) {
		changed = 0;
		draw_window(w);
	}
}

int main(int argc, char *argv[])
{
	App *app;
	Window *w;

	app = new_app(argc, argv);
	w = new_window(app, rect(10,10,448,400), "Arcs Test",
			STANDARD_WINDOW);
	on_window_redraw(w, draw_arcs);
	on_window_mouse_down(w, handle_mouse);
	on_window_mouse_drag(w, handle_mouse);
	on_window_mouse_up(w, handle_mouse);
	show_window(w);
	main_loop(app);
	del_app(app);
	return 0;
}
