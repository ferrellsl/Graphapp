/*
 *  Tab Pane
 *  --------
 *
 *  A program to demonstrate tabbed panes, which allow
 *  multiple controls to be grouped by function so that
 *  unused controls are hidden away.
 *
 *  In a real program, we wouldn't use buttons, but would
 *  instead use special tab-buttons which have no line
 *  at the bottom. This would make the buttons appear
 *  to merge with the appropriate pane.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <graphapp.h>

Control *tab[3];	/* tab buttons */
Control *pane[3];	/* panes, which contain controls */

void select_pane(Control *c)
{
	int which = get_control_value(c);

	bring_control_to_front(pane[which]);
	bring_control_to_front(tab[which]);

	redraw_control(pane[which]);
	redraw_control(tab[which]);
}

void draw_pane_border(Control *c, Graphics *g)
{
	Rect r = get_control_area(c);

	draw_shadow_rect(g, r, DARK_GREY, LIGHT_GREY);
	r = inset_rect(r, 1);
	draw_shadow_rect(g, r, WHITE, WHITE);
}

void quit(Control *c)
{
	exit(0);
}

int main(int argc, char *argv[])
{
	App *app;
	Window *w;
	Control *c;
	Rect pane_rect;

	app = new_app(argc, argv);
	w = new_window(app, rect(10,10,400,200), "Tabbed Pane Test",
			STANDARD_WINDOW);
	set_window_background(w, PALE_GREY);

	/* Create several 'panes', each in the same spot on the window. */

	pane_rect = rect(10,40,380,150);

	pane[2] = new_control(w, pane_rect);
	pane[1] = new_control(w, pane_rect);
	pane[0] = new_control(w, pane_rect);

	set_control_background(pane[2], LIGHT_RED);
	set_control_background(pane[1], LIGHT_GREEN);
	set_control_background(pane[0], LIGHT_BLUE);

	on_control_redraw(pane[2], draw_pane_border);
	on_control_redraw(pane[1], draw_pane_border);
	on_control_redraw(pane[0], draw_pane_border);

	/* Add controls to each pane. */

	c = add_field(pane[0], rect(10,10,100,24), "Hello");

	c = add_text_box(pane[1], rect(10,10,100,100), "Text box");
	c = add_button(pane[1], rect(120,10,80,30), "Quit", quit);

	c = add_label(pane[2], rect(10,10,60,24), "Name:",
		ALIGN_RIGHT + VALIGN_CENTER);
	c = add_field(pane[2], rect(80,10,80,24), "");

	/* Make buttons to allow a pane to be selected. */

	tab[0] = new_tab_button(w, rect(15,10,80,32), "Pane 0", select_pane);
	set_control_value(tab[0], 0);
	set_control_background(tab[0], LIGHT_BLUE);

	tab[1] = new_tab_button(w, rect(105,10,80,32), "Pane 1", select_pane);
	set_control_value(tab[1], 1);
	set_control_background(tab[1], LIGHT_GREEN);

	tab[2] = new_tab_button(w, rect(195,10,80,32), "Pane 2", select_pane);
	set_control_value(tab[2], 2);
	set_control_background(tab[2], LIGHT_RED);

	select_pane(tab[0]);

	show_window(w);
	main_loop(app);
	del_app(app);
	return 0;
}
