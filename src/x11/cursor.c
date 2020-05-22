/*
 *  Cursors.
 *
 *  Platform: X-Windows.
 *
 *  Version: 3.43  2003/04/25  First release.
 *  Version: 3.44  2003/04/29  Rename to app_get_standard_cursor.
 *  Version: 3.57  2005/08/16  Added SIZELR and SIZETB cursors.
 *  Version: 3.60  2007/06/06  Added app_set_window_temp_cursor.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"
#include "cursors.h"

static int app_remember_cursor(App *app, Cursor *c)
{
	int num;
	Cursor **list;

	num = app->num_cursors + 1;
	list = app_realloc(app->cursors, num * sizeof(Cursor *));
	if (list == NULL)
		return 0;
	app->cursors = list;
	app->cursors[num-1] = c;
	app->num_cursors++;
	return 1;
}

static int app_find_cursor(App *app, int shape)
{
	int i;

	for (i=0; i < app->num_cursors; i++) {
		if (cursor_extra(app->cursors[i])->shape == shape)
			return i;
	}
	return -1;
}

static void app_forget_cursor(App *app, Cursor *c)
{
	int i, shift;

	for (i=shift=0; i < app->num_cursors; i++) {
		if (app->cursors[i] == c)
			shift++;
		if (shift && (i+shift < app->num_cursors))
			app->cursors[i] = app->cursors[i+shift];
	}
	if (shift)
		app->cursors = app_realloc(app->cursors,
				(i-shift) * sizeof(Cursor *));
	app->num_cursors -= shift;
}

static Image *app_fit_cursor_image(Image *src, Point *hotspot,
				int width, int height)
{
	Image *dst;
	Colour col, invisible;
	int x, y, dx, dy;

	if ((src->width == width) && (src->height == height))
	{
		/* image is already the correct size */
		return src;
	}

	/* wrong size, so create a new image the correct size */
	dst = app_new_image(width, height, 32);

	/* copy image to top-left, by default */
	dx = dy = 0;

	if (src->width > width)
	{
		/* source image too wide, so center it horizontally */
		dx = (src->width - width) / 2;
	}

	if (src->height > height)
	{
		/* source image too tall, so center it vertically */
		dy = (src->height - height) / 2;
	}

	/* now copy the image to the correct place in the destination */

	invisible = argb(255,255,255,255);
	for (y=0; y < width; y++) {
	  for (x=0; x < height; x++) {
		if ((x+dx < src->width) && (y+dy < src->height))
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
	hotspot->x -= dx;
	hotspot->y -= dy;

	return dst;
}

void app_find_best_cursor_size(App *app, int *width, int *height, int *depth)
{
	Display *disp;
	unsigned int width_return, height_return;

	disp = app_extra(app)->display;

	if (XQueryBestCursor(disp, DefaultRootWindow(disp),
		64, 64, &width_return, &height_return))
	{
		*width = width_return;
		*height = height_return;
		*depth = 1;
	}
	else {	/* guess */
		*width = 16;
		*height = 16;
		*depth = 1;
	}
}

Cursor *app_new_cursor(App *app, Image *img, Point hotspot)
{
	Cursor *c;
	XColor fg, bg;
	Bitmap *b;
	Image *original;
	int width, height, depth;

	/* Ensure we are using an image of the exact size needed. */

	app_find_best_cursor_size(app, &width, &height, &depth);
	original = img;
	img = app_fit_cursor_image(img, &hotspot, width, height);

	/* Generate the cursor. */

	fg.red = fg.green = fg.blue = fg.pixel =  0; /* black */
	bg.red = bg.green = bg.blue = bg.pixel = ~0; /* white */

	if (app->num_windows > 0)
		b = app_image_to_monochrome_bitmap(app->windows[0], img);
	else
		return NULL;

	c = app_zero_alloc(sizeof(Cursor));
	c->app = app;
	c->extra = app_zero_alloc(sizeof(CursorExtra));
	cursor_extra(c)->shape = -1; /* user-defined */
	cursor_extra(c)->cursor =
	  XCreatePixmapCursor(app_extra(app)->display,
		bitmap_extra(b)->handle, bitmap_extra(b)->clipmask,
		&fg, &bg, hotspot.x, hotspot.y);

	app_del_bitmap(b);

	if (original != img)
		app_del_image(img);

	if (! app_remember_cursor(app, c)) {
		app_del_cursor(c);
		return NULL;
	}
	return c;
}

Cursor *app_get_standard_cursor(App *app, int shape)
{
	Cursor *c = NULL;
	int id;

	if ((id = app_find_cursor(app, shape)) >= 0)
		return app->cursors[id];

	if (shape == BLANK_CURSOR)
		c = app_new_cursor(app, app_blank_image, app_blank_hotspot);
	else if (shape == ARROW_CURSOR)
		id = XC_left_ptr;
	else if (shape == WAIT_CURSOR)
		id = XC_watch;
	else if (shape == CARET_CURSOR)
		c = app_new_cursor(app, app_caret_image, app_caret_hotspot);
	else if (shape == CROSS_CURSOR)
		c = app_new_cursor(app, app_cross_image, app_cross_hotspot);
	else if (shape == HAND_CURSOR)
		c = app_new_cursor(app, app_hand_image, app_hand_hotspot);
	else if (shape == GRAB_CURSOR)
		c = app_new_cursor(app, app_grab_image, app_grab_hotspot);
	else if (shape == POINTING_CURSOR)
		c = app_new_cursor(app, app_pointing_image, app_pointing_hotspot);
	else if (shape == PENCIL_CURSOR)
		c = app_new_cursor(app, app_pencil_image, app_pencil_hotspot);
	else if (shape == LASSO_CURSOR)
		c = app_new_cursor(app, app_lasso_image, app_lasso_hotspot);
	else if (shape == DROPPER_CURSOR)
		c = app_new_cursor(app, app_dropper_image, app_dropper_hotspot);
	else if (shape == MAGNIFY_CURSOR)
		c = app_new_cursor(app, app_magnify_image, app_magnify_hotspot);
	else if (shape == MAGPLUS_CURSOR)
		c = app_new_cursor(app, app_magplus_image, app_magplus_hotspot);
	else if (shape == MAGMINUS_CURSOR)
		c = app_new_cursor(app, app_magminus_image, app_magminus_hotspot);
	else if (shape == SIZELR_CURSOR)
		c = app_new_cursor(app, app_sizeLR_image, app_sizeLR_hotspot);
	else if (shape == SIZETB_CURSOR)
		c = app_new_cursor(app, app_sizeTB_image, app_sizeTB_hotspot);
	else
		return NULL;

	if (c != NULL) {
		cursor_extra(c)->shape = shape;
		return c;
	}

	c = app_zero_alloc(sizeof(Cursor));
	c->app = app;
	c->extra = app_zero_alloc(sizeof(CursorExtra));
	cursor_extra(c)->shape = shape;
	cursor_extra(c)->cursor =
		XCreateFontCursor(app_extra(app)->display, id);

	if (! app_remember_cursor(app, c)) {
		app_del_cursor(c);
		return NULL;
	}
	return c;
}

void app_del_cursor(Cursor *c)
{
	app_forget_cursor(c->app, c);
	XFreeCursor(app_extra(c->app)->display, cursor_extra(c)->cursor);
	app_free(c->extra);
	app_free(c);
}

void app_set_window_cursor(Window *win, Cursor *c)
{
	if (c == NULL) {
		XUndefineCursor(app_extra(win->app)->display,
			win_extra(win)->xid);
		c = app_get_standard_cursor(win->app, ARROW_CURSOR);
	}
	else {
		XDefineCursor(app_extra(win->app)->display,
			win_extra(win)->xid,
			cursor_extra(c)->cursor);
	}
	win->cursor = c;
}

void app_set_window_temp_cursor(Window *win, Cursor *c)	//!!
{
	/* Set the temporary cursor of window */
	if (c) {
		if (! (win->flags & TEMP_CURSOR)) {
			win->old_cursor = win->cursor;
			win->flags |= TEMP_CURSOR;
		}
	}
	/* Restore the original cursor of window */
	else {
		win->flags &= ~ TEMP_CURSOR;
		c = win->old_cursor;
	}
	app_set_window_cursor(win, c);
}

Point app_get_cursor_position(App *app)
{
	Display *disp;
	XID w;
	XID root_return, unused_window;
	int root_x_return, root_y_return;
	int unused_x, unused_y;
	unsigned int unused_mask;

	disp = app_extra(app)->display;
	w = DefaultRootWindow(disp);

	XQueryPointer(disp, w, &root_return, &unused_window,
			&root_x_return, &root_y_return,
			&unused_x, &unused_y, &unused_mask);

	return pt(root_x_return, root_y_return);
}

void app_set_cursor_position(App *app, Point p)
{
	Display *disp;
	XID root, unused_window;
	int unused;
	unsigned int unused_mask;

	disp = app_extra(app)->display;
	root = DefaultRootWindow(disp);

	/* Find which root window the cursor is currently on. */

	XQueryPointer(disp, root, &root, &unused_window,
			&unused, &unused,
			&unused, &unused, &unused_mask);

	XWarpPointer(disp, None, root, 0, 0, 0, 0, p.x, p.y);
}
