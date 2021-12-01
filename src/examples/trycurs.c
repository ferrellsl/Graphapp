/*
 *  Try Cursors
 *  -----------
 *
 *  Try different cursors by selecting them with buttons.
 */

#include <stdio.h>
#include <graphapp.h>

App *app;
Window *w;

void try_cursor(Control *b)
{
	int shape = get_control_value(b);
	set_window_cursor(w, get_standard_cursor(app, shape));
}

void move_cursor(Control *b)
{
	Point p = get_cursor_position(app);

	p.x += 15;
	p.y -= 15;

	set_cursor_position(app, p);
}

int main(int argc, char *argv[])
{
	Control *b;
	Rect r;

	app = new_app(argc, argv);
	w = new_window(app, rect(10,10,550,400), "Try Cursors",
			STANDARD_WINDOW);

	r = rect(10,10,150,30);

	b = new_button(w, r, "Move", move_cursor);
	r.y += r.height + 5;

	b = new_button(w, r, "Blank Cursor", try_cursor);
	set_control_value(b, BLANK_CURSOR);
	r.y += r.height + 5;

	b = new_button(w, r, "Arrow Cursor", try_cursor);
	set_control_value(b, ARROW_CURSOR);
	r.y += r.height + 5;

	b = new_button(w, r, "Wait Cursor", try_cursor);
	set_control_value(b, WAIT_CURSOR);
	r.y += r.height + 5;

	b = new_button(w, r, "Caret Cursor", try_cursor);
	set_control_value(b, CARET_CURSOR);
	r.y += r.height + 5;

	b = new_button(w, r, "Cross Cursor", try_cursor);
	set_control_value(b, CROSS_CURSOR);
	r.y += r.height + 5;

	b = new_button(w, r, "Hand Cursor", try_cursor);
	set_control_value(b, HAND_CURSOR);
	r.y += r.height + 5;

	b = new_button(w, r, "Grabbing Cursor", try_cursor);
	set_control_value(b, GRAB_CURSOR);
	r.y += r.height + 5;

	r.y = 10 + r.height + 5;
	r.x += r.width + 5;

	b = new_button(w, r, "Pointing Cursor", try_cursor);
	set_control_value(b, POINTING_CURSOR);
	r.y += r.height + 5;

	b = new_button(w, r, "Pencil Cursor", try_cursor);
	set_control_value(b, PENCIL_CURSOR);
	r.y += r.height + 5;

	b = new_button(w, r, "Lasso Cursor", try_cursor);
	set_control_value(b, LASSO_CURSOR);
	r.y += r.height + 5;

	b = new_button(w, r, "Dropper Cursor", try_cursor);
	set_control_value(b, DROPPER_CURSOR);
	r.y += r.height + 5;

	b = new_button(w, r, "Magnify Cursor", try_cursor);
	set_control_value(b, MAGNIFY_CURSOR);
	r.y += r.height + 5;

	b = new_button(w, r, "MagPlus Cursor", try_cursor);
	set_control_value(b, MAGPLUS_CURSOR);
	r.y += r.height + 5;

	b = new_button(w, r, "MagMinus Cursor", try_cursor);
	set_control_value(b, MAGMINUS_CURSOR);
	r.y += r.height + 5;

	r.y = 10 + r.height + 5;
	r.x += r.width + 5;

	b = new_button(w, r, "SizeLR Cursor", try_cursor);
	set_control_value(b, SIZELR_CURSOR);
	r.y += r.height + 5;

	b = new_button(w, r, "SizeTB Cursor", try_cursor);
	set_control_value(b, SIZETB_CURSOR);
	r.y += r.height + 5;

	show_window(w);
	main_loop(app);
	del_app(app);
	return 0;
}
