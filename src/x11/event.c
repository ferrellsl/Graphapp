/*
 *  Event handling routines.
 *
 *  Platform: X-Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Added event passing for controls.
 *  Version: 3.02  2001/10/10  Added clipboard and selection support.
 *  Version: 3.09  2001/11/13  Fixed disabled controls, added keypad keys.
 *  Version: 3.11  2001/12/12  Added support for menu bar shortcut keys.
 *  Version: 3.17  2002/01/08  Added support for timers.
 *  Version: 3.19  2002/02/15  Added Unicode key compositions.
 *  Version: 3.26  2002/07/31  Added window movement call-back.
 *  Version: 3.30  2002/08/25  Modified timer event dispatch.
 *  Version: 3.31  2002/08/26  Modified timer event dispatch again.
 *  Version: 3.34  2002/12/18  Improved timer support.
 *  Version: 3.35  2002/12/23  Renamed winlist and active_timers.
 *  Version: 3.37  2002/12/31  Sends CONTROL/SHIFT key bits to key_action.
 *  Version: 3.41  2003/03/20  Modal windows, and changed get_mouse_event.
 *  Version: 3.43  2003/04/25  Modal windows now use app->modals stack.
 *  Version: 3.45  2003/05/12  Control key combinations now use uppercase.
 *  Version: 3.47  2003/05/28  Menus now see control-key events first.
 *  Version: 3.50  2004/01/18  Events may be sent to multiple observers.
 *  Version: 3.51  2004/03/28  Supports delayed-deletion.
 *  Version: 3.57  2005/08/16  Added app_process_events.
 *  Version: 3.60  2007/06/06  Improved timer handling using poll.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"
#include <poll.h>	//!!
#include <errno.h>

/*
 *  Mouse events call various window functions.
 *  The main thing done here is to transform the button state *change*
 *  into the current button *state*.
 */
static int app_mouse_motion_buttons(int state)
{
	int buttons = 0;

	if (state & Button1Mask) {
		if (state & ShiftMask)
			buttons |= 4;
		if (state & ControlMask)
			buttons |= 2;
		if (state & Mod1Mask)
			buttons |= 1;
		if (! buttons)
			buttons |= 1;
	}
	if (state & Button2Mask)
		buttons |= 2;
	if (state & Button3Mask)
		buttons |= 4;

	return buttons;
}

