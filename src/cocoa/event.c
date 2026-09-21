/*
 *  Events.
 *
 *  Platform: macOS.
 *
 *  The Cocoa side (cocoa_event.m) turns NSEvents into calls to the
 *  callbacks below, which do what the Windows window procedure does:
 *  locate the control under the mouse, translate keys into App key
 *  codes, resize and close windows, and so on.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

extern App *app_the_app;

/*
 *  Convert pressed buttons and modifiers into App's button bits:
 *  1 = left, 2 = middle (or shift-left), 4 = right (or control-left).
 */
static int app_get_button_state(int pressed, int mods)
{
	int buttons = 0;

	if (pressed & 1) {
		if (mods & GAB_MOD_CTRL)
			buttons |= 4;
		if (mods & GAB_MOD_SHIFT)
			buttons |= 2;
		if (buttons == 0)
			buttons |= 1;
	}
	if (pressed & 4)
		buttons |= 2;
	if (pressed & 2)
		buttons |= 4;

	return buttons;
}

static void app_do_mouse_down(Window *win, int buttons, int x, int y)
{
	int i;
	Point p;
	Control *c;

	if (win == NULL)
		return;

	p.x = x;
	p.y = y;

	c = app_locate_control(win, p);

	while (c) {
		if (c->mouse_down) {
			win->mouse_grab = c;
			p.x = x - c->offset.x;
			p.y = y - c->offset.y;
			for (i=0; c->mouse_down[i]; i++)
				c->mouse_down[i](c, buttons, p);
			if (win->pass_event)
				win->pass_event = 0;
			else
				return;
		}
		c = c->parent;
	}
	if (win->mouse_down) {
		p.x = x;
		p.y = y;
		for (i=0; win->mouse_down[i]; i++)
			win->mouse_down[i](win, buttons, p);
	}
}

static void app_do_mouse_up(Window *win, int buttons, int x, int y)
{
	int i;
	Point p;
	Control *c;

	if (win == NULL)
		return;

	p.x = x;
	p.y = y;

	c = app_locate_control(win, p);

	while (c) {
		if (buttons == 0)
			win->mouse_grab = NULL;
		if (c->mouse_up) {
			p.x = x - c->offset.x;
			p.y = y - c->offset.y;
			for (i=0; c->mouse_up[i]; i++)
				c->mouse_up[i](c, buttons, p);
			if (win->pass_event)
				win->pass_event = 0;
			else
				return;
		}
		c = c->parent;
	}
	if (win->mouse_up) {
		p.x = x;
		p.y = y;
		for (i=0; win->mouse_up[i]; i++)
			win->mouse_up[i](win, buttons, p);
	}
}

static void app_do_mouse_move(Window *win, int buttons, int x, int y)
{
	int i;
	Point p;
	Control *c;

	if (win == NULL)
		return;

	p.x = x;
	p.y = y;

	c = app_locate_control(win, p);

	i = win->flags & TEMP_CURSOR;
	if (c && c->cursor) {
		if (c->cursor != win->cursor)
			app_set_window_temp_cursor(win, c->cursor);
	}
	else if (i != 0)
		app_set_window_temp_cursor(win, NULL);

	if (c && (c->state & TIP_MASK))
		app_handle_tip(c);

	if (buttons) {
		while (c) {
			if (c->mouse_drag) {
				p.x = x - c->offset.x;
				p.y = y - c->offset.y;
				for (i=0; c->mouse_drag[i]; i++)
					c->mouse_drag[i](c, buttons, p);
				if (win->pass_event)
					win->pass_event = 0;
				else
					return;
			}
			c = c->parent;
		}
		if (win->mouse_drag) {
			p.x = x;
			p.y = y;
			for (i=0; win->mouse_drag[i]; i++)
				win->mouse_drag[i](win, buttons, p);
		}
	}
	else {
		while (c) {
			if (c->mouse_move) {
				p.x = x - c->offset.x;
				p.y = y - c->offset.y;
				for (i=0; c->mouse_move[i]; i++)
					c->mouse_move[i](c, buttons, p);
				if (win->pass_event)
					win->pass_event = 0;
				else
					return;
			}
			c = c->parent;
		}
		if (win->mouse_move) {
			p.x = x;
			p.y = y;
			for (i=0; win->mouse_move[i]; i++)
				win->mouse_move[i](win, buttons, p);
		}
	}
}

