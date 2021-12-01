/*
 *  Rainbow
 *  -------
 *  Display a rainbow of colours in a window. This program shows the
 *  use of colours, drawings, scrollbars and labels.
 */

#include <stdio.h>
#include <graphapp.h>

Window *w = NULL;	/* the main window */
Control *s = NULL;	/* scrollbar to control the intensity */
Control *t = NULL;	/* label to display the intensity */
Control *d = NULL;	/* canvas draw the rainbow */
Control *e = NULL;	/* example colour display */
Control *c[3];		/* colour axis checkboxes */
Control *v[3];		/* colour value display labels */
Control *h = NULL;	/* label to display colour as hex */

int cube_size = 8;	/* edge length of colour cube */
int axis = 2;           /* 0 = Red, 1 = Green, 2 = Blue axis */
int intensity = 255;    /* intensity along colour axis */

Colour bg = {0,255,255,255};	/* change using left mouse button */
Colour fg = {0,0,0,0};		/* change using right mouse button */

char *name[3] = { "Red", "Green", "Blue" };

/* use the colour axis value to determine which colour (a,b,c) represents */
Colour axis_colour(int a, int b, int c)
{
	Colour result;

	/* a /= (cube_size - 1); a *= (cube_size - 1); */
	/* b /= (cube_size - 1); b *= (cube_size - 1); */
	/* c /= (cube_size - 1); c *= (cube_size - 1); */

	/* the 'axis' variable determines which colour's intensity is */
	/* controlled by the 'intensity' variable, here given as 'a' */
	switch (axis) {
		case 0: result = rgb(a, b, c); break;
		case 1: result = rgb(c, a, b); break;
		case 2: result = rgb(b, c, a); break;
	}
	return result;
}

/* draw the rainbow on the drawing area */
void draw_rainbow(Control *d, Graphics *g)
{
	int x, y;
	int a, b, c;
	int size = 256/cube_size;

	/* step through one 'face' of the 'colour cube' */
	/* the 'a' colour is the chosen 'axis' colour */
	/* the 'b' colour increases as x increases across the drawing */
	/* the 'c' colour increases as y increases down the drawing */
	/* the axis_colour function decides what 'a', 'b' and 'c' mean */

	a = intensity;
	for (x=0; x < cube_size; x++) {
		b = x * 255/(cube_size-1);
		for (y=0; y < cube_size; y++) {
			c = y * 255/(cube_size-1);
			set_colour(g, axis_colour(a, b, c));
			fill_rect(g, rect(x*size, y*size, size, size));
		}
	}
}

/* draw sample colour display area */
void draw_sample(Control *e, Graphics *g)
{
	Rect r = get_control_area(e);

	set_colour(g, BLACK);
	draw_text(g, r, ALIGN_LEFT | VALIGN_CENTER, "Text", 4);
	set_colour(g, WHITE);
	draw_text(g, r, ALIGN_CENTER | VALIGN_CENTER, "Text", 4);
	set_colour(g, fg);
	draw_text(g, r, ALIGN_RIGHT | VALIGN_CENTER, "Text", 4);
}

/* handle a mouse movement over the window */
void clear_colour_display(Window *w, int buttons, Point p)
{
	/* clear the display of colour component values */
	/* red component */
	set_control_text(v[0], "");
	/* green component */
	set_control_text(v[1], "");
	/* blue component */
	set_control_text(v[2], "");
}

/* handle a mouse movement over the drawing area */
void set_colour_display(Control *d, int buttons, Point p)
{
	int a, b, c;
	Colour colour;
	char str[40];

	a = intensity;
	b = p.x;
	c = p.y;

	/* find out what colour the mouse is over */
	colour = axis_colour(a, b, c);

	/* determine and display the values of each colour component */

	/* red component */
	sprintf(str, "%d", colour.red);
	set_control_text(v[0], str);
	/* green component */
	sprintf(str, "%d", colour.green);
	set_control_text(v[1], str);
	/* blue component */
	sprintf(str, "%d", colour.blue);
	set_control_text(v[2], str);

	/* change the sample colour display */
	if (buttons & RIGHT_BUTTON) {
		fg = colour;
		draw_control(e);
	}
	if (buttons & LEFT_BUTTON) {
		bg = colour;
		set_control_background(e, colour);
	}
	if (buttons) {
		/* change the hex display of the colour */
		sprintf(str, "0x%02x%02x%02x 0x%02x%02x%02x",
			bg.red, bg.green, bg.blue,
			fg.red, fg.green, fg.blue);
		set_control_text(h, str);
	}
}

