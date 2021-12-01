/*
 *  Fast Lines
 *  ----------
 *
 *  How fast is line drawing using server-side
 *  lines, compared with the portable
 *  line-drawing algorithm which uses rectangles?
 *
 *  This program draws many random lines and
 *  times how long it takes to complete.
 *
 *  On my 800MHz Pentium, lines 1 pixel thick
 *  show the greatest speed improvment. Long lines
 *  with lots of clipping can take only 10% the time
 *  of portable lines drawn using rectangles.
 *  Lines with less clipping use about 30% the time
 *  when drawn on the server side.
 *  Lines thicker than 1 pixel are about the same
 *  speed whichever method is used to draw them.
 *  Short lines seem to be faster using portable
 *  rectangle code, rather than server-side lines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <graphapp.h>

App *app;
Window *w;
unsigned int width = 1;
unsigned int length = 800;
long iterations = 1000;

Colour colours[] = {
	{0,0,0,0},
	{0,255,0,0},
	{0,0,255,0},
	{0,0,0,255},
	{0,255,0,255},
	{0,255,255,0},
	{0,0,255,255},
	{0,128,128,128}
};

#define NUM_COLOURS (sizeof(colours)/sizeof(colours[0]))

void go_now(Control *c)
{
	long t1, t2;
	Point p1, p2;
	Graphics *g;
	long i, msec1, msec2;
	App *app;
	char buf[160];

	app = w->app;
	g = get_window_graphics(w);
	set_line_width(g, width);

	/* First test. */

	t1 = current_time(app);
	srand(t1);
	for (i=0; i < iterations; i++) {
		if (i % 128 == 0)
			set_colour(g, colours[(i/100)%NUM_COLOURS]);
		p1.x = (rand() % 1024) - 20;
		p1.y = (rand() % 1024) - 20;
		p2.x = p1.x + (rand() % length) - (length/2);
		p2.y = p1.y + (rand() % length) - (length/2);
		draw_line(g, p1, p2);
	}
	t2 = current_time(app);
	msec1 = t2 - t1;
	sprintf(buf, "It took %ld milliseconds to draw the fast lines.", msec1);
	ask_ok(app, "Results", buf);

	/* Clear the window. */

	set_colour(g, WHITE);
	fill_rect(g, get_window_area(w));
	set_colour(g, BLACK);

	/* Second test. */

	t1 = current_time(app);
	srand(t1);
	for (i=0; i < iterations; i++) {
		if (i % 128 == 0)
			set_colour(g, colours[(i/100)%NUM_COLOURS]);
		p1.x = (rand() % 1024) - 20;
		p1.y = (rand() % 1024) - 20;
		p2.x = p1.x + (rand() % length) - (length/2);
		p2.y = p1.y + (rand() % length) - (length/2);
		portable_draw_line(g, p1, p2);
	}
	t2 = current_time(app);
	msec2 = t2 - t1;
	sprintf(buf, "It took %ld milliseconds to draw the portable lines.", msec2);
	ask_ok(app, "Results", buf);

	if (msec1 == msec2)
		sprintf(buf, "There was no measurable difference in speed.");
	else if (msec1 > msec2)
		sprintf(buf, "Portable lines took %ld%% as much time.",
			100*msec2/msec1);
	else
		sprintf(buf, "Fast lines took %ld%% as much time.",
			100*msec1/msec2);
	ask_ok(app, "Results", buf);

	del_graphics(g);
}

void clear_win(Control *c)
{
	Graphics *g;

	g = get_window_graphics(w);
	set_colour(g, WHITE);
	fill_rect(g, get_window_area(w));
	del_graphics(g);
}

void set_num(Control *c)
{
	char *num;
	char buf[80];

	sprintf(buf, "%ld", iterations);
	num = ask_string(app, "Question",
		"How many lines should the program draw?", buf);
	if (num == NULL)
		return;
	iterations = atol(num);
	free(num);
}

void set_width(Control *c) /* set line thickness */
{
	char *num;
	char buf[80];

	sprintf(buf, "%d", width);
	num = ask_string(app, "Question",
		"Draw lines of what width?", buf);
	if (num == NULL)
		return;
	width = atoi(num);
	free(num);
}

void set_len(Control *c) /* set line length */
{
	char *num;
	char buf[80];

	sprintf(buf, "%d", length);
	num = ask_string(app, "Question",
		"Draw lines of what approximate length?", buf);
	if (num == NULL)
		return;
	length = atoi(num);
	if (length == 0)
		length = 1;
	free(num);
}

void quit_now(Control *c)
{
	exit(0);
}

int main(int argc, char *argv[])
{
	Control *btn[6];

	app = new_app(argc, argv);
	w = new_window(app, rect(10,10,448,400),
			"Line Drawing Speed", STANDARD_WINDOW);
	btn[0] = new_button(w, rect(10,10,90,25), "Exit", quit_now);
	btn[1] = new_button(w, rect(10,40,90,25), "Set Number", set_num);
	btn[2] = new_button(w, rect(10,70,90,25), "Set Width", set_width);
	btn[3] = new_button(w, rect(10,100,90,25), "Set Length", set_len);
	btn[4] = new_button(w, rect(10,130,90,25), "Clear", clear_win);
	btn[5] = new_button(w, rect(10,160,90,25), "Go", go_now);
	show_window(w);
	main_loop(app);
	del_app(app);
	return 0;
}
