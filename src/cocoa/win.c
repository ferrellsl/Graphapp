/*
 *  Windows.
 *
 *  Platform: macOS.
 *
 *  Each App window is an NSWindow (made by the bridge) whose content view
 *  displays a Surface.  All drawing goes to the Surface; the view copies
 *  the changed part to the screen.  Redraw requests are honoured
 *  immediately, by calling the window's redraw handlers on the Surface.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

static int app_bridge_style(long flags)
{
	int style = 0;

	if (flags & TITLEBAR) {
		style |= GAB_TITLED;
		if (flags & CLOSEBOX)
			style |= GAB_CLOSABLE;
		if (flags & MINIMIZE)
			style |= GAB_MINIATURE;
	}
	if (flags & RESIZE)
		style |= GAB_RESIZABLE;
	if (flags & POPUP)
		style |= GAB_POPUP;
	if (flags & FLOATING)
		style |= GAB_FLOATING;
	if (flags & MODAL)
		style |= GAB_MODAL;
	return style;
}

static void app_fill_surface(Surface *s, Colour c)
{
	size_t i, n = (size_t) s->width * s->height;
	uint32_t p = ((uint32_t) c.red << 16) | ((uint32_t) c.green << 8) | c.blue;

	for (i=0; i < n; i++)
		s->pixels[i] = p;
}

/*
 *  Read the client area's position and size back from the native window.
 */
void app_fix_window_area(Window *win)
{
	int x, y, w, h;

	if (gab_window_is_minimised(win_extra(win)->handle))
		return;
	gab_window_get_client(win_extra(win)->handle, &x, &y, &w, &h);
	win->area = rect(x, y, w, h);
}

Window *app_new_window(App *app, Rect area, const char *name, long flags)
{
	Window *win;
	int centered = (flags & CENTERED) ? 1 : 0;

	flags = app_actual_window_flags(flags);

	win = app_zero_alloc(sizeof(struct Window));
	win->app = app;
	win->text = app_copy_string(name);
	win->flags = flags;
	win->area = area;
	win->bg = WHITE;

	win->extra = app_zero_alloc(sizeof(struct WindowExtra));
	if (! app_surface_create(&win_extra(win)->surf,
			area.width, area.height)) {
		app_free(win_extra(win));
		app_free(win->text);
		app_free(win);
		return NULL;
	}
	app_fill_surface(&win_extra(win)->surf, win->bg);

	win_extra(win)->handle = gab_window_create(win, area.x, area.y,
			area.width, area.height, app_bridge_style(flags),
			centered, name);
	if (win_extra(win)->handle == NULL) {
		app_surface_destroy(&win_extra(win)->surf);
		app_free(win_extra(win));
		app_free(win->text);
		app_free(win);
		return NULL;
	}
	gab_window_set_surface(win_extra(win)->handle,
			win_extra(win)->surf.pixels,
			win_extra(win)->surf.width, win_extra(win)->surf.height);
	app_fix_window_area(win);
	if (win->area.width < 1)
		win->area.width = area.width;
	if (win->area.height < 1)
		win->area.height = area.height;

	app_set_window_cursor(win,
		app_get_standard_cursor(app, ARROW_CURSOR));

	app_remember_window(app, win);

	return win;
}

void app_del_window(Window *win)
{
	int i;
	App *app = win->app;

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
	gab_window_destroy(win_extra(win)->handle);
	app_surface_destroy(&win_extra(win)->surf);

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
	app_free(win->text);

	app_free(win_extra(win));
	app_free(win);
}

void app_move_window(Window *win, Rect r)
{
	win->area.x = r.x;
	win->area.y = r.y;

	if (gab_window_is_minimised(win_extra(win)->handle))
		return;

	gab_window_move(win_extra(win)->handle, r.x, r.y);
	app_fix_window_area(win);
	app_place_window_controls(win, 0);
}

void app_size_window(Window *win, Rect r)
{
	win->area.width = r.width;
	win->area.height = r.height;

	if (gab_window_is_minimised(win_extra(win)->handle))
		return;

	/* the bridge reports the new size back through the resized callback */
	gab_window_resize(win_extra(win)->handle, r.width, r.height);
	app_fix_window_area(win);
	app_place_window_controls(win, 1);
}

/*
 *  Redraw part of a window: run its redraw handlers, on the Surface.
 */
