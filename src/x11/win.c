/*
 *  Window creation routines.
 *
 *  Platform: X-Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.02  2001/10/10  Uses MWM hints.
 *  Version: 3.26  2002/07/31  Added window movement call-back.
 *  Version: 3.35  2002/12/23  Renamed app->winlist to app->windows.
 *  Version: 3.41  2003/03/20  Modal windows implemented.
 *  Version: 3.42  2003/03/31  Modal implies Floating, added window icons.
 *  Version: 3.43  2003/04/25  Icon bitmap code moved, modal list added.
 *  Version: 3.49  2003/09/09  Fixed forget_window off-by-one crash.
 *  Version: 3.50  2004/01/18  Moved many common functions to winutil.c.
 *  Version: 3.51  2004/03/28  Supports delayed-deletion.
 *  Version: 3.52  2004/03/29  Uses type-safe private extra data types.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

/*
 *  MWM protocol constants:
 */

typedef struct {
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long input_mode;
	unsigned long status;
} MWM_Hints;

enum MWM_Flags {
	MWM_HINTS_FUNCTIONS   = 1 << 0,
	MWM_HINTS_DECORATIONS = 1 << 1,
	MWM_HINTS_INPUT_MODE  = 1 << 2,
	MWM_HINTS_STATUS      = 1 << 3
};

enum MWM_Functions {
	MWM_FUNC_ALL          = 1 << 0,
	MWM_FUNC_RESIZE       = 1 << 1,
	MWM_FUNC_MOVE         = 1 << 2,
	MWM_FUNC_MINIMIZE     = 1 << 3,
	MWM_FUNC_MAXIMIZE     = 1 << 4,
	MWM_FUNC_CLOSE        = 1 << 5
};

enum MWM_Decorations {
	MWM_DECOR_ALL         = 1 << 0,
	MWM_DECOR_BORDER      = 1 << 1,
	MWM_DECOR_RESIZEH     = 1 << 2,
	MWM_DECOR_TITLE       = 1 << 3,
	MWM_DECOR_MENU        = 1 << 4,
	MWM_DECOR_MINIMIZE    = 1 << 5,
	MWM_DECOR_MAXIMIZE    = 1 << 6
};

enum MWM_Input_Mode {
	MWM_INPUT_MODELESS               = 0,
	MWM_INPUT_APPLICATION_MODAL      = 1,
	MWM_INPUT_SYSTEM_MODAL           = 2,
	MWM_INPUT_FULL_APPLICATION_MODAL = 3
};

/*
 *  MWM protocol atom strings:
 */

static const char * XA_MOTIF_WM_HINTS     = "_MOTIF_WM_HINTS";
/*
static const char * XA_MOTIF_BINDINGS     = "_MOTIF_BINDINGS";
static const char * XA_MOTIF_WM_MESSAGES  = "_MOTIF_WM_MESSAGES";
static const char * XA_MOTIF_WM_OFFSE     = "_MOTIF_WM_OFFSET";
static const char * XA_MOTIF_WM_MENU      = "_MOTIF_WM_MENU";
static const char * XA_MOTIF_WM_INFO      = "_MOTIF_WM_INFO";
*/

/*
 *  This is the function responsible for telling a MWM-compliant
 *  window manager what window decorations we want for this window.
 */
static void app_set_window_manager_style(App *app, XID xwin_id, long flags)
{
	MWM_Hints hints;

	if ((flags & STANDARD_WINDOW) == STANDARD_WINDOW)
		return;

	if (! app_extra(app)->xwmhint)
		app_extra(app)->xwmhint = XInternAtom(app_extra(app)->display, 
			      XA_MOTIF_WM_HINTS, False);

	hints.flags = (MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS);
	hints.functions = MWM_FUNC_MOVE | MWM_FUNC_CLOSE;
	hints.decorations = MWM_DECOR_BORDER;
	hints.input_mode = 0;
	hints.status = 0;

	if (flags & TITLEBAR) {
		hints.functions   |= MWM_FUNC_MOVE;
		hints.decorations |= MWM_DECOR_TITLE;
	}
	if (flags & CLOSEBOX) {
		hints.functions   |= MWM_FUNC_CLOSE;
		hints.decorations |= MWM_DECOR_MENU;
	}
	if (flags & MINIMIZE) {
		hints.functions   |= MWM_FUNC_MINIMIZE;
		hints.decorations |= MWM_DECOR_MINIMIZE;
	}
	if (flags & MAXIMIZE) {
		hints.functions   |= MWM_FUNC_MAXIMIZE;
		hints.decorations |= MWM_DECOR_MAXIMIZE;
	}
	if (flags & RESIZE) {
		hints.functions   |= MWM_FUNC_RESIZE;
		hints.decorations |= MWM_DECOR_RESIZEH;
	}
	if (flags & MODAL) {
		hints.flags       |= MWM_HINTS_INPUT_MODE;
		hints.input_mode   = MWM_INPUT_FULL_APPLICATION_MODAL;
	}
	if (flags & FLOATING) {
		/* need to find out what to do */
	}

	XChangeProperty(app_extra(app)->display, xwin_id,
			app_extra(app)->xwmhint, app_extra(app)->xwmhint,
			32, PropModeReplace,
			(char *) &hints,
			sizeof(MWM_Hints)/sizeof(long));
}

