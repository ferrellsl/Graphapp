/*
 *  Edit Draw
 *  ---------
 *  A GraphApp program for editing simple drawings.
 *  This program written by Loki.
 */

#include <stdio.h>
#include <stdlib.h>
#include <graphapp.h>

/*
 *  Define some data structures.
 */
struct Shape {
	int kind;	/* 1=line, 2=rectangle, 3=ellipse, etc */
	int width;	/* line thickness, default is 1 */
	int colour;	/* index into colour_list (see below) */
	Point p1;	/* start point */
	Point p2;	/* end point */
};

typedef struct Shape Shape;

/*
 *  A list of shapes, implemented as an array.
 */

enum {
	MAX_SHAPES = 100
};
int	num_shapes = 0;
Shape	shape_list[MAX_SHAPES];

/*
 *  A global App object to connect to the windowing system.
 */
App *app;

/*
 *  A global Shape used in the mouse dragging functions.
 */
Shape	shape;

/*
 *  Some lists of shape-kinds and colours, with associated string names.
 */

char * shape_names[] = {
	"Line", 	"Rectangle",	"Box",		"Round Rect",
	"Round Box",	"Ellipse",	"Oval",		"Arc",
	"Pie"
};

enum shape_types {
	LINE,   	RECTANGLE,	BOX,		ROUNDRECT,
	ROUNDBOX,	ELLIPSE,	OVAL,		ARC,
	PIE
};

char * colour_names[] = {
	"Black", 	"Red",  	"Green",	"Blue",
	"Grey", 	"Magenta",	"Yellow",	"Cyan",
	"Dark Grey",	"Dark Red",	"Dark Green",	"Dark Blue"
};

Colour colour_list[] = {
	{0,0,0,0},  	{0,255,0,0},   	{0,0,255,0},  	{0,0,0,255},
	{0,192,192,192},{0,255,0,255},	{0,255,255,0}, 	{0,0,255,255},
	{0,128,128,128},{0,128,0,0},	{0,0,128,0},	{0,0,0,128}
};

#define NUM_SHAPE_TYPES	(sizeof(shape_names)/sizeof(shape_names[0]))
#define NUM_COLOURS	(sizeof(colour_names)/sizeof(colour_names[0]))
#define NUM_DELAYS      2
#define NUM_MODES       2
#define NUM_WIDTHS      10

/*
 *  Some menuitems.
 */

MenuItem * shape_item[NUM_SHAPE_TYPES];
MenuItem * colour_item[NUM_COLOURS];
MenuItem * width_item[NUM_WIDTHS];
MenuItem * delay_item[NUM_DELAYS];
MenuItem * mode_item[NUM_MODES];

/*
 *  Other global variables.
 */

int time_delay = 0;
int xor_mode   = 1;

/*
 *  The main drawing canvas.
 */

Control *canvas;

/*
 *  Help text.
 */

char * help_text [3] = {
	"The program EditDraw was written by Loki, for the "
	"Department of Computer Science, University of Sydney, Australia."
	"\n\n"
	"It is a demonstration of some principles involved with "
	"creating a drawing package.\n",

	"The delay option inserts a half-second delay between "
	"erasing the currently selected object and drawing it again."
	"\n\n"
	"Notice "
	"that the object is totally erased when dragging the mouse using "
	"the XOR drawing mode, but the Paint mode gets quite messy.",

	"\n\n"
	"By using a drawing mode which "
	"preserves information, we can get the picture back at any time "
	"by simply drawing the same object again in the same mode. "
	"The XOR mode preserves information, because some part of "
	"the destination pixels are preserved in the resulting picture.",
};

/*
 *  Maintain a list of shapes.
 */
void add_shape(Shape s)
{
	if (num_shapes < MAX_SHAPES) {
		shape_list[num_shapes] = s;
		num_shapes = num_shapes + 1;
	}
}

void del_shape(int index)
{
	while (index < num_shapes - 1) {
		shape_list[index] = shape_list[index+1];
		index = index + 1;
	}
	num_shapes = num_shapes - 1;
}

/*
 *  Functions for deciding if a point is inside a shape.
 */
int inside_line(Point p, Point p1, Point p2)
{
	/* this should use Bresenham's algorithm */
	/* for now we approximate by checking if we near the ends */
	Rect r;

	r.x = p1.x-2;
	r.y = p1.y-2;
	r.width = 4;
	r.height = 4;
	if (point_in_rect(p, r))
		return 1;

	r.x = p2.x-2;
	r.y = p2.y-2;
	if (point_in_rect(p, r))
		return 1;

	return 0;
}