/*
 *  Return 1 if a modal window is in front of this window, else 0.
 */
int app_modal_in_front(Window *win)
{
	App *app;

	if (win == NULL)
		return 0;

	app = win->app;
	if ((app->num_modals > 0) &&
	    (app->modals[app->num_modals-1] != win))
		return 1;
	return 0;
}

void app_do_close_window(Window *win)
{
	int i;

	if (win) {
		if (win->close)
			for (i=0; win->close[i]; i++)
				win->close[i](win);
		else
			app_hide_window(win);
	}
}

void app_do_move_window(Window *win)
{
	int i;

	if (win) {
		app_fix_window_area(win);
		if (win->move)
			for (i=0; win->move[i]; i++)
				win->move[i](win);
	}
}

void app_do_resize_window(Window *win)
{
	int i;
	Rect r;

	if (win) {
		app_fix_window_area(win);
		r = app_get_window_area(win);
		if (win->menubar)
			win->menubar->ctrl->area.width = r.width;
		app_place_window_controls(win, 1);
		if (win->resize)
			for (i=0; win->resize[i]; i++)
				win->resize[i](win);
	}
}

static void app_stop_all_timers(App *app)
{
	while (app->num_timers)
		app_del_timer(app->timers[0]);
}

static void app_do_shutdown(App *app)
{
	if (app) {
		app_hide_all_windows(app);
		app_stop_all_timers(app);
	}
}

/*
 *  Keys.
 *
 *  Special keys (arrows, function keys, Ctrl/Cmd combinations) go to the
 *  key_action handlers, as Windows WM_KEYDOWN does; typed characters go
 *  to the key_down handlers, as WM_CHAR does.  Cmd is treated like Ctrl,
 *  so Cmd-C/V/X/A and menu shortcuts work the Mac way.
 */

/* macOS virtual key codes. */
enum {
	VK_A_RETURN = 36, VK_A_TAB = 48, VK_A_BACKSPACE = 51, VK_A_ESCAPE = 53,
	VK_A_KP_ENTER = 76,
	VK_A_LEFT = 123, VK_A_RIGHT = 124, VK_A_DOWN = 125, VK_A_UP = 126,
	VK_A_HOME = 115, VK_A_END = 119, VK_A_PGUP = 116, VK_A_PGDN = 121,
	VK_A_FWD_DELETE = 117, VK_A_HELP = 114
};

/* Function key virtual key codes, in F1..F10 order. */
static const int fkey_codes[10] = { 122, 120, 99, 118, 96, 97, 98, 100, 101, 109 };