/*
 *  Window functions:
 */

static Rect app_actual_area(App *app, Rect area, long flags)
{
	Rect screen;

	Display *disp = app_extra(app)->display;

	screen.width  = DisplayWidth(disp, DefaultScreen(disp));
	screen.height = DisplayHeight(disp, DefaultScreen(disp));

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

Window *app_new_window(App *app, Rect area, const char *name, long flags)
{
	XID xid;
	Display *disp;
	XSizeHints xsh;
	XWMHints xwmh;
	Atom atom[1];
	Window *win;
	Rect actual;
	char *dummy[] = {"app", NULL};

	actual = app_actual_area(app, area, flags);
	flags = app_actual_window_flags(flags);

	disp = app_extra(app)->display;

	xid = XCreateSimpleWindow(disp,
		DefaultRootWindow(disp),	/* parent window */
		actual.x,
		actual.y,
		actual.width,
		actual.height,
		0,		/* border width */
		BlackPixel(disp, DefaultScreen(disp)),	/* foreground */
		WhitePixel(disp, DefaultScreen(disp)));	/* background */

	app_set_window_manager_style(app, xid, flags);

	xsh.flags = (PPosition | PSize);
	xsh.x = actual.x;
	xsh.y = actual.y;
	xsh.width = actual.width;
	xsh.height = actual.height;
	XSetStandardProperties(disp,
		xid,	/* the window */
		name,	/* window name */
		name,	/* icon name */
		None,	/* icon pixmap */
		dummy,	/* argv */
		1,	/* argc */
		&xsh	/* size hints */
		);

	xwmh.flags = (InputHint | StateHint);
	xwmh.input = True;
	xwmh.initial_state = NormalState;
	XSetWMHints(disp, xid, &xwmh);

	XSelectInput(disp, xid,
		(KeyPressMask |
		KeyReleaseMask |
		ButtonPressMask |
		ButtonReleaseMask |
		EnterWindowMask |
		LeaveWindowMask |
		PointerMotionMask |
		ButtonMotionMask |
		KeymapStateMask |
		ExposureMask |
		VisibilityChangeMask |
		StructureNotifyMask |
	/*	ResizeRedirectMask |		*/
	/*	SubstructureNotifyMask |	*/
	/*	SubstructureRedirectMask |	*/
	/*	PropertyChangeMask |		*/
		FocusChangeMask |
		ColormapChangeMask));

	win = app_zero_alloc(sizeof(struct Window));
	win->app = app;
	win->text = app_copy_string(name);
	win->flags = flags;
	win->area = actual;
	win->bg = WHITE;
	win->extra = app_zero_alloc(sizeof(struct WindowExtra));
	win_extra(win)->xid = xid;

	atom[0] = XInternAtom(disp, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(disp, xid, atom, 1);
	win_extra(win)->xdel = atom[0];

	if (app_is_true_colour_display(disp))
		win_extra(win)->is_paletted = 0;
	else
		win_extra(win)->is_paletted = 1;
	win_extra(win)->clut = NULL; /* only used for a private palette */

	app_remember_window(app, win);

	return win;
}

void app_del_window(Window *win)
{
	int i;

	app_forget_window(win->app, win);

	if (! win->app->deleting)
	{
		app_hide_window(win);
		app_remember_deleted_window(win->app, win);
		return;
	}

	/* Remove the window's controls from the data structures. */
	for (i = win->num_children - 1; i >= 0; i--)
		app_del_control(win->children[i]);

	/* Remove the window from the screen. */
	XDestroyWindow(app_extra(win->app)->display, win_extra(win)->xid);

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

/*
 *  Manipulating windows:
 */
void app_move_window(Window *win, Rect r)
{
	Rect actual;

	win->area.x = r.x;
	win->area.y = r.y;
	actual = app_actual_area(win->app, win->area, win->flags);
	win->area = actual;

	XMoveWindow(app_extra(win->app)->display, win_extra(win)->xid,
		actual.x, actual.y);

	app_place_window_controls(win, 0);
}

void app_size_window(Window *win, Rect r)
{
	Rect actual;

	win->area.width = r.width;
	win->area.height = r.height;
	actual = app_actual_area(win->app, win->area, win->flags);
	win->area = actual;

	XResizeWindow(app_extra(win->app)->display, win_extra(win)->xid,
		actual.width, actual.height);

	app_place_window_controls(win, 1);
}

void app_show_window(Window *win)
{
	int i;

	if ((win->state & VISIBLE) == 0)
		win->app->visible_windows++;
	win->state |= VISIBLE;
	if (win->flags & MODAL)
		app_remember_modal(win->app, win);
	XMapRaised(app_extra(win->app)->display, win_extra(win)->xid);
	app_place_window_controls(win, 0);
	if (win->resize)
		for (i=0; win->resize[i]; i++)
			win->resize[i](win);
}

void app_hide_window(Window *win)
{
	if (win->state & VISIBLE)
		win->app->visible_windows--;
	win->state &= ~ VISIBLE;
	if (win->flags & MODAL)
		app_forget_modal(win->app, win);
	XUnmapWindow(app_extra(win->app)->display, win_extra(win)->xid);
	app_place_window_controls(win, 0);
}

/*
 *  Functions for setting a window's title text:
 */
void app_set_window_title(Window *win, const char *title)
{
	char *list[1];
	XTextProperty xtext;

	list[0] = (char *) title; /* cast away const */
	XStringListToTextProperty(list, 1, &xtext);
	XSetWMName(app_extra(win->app)->display,
		win_extra(win)->xid, &xtext);
	XSetWMIconName(app_extra(win->app)->display,
		win_extra(win)->xid, &xtext);
	if (win->text)
		app_free(win->text);
	win->text = app_copy_string(title);
}

char * app_get_window_title(Window *win)
{
	return win->text;
}

/*
 *  Functions for setting a window's icon:
 */
static Rect app_find_best_icon_size(Image *icon, XIconSize *sizes,
		int num_sizes)
{
	int n, width, height, w_max, h_max, w_best, h_best;

	if (num_sizes <= 0)
		return rect(0,0,32,32);

	w_best = h_best = 0;

	for (n=0; n < num_sizes; n++) {
		width = sizes[n].min_width;
		height = sizes[n].min_height;
		w_max = sizes[n].max_width;
		h_max = sizes[n].max_height;

		while ((width <= w_max) && (height <= h_max))
		{
			if ((width == icon->width) &&
			    (height == icon->height))
			{
				/* sizes match exactly! */
				w_best = width;
				h_best = height;
				n = num_sizes; /* skip the remainder */
				break;
			}
			else if ((width >= icon->width) &&
				 (height >= icon->height))
			{
				if (w_best == 0) {
					/* initialise best match size */
					w_best = width;
					h_best = height;
				}
				else if ((width < w_best) &&
					 (height < h_best)) {
					/* tighter fit to given icon */
					w_best = width;
					h_best = height;
				}
			}
			width += sizes[n].width_inc;
			height += sizes[n].height_inc;
		}
	}
	if (w_best == 0) {
		/* found no size larger than given icon! */
		/* so choose largest valid size */
		for (n=0; n < num_sizes; n++) {
			if (sizes[n].max_width > w_best) {
				w_best = sizes[n].max_width;
				h_best = sizes[n].max_height;
			}
		}
	}
	return rect(0, 0, w_best, h_best);
}

static Image * app_find_best_sized_icon(Window *win, Image *icon)
{
	Display *disp;
	XIconSize *sizes;
	int num_sizes;
	Image *img;
	Rect dr, sr;
	Graphics *dst;
	int x, y;
	Colour col;

	disp = app_extra(win->app)->display;

	if (XGetIconSizes(disp, win_extra(win)->xid, &sizes, &num_sizes))
	{
		sr = app_get_image_area(icon);
		dr = app_find_best_icon_size(icon, sizes, num_sizes);
		XFree(sizes);

		if ((dr.width == icon->width) &&
		    (dr.height == icon->height))
		{
			/* correct size: use the icon as is */
			img = icon;
		}
		else if (dr.width < icon->width) {
			/* icon too big: scale it down */
			img = app_scale_image(icon, dr, sr);
		}
		else {
			/* icon too small: center it within a new image */
			img = app_new_image(dr.width, dr.height, 32);

			col = argb(255,255,255,255);
			for (y=0; y < img->height; y++)
				for (x=0; x < img->width; x++)
					img->data32[y][x] = col;

			dr.x = ((dr.width - sr.width) / 2);
			dr.y = ((dr.height - sr.height) / 2);
			dr.width = sr.width;
			dr.height = sr.height;

			dst = app_get_image_graphics(img);
			app_draw_image(dst, dr, icon, sr);
			app_del_graphics(dst);
		}
	}
	else {
		/* use existing icon and hope it works */
		img = icon;
	}
	return img;
}

void app_set_window_icon(Window *win, Image *icon)
{
	XWMHints xwmh;
	Image *original;
	Bitmap *bmap; /* must have depth 1 because X11 ICCCM says so */

	if (icon == NULL) {
		xwmh.flags = (IconPixmapHint | IconMaskHint);
		xwmh.icon_pixmap = None;
		xwmh.icon_mask = None;

		XSetWMHints(app_extra(win->app)->display,
				win_extra(win)->xid, &xwmh);

		if (win_extra(win)->icon_bitmap) {
			app_del_bitmap(win_extra(win)->icon_bitmap);
			win_extra(win)->icon_bitmap = NULL;
		}
		return;
	}

	original = icon;

	icon = app_find_best_sized_icon(win, icon);
	bmap = app_image_to_monochrome_bitmap(win, icon);

	xwmh.flags = (IconPixmapHint | IconMaskHint);
	xwmh.icon_pixmap = bitmap_extra(bmap)->handle;
	xwmh.icon_mask = bitmap_extra(bmap)->clipmask;

	XSetWMHints(app_extra(win->app)->display,
			win_extra(win)->xid, &xwmh);

	if (win_extra(win)->icon_bitmap) {
		app_del_bitmap(win_extra(win)->icon_bitmap);
		win_extra(win)->icon_bitmap = bmap;
	}

	if (icon != original)
		app_del_image(icon);
}

/*
 *  Windows palettes:
 */
void app_set_window_palette(Window *win, Palette *pal)
{
	CLUT *clut;
	Display *disp;
	Colormap cmap;

	if (! win_extra(win)->is_paletted)
		return; /* true colour displays don't use palettes */

	disp = app_extra(win->app)->display;

	if (pal == NULL) {
		/* go back to using the shared system palette */
		cmap = DefaultColormap(disp, DefaultScreen(disp));
		XSetWindowColormap(disp, win_extra(win)->xid, cmap);
		if (win->pal) {
			app_del_palette(win->pal);
			win->pal = NULL;
		}
		if (win_extra(win)->clut) {
			app_del_clut(win_extra(win)->clut);
			win_extra(win)->clut = NULL;
		}
		return;
	}

	/* create the CLUT for the window and store the colours */
	clut = app_new_clut(disp, win_extra(win)->xid,
		pal->size, pal->element);
	XSetWindowColormap(disp, win_extra(win)->xid, clut->cmap);

	/* remove any old info and swap over to the new */
	if (win->pal)
		app_del_palette(win->pal);
	if (win_extra(win)->clut)
		app_del_clut(win_extra(win)->clut);
	win->pal = app_new_palette_from_clut(clut);
	win_extra(win)->clut = clut;
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
	Display *disp;
	XID unused_root, window, sub_window;
	int i;

	disp = app_extra(app)->display;
	window = DefaultRootWindow(disp);

	/* find the deep window */
	while (XQueryPointer(disp, window, &unused_root, &sub_window,
		&i, &i, &i, &i, &i) && (sub_window != None))
			window = sub_window;

	i = app->num_windows;
	while (i--) {
		Window *win = app->windows[i];
		if (win_extra(win)->xid == window)
			return win;
	}
	return NULL;
}
