/*
 *  Moire
 *  -----
 *  This version copyright (c) 1996-2003 by Lachlan Patrick.
 *  Written using GraphApp.
 *
 *  This is a complex example not for beginners. It makes use of
 *  timer functions, colour, windows and keyboard call-backs.
 *  It shows Moire patterns, which is when the algorithm used
 *  to draw lines creates interesting patterns.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <graphapp.h>

/*
 *  Global variables.
 */
int width = 400;
int height = 240;
int banner_height = 40;
char *banner = "Press a number to change the step size, Q to stop, R,G,B,C,M,Y,K for colours.";

Font *font;

int step_size = 3;
int step_location = 0;
Point origin;

Colour colour = {0,0,0,255}; /* BLUE */

/*
 *  Functions.
 */
void change_location(void)
{
	time_t t;

	srand((unsigned) time(&t));

	origin.x = rand() % width;
	origin.y = rand() % height;

	step_location = 0;
}

void handle_mouse(Window *w, int buttons, Point xy)
{
	origin = xy;
	step_location = 0;
	redraw_window(w);
}

void handle_keydown(Window *w, unsigned long key)
{
	if (isdigit(key))
		step_size = key - '0';
	else if ((key == 'q') || (key == 'Q') || (key == ESC))
		exit(0);
	else switch (key) {
		case 'r': case 'R': colour = RED; break;
		case 'g': case 'G': colour = GREEN; break;
		case 'c': case 'C': colour = CYAN; break;
		case 'm': case 'M': colour = MAGENTA; break;
		case 'y': case 'Y': colour = YELLOW; break;
		case 'k': case 'K': colour = BLACK; break;
		case 'b': case 'B':
			if (colours_equal(colour, BLUE))
				colour = BLACK;
			else
				colour = BLUE;
			break;
		default: break;
	}
}

void paint_window(Window *w, Graphics *g)
{
	Rect r = get_window_area(w);

	if (banner_height > 0)
	{
		r.y = height;
		r.height = banner_height;

		set_colour(g, BLACK);
		draw_text(g, r, ALIGN_CENTER | VALIGN_CENTER,
				banner, strlen(banner));
	}
}

void shape_window(Window *w)
{
	Rect r = get_window_area(w);

	banner_height = font_height(font)*2+8;

	if (r.height < 4*banner_height) /* too small for banner */
		banner_height = 0;

	height = r.height - banner_height;
	width = r.width;

	change_location();
}

/* create and display the window */
Window *init_window(App *app)
{
	Window *w;

	banner_height = font_height(font)*2+8;

	w = new_window(app, rect(0,0,width,height+banner_height),
			"Moire", STANDARD_WINDOW | CENTERED );
	on_window_mouse_up(w, handle_mouse);
	on_window_key_down(w, handle_keydown);
	on_window_redraw(w, paint_window);
	on_window_resize(w, shape_window);
	show_window(w);
	return w;
}

void draw_and_step(Timer *t)
{
	Window *w = t->data;
	Graphics *g;

	g = get_window_graphics(w);
	set_colour(g, colour);

	if (step_location <= height) {
		draw_line(g, origin, pt(0, height-step_location));
		draw_line(g, origin, pt(width, step_location));
	}
	if (step_location <= width) {
		draw_line(g, origin, pt(step_location, 0));
		draw_line(g, origin, pt(width-step_location, height));
	}
	del_graphics(g);

	step_location += step_size;

	if ((step_location > width+100)
		&& (step_location > height+100))
	{
		change_location();
		redraw_window(w);
	}
}

int main(int argc, char *argv[])
{
	App *app;
	Window *win;
	Timer *t;

	app = new_app(argc, argv);
	font = new_font(app, "unifont", PLAIN, 16);
	win = init_window(app);
	t = new_timer(app, draw_and_step, 80); /* 80 millisecond timer */
	t->data = win;
	main_loop(app);
	return 0;
}

