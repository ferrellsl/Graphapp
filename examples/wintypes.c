/*
 *  Window Types
 *  ------------
 *  Demonstrate the different window types possible
 *  in a MWM-compatible window manager.
 */

#include <stdio.h>
#include <string.h>
#include <graphapp.h>

App *app;
int i = 50;

void display_info(Window *w, Graphics *g)
{
	char *info;

	info = get_window_data(w);
	set_rgb(g, BLACK);
	draw_utf8(g, pt(5,5), info, strlen(info));
}

void demonstrate(char *s, unsigned long style)
{
	Window *w;

	w = new_window(app, rect(i,i,200,100), s, style); i+=25;
	set_window_data(w, s);
	on_window_redraw(w, display_info);
	show_window(w);
	/* main_loop(app); */
	/* del_window(w); */
}

int main(int argc, char *argv[])
{
	app = new_app(argc, argv);

	demonstrate("TITLEBAR+CLOSEBOX", TITLEBAR | CLOSEBOX);
	demonstrate("TITLEBAR+RESIZE", TITLEBAR | RESIZE);
	demonstrate("TITLEBAR+MINIMIZE", TITLEBAR | MINIMIZE);
	demonstrate("TITLEBAR+MAXIMIZE", TITLEBAR | MAXIMIZE);
	demonstrate("TITLEBAR+FLOATING", TITLEBAR | CLOSEBOX | FLOATING);
	demonstrate("TITLEBAR+MODAL", TITLEBAR | MODAL);
	demonstrate("SIMPLE_WINDOW", SIMPLE_WINDOW);
	demonstrate("STANDARD_WINDOW", STANDARD_WINDOW);

	main_loop(app);
	return 0;
}
