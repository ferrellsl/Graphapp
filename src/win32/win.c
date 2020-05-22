/*
 *  Window creation routines.
 *
 *  Platform: Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/16  Updated.
 *  Version: 3.26  2002/07/31  Added window movement call-back.
 *  Version: 3.35  2002/12/23  Renamed app->winlist to app->windows.
 *  Version: 3.41  2003/03/20  Modal windows implemented.
 *  Version: 3.42  2003/03/28  Modal implies Floating, added window icons.
 *  Version: 3.43  2003/04/25  Added icons, modal stack, window classes.
 *  Version: 3.49  2003/09/09  Fixed forget_window off-by-one crash.
 *  Version: 3.50  2004/01/18  Moved many common functions to winutil.c.
 *  Version: 3.51  2004/03/28  Supports delayed-deletion.
 *  Version: 3.52  2004/03/29  Uses type-safe private extra data types.
 *  Version: 3.57  2004/03/29  Uses type-safe private extra data types.
 *  Version: 3.60  2007/06/06  Windows classes now shared. Added a function.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

/*
 *  Global window class name, and space enough to print it
 *  (even if %p pointers are written in binary).
 */
#define CLASS_NAME_PREFIX	"GraphApp"
#define CLASS_NAME_SIZE		(sizeof(CLASS_NAME_PREFIX) + 24)	//!!

/*
 *  Functions:
 */
static Rect app_actual_area(App *app, Rect area, long flags)
{
	Rect screen, frame, border;
	Rect menubar, scrollbar;

	screen.width  = GetSystemMetrics(SM_CXSCREEN);
	screen.height = GetSystemMetrics(SM_CYSCREEN);
	frame.width   = GetSystemMetrics(SM_CXFRAME);
	frame.height  = GetSystemMetrics(SM_CYFRAME);
	menubar.height   = GetSystemMetrics(SM_CYMENU) + 1;
	scrollbar.width  = GetSystemMetrics(SM_CXVSCROLL);
	scrollbar.height = GetSystemMetrics(SM_CYHSCROLL);

	if (flags & TITLEBAR) {
		int titlebar_height = GetSystemMetrics(SM_CYCAPTION);

		area.y -= titlebar_height;
		area.height += titlebar_height;

		border.width  = GetSystemMetrics(SM_CXDLGFRAME);
		border.height = GetSystemMetrics(SM_CYDLGFRAME);
	}
	else {
		border.width  = GetSystemMetrics(SM_CXBORDER);
		border.height = GetSystemMetrics(SM_CYBORDER);
	}

	if (flags & RESIZE) {
		area.x -= frame.width;
		area.y -= frame.height;
		area.width += frame.width * 2;
		area.height += frame.height * 2;
	}
	else if (! (flags & POPUP)) {
		area.x -= border.width;
		area.y -= border.height;
		area.width += border.width * 2;
		area.height += border.height * 2;
	}

	if (area.width > screen.width)
		area.width = screen.width;
	if (area.height > screen.height)
		area.height = screen.height;

	if (flags & CENTERED) {
		area.x = (screen.width - area.width) / 2;
		area.y = (screen.height - area.height) / 2;
	}

	if (area.x < 0)
		area.x = 0;
	if (area.y < 0)
		area.y = 0;
	if (area.x + area.width > screen.width)
		area.x = screen.width - area.width;
	if (area.y + area.height > screen.height)
		area.y = screen.height - area.height;

	return area;
}

static long app_window_style(long flags)
{
	long style = 0L;

	if (flags & POPUP)
		style |= WS_POPUP;
	else if (flags & TITLEBAR)
		style |= (WS_CAPTION | WS_OVERLAPPED);
	else
		style |= WS_BORDER | WS_POPUP; /* this !breaks on my Win9* PC */
	if (flags & CLOSEBOX)
		style |= WS_SYSMENU;
	if (flags & MINIMIZE)
		style |= WS_MINIMIZEBOX;
	if (flags & MAXIMIZE)
		style |= WS_MAXIMIZEBOX;
	if (flags & RESIZE)
		style |= WS_THICKFRAME;
	return style;
}

