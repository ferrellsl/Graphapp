/*
 *  Timing functions.
 *
 *  Platform: X-Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.11  2001/12/03  Discarded app_sleep function.
 *  Version: 3.17  2002/01/08  Added support for timers.
 *  Version: 3.30  2002/08/25  Fixed deletion bugs (timer->app was NULL).
 *  Version: 3.31  2002/08/26  Modified select call in delay function.
 *  Version: 3.34  2002/12/18  Now app_delay returns elapsed milliseconds.
 *  Version: 3.35  2002/12/23  Renamed active_timers to num_timers.
 *  Version: 3.57  2005/08/16  Uses App->socket_fd (not ConnectionNumber).
 *  Version: 3.60  2007/06/06  Improved timing using poll.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#ifdef USE_ALARM
static void alarm_handler(int signo)
{
}

static int set_alarm_handler(int set)
{
	struct sigaction sa;
	int error;

	memset (&sa, 0, sizeof (struct sigaction));
	sa.sa_handler = set ? alarm_handler : SIG_IGN;

	do error = sigaction (SIGALRM, &sa, NULL);
	while ((error == -1) && errno == EINTR);

	return ! error;
}

static int SetTimer()
{
	struct itimerval itv;

	if (set_alarm_handler(1)) {
		memset(&itv, 0, sizeof (struct itimerval));
		itv.it_value.tv_sec = TIMER_INTERVAL / 1000;
		itv.it_value.tv_usec = (TIMER_INTERVAL % 1000) * 1000;
		itv.it_interval = itv.it_value;
		if (! setitimer (ITIMER_REAL, &itv, NULL))
			return 1;
	}
	return 0;
}

static void KillTimer()
{
	struct itimerval itv;

	memset(&itv, 0, sizeof (struct itimerval));
	setitimer (ITIMER_REAL, &itv, NULL);

	set_alarm_handler(0);
}
#endif

#if 0	//!!
/*
 *  Delay execution for a given number of milliseconds.
 *  This function should be used sparingly.
 */
int app_delay(App *app, int milliseconds)
{
	struct timeval t;
	fd_set rmask, wmask, emask;
	int fd;
	int retval;
	int sec, usec, remaining;

	if (milliseconds < 0)
		return 0;

    #ifdef USE_ALARM
	set_alarm_handler(0);
    #endif

	t.tv_sec = sec = milliseconds / 1000;
	t.tv_usec = usec = ((unsigned long) milliseconds % 1000) * 1000;

	if (app) {
		XFlush(app_extra(app)->display);
		fd = app->socket_fd;

		FD_ZERO(&rmask);
		FD_ZERO(&wmask);
		FD_ZERO(&emask);

		FD_SET(fd, &rmask);
		/*FD_SET(fd, &wmask);*/
		FD_SET(fd, &emask);

		retval = select(fd+1, &rmask, &wmask, &emask, &t);

	}
	else {
		retval = select(0, NULL, NULL, NULL, &t);
	}

	/* If using Linux and some System V systems,
	 * the timeval now contains the time _not_ slept.
	 * Should be zero if nothing interrupted the call,
	 * or some value less than the initial value otherwise.
	 * On BSD systems, the value will generally be
	 * untouched. Therefore, we examine the value,
	 * and if it is untouched, we assume we are using BSD
	 * and that the entire time was spent sleeping
	 * (because it's difficult to find out exactly how
	 * much time _was_ spent sleeping).
	 * If the value is different, we assume we're using
	 * a System V or Linux system, and subtract the
	 * value from the initial value to determine how
	 * much time was spent sleeping, and return that. */

	if (retval == -1) {
		if (errno == EINTR)
			remaining = milliseconds/2; /* guess */
		else
			remaining = milliseconds;
		errno = 0;
	}
	else if ((sec == t.tv_sec) && (usec == t.tv_usec)) {
		/* assume BSD */
		remaining = 0;
	}
	else {
		/* assume System V or Linux */
		remaining = t.tv_usec / 1000;
		remaining += t.tv_sec * 1000;
	}

    #ifdef USE_ALARM
	set_alarm_handler(1);
    #endif
	return (milliseconds - remaining);
}
#endif

int app_delay(App *app, int milliseconds)
{
	if (milliseconds < 0)
		return 0;

#ifdef USE_ALARM
	set_alarm_handler(0);
#endif

	if (milliseconds > 5000) {
		sleep(milliseconds / 1000);
		milliseconds %= 1000;
	}
	usleep(1000 * milliseconds);

#ifdef USE_ALARM
	set_alarm_handler(1);
#endif
	return milliseconds;
}

/*
 *  Report current time in milliseconds.	//!!
 */
unsigned long app_current_time(App *app)
{
	struct timeval tv;
	gettimeofday (&tv, NULL);
	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

/*
 *  Create a new timer.
 */
Timer * app_new_timer(App *app, TimerAction action, int milliseconds) //!!
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

#ifdef USE_ALARM
	/* try to start a generic Alarm timer, failure is okay */
	if (! app_extra(app)->timer_id && SetTimer())	//!!
		app_extra(app)->timer_id = 1;
#endif

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
	if (shift)
		app->timers = app_realloc(app->timers,
				(i-shift) * sizeof(Timer *));
	app->num_timers -= shift;

#ifdef USE_ALARM
	if (app->num_timers == 0 && app_extra(app)->timer_id) {	//!!
		app_extra(app)->timer_id = 0;
		KillTimer();
	}
#endif

	app_free(t);
}


/*
 *  Reset the timer.
 */
void app_reset_timer(Timer *t, int milliseconds) {	//!!
	t->last_time = app_current_time(t->app);
	t->milliseconds = milliseconds;
}
