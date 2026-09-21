/*
 *  Timers.
 *
 *  Platform: macOS.
 *
 *  Timers are the portable kind: they are checked by the event loop
 *  (which wakes up in time to run them) rather than by the OS.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

#include <time.h>
#include <unistd.h>

int app_delay(App *app, int milliseconds)
{
	if (app)
		app_flush_all_windows(app);
	if (milliseconds > 0)
		usleep((useconds_t) milliseconds * 1000);
	return milliseconds;
}

/*
 *  Report current time in milliseconds since some arbitrary moment.
 *  Not reliable for timing events.
 */
unsigned long app_current_time(App *app)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (unsigned long) ts.tv_sec * 1000UL + (unsigned long) ts.tv_nsec / 1000000UL;
}

/*
 *  Create a new timer.
 */
Timer * app_new_timer(App *app, TimerAction action, int milliseconds)
{
	Timer *t;
	Timer **list;
	int num;

	t = app_zero_alloc(sizeof(Timer));
	if (t == NULL)
		return NULL;
	t->app = app;
	t->action = action;
	t->milliseconds = milliseconds;
	t->last_time = app_current_time(app);
	num = app->num_timers + 1;
	list = app_realloc(app->timers, num * sizeof(Timer *));
	if (list == NULL) {
		app_free(t);
		return NULL;
	}
	app->timers = list;
	app->timers[num-1] = t;
	app->num_timers++;

	return t;
}

/*
 *  Stop a timer by removing it from the list and deleting it.
 */
void app_del_timer(Timer *t)
{
	int i, shift;
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

	app_free(t);
}

/*
 *  Reset the timer.
 */
void app_reset_timer(Timer *t, int milliseconds)
{
	t->last_time = app_current_time(t->app);
	t->milliseconds = milliseconds;
}
