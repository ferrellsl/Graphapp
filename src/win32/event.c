/*
 *  Event handling routines.
 *
 *  Platform: Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/10/01  Changed redrawing slightly.
 *  Version: 3.09  2001/11/13  Fixed disabled controls, fixed keypad keys.
 *  Version: 3.11  2001/12/12  Added support for menu bar shortcut keys.
 *  Version: 3.14  2001/12/17  Fixed bug in Ctrl key handling.
 *  Version: 3.17  2002/01/08  Added support for timers.
 *  Version: 3.19  2002/02/15  Added Unicode key compositions.
 *  Version: 3.20  2002/02/18  Fixed numeric keypad problems.
 *  Version: 3.26  2002/07/31  Added window movement call-back.
 *  Version: 3.28  2002/08/16  Added Windows timer support.
 *  Version: 3.34  2002/12/18  Improved timer support.
 *  Version: 3.35  2002/12/23  Renamed winlist and active_timers.
 *  Version: 3.37  2002/12/31  Sends CONTROL/SHIFT key bits to key_action.
 *  Version: 3.41  2003/03/20  Modal windows, mouse capture, get_mouse_event.
 *  Version: 3.42  2003/04/09  Timer events ignored by get_mouse_event.
 *  Version: 3.43  2003/04/25  Added modal window stack.
 *  Version: 3.45  2003/05/12  Finally fixed Control-key menu bug.
 *  Version: 3.47  2003/05/28  Menus now see control-key events first.
 *  Version: 3.50  2004/01/18  Events may be sent to multiple observers.
 *  Version: 3.51  2004/03/28  Supports delayed-deletion.
 *  Version: 3.56  2005/08/09  Silenced some WPARAM conversion warnings.
 *  Version: 3.57  2005/08/16  Added app_process_events, TEMP_CURSORs, VK_TAB.
 *  Version: 3.60  2007/06/06  Timers, tool-tips, temp cursors.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

static int app_get_button_state(int wParam)
{
	int buttons = 0;

	if (wParam & MK_LBUTTON) {
		if (wParam & MK_CONTROL)
			buttons |= 4;
		if (wParam & MK_SHIFT)
			buttons |= 2;
		if (buttons == 0)
			buttons |= 1;
	}
	if (wParam & MK_MBUTTON)
		buttons |= 2;
	if (wParam & MK_RBUTTON)
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
	if (c && c->cursor) {	//!!
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

/*
 *  Handle WM_KEYDOWN events, which occur prior to being
 *  translated into WM_CHAR events, and hence can contain
 *  Ctrl key combinations and cursor keys, functions keys, etc.
 *  Returns 0 if the event wasn't handled, non-zero otherwise.
 */
static int app_do_key_click(Window *win, unsigned long ch, int shft, int ctrl)
{
	int pass_to = 0;

	if (win == NULL)
		return 0;

	switch (ch)
	{
		case VK_SHIFT:
		case VK_LSHIFT:
		case VK_RSHIFT:
		case VK_CONTROL:
		case VK_LCONTROL:
		case VK_RCONTROL:
		case VK_MENU:
		case VK_LMENU:
		case VK_RMENU:
			return 0;

		case VK_TAB:      pass_to = 2; break;	//!!

		case VK_INSERT:   ch = INS;   pass_to = 2; break;
		case VK_DELETE:   ch = DEL;   pass_to = 2; break;
		case VK_HOME:     ch = HOME;  pass_to = 2; break;
		case VK_END:      ch = END;   pass_to = 2; break;
		case VK_PRIOR:    ch = PGUP;  pass_to = 2; break;
		case VK_NEXT:     ch = PGDN;  pass_to = 2; break;

		case VK_LEFT:     ch = LEFT;  pass_to = 2; break;
		case VK_UP:       ch = UP;    pass_to = 2; break;
		case VK_RIGHT:    ch = RIGHT; pass_to = 2; break;
		case VK_DOWN:     ch = DOWN;  pass_to = 2; break;

		default: break;
	}

	if ((ch >= VK_F1) && (ch <= VK_F10)) {
		ch = ch + F1 - VK_F1;
		pass_to = 2;
	}

	if (ctrl) {
		pass_to = 2;
		ch |= CONTROL;
	}
	if ((pass_to == 2) && (shft))
		ch |= SHIFT;

	return app_send_key_value(win, ch, pass_to);
}