int inside_shape(Point p, Shape s)
{
	Rect r;

	r = rect(s.p1.x, s.p1.y, s.p2.x - s.p1.x, s.p2.y - s.p1.y);
	r = rect_abs(r);

	if (s.kind == LINE) {
		return inside_line(p, s.p1, s.p2);
	}
	else if (s.kind == RECTANGLE) {
		return point_in_rect(p,r);
	}
	else if (s.kind == BOX) {
		return (point_in_rect(p,r) &&
			(! point_in_rect(p,inset_rect(r,2))));
	}
	else if (s.kind == ROUNDRECT) {
		return point_in_rect(p,r);
	}
	else if (s.kind == ROUNDBOX) {
		return point_in_rect(p,r);
	}
	else if (s.kind == ELLIPSE) {
		return point_in_rect(p,r);
		/* this is a poor guess since ellipses are round */
	}
	else if (s.kind == OVAL) {
		return (point_in_rect(p,r) &&
			(! point_in_rect(p,inset_rect(r,2))));
		/* this is a poor guess since ellipses are round */
	}
	else if (s.kind == ARC) {
		return (point_in_rect(p,r) &&
			(! point_in_rect(p,inset_rect(r,2))));
		/* this is a poor guess since ellipses are round */
	}
	else if (s.kind == PIE) {
		return (point_in_rect(p,r) &&
			(! point_in_rect(p,inset_rect(r,2))));
		/* this is a poor guess since ellipses are round */
	}
	return 0;
}

/*
 *  Given a point where the user has clicked, find the nearby shape.
 *  Search from the end of the list backwards, to find the frontmost shape.
 */
int find_shape(Point p)
{
	int i;

	for (i = num_shapes - 1; i >= 0; i=i-1)
	{
		if (inside_shape(p, shape_list[i])) {
			return i;
		}
	}
	return -1; /* not found */
}

/*
 *  Draw a single shape.
 */
void draw_shape(Graphics *g, Shape s)
{
	Rect r;

	r = rect(s.p1.x, s.p1.y, s.p2.x - s.p1.x, s.p2.y - s.p1.y);
	r = rect_abs(r);

	set_colour(g, colour_list[s.colour]);
	set_line_width(g, s.width);

	if (s.kind == LINE) {
		draw_line(g, s.p1, s.p2);
	}
	else if (s.kind == RECTANGLE) {
		fill_rect(g, r);
	}
	else if (s.kind == BOX) {
		draw_rect(g, r);
	}
	else if (s.kind == ROUNDRECT) {
		draw_round_rect(g, r);
	}
	else if (s.kind == ROUNDBOX) {
		fill_round_rect(g, r);
	}
	else if (s.kind == ELLIPSE) {
		fill_ellipse(g, r);
	}
	else if (s.kind == OVAL) {
		draw_ellipse(g, r);
	}
	else if (s.kind == ARC) {
		draw_arc(g, r, 45, -45);
	}
	else if (s.kind == PIE) {
		fill_arc(g, r, 45, -45);
	}
}

/*
 *  Draw the entire list of shapes.
 */
void draw_all_shapes(Graphics *g)
{
	int i;

	set_paint_mode(g);

	for (i=0; i < num_shapes; i=i+1) {
		draw_shape(g, shape_list[i]);
	}
}

void draw_canvas(Control *c, Graphics *g)
{
	Rect r = get_control_area(c);

	draw_all_shapes(g);

	set_colour(g, rgb(0,0,0));
	set_line_width(g, 1);
	draw_rect(g, r);
}

void resize_canvas(Window *w)
{
	Graphics *g;
	Rect r = get_window_area(w);

	r.y += 24;
	r.height -= 24;
	r = inset_rect(r, 10);
	set_control_area(canvas, r);
	g = get_control_graphics(canvas);
	draw_canvas(canvas, g);
	del_graphics(g);
}

/*
 *  Create a shape by dragging the mouse.
 */
void start_drag(Control *c, int buttons, Point p)
{
	Graphics *g = get_control_graphics(c);
	Colour bg = get_control_background(c);

	shape.p1 = p;			/* set start-point of the shape */
	shape.p2 = p;			/* set end-point to be the same */
	if (xor_mode)
		set_xor_mode(g, bg);/* set drawing mode to XOR */
	draw_shape(g, shape);		/* draw new shape */

	del_graphics(g);
}

void start_window_drag(Window *w, int buttons, Point p)
{
	Control *c = get_window_data(w);
	/* convert to canvas co-ordinates */
	p.x -= c->area.x;
	p.y -= c->area.y;
	start_drag(c, buttons, p);
}

void end_drag(Control *c, int buttons, Point p)
{
	Graphics *g = get_control_graphics(c);

	shape.p2 = p;			/* set end-point of this shape */
	set_paint_mode(g);		/* set drawing mode to normal */
	draw_shape(g, shape);		/* draw new shape */
	add_shape(shape);		/* add the shape to the list */

	del_graphics(g);
}

void end_window_drag(Window *w, int buttons, Point p)
{
	Control *c = get_window_data(w);
	/* convert to canvas co-ordinates */
	p.x -= c->area.x;
	p.y -= c->area.y;
	end_drag(c, buttons, p);
}

void drag_shape(Control *c, int buttons, Point p)
{
	Graphics *g = get_control_graphics(c);
	Colour bg = get_control_background(c);

	if (xor_mode)
		set_xor_mode(g, bg);/* set drawing mode to XOR */
	delay(app, time_delay);	/* to see what's happening */
	draw_all(app);
	draw_shape(g, shape);		/* cancel old shape */
	draw_all(app);		/* make changes visible now */
	delay(app, time_delay);	/* to see what's happening */
	draw_all(app);
	shape.p2 = p;			/* move end-point of this shape */
	draw_shape(g, shape);		/* draw new shape */

	del_graphics(g);
}

