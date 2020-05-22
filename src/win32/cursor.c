/*
 *  Cursors.
 *
 *  Platform: Windows.
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

static Colour app_monochrome(Colour c)
{
	int min, max, g, b;

	max = min = c.red;
	g = c.green;
	if      (g < min) min = g;
	else if (g > max) max = g;
	b = c.blue;
	if      (b < min) min = b;
	else if (b > max) max = b;

	if (min > 0xE0) 	c.red = c.green = c.blue = 255;
	else if (max < 0x10)	c.red = c.green = c.blue = 0;
	else if (max < 0x60)	c.red = c.green = c.blue = 0;
	else if (max < 0xD0)	c.red = c.green = c.blue = 0;
	else			c.red = c.green = c.blue = 255;

	return c;
}

static Colour app_get_monochrome_pixel_at(Image *img, int x, int y)
{
	if (img->depth == 8)
		return app_monochrome(img->cmap[img->data8[y][x]]);
	else
		return app_monochrome(img->data32[y][x]);
}

void app_find_best_cursor_size(App *app, int *width, int *height, int *depth)
{
	*width = GetSystemMetrics(SM_CXCURSOR);
	*height = GetSystemMetrics(SM_CYCURSOR);
	*depth = 1;
}

Cursor *app_new_cursor(App *app, Image *img, Point hotspot)
{
	Cursor *c;
	byte *and_mask; /* black is 0, white is 0, transparent is 1 */
	byte *xor_mask; /* black is 0, white is 1, transparent is 0 */
	Colour col;
	Image *original;
	int width, height, depth;
	int x, y, total, rowbytes;

	/* Ensure we are using an image of the exact size needed. */

	app_find_best_cursor_size(app, &width, &height, &depth);
	original = img;
	img = app_fit_cursor_image(img, &hotspot, width, height);

	/* Generate and_mask and xor_mask from image. */

	app_find_best_cursor_size(app, &width, &height, &depth);

	rowbytes = (width + 7) / 8;
	total = rowbytes * height;
	and_mask = app_zero_alloc(total);
	xor_mask = app_zero_alloc(total);

	for (y=0; y < height; y++) {
		for (x=0; x < width; x++) {
			if ((y >= img->height) || (x >= img->width))
				continue; /* Outside given image. */
			col = app_get_monochrome_pixel_at(img, x, y);
			if (col.alpha >= 0x80)
				and_mask[y*rowbytes+x/8] |= 1 << (7-x%8);
			else if (col.red >= 0x80)
				xor_mask[y*rowbytes+x/8] |= 1 << (7-x%8);
		}
	}

	/* Generate the cursor. */

	c = app_zero_alloc(sizeof(Cursor));
	c->app = app;
	c->extra = app_zero_alloc(sizeof(CursorExtra));
	cursor_extra(c)->shape = -1; /* user defined */
	cursor_extra(c)->cursor =
		CreateCursor(app_extra(app)->this_instance,
				hotspot.x, hotspot.y, width, height,
				and_mask, xor_mask);
	app_free(and_mask);
	app_free(xor_mask);

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
	HCURSOR hc = 0;

	if ((id = app_find_cursor(app, shape)) >= 0)
		return app->cursors[id];

	if (shape == BLANK_CURSOR)
		c = app_new_cursor(app, app_blank_image, app_blank_hotspot);
	else if (shape == ARROW_CURSOR)
		hc = LoadCursor(NULL, IDC_ARROW);
	else if (shape == WAIT_CURSOR)
		hc = LoadCursor(NULL, IDC_WAIT);
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
	cursor_extra(c)->cursor = hc;

	if (! app_remember_cursor(app, c)) {
		app_del_cursor(c);
		return NULL;
	}
	return c;
}

void app_del_cursor(Cursor *c)
{
	app_forget_cursor(c->app, c);
	DestroyCursor(cursor_extra(c)->cursor);
	app_free(c->extra);
	app_free(c);
}

void app_set_window_cursor(Window *win, Cursor *c)	//!!
{
	HWND hwnd = win_extra(win)->hwnd;

	if (c == NULL) {
		c = app_get_standard_cursor(win->app, ARROW_CURSOR);
		SetClassLongPtr(hwnd, GCLP_HCURSOR, 0L);
	}
	else if (! (win->flags & TEMP_CURSOR)) {
		SetClassLongPtr(hwnd, GCLP_HCURSOR,
				(LONG_PTR) cursor_extra(c)->cursor);
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
			/*
			 * If the class cursor is not NULL, the system restores
			 * the class cursor each time the mouse is moved.
			 */
			SetClassLongPtr(win_extra(win)->hwnd, GCLP_HCURSOR, 0L);
		}
	}
	/* Restore the original cursor of window */
	else {
		win->flags &= ~ TEMP_CURSOR;
		c = win->old_cursor;
		SetClassLongPtr(win_extra(win)->hwnd, GCLP_HCURSOR,
				(LONG_PTR) cursor_extra(c)->cursor);   
	}
	SetCursor((HCURSOR) cursor_extra(c)->cursor);
	win->cursor = c;
}

Point app_get_cursor_position(App *app)
{
	POINT pt;
	GetCursorPos(&pt);
	return pt(pt.x, pt.y);
}

void app_set_cursor_position(App *app, Point p)
{
	SetCursorPos(p.x, p.y); 
}