static long app_window_extended_style(long flags)
{
	long style = WS_EX_NOPARENTNOTIFY;	//!!

	if (flags & FLOATING)
		style |= WS_EX_TOPMOST;
	return style;
}

static void app_get_class_name(App *app, Window *win, char *class_name)	//!!
{
	sprintf(class_name, "%s %p", CLASS_NAME_PREFIX,
		win ? win : (void *) app);
}

static void app_register_window_class(App *app, const char *class_name)
{
	WNDCLASS wndclass;

	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = app_winproc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = sizeof(void *);
	wndclass.hInstance     = app_extra(app)->this_instance;
	wndclass.hIcon         = 0L;
	wndclass.hCursor       = 0L;
	wndclass.hbrBackground = 0L;
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = class_name;

	RegisterClass(&wndclass);
}

static void app_unregister_window_class(App *app, const char *class_name)
{
	UnregisterClass(class_name, app_extra(app)->this_instance);
}

Window *app_new_window(App *app, Rect area, const char *name, long flags)
{
	HWND hwnd;
	Window *win;
	Rect actual;
	long style, exstyle;
	char class_name[CLASS_NAME_SIZE];

	actual = app_actual_area(app, area, flags);
	flags = app_actual_window_flags(flags);
	style = app_window_style(flags);
	exstyle = app_window_extended_style(flags);

	win = app_zero_alloc(sizeof(struct Window));
	win->app = app;
	win->text = app_copy_string(name);
	win->flags = flags;
	win->area = area;
	win->bg = WHITE;

	/*
	 * BASE windows use one common class (icon, cursor ...),
	 * other windows use own class.	//!!
	 */
	{
		int is_base = (flags & BASE) != 0L;

		app_get_class_name(app, is_base ? NULL : win,
				class_name);
		if (! (is_base && app_extra(app)->window_number++))
		app_register_window_class(app, class_name);
	}

	hwnd = (flags & POPUP) ? GetActiveWindow() : NULL;
	hwnd = CreateWindowEx(exstyle,
		class_name,
		name,
		style,
		actual.x,
		actual.y,
		actual.width,
		actual.height,
		hwnd,		/* parent window handle */
		NULL,		/* menu handle */
		app_extra(app)->this_instance,
		NULL);

	win->extra = app_zero_alloc(sizeof(struct WindowExtra));
	win_extra(win)->hwnd = hwnd;

	app_set_window_cursor(win,
		app_get_standard_cursor(app, ARROW_CURSOR));

	SetWindowLongPtr(hwnd, 0, (LONG_PTR) win);

	app_remember_window(app, win);

	return win;
}

void app_del_window(Window *win)
{
	int i;
	App *app = win->app;	//!!
	HWND hwnd = win_extra(win)->hwnd;
	char class_name[CLASS_NAME_SIZE];

	app_forget_window(app, win);

	if (! app->deleting)
	{
		app_hide_window(win);
		app_remember_deleted_window(app, win);
		return;
	}

	/* Remove the window's controls from the data structures. */
	for (i = win->num_children - 1; i >= 0; i--)
		app_del_control(win->children[i]);

	/* Remove the window from the screen. */
	if (win_extra(win)->hicon)
		DestroyIcon(win_extra(win)->hicon);
	DestroyWindow(hwnd);

	/* Un-register BASE windows class only once. */	//!!
	{
		int is_base = (win->flags & BASE) != 0L;

		if (! (is_base && --app_extra(app)->window_number)) {
			app_get_class_name(app, is_base ? NULL : win,
					class_name);
			app_unregister_window_class(app, class_name);
		}
	}

	/* Discard arrays of function pointers. */
	app_free(win->close);
	app_free(win->move);
	app_free(win->resize);
	app_free(win->redraw);
	app_free(win->mouse_down);
	app_free(win->mouse_up);
	app_free(win->mouse_drag);
	app_free(win->mouse_move);
	app_free(win->key_down);
	app_free(win->key_action);

	/* Tidy up. */
	if (win->pal)
		app_del_palette(win->pal);

	app_free(win_extra(win));
	app_free(win);
}