static void app_do_close_window(HWND hwnd, Window *win)
{
	int i;

	if (win) {
		if (win->close)
			for (i=0; win->close[i]; i++)
				win->close[i](win);
		else
			app_hide_window(win);
	}
	else
		ShowWindow(hwnd, SW_HIDE);
}

static void app_fix_window_area(Window *win)
{
	RECT r;
	POINT p;
	Rect area;

	if (! (win->state & VISIBLE))
		return;
	if (IsIconic(win_extra(win)->hwnd))
		return;

	GetClientRect(win_extra(win)->hwnd, &r);
	area.x = p.x = r.left;
	area.y = p.y = r.top;
	area.width = r.right - r.left;
	area.height = r.bottom - r.top;
	win->area = area;
	ClientToScreen(win_extra(win)->hwnd, &p);
	win->area.x = p.x;
	win->area.y = p.y;
}

static void app_do_move_window(Window *win)
{
	int i;

	if (win) {
		app_fix_window_area(win);
		if (win->move)
			for (i=0; win->move[i]; i++)
				win->move[i](win);
	}
}

static void app_do_resize_window(Window *win)
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

static void app_do_redraw_window(HWND hwnd, Window *win)
{
	int i;
	HDC dc;
	PAINTSTRUCT ps;
	HPALETTE oldpal = 0;
	Graphics *g;

	if (win == NULL) {
		dc = BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
		return;
	}

	dc = BeginPaint(hwnd, &ps);

	if (win->pal) {
		oldpal = SelectPalette(dc, win_extra(win)->winpal, 0);
		RealizePalette(dc);
	}

	win->redraw_rgn = app_new_rect_region(
			rect(ps.rcPaint.left, ps.rcPaint.top,
			ps.rcPaint.right - ps.rcPaint.left,
			ps.rcPaint.bottom - ps.rcPaint.top));
	g = app_get_window_redraw(dc, win);
	app_set_rgb(g, win->bg);
	app_fill_rect(g, rect(0, 0, win->area.width, win->area.height));
	app_set_rgb(g, BLACK);
	if (win->redraw)
		for (i=0; win->redraw[i]; i++)
			win->redraw[i](win, g);
	app_do_draw_controls(g, win->num_children, win->children, 1);
	app_del_graphics(g);
	app_del_region(win->redraw_rgn);
	win->redraw_rgn = NULL;

	if (win->pal)
		SelectPalette(dc, oldpal, 0);

	EndPaint(hwnd, &ps);
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
	if (app_extra(app)->timer_id)
		return 0;
	app_delay(app, TIMER_INTERVAL);
	app_do_portable_timers(app);
	app_do_delayed_deletion(app);	//!!
	return 1;
}

static void app_stop_all_timers(App *app)
{
	while (app->num_timers)
		app_del_timer(app->timers[0]);
}

static void app_do_shutdown(Window *win)
{
	if (win) {
		if (win->app) {
			app_hide_all_windows(win->app);
			app_stop_all_timers(win->app);
		}
	}
}

/*
 *  Return 1 if a modal window is in front of this window, else 0.
 */
static int app_modal_in_front(Window *win)
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

