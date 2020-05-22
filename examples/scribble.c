/*
 *  Scribble
 *  --------
 *  This program lets the user scribble on a window
 *  with the mouse. It was adapted from the applet
 *  found on pages 10-12 of "Java in a Nutshell",
 *  2nd edition, by David Flanagan, as a comparison.
 */

#include <stdio.h>
#include <graphapp.h>

/** Scribble application variables: **/

  Point last_position;      /* Store the last mouse position. */
  Colour current_colour;    /* Store the current colour. */
  Control *clear_button;    /* Button to clear the window. */
  Control *colour_label;    /* Label saying "Colour:" */
  Control *colour_choices;  /* The colour dropdown list. */

  char * colour_names[] = {
	"Black", "Red", "Yellow", "Green", NULL
  };
  Colour colours[] = {
	{0,0,0,0}, {0,255,0,0}, {0,255,255,0}, {0,0,255,0}
  };

/** This function is called when the user clicks the mouse
 ** to start scribbling. **/
void mouseDown(Window *w, int buttons, Point p)
{
	last_position = p;
}

/** This function is called when the user drags the mouse. **/
void mouseDrag(Window *w, int buttons, Point p)
{
	Graphics *g = get_window_graphics(w);
	set_colour(g, current_colour);
	draw_line(g, last_position, p);
	last_position = p;
	del_graphics(g);
}

/** This function is called when the user clicks on the
 ** clear button to clear the window. **/
void selectClear(Control *b)
{
	Window *w = parent_window(b);
	Graphics *g = get_window_graphics(w);
	set_colour(g, WHITE);
	fill_rect(g, get_window_area(w));
	del_graphics(g);
}

/** This function is called when the user selects a
 ** colour from the dropdown list. **/
void selectColour(Control *c)
{
	int which = get_control_value(c);
	current_colour = colours[which];
}

/** This function is called to initialise the Scribble
 ** application, from the main function. **/
void init(App *app)
{
	/* Create the application window. */
	Window *w = new_window(app, rect(50,50,480,400), "Scribble",
				STANDARD_WINDOW);
	on_window_mouse_down(w, mouseDown);
	on_window_mouse_drag(w, mouseDrag);

	/* Create a button and add it to the window. */
	clear_button = new_button(w, rect(100,10,80,24), "Clear",
				selectClear);

	/* Create a dropdown list of colours and add it */
	/* to the window, with a label saying what it is. */
	colour_label = new_label(w, rect(200,10,80,24),
				"Colour: ", ALIGN_RIGHT | VALIGN_CENTER);
	set_control_background(colour_label, WHITE);

	colour_choices = new_drop_list(w, rect(300,10,80,24),
				colour_names, selectColour);
	show_window(w);
}

/** The main function is the starting point for the
 ** application, and it is here that everything is
 ** initialised. **/
int main(int argc, char *argv[])
{
	App *app = new_app(argc, argv);
	init(app);
	main_loop(app);
	del_app(app);
	return 0;
}