/* handle a scrollbar event which changes the intensity value */
void change_intensity(Control *s)
{
	int value;
	int new_intensity;
	char str[40];

	/* the top represents 100%, the bottom 0% intensity */
	value = get_control_value(s);
	new_intensity = 255 - value;
	/* if the new intensity is the same, we ignore the change */
	if (new_intensity == intensity)
		return;
	/* change the intensity of the axis colour */
	intensity = new_intensity;
	/* change the percentage displayed in the intensity label */
	sprintf(str, "%d%%", (int)((long)intensity)*100/255);
	set_control_text(t, str);
	/* draw the new rainbow */
	draw_control(d);
}

/* handle a scrollbar event which changes the colour cube size */
void change_cube_size(Control *s)
{
	int value;

	/* the right represents 6, the left 0 intensity */
	value = get_control_value(s);
	/* map 0...6 to 1...7 */
	value = value + 1;
	/* map 1...7 to 2,4,8,16,32,64,128,256 */
	value = 1<<value;
	/* if the new value is the same, we ignore the change */
	if (value == cube_size)
		return;
	/* change the size */
	cube_size = value;
	/* draw the new rainbow */
	draw_control(d);
}

/* handle a checkbox event which changes the colour axis */
void change_axis(Control *chk)
{
	/* uncheck the previously checked checkbox */
	if (chk != c[axis])
		uncheck(c[axis]);
	/* set new colour axis from 'value' stored in this checkbox */
	axis = get_control_value(chk);
	/* ensure the correct checkbox is checked */
	check(c[axis]);
	/* draw the new rainbow */
	draw_control(d);
}

/* the window's redraw function merely draws a box around the edge */
void draw_edge(Window *w, Graphics *g)
{
	set_colour(g, BLACK);
	draw_rect(g, rect(5,5,480,300));
}

/* starting point of the program */
int main(int argc, char *argv[])
{
	int i;
	App *app;

	/* start the application */
	app = new_app(argc, argv);

	/* create the window */
	w = new_window(app, rect(50,50,490,310), "Rainbow",
			TITLEBAR + CLOSEBOX + MINIMIZE);
	on_window_redraw(w, draw_edge);
	on_window_mouse_move(w, clear_colour_display);

	/* create the drawing area and set a mouse movement callback */
	d = new_control(w, rect(10,10,256,256));
	on_control_redraw(d, draw_rainbow);
	on_control_mouse_move(d, set_colour_display);
	on_control_mouse_down(d, set_colour_display);
	on_control_mouse_drag(d, set_colour_display);

	/* create the scrollbar which controls colour intensity */
	s = new_scroll_bar(w, rect(280,10,20,256), 255, 15,
		change_intensity);

	/* create the scrollbar which controls cube size */
	s = new_scroll_bar(w, rect(10, 280, 256, 20), 6, 1,
		change_cube_size);
	change_scroll_bar(s, 2, 6, 1);

	/* create the colour axis checkboxes */
	new_label(w, rect(320,10,100,20), "Colour Axis", ALIGN_LEFT);
	for (i=0; i < 3; i++) {
		c[i] = new_check_box(w, rect(330,35+i*25,90,20),
				name[i], change_axis);
		set_control_value(c[i], i);
		v[i] = new_label(w, rect(430,35+i*25,40,20),
				"0", ALIGN_RIGHT);
		set_control_background(v[i], WHITE);
	}
	check(c[axis]);

	/* create the intensity display */
	new_label(w, rect(320,120,150,20), "Intensity", ALIGN_LEFT);
	t = new_label(w, rect(330,145,140,20), "100%", ALIGN_LEFT);
	set_control_background(t, WHITE);

	/* create the hex display */
	new_label(w, rect(320,175,150,20), "Background    Text",
			ALIGN_LEFT);
	h = new_label(w, rect(330,200,140,20), "0xffffff 0x000000",
			ALIGN_LEFT);
	set_control_background(h, WHITE);

	/* create the sample label */
	new_label(w, rect(320,230,150,20), "Sample", ALIGN_LEFT);
	e = new_control(w, rect(330,255,140,20));
	on_control_redraw(e, draw_sample);

	/* show the window */
	show_window(w);
	main_loop(app);
	del_app(app);
	return 0;
}

