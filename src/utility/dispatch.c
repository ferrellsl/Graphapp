/*
 *  Dispatching events.
 *
 *  Platform: Neutral.
 *
 *  Version: 3.50  2004/01/18  First release (moved from event.c).
 *  Version: 3.57  2005/08/16  Added auto-tabstop handling.
 *  Version: 3.60  2007/06/06  Fixed some bugs.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"

static Control *app_next_tab_stop(Control **children,
	int i, int end, int step)
{
	Control *c;

	while ((i += step) != end) {
		c = children[i];

		if ((c->state & (ENABLED | VISIBLE)) != (ENABLED | VISIBLE))
			continue;	//!!

		if (c->state & TABSTOP)
			return c;

		if (c->num_children > 0) {
			c = app_next_tab_stop(c->children,
				(step < 0) ? c->num_children : -1, 
				(step < 0) ? -1 : c->num_children, step);
			if (c != NULL)
				return c;
		}
	}
	return NULL;
}

static int app_handle_tab_stop(Control *c, unsigned long ch)
{
	int i, end, step;
	Control *parent, **children, *next;

	parent = c->parent;

	/* search of the next control from upper level */
	if (((ch & CONTROL) != 0) && (parent != NULL)) {
		ch &= ~ CONTROL;
		c = parent;
		parent = c->parent;
	}

	if (parent != NULL) {
		children = parent->children;
		end = parent->num_children;
	} else {
		children = app_parent_window(c)->children;
		end = app_parent_window(c)->num_children;
	}

	/* current index */
	for (i=0; ; i++)
		if (children[i] == c)
			break;

	step = ! (ch & SHIFT) ? -1 : 1;
	next = app_next_tab_stop(children, i, (step < 0) ? -1 : end, step);
	if (next == NULL) {
		if (parent != NULL)
			return app_handle_tab_stop(parent, ch);
		/* else search backward */
		next = app_next_tab_stop(children,
			(step < 0) ? end : -1, i + step, step);
	}
	if (next != NULL)
		app_set_focus(next);
	return 2;
}

/*
 *  Handle a Unicode key being typed.
 *  Pass it to the correct key_down handler, or return 0 if none.
 *  Do not send it to the menu short cut handler, since
 *  we know this is not a Ctrl key combination.
 */
int app_do_key_down(Window *win, unsigned long ch)
{
	int i;
	Control *c;

	if (win == NULL)
		return 0;

	if (ch == '\r')
		ch = '\n';

	c = win->key_focus;
	win->pass_event = 0;

	while (c) {
		if (c->key_down) {
			for (i=0; c->key_down[i]; i++)
				c->key_down[i](c, ch);
			if (win->pass_event)
				win->pass_event = 0;
			else
				return 1;
		}
		c = c->parent;
	}
	if (win->key_down) {
		for (i=0; win->key_down[i]; i++)
			win->key_down[i](win, ch);
		if (win->pass_event)
			win->pass_event = 0;
		else
			return 1;
	}
	return 0;
}

/*
 *  Send a Unicode key value to the appropriate control in a window.
 *  The key event will first go to the control with focus (if any)
 *  then fall through the control hierarchy to the first control
 *  that wants key events. That control can pass the event upwards.
 *  Eventually it hits the window which either has a key handler
 *  call-back or doesn't.
 *  Returns zero if the key wasn't handled anywhere, non-zero otherwise.
 */
int app_send_key_value(Window *win, unsigned long ch, int pass_to)
{
	int i;
	Control *c;

	if (win == NULL)
		return 0;

	if (ch == '\r')
		ch = '\n';

	c = win->key_focus;
	win->pass_event = 0;

	if (( (char)ch == '\t') && (c != NULL) && (c->state & TABSTOP))
		return app_handle_tab_stop(c, ch);

	if (pass_to == 1) {
		while (c) {
			if (c->key_down) {
				for (i=0; c->key_down[i]; i++)
					c->key_down[i](c, ch);
				if (win->pass_event)
					win->pass_event = 0;
				else
					return 1;
			}
			c = c->parent;
		}
		if (win->key_down) {
			for (i=0; win->key_down[i]; i++)
				win->key_down[i](win, ch);
			if (win->pass_event)
				win->pass_event = 0;
			else
				return 1;
		}
	}
	else if (pass_to == 2) {
		if ((win->menubar) && (ch & CONTROL)) {
			if (app_activate_menu_bar_short_cut(win->menubar, ch))
			{
				if (win->pass_event)
					win->pass_event = 0;
				else
					return 2;
			}
		}
		while (c) {
			if (c->key_action) {
				for (i=0; c->key_action[i]; i++)
					c->key_action[i](c, ch);
				if (win->pass_event)
					win->pass_event = 0;
				else
					return 2;
			}
			c = c->parent;
		}
		if (win->key_action) {
			for (i=0; win->key_action[i]; i++)
				win->key_action[i](win, ch);
			if (win->pass_event)
				win->pass_event = 0;
			else
				return 2;
		}
	}

	return 0;
}

/*
 *  Handle Alt key presses as Unicode compositions.
 *  If this isn't a composition, returns 0 so another function
 *  can handle the event.
 */
int app_do_alt_key_down(Window *win, unsigned long ch, int alt)
{
	long unicode;
	int compose_key = win->app->compose_key;

	/* handle Alt key presses as Unicode compositions */

	if (compose_key) {
		unicode = app_compose_unicode(compose_key, ch);
		if (unicode >= 0) {
			/* found a composition */
			win->app->compose_key = 0; /* clear memory */
			app_do_key_down(win, unicode);
			return 1;
		}
		else if (alt) {
			/* wasn't a valid combination */
			/* so cycle new Alt-key value into memory */
			win->app->compose_key = ch;
			return 1;
		}
		else {
			/* normal key click */
			/* so clear memory, and send normal char */
			win->app->compose_key = 0;
		}
	}
	else if (alt) {
		/* begin memory */
		win->app->compose_key = ch;
		return 1;
	}
	return 0;
}