void app_move_window(Window *win, Rect r)
{
	Rect actual;

	win->area.x = r.x;
	win->area.y = r.y;
	actual = app_actual_area(win->app, win->area, win->flags);
	win->area = actual;

	if (IsIconic(win_extra(win)->hwnd))
		return;

	SetWindowPos(win_extra(win)->hwnd, HWND_NOTOPMOST,
		actual.x, actual.y, actual.width, actual.height,
		SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);

	app_place_window_controls(win, 0);
}

void app_size_window(Window *win, Rect r)
{
	Rect actual;

	win->area.width = r.width;
	win->area.height = r.height;
	actual = app_actual_area(win->app, win->area, win->flags);
	win->area = actual;

	if (IsIconic(win_extra(win)->hwnd))
		return;

	SetWindowPos(win_extra(win)->hwnd, HWND_NOTOPMOST,
		actual.x, actual.y, actual.width, actual.height,
		SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);

	app_place_window_controls(win, 1);
}

void app_redraw_rect(Window *win, Rect r)
{
	RECT R;

	if (! (win->state & VISIBLE))
		return;

	R.left = r.x;
	R.right = r.x + r.width;
	R.top = r.y;
	R.bottom = r.y + r.height;

	InvalidateRect(win_extra(win)->hwnd, &R, 1);
	UpdateWindow(win_extra(win)->hwnd);
}

void app_show_window(Window *win)
{
	if ((win->state & VISIBLE) == 0)
		win->app->visible_windows++;
	win->state |= VISIBLE;
	if (win->flags & MODAL)
		app_remember_modal(win->app, win);
	ShowWindow(win_extra(win)->hwnd, SW_SHOWNORMAL);
	/* WM_SIZE is not sent to popup windows */
	app_place_window_controls(win, 0);
	UpdateWindow(win_extra(win)->hwnd);
}

void app_hide_window(Window *win)
{
	if (win->state & VISIBLE)
		win->app->visible_windows--;
	win->state &= ~ VISIBLE;
	if (win->flags & MODAL)
		app_forget_modal(win->app, win);
	ShowWindow(win_extra(win)->hwnd, SW_HIDE);
	app_place_window_controls(win, 0);
}

void app_set_window_title(Window *win, const char *title)
{
	SetWindowText(win_extra(win)->hwnd, title);
	if (win->text)
		app_free(win->text);
	win->text = app_copy_string(title);
}

char * app_get_window_title(Window *win)
{
	return win->text;
}

static Image *app_fit_icon_image(Image *src, int width, int height)
{
	Image *dst;
	Colour col, invisible;
	int x, y, dx, dy;

	if ((src->width == width) && (src->height == height))
	{
		/* image is already the correct size */
		return src;
	}
	else if ((src->width > width) || (src->height > height))
	{
		/* image is too big, so scale it down */
		return app_scale_image(src, rect(0, 0, width, height),
			app_get_image_area(src));
	}

	/* wrong size, so create a new image the correct size */
	dst = app_new_image(width, height, 32);

	/* copy image aligned centrally, by default */

	dx = (src->width - width) / 2;
	dy = (src->height - height) / 2;

	/* now copy the image to the correct place in the destination */

	invisible = argb(255,255,255,255);
	for (y=0; y < width; y++) {
	  for (x=0; x < height; x++) {
		if ((x+dx >= 0) && (x+dx < src->width) &&
		    (y+dy >= 0) && (y+dy < src->height))
		{
			if (src->depth == 8)
				col = src->cmap[src->data8[y+dy][x+dx]];
			else
				col = src->data32[y+dy][x+dx];
			dst->data32[y][x] = col;
		}
		else {
			dst->data32[y][x] = invisible;
		}
	  }
	}

	return dst;
}

static void app_find_best_icon_size(App *app,
		int *width, int *height, int *depth)
{
	*width = GetSystemMetrics(SM_CXICON);
	*height = GetSystemMetrics(SM_CYICON);
	*depth = 32;
}

