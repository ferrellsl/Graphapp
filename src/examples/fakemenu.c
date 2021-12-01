/*
 *  Fake Menu
 *  ----------
 *
 *  An experiment in creating a menu using controls.
 */

#include <stdio.h>
#include <stdlib.h>
#include <graphapp.h>

App *app;
Window *w;
Control *mb;
Control *cover = NULL;
Control *menu = NULL;

void click_menu_bar(Control *mb, int buttons, Point xy)
{
	xy.x += mb->area.x;
	xy.y = mb->area.y + mb->area.height;

	if (menu) {
		Rect r = menu->area;
		del_control(menu);
		redraw_rect(w, r);
	}
	menu = new_control(w, rect(xy.x, xy.y, 80, 100));
	set_control_background(menu, PALE_GREY);
}

void draw_menu_bar(Control *mb, Graphics *g)
{
	char *names[] = { "File", "Edit", "View" };
	int i;

	for (i=0; i < sizeof(names) / sizeof(names[0]); i++)
		draw_utf8(g, pt(5+i*60, 5), names[i], strlen(names[i]));
}

void quit_now(Control *c)
{
	exit(0);
}

int main(int argc, char *argv[])
{
	app = new_app(argc, argv);
	w = new_window(app, rect(10,10,448,400),
			"Fake Menu Test", STANDARD_WINDOW);
	new_button(w, rect(10,10,90,25), "Exit", quit_now);
	mb = new_control(w, rect(110,10,200,25));
	set_control_background(mb, PALE_GREY);
	on_control_mouse_down(mb, click_menu_bar);
	on_control_redraw(mb, draw_menu_bar);
	show_window(w);
	main_loop(app);
	del_app(app);
	return 0;
}