LRESULT FAR PASCAL _export
app_winproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Window *win = NULL;

	if (GetWindowLongPtr(hwnd, GWLP_WNDPROC) == (LONG_PTR) app_winproc)
	{
		/* must be an App window */
		win = (Window *) GetWindowLongPtr(hwnd, 0);
	}

	if (win == NULL)
		return DefWindowProc(hwnd, msg, wParam, lParam);

	switch (msg)
	{
	  case WM_MOUSEMOVE:
		if (app_modal_in_front(win))
			return 0;
		app_do_mouse_move(win, app_get_button_state((int) wParam),
					LOWORD(lParam), HIWORD(lParam));
		return 0;

	  case WM_LBUTTONDOWN:
	  case WM_MBUTTONDOWN:
	  case WM_RBUTTONDOWN:
		if (app_modal_in_front(win))
			return 0;
		SetCapture(hwnd);
		app_do_mouse_down(win, app_get_button_state((int) wParam),
					LOWORD(lParam), HIWORD(lParam));
		return 0;

	  case WM_LBUTTONUP:
	  case WM_MBUTTONUP:
	  case WM_RBUTTONUP:
		if (! (wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON))) //!!
			ReleaseCapture();
		if (app_modal_in_front(win))
			return 0;
		app_do_mouse_up(win, app_get_button_state((int) wParam),
					LOWORD(lParam), HIWORD(lParam));
		return 0;

	  case WM_KEYDOWN:
		if (app_modal_in_front(win))
			return 0;
		if (app_do_key_click(win, (unsigned long) wParam,
			GetKeyState(VK_SHIFT) < 0,
			GetKeyState(VK_CONTROL) < 0))
			return 0;
		break;

	  case WM_CHAR:
		if (app_modal_in_front(win))
			break;
		if (GetKeyState(VK_CONTROL) < 0)
			break;
		if (app_do_alt_key_down(win, (unsigned long) wParam, 0))
			break;
		app_do_key_down(win, (unsigned long) wParam);
		break;

	  case WM_SYSCHAR:
		if (wParam == ' ') /* allow Alt-Space to go to Windows */
			break;
		if (app_modal_in_front(win))
			return 0;
		app_do_alt_key_down(win, (unsigned long) wParam, 1);
		return 0; /* to prevent Windows beeping at us */

	  case WM_PALETTECHANGED:
		if ((HWND) wParam == hwnd)
			return 0;
		return app_realize_palette(win);

	  case WM_QUERYNEWPALETTE:
		return app_realize_palette(win);

	  case WM_SETFOCUS:
		app_realize_palette(win);
		if (win->flags & TEMP_CURSOR)	//!!
			SetCursor((HCURSOR) cursor_extra(win->cursor)->cursor);
		return 0;

	  case WM_MOVE:
		if (! (win->state & VISIBLE))
			break;
		app_do_move_window(win);
		return 0;

	  case WM_SIZE:
		if (! (win->state & VISIBLE))
			break;
		if ((wParam == SIZE_RESTORED) ||
			(wParam == SIZE_MINIMIZED) ||
			(wParam == SIZE_MAXIMIZED))
		{
			app_do_resize_window(win);
			return 0;
		}
		break;

	  case WM_PAINT:
		if (! (win->state & VISIBLE))
			break;
		app_do_redraw_window(hwnd, win);
		return 0;

	  case WM_ERASEBKGND:
		return 1;

	  case WM_TIMER:
		if (wParam == 1)
			app_do_portable_timers(win->app);
		return 0;

	  case WM_NCHITTEST:	//!!
		if (app_modal_in_front(win))
			return HTNOWHERE;
		/* delete temp. cursor on window borders */
		if (win->flags & TEMP_CURSOR) {
			int x, y;
			x = LOWORD(lParam) - win->area.x;
			y = HIWORD(lParam) - win->area.y;
			if (x < 0 || y < 0 || x >= win->area.width
				|| y >= win->area.height)
				app_set_window_temp_cursor(win, NULL);
		}
		break;

	  case WM_ACTIVATE:	//!!
		/* Keep active of top modal window */
		if (LOWORD (wParam) != WA_INACTIVE)
		{
			App *app = win->app;
			if (app->num_modals != 0) {
				HWND hw = win_extra(
					app->modals[app->num_modals-1])->hwnd;
				if (hw != hwnd) {
					PostMessage(hw, WM_ACTIVATE, 1, 0);
					return 0;
				}
			}
			break;
		}
		if (! (win->flags & POPUP)
			|| hwnd == GetParent((HWND) lParam))
			break;
		/* hide parent popup-windows */
		for (;;) {
			app_do_close_window(hwnd, win);
			hwnd = GetParent(hwnd);
			if (GetWindowLongPtr(hwnd, GWLP_WNDPROC) != (LONG_PTR) app_winproc)
				break;
			win = (Window *) GetWindowLongPtr(hwnd, 0);
			if (! (win->flags & POPUP) || hwnd == (HWND) lParam)
				break;
		}
		return 0;

	  case WM_CLOSE:
		app_do_close_window(hwnd, win);
		return 0;

	  case WM_QUERYENDSESSION:
		return 1; /* always allow system shutdown */

	  case WM_ENDSESSION:
		if (wParam)
			app_do_shutdown(win);
		return 0;

	  default:
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

/*
 *  This function handles mouse events, flagging all others.
 *  It is only used by the special app_get_mouse_event function.
 *  It returns:
 *	-1 if the event has been handled and the poll loop can continue
 *	 0 if the event is a keyboard or redraw event (stop the loop)
 *	 1 if the event is a mouse event (stop the loop)
 */
