/*
 *  Clock
 *  -----
 *  Digital clock program written using GraphApp.
 *
 *  This is a complex example which demonstrates the use of
 *  timer functions, fonts, and windows. It also uses some
 *  standard C time functions.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <graphapp.h>

App *app;
Window *the_window;
Font *the_font;
int state = 0; /* 0 = show the time, 1 = show the date */
int hour = -1, minute = -1;
int day = -1, month = -1;

char *month_name[] = {
	"", "January", "February", "March",
	"April", "May", "June",
	"July", "August", "September",
	"October", "November", "December"
};

void draw_clock(Window *w, Graphics *g)
{
	int width, height;
	char the_string[80];
	Rect r = get_window_area(w);

	if (state == 0)
		sprintf(the_string, "%d:%2.2d", hour, minute);
	else
		sprintf(the_string, "%d %3.3s", day, month_name[month]);
	set_font(g, the_font);
	width = font_width(the_font, the_string, strlen(the_string));
	height = font_height(the_font);
	draw_utf8(g, pt((r.width-width)/2, (r.height-height)/2),
		the_string, strlen(the_string));
}

void update_clock(Timer *unused)
{
	struct tm *the_time;
	time_t seconds;
	int newhour, newminute, newday, newmonth;

	tzset();
	time(&seconds);
	the_time = localtime(&seconds);

	newhour = the_time->tm_hour;
	if (newhour > 12)
		newhour -= 12;
	newminute = the_time->tm_min;

	newday = the_time->tm_mday;
	newmonth = the_time->tm_mon + 1;

	if ((hour != newhour) || (minute != newminute)
		|| (day != newday) || (month != newmonth))
	{
		hour = newhour;
		minute = newminute;
		day = newday;
		month = newmonth;
		redraw_window(the_window);
	}
}

void handle_mouse(Window *w, int buttons, Point xy)
{
	if (buttons && !state) {
		state = 1;
		redraw_window(the_window);
	}
	else if (!buttons && state) {
		state = 0;
		redraw_window(the_window);
	}
}

void handle_key(Window *w, unsigned long key)
{
	if ((key == ESC)
	 || (key == 'q')
	 || (key == 'Q')
	 || (key == 17))	/* ctrl-q */
		exit(0);
}

int main(int argc, char *argv[])
{
	app = new_app(argc, argv);
	the_font = new_font(app, "unifont", BOLD, 16);
	the_window = new_window(app,
			rect(3,3,font_width(the_font,"HH:MM.",6),18),
			"Clock", FLOATING);
	on_window_redraw(the_window, draw_clock);
	set_window_background(the_window, rgb(0x66,0x99,0xCC));
	on_window_key_down(the_window, handle_key);
	on_window_mouse_up(the_window, handle_mouse);
	on_window_mouse_down(the_window, handle_mouse);
	update_clock(NULL);
	show_window(the_window);
	new_timer(app, update_clock, 15000); /* 15 second timer */
	main_loop(app);
	return 0;
}