void app_do_redraw_window(Window *win, Rect r)
{
	int i;
	Graphics *g;
	Rect all;

	if (win->redraw_rgn)
		return;	/* already redrawing */

	all = app_get_window_area(win);
	r = app_clip_rect(r, all);
	if ((r.width <= 0) || (r.height <= 0))
		return;

	win->redraw_rgn = app_new_rect_region(r);
	g = app_get_window_redraw(win);
	app_set_rgb(g, win->bg);
	app_fill_rect(g, all);
	app_set_rgb(g, BLACK);
	if (win->redraw)
		for (i=0; win->redraw[i]; i++)
			win->redraw[i](win, g);
	app_do_draw_controls(g, win->num_children, win->children, 1);
	app_del_graphics(g);	/* flushes to the screen */
	app_del_region(win->redraw_rgn);
	win->redraw_rgn = NULL;
}

void app_redraw_rect(Window *win, Rect r)
{
	if (! (win->state & VISIBLE))
		return;
	app_do_redraw_window(win, r);
}

void app_show_window(Window *win)
{
	int first_time = ! (win->state & VISIBLE);

	if (first_time)
		win->app->visible_windows++;
	win->state |= VISIBLE;
	if (win->flags & MODAL)
		app_remember_modal(win->app, win);
	gab_window_show(win_extra(win)->handle);
	app_fix_window_area(win);
	if (first_time && ! (win->flags & POPUP))
		app_do_resize_window(win);	/* as a native show would */
	else
		app_place_window_controls(win, 0);
	app_do_redraw_window(win, app_get_window_area(win));
	app_flush_window(win);
	gab_flush_windows();
}

void app_hide_window(Window *win)
{
	if (win->state & VISIBLE)
		win->app->visible_windows--;
	win->state &= ~ VISIBLE;
	if (win->flags & MODAL)
		app_forget_modal(win->app, win);
	gab_window_hide(win_extra(win)->handle);
	app_place_window_controls(win, 0);
}

void app_set_window_title(Window *win, const char *title)
{
	gab_window_set_title(win_extra(win)->handle, title);
	if (win->text)
		app_free(win->text);
	win->text = app_copy_string(title);
}

char * app_get_window_title(Window *win)
{
	return win->text;
}

/*
 *  macOS windows have no icon of their own; the closest thing is the
 *  application's Dock icon, which the bridge sets.
 */
void app_set_window_icon(Window *w, Image *icon)
{
	uint32_t *argb;
	int x, y;
	Colour col;

	if (icon == NULL || ((icon->depth != 8) && (icon->depth != 32)))
		return;
	argb = malloc((size_t) icon->width * icon->height * sizeof(uint32_t));
	if (argb == NULL)
		return;
	for (y=0; y < icon->height; y++) {
		for (x=0; x < icon->width; x++) {
			if (icon->depth == 8)
				col = icon->cmap[icon->data8[y][x]];
			else
				col = icon->data32[y][x];
			argb[(size_t) y * icon->width + x] =
				((col.alpha > 0x7F) ? 0u : 0xFF000000u) |
				((uint32_t) col.red << 16) |
				((uint32_t) col.green << 8) | col.blue;
		}
	}
	gab_window_set_icon(win_extra(w)->handle, argb, icon->width, icon->height);
	free(argb);
}

/*
 *  Palettes are meaningless on a true-colour display, but the App
 *  keeps the one it is given so that app_get_window_palette works.
 */
void app_set_window_palette(Window *win, Palette *pal)
{
	if (win->pal)
		app_del_palette(win->pal);
	if (pal)
		win->pal = app_new_palette(pal->size, pal->element);
	else
		win->pal = NULL;
}

Palette *app_get_window_palette(Window *win)
{
	return win->pal;
}

Window *app_get_window_under_cursor(App *app)
{
	return (Window *) gab_window_under_cursor();
}

/*
 *  Flushing: hand the changed parts of a window's Surface to the view.
 */
void app_flush_window(Window *w)
{
	Surface *s;

	if (w == NULL || w->extra == NULL)
		return;
	s = &win_extra(w)->surf;
	if (s->has_dirty) {
		gab_window_dirty(win_extra(w)->handle, s->dirty.x, s->dirty.y,
				s->dirty.width, s->dirty.height);
		s->has_dirty = 0;
	}
}

void app_flush_all_windows(App *app)
{
	int i;

	for (i=0; i < app->num_windows; i++)
		app_flush_window(app->windows[i]);
	gab_flush_windows();
}
