/*
 *  Fields
 *  ------
 *
 *  A program to demonstrate text fields.
 */

#include <stdio.h>
#include <string.h>
#include <graphapp.h>

void move_focus(Window *w)
{
	Control *c;
	int i, max;

	max = w->num_children;
	if (max == 0)
		return;	/* no child controls for focus */

	c = w->key_focus;
	if (c == NULL) {
		/* nothing has focus, so set focus to first control */
		set_focus(w->children[max-1]);
		return;
	}

	/* move focus to next child, or back to first control */
	for (i=0; i < max; i++) {
		if (w->children[i] == c) {
			if (i > 0)
				c = w->children[i-1]; /* next */
			else
				c = w->children[max-1]; /* first */
			set_focus(c);
			break;
		}
	}
}

void window_key_down(Window *w, unsigned long key)
{
	if (key == '\t')
		move_focus(w);
}

int main(int argc, char *argv[])
{
	App *app;
	Window *w;
	Control *c;

	app = new_app(argc, argv);
	w = new_window(app, rect(10,10,200,200), "Text Field Test",
			STANDARD_WINDOW);
	on_window_key_down(w, window_key_down);

	/* Try some English text. */
	c = new_field(w, rect(10,10,150,24), "Hello");
	set_control_text(c, "Hello world");

	/* Try some Chinese text, UTF-8 encoded. */
	c = new_field(w, rect(10,44,150,24),
		"我能吞下玻璃而不伤身体。");

	/* Try a blank field. */
	c = new_field(w, rect(10,78,150,24), NULL);
	app_set_field_allowed_chars(c, "0123456789");

	/* Try a password field. */
	c = new_password_field(w, rect(10,112,150,24), "testing");

	show_window(w);
	move_focus(w);
	main_loop(app);
	del_app(app);
	return 0;
}
