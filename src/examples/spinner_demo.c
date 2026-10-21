/*
 *  Busy spinner demo
 *  -----------------
 *
 *  "Do It!" does five seconds of busy work (sums of square roots), calling
 *  spinner_step() as it goes; the spinner turns smoothly the whole time,
 *  and the window can still be moved.  "Wait" shows the spinner while the
 *  program sits in its main loop for three seconds: its timer turns it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "graphapp.h"
#include "spinner.h"

App *app;
Window *w;
Window *waiting = NULL;

static void about_this_program(MenuItem *mi)
{
	ask_ok(app, mi->text, "Demo by: Steve Ferrell");
}

static void do_it(Control *btn)
{
	Window *spin = new_spinner_window(app, "Working", "Busy...");
	double start = spinner_seconds(), sum = 0;
	long i = 0;
	int halfway = 0;

	while (spinner_seconds() - start < 5) {
		sum += sqrt((double) i++);
		if ((i & 0xFFF) == 0) {             /* every few thousand: a moment */
			spinner_step(spin);
			if (!halfway && spinner_seconds() - start > 2.5) {
				set_spinner_message(spin, "Almost done...");
				halfway = 1;
			}
		}
	}
	del_spinner_window(spin);
	ask_ok(app, "Done", "Process complete. Press OK to continue.");
}

static void stop_waiting(Timer *t)
{
	del_timer(t);
	del_spinner_window(waiting);
	waiting = NULL;
}

static void wait_a_bit(Control *btn)
{
	if (waiting) return;
	waiting = new_spinner_window(app, "Waiting", "Waiting for a reply...");
	new_timer(app, stop_waiting, 3000);
}

int main(int argc, char *argv[])
{
	MenuBar *mb;
	Menu *m;

	app = new_app(argc, argv);
	w = new_window(app, rect(50, 30, 280, 100), "Busy Spinner Demo", STANDARD_WINDOW);
	mb = new_menu_bar(w);
	m = new_menu(mb, "About");
	new_menu_item(m, "About", 'A', about_this_program);
	new_button(w, rect(10, 30, 120, 30), "Do It!", do_it);
	new_button(w, rect(150, 30, 120, 30), "Wait", wait_a_bit);
	show_window(w);
	main_loop(app);
	del_app(app);
	return 0;
}