int app_get_mouse_events(App *app, MSG *event, int *btns, Point *p)
{
	HWND hwnd = event->hwnd;
	UINT msg = event->message;
	WPARAM wParam = event->wParam;
	LPARAM lParam = event->lParam;
	int i;
	Window *w;
	Window *win;

	/* Find window to which this message is directed */
	win = NULL;
	for (i=0; i < app->num_windows; i++) {
		w = app->windows[i];
		if (win_extra(w)->hwnd == hwnd) {
			win = w;
			break;
		}
	}

	if (win != NULL)
	{
	 switch (msg)
	 {
	  case WM_LBUTTONUP:
	  case WM_MBUTTONUP:
	  case WM_RBUTTONUP:
		if (wParam == 0)
			ReleaseCapture();
		/* fall through */

	  case WM_MOUSEMOVE:
	  case WM_LBUTTONDOWN:
	  case WM_MBUTTONDOWN:
	  case WM_RBUTTONDOWN:
		if (app_modal_in_front(win))
			return -1; /* ignore mouse */
		*btns = app_get_button_state((int) wParam);
		p->x = LOWORD(lParam);
		p->y = HIWORD(lParam);
		if (*btns == 0)
			win->mouse_grab = NULL;
		return 1; /* mouse event: exit loop */

	  case WM_KEYDOWN:
	  case WM_CHAR:
	  case WM_SYSCHAR:
		if (app_modal_in_front(win))
			return -1; /* ignore keyboard */

		/* fall through */

	  case WM_PALETTECHANGED:
	  case WM_QUERYNEWPALETTE:
	  case WM_MOVE:
	  case WM_SIZE:
	  case WM_CLOSE:
	  case WM_QUERYENDSESSION:
	  case WM_ENDSESSION:
		/* keyboard or window event */
		PostMessage(hwnd, msg, wParam, lParam); /* do later */
		return 0; /* exit loop */

	  case WM_PAINT:
		DispatchMessage(event); /* process redraw now */
		return 0; /* exit loop */

	  case WM_TIMER:
		return -1; /* timer event: ignore it */

	  case WM_SETFOCUS:
	  case WM_KILLFOCUS:
		break; /* focus event: process and continue */

	  default:
		break; /* other event: process and continue */
	 }
	}

	DispatchMessage(event);
	return -1; /* other event: process and continue loop */
}

/*
 *  Loop until an event is found.
 *  Break from the loop if it is a mouse, keyboard, redraw or
 *  other important window event.
 *  Return 1 if a mouse event is found, 0 if keyboard/redraw/window.
 *  For all other events, continue looping.
 */
int app_get_mouse_event(App *app, int *buttons, Point *p)
{
	MSG event;
	int kind;

	*buttons = 0;

	while (app->visible_windows > 0) {
		if (GetMessage(&event, NULL, 0, 0)) {
			TranslateMessage(&event);
			kind = app_get_mouse_events(app, &event, buttons, p);

			if (kind >= 0) {
				/* mouse/keyboard/redraw event: break */
				return kind;
			}
		}
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
	MSG event;
	int did_event = 0;

	if (app_do_timers(app))
		return 1;

	if (app->visible_windows > 0) {
		if (GetMessage(&event, NULL, 0, 0)) {
			did_event = 1;
			TranslateMessage(&event);
			DispatchMessage(&event);
		}
	}

	app_do_delayed_deletion(app);

	return did_event;
}

int app_process_events(App *app)
{
	MSG event;
	int result = 0;

	if (app->num_timers != 0)	//!!
		app_do_portable_timers(app);

	if (app->visible_windows > 0) {
		while (PeekMessage(&event, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&event);
			DispatchMessage(&event);
		}
		result = 1;
	}

	app_do_delayed_deletion(app);

	return result;
}

int app_do_event(App *app)
{
	MSG event;
	int did_event = 0;

	if (app->visible_windows > 0) {
		if (PeekMessage(&event, NULL, 0, 0, PM_REMOVE)) {
			did_event = 1;
			TranslateMessage(&event);
			DispatchMessage(&event);
		}
	}
	return did_event;
}

int app_peek_event(App *app)
{
	MSG event;
	int i;

	for (i=0; i < app->num_windows; i++) {
		if (app->windows[i]->redraw_rgn != NULL)
			return 1;
	}
	if (app->visible_windows > 0)
		if (PeekMessage(&event, NULL, 0, 0, PM_NOREMOVE))
			return 1;
	return 0;
}

void app_draw_all(App *app)
{
	/* do nothing, since this platform does not buffer graphics */
}