static void app_do_key(Window *win, int keycode, unsigned long ch, int mods)
{
	int i;
	int special = 0;
	int shift = (mods & GAB_MOD_SHIFT) != 0;
	int ctrl = (mods & (GAB_MOD_CTRL | GAB_MOD_CMD)) != 0;
	unsigned long code = 0;

	if (win == NULL)
		return;
	if (app_modal_in_front(win))
		return;

	switch (keycode) {
	  case VK_A_LEFT:       code = LEFT;  special = 1; break;
	  case VK_A_RIGHT:      code = RIGHT; special = 1; break;
	  case VK_A_UP:         code = UP;    special = 1; break;
	  case VK_A_DOWN:       code = DOWN;  special = 1; break;
	  case VK_A_HOME:       code = HOME;  special = 1; break;
	  case VK_A_END:        code = END;   special = 1; break;
	  case VK_A_PGUP:       code = PGUP;  special = 1; break;
	  case VK_A_PGDN:       code = PGDN;  special = 1; break;
	  case VK_A_FWD_DELETE: code = DEL;   special = 1; break;
	  case VK_A_HELP:       code = INS;   special = 1; break;
	  case VK_A_TAB:        code = '\t';  special = 1; break;
	  default:
		for (i=0; i < 10; i++) {
			if (keycode == fkey_codes[i]) {
				code = F1 + i;
				special = 1;
			}
		}
		break;
	}

	/* Ctrl/Cmd combinations: to the menu shortcuts and key_action. */
	if (ctrl) {
		if (! special) {
			if (ch == 0 || ch >= 0xF700)
				return;
			code = ch;
			if ((code >= 'a') && (code <= 'z'))
				code = code - 'a' + 'A';
		}
		code |= CONTROL;
		if (shift)
			code |= SHIFT;
		app_send_key_value(win, code, 2);
		return;
	}

	if (special) {
		if (shift)
			code |= SHIFT;
		if (app_send_key_value(win, code, 2))
			return;
		if (keycode != VK_A_TAB)
			return;
		/* unhandled Tab is an ordinary character */
	}

	/* Typed characters. */
	switch (keycode) {
	  case VK_A_RETURN:
	  case VK_A_KP_ENTER:  ch = '\r'; break;
	  case VK_A_BACKSPACE: ch = BKSP; break;
	  case VK_A_ESCAPE:    ch = ESC;  break;
	  case VK_A_TAB:       ch = '\t'; break;
	  default:
		if (ch < 32 || (ch >= 0xF700 && ch <= 0xF8FF))
			return;	/* not a character */
		break;
	}
	app_do_key_down(win, ch);
}

/*
 *  Callbacks from the Cocoa side.
 */

static void cb_mouse(void *w, int kind, int pressed, int mods, int x, int y)
{
	Window *win = (Window *) w;
	int buttons;

	if (win == NULL || app_modal_in_front(win))
		return;

	buttons = app_get_button_state(pressed, mods);

	switch (kind) {
	  case GAB_MOUSE_DOWN:
		app_do_mouse_down(win, buttons, x, y);
		break;
	  case GAB_MOUSE_UP:
		app_do_mouse_up(win, buttons, x, y);
		break;
	  default:
		app_do_mouse_move(win, buttons, x, y);
		break;
	}
}

static void cb_key(void *w, int keycode, unsigned long ch, int mods)
{
	app_do_key((Window *) w, keycode, ch, mods);
}

static void cb_resized(void *w, int width, int height)
{
	Window *win = (Window *) w;
	Surface *s;
	Surface fresh;

	if (win == NULL)
		return;
	if (width < 1)
		width = 1;
	if (height < 1)
		height = 1;

	s = &win_extra(win)->surf;
	if ((s->width != width) || (s->height != height)) {
		if (! app_surface_create(&fresh, width, height))
			return;
		{
			size_t i, n = (size_t) width * height;
			uint32_t p = ((uint32_t) win->bg.red << 16) |
				((uint32_t) win->bg.green << 8) | win->bg.blue;
			for (i=0; i < n; i++)
				fresh.pixels[i] = p;
		}
		app_surface_destroy(s);
		*s = fresh;
		gab_window_set_surface(win_extra(win)->handle,
				s->pixels, s->width, s->height);
	}

	if (win->state & VISIBLE) {
		app_do_resize_window(win);
		app_do_redraw_window(win, app_get_window_area(win));
	}
	else {
		win->area.width = width;
		win->area.height = height;
	}
}

static void cb_moved(void *w)
{
	Window *win = (Window *) w;

	if (win && (win->state & VISIBLE))
		app_do_move_window(win);
}

static void cb_close(void *w)
{
	app_do_close_window((Window *) w);
}

static void cb_quit(void)
{
	app_do_shutdown(app_the_app);
}

/*
 *  Keep a modal window in front: activating any other window of the
 *  App while a modal window is up brings the modal window forward.
 */
static void cb_activated(void *w)
{
	Window *win = (Window *) w;
	App *app;
	Window *top;

	if (win == NULL)
		return;
	app = win->app;
	if (app->num_modals == 0)
		return;
	top = app->modals[app->num_modals-1];
	if (top != win && (top->state & VISIBLE))
		gab_window_show(win_extra(top)->handle);
}