static void app_do_mouse_motion(Window *win, int state, int x, int y)
{
	int i;
	int buttons;
	Point p;
	Control *c;

	p.x = x;
	p.y = y;
	buttons = app_mouse_motion_buttons(state);

	c = app_locate_control(win, p);

	i = win->flags & TEMP_CURSOR;	//!!
	if (c && c->cursor) {
		if (c->cursor != win->cursor)
			app_set_window_temp_cursor(win, c->cursor);
	}
	else if (i != 0)
		app_set_window_temp_cursor(win, NULL);

	if (c && (c->state & TIP_MASK))	//!!
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

static int app_mouse_press_buttons(int state, int pressed)
{
	int buttons, prior;

	/* determine state of buttons just prior */
	prior = 0;
	if (state & Button1Mask) {
		if (state & ShiftMask)
			prior |= 4;
		if (state & ControlMask)
			prior |= 2;
		if (state & Mod1Mask)
			prior |= 1;
		if (! prior)
			prior |= 1;
	}
	if (state & Button2Mask)
		prior |= 2;
	if (state & Button3Mask)
		prior |= 4;

	/* now apply the change */
	buttons = prior;
	if (pressed == Button1) {
		if (state & ShiftMask)
			buttons |= 4;
		if (state & ControlMask)
			buttons |= 2;
		if (state & Mod1Mask)
			buttons |= 1;
		if (! buttons)
			buttons |= 1;
	}
	else if (pressed == Button2)
		buttons |= 2;
	else if (pressed == Button3)
		buttons |= 4;

	if (buttons == prior)
		return -1;	/* no change */
	return buttons;
}

static void app_do_mouse_press(Window *win, int state, int x, int y,
	int pressed)
{
	int i;
	int buttons;
	Point p;
	Control *c;

	p.x = x;
	p.y = y;

	buttons = app_mouse_press_buttons(state, pressed);

	if (buttons != -1)
	{
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
}

static int app_mouse_release_buttons(int state, int released)
{
	int buttons, prior;

	/* determine state of buttons just prior */
	prior = 0;
	if (state & Button1Mask) {
		if (state & ShiftMask)
			prior |= 4;
		if (state & ControlMask)
			prior |= 2;
		if (state & Mod1Mask)
			prior |= 1;
		if (! prior)
			prior |= 1;
	}
	if (state & Button2Mask)
		prior |= 2;
	if (state & Button3Mask)
		prior |= 4;

	/* now apply the change */
	buttons = 0;
	if (released != Button1)
		if (state & Button1Mask) {
			if (state & ShiftMask)
				buttons |= 4;
			if (state & ControlMask)
				buttons |= 2;
			if (state & Mod1Mask)
				buttons |= 1;
			if (! buttons)
				buttons |= 1;
		}
	if (released != Button2)
		if (state & Button2Mask)
			buttons |= 2;
	if (released != Button3)
		if (state & Button3Mask)
			buttons |= 4;

	if (buttons == prior)
		return -1;	/* no change */
	return buttons;
}

static void app_do_mouse_release(Window *win, int state, int x, int y,
	int released)
{
	int i;
	int buttons;
	Point p;
	Control *c;

	p.x = x;
	p.y = y;
	buttons = app_mouse_release_buttons(state, released);

	if (buttons != -1)
	{
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
}

/*
 *  Convert XK_ X keyboard constants into Unicode characters
 *  or key actions. Unicode characters are sent to the key_down
 *  function of the window, while arrow keys, home, end, etc are
 *  sent to the key_action function. These two are kept separate
 *  because we don't wish to confuse things by just mapping
 *  arrow keys onto Unicode and sending them all to the same
 *  fuction; that would be bad news, because we'd have no way
 *  to distinguish between the user inputting a Unicode char,
 *  and using an arrow key, for instance.
 *
 *  Note: we do map the arrow keys etc onto sensible-seeming
 *  Unicode values, even though they are sent to a different
 *  function. This is incidental and should not be relied upon.
 *  They could have just as easily been mapped onto -1, -2, -3 etc.
 *
 *  The X to Unicode mapping was provided by the folks at XFree86.
 *  Eventually, we can one day hope that Unicode will be the
 *  default key mapping arrangement inside X, and UTF-8 the
 *  default text transfer encoding between X applications, but
 *  until then, we're stuck with the large look-up table found
 *  in keys2ucs.c. It's based on keysym2ucs.c from XFree86,
 *  with the header file removed and function name changed.
 */
static void app_translate_key(Window *win, KeySym key, int shft, int ctrl)
{
	unsigned long value;
	int pass_to = 0; /* 1 is == Unicode key_down, 2 is key_action */

	/* normal keyboard chars get passed to the key_down handler */
	if ((key >= 0x0020 && key <= 0x007e) ||
		(key >= 0x00a0 && key <= 0x00ff))
	{
		value = key;
		pass_to = 1;	/* pass to key_down handler */
	}

	/* other things defined by ASCII get passed there too */
	else if ((key >= XK_BackSpace) && (key <= XK_Escape)) {
		value = (key & 0x00FF); /* mask */
		pass_to = 1;	/* pass to key_down handler */
	}

	/* translate keypad numbers into ASCII digits */
	else if ((key >= XK_KP_0) && (key <= XK_KP_9)) {
		value = key - XK_KP_0 + '0';
		pass_to = 1;	/* pass to key_down handler */
	}

	/* translate other keypad keys into ASCII values */
	else if (key == XK_KP_Space) {
		value = ' ';
		pass_to = 1;	/* pass to key_down handler */
	}
	else if (key == XK_KP_Tab) {
		value = '\t';
		pass_to = 1;	/* pass to key_down handler */
	}
	else if (key == XK_KP_Enter) {
		value = '\n';
		pass_to = 1;	/* pass to key_down handler */
	}
	else if (key == XK_KP_Equal) {
		value = '=';
		pass_to = 1;	/* pass to key_down handler */
	}
	else if (key == XK_KP_Multiply) {
		value = '*';
		pass_to = 1;	/* pass to key_down handler */
	}
	else if (key == XK_KP_Add) {
		value = '+';
		pass_to = 1;	/* pass to key_down handler */
	}
	else if (key == XK_KP_Subtract) {
		value = '-';
		pass_to = 1;	/* pass to key_down handler */
	}
	else if (key == XK_KP_Decimal) {
		value = '.';
		pass_to = 1;	/* pass to key_down handler */
	}
	else if (key == XK_KP_Divide) {
		value = '/';
		pass_to = 1;	/* pass to key_down handler */
	}

	/* translate arrow key combinations into Unicode arrow symbols */
	else if ((key >= XK_Left) && (key <= XK_Down)) {
		value = key - XK_Left + LEFT;
		pass_to = 2;	/* pass to key_action handler */
	}

	/* translate function keys into Unicode circled numbers */
	else if ((key >= XK_F1) && (key <= XK_F10)) {
		value = key - XK_F1 + F1;
		pass_to = 2;	/* pass to key_action handler */
	}
	else if ((key >= XK_KP_F1) && (key <= XK_KP_F4)) {
		value = key - XK_KP_F1 + F1;
		pass_to = 2;	/* pass to key_action handler */
	}

	/* translate other keyboard keys into 'Unicode equivalents' */
	else if (key == XK_Prior) {
		value = PGUP;
		pass_to = 2;	/* pass to key_action handler */
	}
	else if (key == XK_Next) {
		value = PGDN;
		pass_to = 2;	/* pass to key_action handler */
	}
	else if (key == XK_End) {
		value = END;
		pass_to = 2;	/* pass to key_action handler */
	}
	else if (key == XK_Home) {
		value = HOME;
		pass_to = 2;	/* pass to key_action handler */
	}
	else if (key == XK_Insert) {
		value = INS;
		pass_to = 2;	/* pass to key_action handler */
	}
	else if (key == XK_Delete) {
		value = DEL;
		pass_to = 2;	/* pass to key_action handler */
	}

	/* lastly search a table to convert to a Unicode character */
	else {
		value = app_keysym_to_ucs(key); /* see keys2ucs.c */
		if (value != -1)
			pass_to = 1;	/* pass to key_down handler */
	}

	/* now signal Ctrl and Shift key combinations */
	if (ctrl) {
		if (pass_to)
			pass_to = 2;	/* pass to key_action handler */
		value |= CONTROL;
	}
	if ((pass_to == 2) && (shft))
		value |= SHIFT;

	app_send_key_value(win, value, pass_to);
}

static void app_do_key_press(Window *win, XKeyEvent *xkey)
{
	char s[20];
	int i;
	KeySym key;
	int shft, ctrl, alt;

	/* remember Ctrl key clicks, but factor them out */
	shft = (xkey->state & ShiftMask);
	ctrl = (xkey->state & ControlMask);
	alt  = (xkey->state & Mod1Mask);
	if (ctrl) {
		xkey->state &= ~ControlMask;
		xkey->state |=  ShiftMask; /* so Ctrl-X is upper case */
	}

	/* make key contain an XK_* keycode */
	i = XLookupString(xkey, s, sizeof(s), &key,
			&app_extra(win->app)->xcompose);
	s[i] = '\0';

	if ((key == XK_Shift_L) || (key == XK_Shift_R)
	 || (key == XK_Control_L) || (key == XK_Control_R)
	 || (key == XK_Caps_Lock) || (key == XK_Caps_Lock)
	 || (key == XK_Alt_L) || (key == XK_Alt_R))
		return;

	/* translate '\r' into '\n' */
	if (key == XK_Return)
		key = XK_Linefeed;

	/* handle Alt key presses as Unicode compositions */
	if (app_do_alt_key_down(win, key, alt))
		return;

	app_translate_key(win, key, shft, ctrl);
}

/*
 *  The user just tried to close the window:
 */
static void app_do_close_window(Window *win)
{
	int i;

	if (win->close)
		for (i=0; win->close[i]; i++)
			win->close[i](win);
	else
		app_hide_window(win);
}

/*
 *  The user has resized the window:
 */
static void app_do_resize_window(Window *win, int x, int y, int w, int h)
{
	int i;

	if ((win->area.x != x) || (win->area.y != y)) {
		win->area.x = x;
		win->area.y = y;
		if (win->move)
			for (i=0; win->move[i]; i++)
				win->move[i](win);
	}
	if ((win->area.width != w) || (win->area.height != h)) {
		win->area.width = w;
		win->area.height = h;
		if (win->menubar)
			win->menubar->ctrl->area.width = w;
		app_place_window_controls(win, 1);
		if (win->resize)
			for (i=0; win->resize[i]; i++)
				win->resize[i](win);
	}
}

/*
 *  Some part(s) of the window have been exposed and now need to
 *  be redrawn. Draw the window first, then all child controls,
 *  from back- to front-most.
 */
static void app_do_redraw_window(Window *win)
{
	int i;
	Graphics *g;

	g = app_get_window_graphics(win);

	app_set_rgb(g, win->bg);
	app_fill_rect(g, rect(0, 0, win->area.width, win->area.height));
	app_set_rgb(g, BLACK);

	if (win->redraw)
		for (i=0; win->redraw[i]; i++)
			win->redraw[i](win, g);
	app_do_draw_controls(g, win->num_children, win->children, 1);

	app_del_graphics(g);
}

static void app_union_redraw_region(Window *win, int x, int y, int w, int h)
{
	Rect r;

	if (! win->redraw_rgn)
		win->redraw_rgn = app_new_region();
	if (win->redraw_rgn) {
		r.x = x;
		r.y = y;
		r.width = w;
		r.height = h;
		app_union_region_with_rect(win->redraw_rgn, r,
			win->redraw_rgn);
	}
}

static void app_del_redraw_region(Window *win)
{
	if (win->redraw_rgn)
		app_del_region(win->redraw_rgn);
	win->redraw_rgn = NULL;
}

void app_redraw_rect(Window *win, Rect r)
{
	if (! (win->state & VISIBLE))
		return;

	app_union_redraw_region(win, r.x, r.y, r.width, r.height);
	app_do_redraw_window(win);
	app_del_redraw_region(win);
}

static void app_do_portable_timers(App *app)	//!!
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

static int app_do_timers(App *app)	//!!
{
	if (app->num_timers == 0)
		return 0;
	while (app_peek_event(app))
		app_do_event(app);
#ifdef USE_ALARM
	if (app_extra(app)->timer_id)
		return 0;
#endif
	app_delay(app, TIMER_INTERVAL);
	app_do_portable_timers(app);
	app_do_delayed_deletion(app);	//!!
	return 1;
}

/*
 *  This is the function which handles all X-Windows events as
 *  they arrive, and dispatches them to the approprite functions.
 */
static void app_winproc(App *app, XEvent *e)
{
	int i, modal_in_front;
	Window *w;
	Window *win;

	/* Find window to which this message is directed */
	win = NULL;
	for (i=0; i < app->num_windows; i++) {
		w = app->windows[i];
		if (win_extra(w)->xid == e->xany.window) {
			win = w;
			break;
		}
	}
	if (! win)
		return; /* not found */

	/* Check if a modal window is in front of this window */
	if ((app->num_modals > 0) &&
	    (app->modals[app->num_modals-1] != win))
		modal_in_front = 1;
	else
		modal_in_front = 0;

	/* Remember request number in case we need it */
	app_extra(app)->last_event_number = e->xany.serial;

	if (win_extra(win)->exposed) {
		/* need to handle GraphicsExpose events before all else */
		/* otherwise scrolling a window could go horribly wrong */
		win_extra(win)->exposed -= 1; /* to avoid infinite loop */
		if ((e->type == GraphicsExpose) || (e->type == NoExpose))
			win_extra(win)->exposed = 0;
		else
			return;	/* ignore all other kinds of events */
	}

	switch (e->type)
	{
	  case MotionNotify:
		if (modal_in_front)
			return;
		app_extra(app)->last_event_time = e->xmotion.time;
		app_do_mouse_motion(win, e->xmotion.state,
			e->xmotion.x, e->xmotion.y);
		return;

	  case ButtonPress:
		if (modal_in_front)
			return;
		app_extra(app)->last_event_time = e->xbutton.time;
		app_do_mouse_press(win, e->xbutton.state,
			e->xbutton.x, e->xbutton.y, e->xbutton.button);
		return;

	  case ButtonRelease:
		if (modal_in_front)
			return;
		app_extra(app)->last_event_time = e->xbutton.time;
		app_do_mouse_release(win, e->xbutton.state,
			e->xbutton.x, e->xbutton.y, e->xbutton.button);
		return;

	  case KeyPress:
		if (modal_in_front)
			return;
		app_extra(app)->last_event_time = e->xkey.time;
		app_do_key_press(win, &e->xkey);
		return;

	  case KeyRelease:
		if (modal_in_front)
			return;
		app_extra(app)->last_event_time = e->xkey.time;
		return;

	  case FocusIn:
		return;

	  case FocusOut:
		return;

	  case MappingNotify:
		XRefreshKeyboardMapping(&e->xmapping);
		return;
	/*
	  case ResizeRequest:
		app_do_resize_window(win,
			win->area.x,
			win->area.y,
			e->xresizerequest.width,
			e->xresizerequest.height);
		return;
	*/
	  case ConfigureNotify:
		app_do_resize_window(win,
			e->xconfigurerequest.x,
			e->xconfigurerequest.y,
			e->xconfigurerequest.width,
			e->xconfigurerequest.height);
		return;

	  case GraphicsExpose:
	  case Expose:
		app_union_redraw_region(win,
			e->xexpose.x, e->xexpose.y,
			e->xexpose.width, e->xexpose.height);
		if (e->xexpose.count == 0) {
			app_do_redraw_window(win);
			app_del_redraw_region(win);
		}
		return;

	  case ClientMessage:
		if (((Atom *) &e->xclient.data)[0] == win_extra(win)->xdel)
			app_do_close_window(win);
		return;

	  case DestroyNotify:
		app_do_close_window(win);
		return;

	  case SelectionRequest:
		app_extra(app)->last_event_time = e->xselectionrequest.time;
		app_send_clipboard(app, &e->xselectionrequest);
		return;

	  case SelectionClear:
		app_extra(app)->last_event_time = e->xselectionclear.time;
		return;
	/*
	  case SelectionNotify:
		app_extra(app)->last_event_time = e->xselection.time;
		XFree(app_receive_clipboard(app, &e->xselection, NULL));
		return;
	*/
	  default:
		break;
	}
}

/*
 *  This function handles mouse events, flagging all others.
 *  It is only used by the special app_get_mouse_event function.
 *  It returns:
 *	-1 if the event can be ignored
 *	 0 if the event is a keyboard or redraw event
 *	 1 if the event is a mouse event
 */
static int app_get_mouse_events(App *app, XEvent *e, int *btns, Point *xy)
{
	int i;
	Window *w;
	Window *win;
	int buttons;

	/* Find window to which this message is directed */
	win = NULL;
	for (i=0; i < app->num_windows; i++) {
		w = app->windows[i];
		if (win_extra(w)->xid == e->xany.window) {
			win = w;
			break;
		}
	}
	if (! win)
		return -1; /* window not found; ignore this event */

	switch (e->type)
	{
	  case MotionNotify:
		app_extra(app)->last_event_time = e->xmotion.time;
		xy->x = e->xmotion.x;
		xy->y = e->xmotion.y;
		buttons = app_mouse_motion_buttons(e->xmotion.state);
		if (buttons != -1)
			*btns = buttons;
		return 1;

	  case ButtonPress:
		app_extra(app)->last_event_time = e->xbutton.time;
		xy->x = e->xbutton.x;
		xy->y = e->xbutton.y;
		buttons = app_mouse_press_buttons(e->xbutton.state,
				e->xbutton.button);
		if (buttons != -1)
			*btns = buttons;
		return 1;

	  case ButtonRelease:
		app_extra(app)->last_event_time = e->xbutton.time;
		xy->x = e->xbutton.x;
		xy->y = e->xbutton.y;
		buttons = app_mouse_release_buttons(e->xbutton.state,
				e->xbutton.button);
		if (buttons != -1)
			*btns = buttons;
		if (buttons == 0)
			win->mouse_grab = NULL;
		return 1;

	  case KeyPress:
	  case KeyRelease:
		return 0; /* keyboard event; keep it */

	  case GraphicsExpose:
	  case Expose:
		return 0; /* draw event; keep it */

	  case MappingNotify:
	  case ConfigureNotify:
	  case ClientMessage:
	  case DestroyNotify:
	  case SelectionRequest:
	  case SelectionClear:
	  case SelectionNotify:
		return 0; /* window event; keep it */

	  case FocusIn:
	  case FocusOut:
		return -1; /* focus event; ignore it */

	  default:
		break;
	}

	return -1; /* other event; ignore it */
}

/*
 *  Return 1 if a mouse event is found, 0 otherwise.
 *  Absorbing all events which can be ignored.
 */
int app_get_mouse_event(App *app, int *buttons, Point *p)
{
	XEvent event;
	int kind;

	*buttons = 0;

	while (app->visible_windows > 0) {
		XPeekEvent(app_extra(app)->display, &event);
		kind = app_get_mouse_events(app, &event, buttons, p);

		if (kind != 0) {
			/* keyboard, draw events remain, else absorb */
			XNextEvent(app_extra(app)->display, &event);
		}

		if (kind >= 0) {
			/* mouse, keyboard, draw events break */
			return kind;
		}
	}
	return 0;
}

/*
 *  Handle all events until there are no more windows:
 */
void app_main_loop(App *app)
{
	while (app_wait_event(app))
		continue;
}

int app_wait_event(App *app)
{
	XEvent event;
	int result = 0;

	if (app_do_timers(app))
			return 1;

	if (app->visible_windows > 0) {
#ifdef USE_ALARM	//!!
		for (;;) {
			struct pollfd fdinfo;

			XFlush(app_extra(app)->display);
			fdinfo.fd = app->socket_fd;
			fdinfo.events = POLLIN;

			result = poll(&fdinfo, 1, -1);

			if ((result == -1) && (errno = EINTR))
				app_do_portable_timers(app);
			else
				break;
		}
#endif
		XNextEvent(app_extra(app)->display, &event);
		result = 1;
		app_winproc(app, &event);
	}

	app_do_delayed_deletion(app);

	return result;
}

int app_process_events(App *app)
{
	XEvent event;
	int result = 0;

	if (app->num_timers != 0)	//!!
		app_do_portable_timers(app);

	if (app->visible_windows > 0) {
		Display *disp = app_extra(app)->display;
		while (XPending(disp)) {
			XNextEvent(disp, &event);
			app_winproc(app, &event);
		}
		result = 1;
	}

	app_do_delayed_deletion(app);

	return result;
}

int app_do_event(App *app)
{
	XEvent event;
	int result = 0;

	if (app->visible_windows > 0) {
		if (app_peek_event(app)) {
			XNextEvent(app_extra(app)->display, &event);
			result = 1;
			app_winproc(app, &event);
		}
	}
	return result;
}

int app_peek_event(App *app)
{
	int i;

	for (i=0; i < app->num_windows; i++) {
		if (app->windows[i]->redraw_rgn != NULL)
			return 1;
	}
	if (app->visible_windows > 0)
		if (XPending(app_extra(app)->display))
			return 1;
	return 0;
}

void app_draw_all(App *app)
{
	XSync(app_extra(app)->display, 0);
}
