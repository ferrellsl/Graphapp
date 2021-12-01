/*
 *  Smiley
 *  ------
 *  This program draws a smiley inside a window. It uses a drawing
 *  area with a redraw call-back function to ensure the smile is
 *  always drawn as needed.
 *
 *  Note there is a problem with this program: if you resize the
 *  window to be too small, it draws the smile incorrectly. Why?
 *  The answer is in the drawarc function drawing the smile.
 *  If the face rectangle r1 gets too small, r2 (which is r1 inset
 *  by 30 pixels) gets a tiny or negative width and/or height.
 */

#include <stdio.h>
#include <graphapp.h>

void draw_smile(Control *c, Graphics *g)
{
	Rect r = get_control_area(c);
	Rect r1 = inset_rect(r, 10);	/* inset r1 from edge */
	Rect r2;
	Point p;

	set_colour(g, LIGHT_BLUE);		/* blue face */
	fill_ellipse(g, r1);		/* draw face */

	set_colour(g, RED);			/* red lips */
	set_line_width(g, 2);		/* thicker lips */
	r2 = inset_rect(r1, 30);		/* inset from face rectangle */
	draw_arc(g, r2, 270-60, 270+60);	/* smile between arc angles */

	set_colour(g, BROWN);		/* brown eyes */
	p.x = r1.x + r1.width  * 1/2;
	p.y = r1.y + r1.height * 1/4;

	r2 = rect(p.x - 30, p.y, 20,20);
	fill_ellipse(g, r2);		/* draw left eye */

	r2 = rect(p.x + 10, p.y, 20,20);
	fill_ellipse(g, r2);		/* draw right eye */
}

void resize_smile(Window *w)
{
	/* retrieve remembered drawing */
	Control *c = get_window_data(w);
	size_control(c, get_window_area(w));
}

int main(int argc, char *argv[])
{
	App *app;
	Window *w;
	Control *c;

	app = new_app(argc, argv);
	w = new_window(app, rect(50,50,120,120), "Smile!",
			STANDARD_WINDOW);
	c = new_control(w, rect(0,0,120,120));
	on_control_redraw(c, draw_smile);

	set_window_data(w, c); /* store the pointer with the window */
	on_window_resize(w, resize_smile);
	show_window(w);
	main_loop(app);
	del_app(app);
	return 0;
}