void app_set_window_icon(Window *w, Image *icon)
{
	int width, height, depth;
	Bitmap *b;
	Image *original;
        ICONINFO info;
        HICON hicon;

	if (icon == NULL) {
                SetClassLongPtr(win_extra(w)->hwnd, GCLP_HICON,
                        (LONG_PTR) LoadIcon(NULL, IDI_APPLICATION));
		if (win_extra(w)->hicon) {
			DestroyIcon(win_extra(w)->hicon);
			win_extra(w)->hicon = 0;
		}
		return;
	}

	original = icon;
	app_find_best_icon_size(w->app, &width, &height, &depth);
	icon = app_fit_icon_image(icon, width, depth);

	b = app_image_to_bitmap(w, icon);
       	info.fIcon = TRUE;
       	info.xHotspot = 0;
        info.yHotspot = 0;
        info.hbmMask = bitmap_extra(b)->clipmask;
        info.hbmColor = bitmap_extra(b)->handle;
        hicon = CreateIconIndirect(&info);
	app_del_bitmap(b);

	SetClassLongPtr(win_extra(w)->hwnd, GCLP_HICON, (LONG_PTR) hicon);

	if (win_extra(w)->hicon) {
		DestroyIcon(win_extra(w)->hicon);
		win_extra(w)->hicon = hicon;
	}

	if (icon != original)
		app_del_image(icon);
}

int app_realize_palette(Window *win)
{
	HWND hwnd;
	HDC dc;
	HPALETTE pal, oldpal;
	int i;

	if (win == NULL)
		return 0;
	if (win_extra(win)->winpal == 0)
		return 0;

	hwnd = win_extra(win)->hwnd;
	pal = win_extra(win)->winpal;

	dc = GetDC(hwnd);
	oldpal = SelectPalette(dc, pal, FALSE);
	i = RealizePalette(dc);
	SelectPalette(dc, oldpal, FALSE);
	ReleaseDC(hwnd, dc);
	if (i > 0)
		InvalidateRect(hwnd, NULL, FALSE);
	return i;
}

void app_set_window_palette(Window *win, Palette *pal)
{
	LOGPALETTE *logpal;
	HDC dc;
	Palette *old;
	int i, size;
	Colour *elem;

	if (! pal) {
		/* go back to using the system palette */
		if (win->pal) {
			app_del_palette(win->pal);
			win->pal = NULL;
		}
		if (win_extra(win)->winpal) {
			DeleteObject(win_extra(win)->winpal);
			win_extra(win)->winpal = 0;
		}
		dc = GetDC(win_extra(win)->hwnd);
		RealizePalette(dc);
		InvalidateRect(win_extra(win)->hwnd, NULL, FALSE);
		return;
	}

	old = win->pal;
	size = pal->size;
	elem = pal->element;
	win->pal = app_new_palette(size, elem);

	if (old)
		app_del_palette(old);
	if (win_extra(win)->winpal)
		DeleteObject(win_extra(win)->winpal);

	/* create logical palette then handle to palette */

	logpal = app_alloc(sizeof(LOGPALETTE)+ size*sizeof(PALETTEENTRY));

	logpal->palNumEntries = size;
	logpal->palVersion = 0x300;

	for (i = 0; i < size; i++) {
	    logpal->palPalEntry[i].peRed   = elem[i].red;
	    logpal->palPalEntry[i].peGreen = elem[i].green;
	    logpal->palPalEntry[i].peBlue  = elem[i].blue;
	    logpal->palPalEntry[i].peFlags = PC_NOCOLLAPSE;
	}
	win_extra(win)->winpal = CreatePalette(logpal);
	app_free(logpal); /* discard temporary logical palette */

	if (IsWindowVisible(win_extra(win)->hwnd))
		app_realize_palette(win);
}

Palette *app_get_window_palette(Window *win)
{
	return win->pal;
}

/*
 *  Locate the window under mouse cursor:	//!!
 */
Window *app_get_window_under_cursor(App *app)
{
	HWND hwnd;
	POINT pt;

	GetCursorPos(&pt);
	hwnd = WindowFromPoint(pt);
	if (GetWindowLongPtr(hwnd, GWLP_WNDPROC) == (LONG_PTR) app_winproc)
		return (Window *) GetWindowLongPtr(hwnd, 0);
	return NULL;
}