void window_drag_shape(Window *w, int buttons, Point p)
{
	Control *c = get_window_data(w);
	/* convert to canvas co-ordinates */
	p.x -= c->area.x;
	p.y -= c->area.y;
	drag_shape(c, buttons, p);
}

/*
 *  Menu call-back functions below.
 */

void select_shape(MenuItem *me)
{
	uncheck_menu_item(shape_item[shape.kind]);
	shape.kind = me->value;
	check_menu_item(me);
}

void select_colour(MenuItem *me)
{
	uncheck_menu_item(colour_item[shape.colour]);
	shape.colour = me->value;
	check_menu_item(me);
}

void select_width(MenuItem *me)
{
	uncheck_menu_item(width_item[shape.width]);
	shape.width = me->value;
	check_menu_item(me);
}

void select_delay(MenuItem *me)
{
	int i;

	time_delay = me->value;

	for (i=0; i < NUM_DELAYS; i++) {
		uncheck_menu_item(delay_item[i]);
		if (me == delay_item[i])
			check_menu_item(me);
	}
}

void select_mode(MenuItem *me)
{
	int i;

	xor_mode = me->value;

	for (i=0; i < NUM_MODES; i++) {
		uncheck_menu_item(mode_item[i]);
		if (me == mode_item[i])
			check_menu_item(me);
	}
}

void about_this_program(MenuItem *me)
{
	ask_ok(app, me->text, help_text[0]);
}

void help_on_delays(MenuItem *me)
{
	ask_ok(app, me->text, help_text[1]);
}

void help_on_modes(MenuItem *me)
{
	ask_ok(app, me->text, help_text[2]);
}

void quit(MenuItem *me)
{
	exit(0);
}

/*
 *  Initialise the global variable 'shape'.
 */
void init_shape(void)
{
	shape.kind	= LINE;
	shape.width	= 3;
	shape.colour	= 1;
	shape.p1	= pt(0,0);
	shape.p2	= pt(0,0);
}

/*
 *  The main function.
 */
int main(int argc, char *argv[])
{
	Window *w;
	MenuBar *mb;
	Menu *m;
	MenuItem *mi;
	int i;
	char name[32];

	app = new_app(argc, argv);
	w = new_window(app, rect(0,0,500,500), "Edit Draw",
		STANDARD_WINDOW);
	on_window_resize(w, resize_canvas);

	init_shape();

	mb = new_menu_bar(w);
	m = new_menu(mb, "File");
	mi = new_menu_item(m, "Quit", 'Q', quit);

	m = new_menu(mb, "Shape");
	for (i=0; i < NUM_SHAPE_TYPES; i=i+1) {
		shape_item[i] = new_menu_item(m, shape_names[i],
					0, select_shape);
		shape_item[i]->value = i;
	}
	select_shape(shape_item[0]);

	m = new_menu(mb, "Colour");
	for (i=0; i < NUM_COLOURS; i=i+1) {
	    colour_item[i] = new_menu_item(m, colour_names[i],
					0, select_colour);
	    colour_item[i]->value = i;
	}
	select_colour(colour_item[1]);

	m = new_menu(mb, "Width");
	for (i=0; i < NUM_WIDTHS; i=i+1) {
		sprintf(name, "%d", i);
		width_item[i] = new_menu_item(m, name, 0, select_width);
		width_item[i]->value = i;
	}
	select_width(width_item[1]);

	m = new_menu(mb, "Options");
	delay_item[0] = new_menu_item(m, "No delay", 0, select_delay);
	delay_item[1] = new_menu_item(m, "Use delay", 0, select_delay);
	delay_item[1]->value = 50;
	select_delay(delay_item[0]);

	mi = new_menu_item(m, "-", 0, NULL);

	mode_item[0] = new_menu_item(m, "Use XOR mode",  0, select_mode);
	mode_item[1] = new_menu_item(m, "Use paint mode", 0, select_mode);
	mode_item[0]->value = 1;
	mode_item[1]->value = 0;
	select_mode(mode_item[0]);

	m = new_menu(mb, "Help");
	mi = new_menu_item(m, "About EditDraw", 0, about_this_program);
	mi = new_menu_item(m, "-", 0, NULL);
	mi = new_menu_item(m, "Use of Delays", 0, help_on_delays);
	mi = new_menu_item(m, "Drawing Modes", 0, help_on_modes);

	canvas = new_control(w, rect(10,30,480,480));
	on_control_redraw(canvas, draw_canvas);
	set_control_background(canvas, rgb(240,240,240));
	on_control_mouse_down(canvas, start_drag);
	on_control_mouse_up(canvas, end_drag);
	on_control_mouse_drag(canvas, drag_shape);

	set_window_data(w, canvas);
	on_window_mouse_down(w, start_window_drag);
	on_window_mouse_up(w, end_window_drag);
	on_window_mouse_drag(w, window_drag_shape);

	show_window(w);

	main_loop(app);
	del_app(app);
	return 0;
}