void app_install_bridge_callbacks(App *app)
{
	GABCallbacks cb;

	memset(&cb, 0, sizeof(cb));
	cb.mouse = cb_mouse;
	cb.key = cb_key;
	cb.resized = cb_resized;
	cb.moved = cb_moved;
	cb.close = cb_close;
	cb.quit = cb_quit;
	cb.activated = cb_activated;
	gab_set_callbacks(&cb);
}

/*
 *  The event loop.
 */

static void app_do_portable_timers(App *app)
{
	Timer *t;
	unsigned long now;
	int i;

	now = app_current_time(app);

	for (i=0; i < app->num_timers; i++) {
		t = app->timers[i];
		if (now - t->last_time >= (unsigned long) t->milliseconds) {
			t->action(t);
			t->last_time = now;
		}
	}
}

/* Milliseconds until the next timer is due; negative if there are none. */
static int app_next_timer_wait(App *app)
{
	unsigned long now = app_current_time(app);
	long wait, best = -1;
	int i;

	for (i=0; i < app->num_timers; i++) {
		Timer *t = app->timers[i];
		wait = (long) t->milliseconds - (long) (now - t->last_time);
		if (wait < 0)
			wait = 0;
		if (best < 0 || wait < best)
			best = wait;
	}
	return (int) best;
}

/*
 *  Get the mouse event that ends a tracking loop (menus, drop lists).
 *
 *  Loops until a mouse event is found, which is returned (not
 *  dispatched) and gives 1.  A keyboard event gives 0, and is left
 *  queued to be handled later.  Other events are handled normally.
 */
int app_get_mouse_event(App *app, int *buttons, Point *p)
{
	GABMouse m;
	Window *win;
	int kind;

	*buttons = 0;

	while (app->visible_windows > 0) {
		app_flush_all_windows(app);
		kind = gab_next_event(-1, &m);

		if (kind == GAB_EV_MOUSE) {
			win = (Window *) m.win;
			if (app_modal_in_front(win))
				continue;	/* ignore mouse */
			*buttons = app_get_button_state(m.pressed, m.mods);
			p->x = m.x;
			p->y = m.y;
			if (*buttons == 0)
				win->mouse_grab = NULL;
			return 1;
		}
		if (kind == GAB_EV_KEY)
			return 0;
	}
	return 0;
}

void app_main_loop(App *app)
{
	while (app_wait_event(app))
		continue;
}

int app_wait_event(App *app)
{
	int did_event = 0;

	if (app->visible_windows > 0) {
		app_flush_all_windows(app);
		gab_next_event(app_next_timer_wait(app), NULL);
		did_event = 1;
	}

	if (app->num_timers != 0)
		app_do_portable_timers(app);

	app_do_delayed_deletion(app);
	app_flush_all_windows(app);

	return did_event;
}

int app_process_events(App *app)
{
	int result = 0;
	int guard;

	if (app->num_timers != 0)
		app_do_portable_timers(app);

	if (app->visible_windows > 0) {
		/* the guard stops a flood of mouse moves starving the caller */
		for (guard = 0; guard < 1000; guard++)
			if (gab_next_event(0, NULL) == GAB_EV_NONE)
				break;
		result = 1;
	}

	app_do_delayed_deletion(app);
	app_flush_all_windows(app);

	return result;
}

int app_do_event(App *app)
{
	int did_event = 0;

	if (app->visible_windows > 0) {
		if (gab_next_event(0, NULL) != GAB_EV_NONE)
			did_event = 1;
		app_flush_all_windows(app);
	}
	return did_event;
}

int app_peek_event(App *app)
{
	int i;

	for (i=0; i < app->num_windows; i++) {
		if (app->windows[i]->redraw_rgn != NULL)
			return 1;
	}
	if (app->visible_windows > 0)
		if (gab_events_pending())
			return 1;
	return 0;
}

/*
 *  Drawing is buffered on this platform, so this makes it visible.
 */
void app_draw_all(App *app)
{
	app_flush_all_windows(app);
}
