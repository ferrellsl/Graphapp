/*
 *  Window utility functions.
 *
 *  Platform: Neutral.
 *
 *  Version: 3.50  2004/01/18  First release (factored out of win.c).
 *  Version: 3.60  2007/06/06  Modal windows aren't necessarily floating.
 *  Version: 3.63  2010/11/21  Added app_get_window_rect, some consts.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"


/*
 *  Internal functions:
 */


/*
 *  For creating windows:
 */
long app_actual_window_flags(long flags)
{
	if (! (flags & TITLEBAR)) {
		flags &= ~MINIMIZE;
		flags &= ~MAXIMIZE;
		flags &= ~CLOSEBOX;
	}
	flags &= ~CENTERED; /* already handled */
#if 0	//!!
	if (flags & MODAL)
		flags |= FLOATING;
#endif
	return flags;
}

/*
 *  Remember or forget a window within the list of windows.
 */
int app_remember_window(App *app, Window *win)
{
	Window **newlist;
	int i, bytes;

	i = app->num_windows;
	bytes = (i+1) * sizeof(Window *);
	newlist = app_realloc(app->windows, bytes);
	if (newlist) {
		newlist[i] = win;
		app->windows = newlist;
		app->num_windows++;
		return 1;	/* success */
	}
	else
		return 0;	/* realloc failed */
}

int app_forget_window(App *app, Window *win)
{
	Window **newlist;
	int i, bytes;

	/* find window to remove from array */
	for (i=0; i < app->num_windows; i++) {
		if (app->windows[i] == win)
			break;
	}
	if (i >= app->num_windows)
		return 1;	/* not found => success */
	/* copy next elements down over */
	for (; i < app->num_windows - 1; i++)
		app->windows[i] = app->windows[i+1];

	i = app->num_windows;
	if (i == 1) {
		app_free(app->windows);
		app->windows = NULL;
		app->num_windows = 0;
		return 1;	/* success */
	}
	bytes = (i-1) * sizeof(Window *);
	newlist = app_realloc(app->windows, bytes);
	if (newlist) {
		app->windows = newlist;
		app->num_windows--;
		return 1;	/* success */
	}
	else
		return 0;	/* realloc failed */
}

/*
 *  The following two functions ensure that modal windows
 *  are stored in a stack. The event loop uses this stack
 *  to allow a recently shown modal to receive mouse and
 *  keyboard events but any other windows behind it (modal
 *  or otherwise) will not.
 */
int app_remember_modal(App *app, Window *win)
{
	Window **newlist;
	int i, bytes;

	i = app->num_modals;
	bytes = (i+1) * sizeof(Window *);
	newlist = app_realloc(app->modals, bytes);
	if (newlist) {
		newlist[i] = win;
		app->modals = newlist;
		app->num_modals++;
		return 1;	/* success */
	}
	else
		return 0;	/* realloc failed */
}

int app_forget_modal(App *app, Window *win)
{
	Window **newlist;
	int i, bytes;

	/* find window to remove from array */
	for (i=0; i < app->num_modals; i++) {
		if (app->modals[i] == win)
			break;
	}
	if (i >= app->num_modals)
		return 1;	/* not found => success */
	/* copy next elements down over */
	for (; i < app->num_modals; i++)
		app->modals[i] = app->modals[i+1];

	i = app->num_modals;
	if (i == 1) {
		app_free(app->modals);
		app->modals = NULL;
		app->num_modals = 0;
		return 1;	/* success */
	}
	bytes = (i-1) * sizeof(Window *);
	newlist = app_realloc(app->modals, bytes);
	if (newlist) {
		app->modals = newlist;
		app->num_modals--;
		return 1;	/* success */
	}
	else
		return 0;	/* realloc failed */
}


/*
 *  Public functions:
 */


/*
 *  Drawing windows:
 */
void app_draw_window(Window *win)
{
	int i;
	Graphics *g;

	if (! (win->state & VISIBLE))
		return;

	if (win->redraw) {
		g = app_get_window_graphics(win);
		for (i=0; win->redraw[i]; i++)
			win->redraw[i](win, g);
		app_del_graphics(g);
	}
}

void app_redraw_window(Window *win)
{
	app_redraw_rect(win, rect(0, 0, win->area.width, win->area.height));
}

Rect app_get_window_area(const Window *win) /* Deprecated */
{
	return rect(0, 0, win->area.width, win->area.height);
}

Rect app_get_window_rect(const Window *win)
{
	return rect(0, 0, win->area.width, win->area.height);
}


/*
 *  Set event handlers:
 */
void app_on_window_close(Window *win, WindowFunc close)
{
	win->close = (WindowFunc *)
		app_add_array_element((void **) win->close, close);
}

void app_on_window_move(Window *win, WindowFunc move)
{
	win->move = (WindowFunc *)
		app_add_array_element((void **) win->move, move);
}

void app_on_window_resize(Window *win, WindowFunc resize)
{
	win->resize = (WindowFunc *)
		app_add_array_element((void **) win->resize, resize);
}

void app_on_window_redraw(Window *win, WindowDrawFunc redraw)
{
	win->redraw = (WindowDrawFunc *)
		app_add_array_element((void **) win->redraw, redraw);
}

void app_on_window_mouse_down(Window *win, WindowMouseFunc mouse_down)
{
	win->mouse_down = (WindowMouseFunc *)
		app_add_array_element((void **) win->mouse_down, mouse_down);
}

void app_on_window_mouse_up(Window *win, WindowMouseFunc mouse_up)
{
	win->mouse_up = (WindowMouseFunc *)
		app_add_array_element((void **) win->mouse_up, mouse_up);
}

void app_on_window_mouse_drag(Window *win, WindowMouseFunc mouse_drag)
{
	win->mouse_drag = (WindowMouseFunc *)
		app_add_array_element((void **) win->mouse_drag, mouse_drag);
}

void app_on_window_mouse_move(Window *win, WindowMouseFunc mouse_move)
{
	win->mouse_move = (WindowMouseFunc *)
		app_add_array_element((void **) win->mouse_move, mouse_move);
}

void app_on_window_key_down(Window *win, WindowKeyFunc key_down)
{
	win->key_down = (WindowKeyFunc *)
		app_add_array_element((void **) win->key_down, key_down);
}

void app_on_window_key_action(Window *win, WindowKeyFunc key_action)
{
	win->key_action = (WindowKeyFunc *)
		app_add_array_element((void **) win->key_action, key_action);
}

void app_set_window_background(Window *w, Colour col)
{
	w->bg = col;
	app_redraw_window(w);
}

Colour app_get_window_background(Window *w)
{
	return w->bg;
}

void app_set_window_data(Window *win, void *data)
{
	win->data = data;
}

void * app_get_window_data(Window *win)
{
	return win->data;
}

/*
 *  Utility functions:
 */
void app_hide_all_windows(App *app)
{
	int i;

	for (i=app->num_windows-1; i >= 0; i--)
		app_hide_window(app->windows[i]);
}

void app_del_all_windows(App *app)
{
	int i;

	for (i=app->num_windows-1; i >= 0; i--)
		app_del_window(app->windows[i]);
}
