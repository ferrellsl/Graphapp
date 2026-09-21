/*
 *  Mouse cursors.
 *
 *  Platform: macOS (NSCursor, via the bridge).
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

static int app_remember_cursor(App *app, Cursor *c)
{
	int i;
	Cursor **list;

	i = app->num_cursors;
	list = app_realloc(app->cursors, (i+1) * sizeof(Cursor *));
	if (list == NULL)
		return 0;
	list[i] = c;
	app->cursors = list;
	app->num_cursors++;
	return 1;
}

static Cursor *app_find_cursor(App *app, int shape)
{
	int i;

	for (i=0; i < app->num_cursors; i++)
		if (cursor_extra(app->cursors[i])->shape == shape)
			return app->cursors[i];
	return NULL;
}

static void app_forget_cursor(App *app, Cursor *c)
{
	int i;

	for (i=0; i < app->num_cursors; i++) {
		if (app->cursors[i] != c)
			continue;
		if (i < app->num_cursors-1)
			memmove(&app->cursors[i], &app->cursors[i+1],
				(app->num_cursors-1-i) * sizeof(Cursor *));
		app->num_cursors--;
		return;
	}
}

void app_find_best_cursor_size(App *app, int *width, int *height, int *depth)
{
	*width = 32;
	*height = 32;
	*depth = 32;
}

/*
 *  Create a cursor from an image; transparent pixels stay transparent.
 */
Cursor *app_new_cursor(App *app, Image *img, Point hotspot)
{
	Cursor *c;
	uint32_t *argb;
	int x, y;
	Colour col;

	if (! img || ((img->depth != 8) && (img->depth != 32)))
		return NULL;

	argb = malloc((size_t) img->width * img->height * sizeof(uint32_t));
	if (argb == NULL)
		return NULL;
	for (y=0; y < img->height; y++) {
		for (x=0; x < img->width; x++) {
			if (img->depth == 8)
				col = img->cmap[img->data8[y][x]];
			else
				col = img->data32[y][x];
			argb[(size_t) y * img->width + x] =
				((col.alpha > 0x7F) ? 0u : 0xFF000000u) |
				((uint32_t) col.red << 16) |
				((uint32_t) col.green << 8) | col.blue;
		}
	}

	c = app_zero_alloc(sizeof(Cursor));
	c->app = app;
	c->extra = app_zero_alloc(sizeof(struct CursorExtra));
	cursor_extra(c)->shape = -1;
	cursor_extra(c)->handle = gab_cursor_from_argb(argb, img->width,
			img->height, hotspot.x, hotspot.y);
	free(argb);
	app_remember_cursor(app, c);
	return c;
}

Cursor *app_get_standard_cursor(App *app, int shape)
{
	Cursor *c;

	c = app_find_cursor(app, shape);
	if (c)
		return c;

	c = app_zero_alloc(sizeof(Cursor));
	c->app = app;
	c->extra = app_zero_alloc(sizeof(struct CursorExtra));
	cursor_extra(c)->shape = shape;
	cursor_extra(c)->handle = gab_cursor_standard(shape);
	app_remember_cursor(app, c);
	return c;
}

void app_del_cursor(Cursor *c)
{
	if (c == NULL)
		return;
	app_forget_cursor(c->app, c);
	if (cursor_extra(c)->handle)
		gab_cursor_release(cursor_extra(c)->handle);
	app_free(cursor_extra(c));
	app_free(c);
}

void app_set_window_cursor(Window *win, Cursor *c)
{
	if (win == NULL)
		return;
	if (c == NULL)
		c = app_get_standard_cursor(win->app, ARROW_CURSOR);
	if (! (win->flags & TEMP_CURSOR))
		gab_window_set_cursor(win_extra(win)->handle,
				cursor_extra(c)->handle);
	win->cursor = c;
}

/*
 *  A temporary cursor replaces the window's normal cursor until this
 *  is called again with NULL, which restores the original.
 */
void app_set_window_temp_cursor(Window *win, Cursor *c)
{
	if (win == NULL)
		return;
	if (c) {
		if (! (win->flags & TEMP_CURSOR)) {
			win->old_cursor = win->cursor;
			win->flags |= TEMP_CURSOR;
		}
	}
	else {
		win->flags &= ~TEMP_CURSOR;
		c = win->old_cursor;
		if (c == NULL)
			c = app_get_standard_cursor(win->app, ARROW_CURSOR);
	}
	gab_window_set_cursor(win_extra(win)->handle, cursor_extra(c)->handle);
	win->cursor = c;
}

Point app_get_cursor_position(App *app)
{
	Point p;

	gab_cursor_get_position(&p.x, &p.y);
	return p;
}

void app_set_cursor_position(App *app, Point p)
{
	gab_cursor_set_position(p.x, p.y);
}
