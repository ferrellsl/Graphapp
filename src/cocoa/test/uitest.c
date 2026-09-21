/*
 *  End-to-end test of the Cocoa backend: a real window with controls,
 *  driven by synthetic mouse and key events, with snapshots of the view.
 *
 *    build-macos/test/uitest <output-dir>
 *
 *  Steps run from a timer, so they go through the real event loop:
 *    1. snapshot of the initial window
 *    2. click the button          -> counter label changes
 *    3. type into the text field  -> field text changes
 *    4. click the check box       -> checked
 *    5. Cmd-D                     -> menu shortcut fires
 *    6. press/drag/release on the File menu -> item chosen (mouse tracking)
 *    7. resize the window         -> controls re-laid out, surface resized
 *    8. snapshot, report, quit
 */

#include "appint.h"

#ifndef MENU_ITEM_Y
#define MENU_ITEM_Y 52	/* second item of the File menu */
#endif

static App *app;
static Window *win;
static Control *button, *label, *field, *check;
static const char *outdir = ".";
static int step = 0;
static int clicks = 0, shortcut_fired = 0, menu_fired = 0;
static int failures = 0;

static void expect(int cond, const char *what)
{
	printf("  %s  %s\n", cond ? "PASS" : "FAIL", what);
	if (! cond)
		failures++;
}

static void on_click(Control *c)
{
	char text[40];

	clicks++;
	sprintf(text, "clicks = %d", clicks);
	app_set_control_text(label, text);
}

static void on_shortcut(MenuItem *mi) { shortcut_fired++; }
static void on_menu(MenuItem *mi)     { menu_fired++; }

static void snapshot(const char *name)
{
	char path[512];

	sprintf(path, "%s/%s", outdir, name);
	printf("snapshot %s: %s\n", name,
		gab_test_snapshot(win_extra(win)->handle, path) ? "ok" : "FAILED");
}

static void click(int x, int y, int button_no)
{
	void *h = win_extra(win)->handle;

	gab_test_mouse(h, GAB_MOUSE_DOWN, x, y, button_no, 0);
	gab_test_mouse(h, GAB_MOUSE_UP, x, y, button_no, 0);
}

static void type(int keycode, const char *chars)
{
	gab_test_key(win_extra(win)->handle, keycode, chars, 0);
}

static void tick(Timer *t)
{
	void *h = win_extra(win)->handle;
	Rect r;
	char *text;

	switch (step++) {
	  case 0:
		snapshot("ui1_initial.png");
		r = button->area;
		click(r.x + r.width/2, r.y + r.height/2, 1);
		click(r.x + r.width/2, r.y + r.height/2, 1);
		break;

	  case 1:
		expect(clicks == 2, "button clicked twice");
		app_set_focus(field);
		type(4, "h");
		type(34, "i");
		type(49, " ");
		type(13, "Z");
		break;

	  case 2:
		text = app_get_control_text(field);
		expect(text && ! strcmp(text, "hi Z"), "field holds \"hi Z\"");
		if (text)
			printf("        field text = \"%s\"\n", text);
		type(51, "\x7f");	/* backspace */
		break;

	  case 3:
		text = app_get_control_text(field);
		expect(text && ! strcmp(text, "hi "), "backspace removed one char");
		r = check->area;
		click(r.x + 8, r.y + r.height/2, 1);
		break;

	  case 4:
		expect(app_is_checked(check), "check box checked");
		gab_test_key(h, 2, "d", GAB_MOD_CMD);	/* Cmd-D */
		break;

	  case 5:
		expect(shortcut_fired == 1, "Cmd-D menu shortcut fired");
		/* Pull-down menu: press on "File", drag onto an item, release.
		   The menu code tracks the mouse in a blocking loop, so all
		   three events are queued before the press is dispatched. */
		gab_test_mouse(h, GAB_MOUSE_DOWN, 12, 8, 1, 0);
		gab_test_mouse(h, GAB_MOUSE_DRAG, 20, MENU_ITEM_Y, 1, 0);
		gab_test_mouse(h, GAB_MOUSE_UP, 20, MENU_ITEM_Y, 1, 0);
		break;

	  case 6:
		expect(menu_fired == 1, "pull-down menu item chosen by mouse");
		app_size_window(win, rect(0, 0, 420, 260));
		break;

	  case 7:
		expect(win->area.width == 420 && win->area.height == 260,
			"window resized to 420x260");
		expect(win_extra(win)->surf.width == 420 &&
			win_extra(win)->surf.height == 260,
			"surface resized with it");
		snapshot("ui2_final.png");
		printf("\n%s\n", failures ? "SOME TESTS FAILED" : "ALL PASSED");
		app_hide_all_windows(app);
		break;
	}
}

int main(int argc, char *argv[])
{
	MenuBar *mb;
	Menu *m;

	if (argc > 1)
		outdir = argv[1];

	app = app_new_app(argc, argv);
	if (! app || ! app->gui_available) {
		fprintf(stderr, "no GUI\n");
		return 1;
	}

	win = app_new_window(app, rect(150, 150, 320, 200), "UI test", STANDARD_WINDOW);
	mb = app_new_menu_bar(win);
	m = app_new_menu(mb, "File");
	app_new_menu_item(m, "Shortcut item", CONTROL | 'D', on_shortcut);
	app_new_menu_item(m, "Other item", 0, on_menu);

	button = app_new_button(win, rect(20, 40, 100, 28), "Click me", on_click);
	label = app_new_label(win, rect(140, 40, 160, 28), "clicks = 0", ALIGN_LEFT);
	field = app_new_field(win, rect(20, 90, 200, 28), "");
	check = app_new_check_box(win, rect(20, 140, 160, 24), "Check me", NULL);

	app_show_window(win);
	app_new_timer(app, tick, 400);
	app_main_loop(app);

	return failures ? 1 : 0;
}
