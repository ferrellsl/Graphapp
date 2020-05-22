/*
 *  Timing functions.
 *
 *  Platform: Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/17  Corrected for Win32.
 *  Version: 3.11  2001/12/03  Discarded app_sleep function.
 *  Version: 3.17  2002/01/08  Added support for timers.
 *  Version: 3.28  2002/08/16  Added Windows timer support.
 *  Version: 3.31  2002/08/26  Fixed deletion bugs.
 *  Version: 3.34  2002/12/18  Now app_delay returns elapsed milliseconds.
 *  Version: 3.35  2002/12/23  Renamed active_timers to num_timers.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

int app_delay(App *app, int milliseconds)
{
	Sleep(milliseconds);
	return milliseconds;
}

/*
 *  Report current time in milliseconds since initialisation of
 *  the graphics interface. Not reliable for timing events.
 */
unsigned long app_current_time(App *app)
{
	return GetTickCount();
}

/*
 *  Create a new timer.
 *  Try to start a Windows timer, but if that fails,
 *  use the portable timing technique.
 */
Timer * app_new_timer(App *app, TimerAction action, int milliseconds)
{
	Timer *t;
	Timer **list;
	int num;
	HWND hwnd = 0;

	t = app_zero_alloc(sizeof(Timer));
	if (t == NULL)
		return NULL;
	t->app = app;
	t->action = action;
	t->milliseconds = milliseconds;
	t->last_time = app_current_time(app);	//!!
	num = app->num_timers + 1;
	list = app_realloc(app->timers, num * sizeof(Timer *));
	if (list == NULL) {
		app_free(t);
		return NULL;
	}
	app->timers = list;
	app->timers[num-1] = t;
	app->num_timers++;

	/* find a HWND to use */
	if (app_extra(app)->timer_win == NULL)
		app_extra(app)->timer_win =
			app_new_window(app, rect(0,0,0,0), "Timer Window", 0);
	if (app_extra(app)->timer_win)
		hwnd = win_extra(app_extra(app)->timer_win)->hwnd;

	/* try to start a generic Windows timer, failure is okay */
	if (! app_extra(app)->timer_id)
		if (hwnd && SetTimer(hwnd, 1, TIMER_INTERVAL, NULL))
			app_extra(app)->timer_id = 1;

	return t;
}

/*
 *  Stop a timer by removing it from the list and deleting it.
 */
void app_del_timer(Timer *t)
{
	int i, shift;
	Window *w;
	App *app = t->app;

	for (i=shift=0; i < app->num_timers; i++) {
		if (app->timers[i] == t)
			shift++;
		if (shift && (i+shift < app->num_timers))
			app->timers[i] = app->timers[i+shift];
	}
	if (shift) {
		app->timers = app_realloc(app->timers,
				(i-shift) * sizeof(Timer *));
		app->num_timers -= shift;
	}

	if (app->num_timers == 0) {
		w = app_extra(app)->timer_win;
		if (w && win_extra(w)->hwnd && app_extra(app)->timer_id) {
			KillTimer(win_extra(w)->hwnd,
				app_extra(app)->timer_id);
			app_extra(app)->timer_id = 0;
		}
	}

	app_free(t);
}

/*
 *  Reset the timer.
 */
void app_reset_timer(Timer *t, int milliseconds) {	//!!
	t->last_time = app_current_time(t->app);
	t->milliseconds = milliseconds;
}
