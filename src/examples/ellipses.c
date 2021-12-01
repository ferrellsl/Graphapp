/*
 *  Ellipses
 *  --------
 *
 *  A program to test the drawing of ellipses.
 *  It draws several ellipses of several sizes and
 *  line thicknesses across the window.
 */

#include <stdio.h>
#include <graphapp.h>

void draw_ellipses(Window *w, Graphics *g)
{
	int h, t, i;
	Rect r;

	r = rect(1,1,1,1);

	for (h=1; h <= 25; h++)	/* try several ellipse heights */
	{
		r.height = h;

		for (t=1; t <= h/2+1; t++)	/* line widths */
		{
			set_line_width(g, t);

			for (i=1; i <= 25; i++) { /* ellipse widths */
				r.width = i;
				draw_ellipse(g, r);
				r.x += r.width + 5;
			}

			r.y += r.height + 2; /* move down one line */
			r.x = 1;
		}

		r.y += 1; /* small space down to next ellipse height*/
	}
}

int main(int argc, char *argv[])
{
	App *app;
	Window *w;

	app = new_app(argc, argv);
	w = new_window(app, rect(10,10,448,400), "Ellipses Test",
			STANDARD_WINDOW);
	on_window_redraw(w, draw_ellipses);
	show_window(w);
	main_loop(app);
	del_app(app);
	return 0;
}
