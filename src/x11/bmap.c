/*
 *  Bitmaps.
 *
 *  Platform: X-Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/07/07  New bitmaps are now transparent.
 *  Version: 3.11  2001/12/12  Now guards against zero-area bitmaps.
 *  Version: 3.42  2003/03/28  Added monochrome bitmap constructor.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

/*
 *  Internal function used to wait until the X Server has
 *  actually created the bitmap. Returns 1 on successful
 *  creation of a bitmap, 0 if the server ran out of memory.
 */
int app_bitmap_created(Display *disp, Pixmap bmap)
{
	XID root_win;
	int x, y;
	unsigned int w, h, border_width, depth;

	if (XGetGeometry(disp, bmap, &root_win, &x, &y, &w, &h,
		&border_width, &depth))
		return 1;
	else
		return 0;
}

/*
 *  Create a fully-transparent bitmap.
 */
Bitmap *app_new_bitmap(Window *win, int width, int height)
{
	Bitmap *b;
	int depth;
	Display *disp;

	b = app_zero_alloc(sizeof(struct Bitmap));
	b->win = win;
	b->area = rect(0,0,width,height);
	b->extra = app_zero_alloc(sizeof(struct BitmapExtra));

	disp = app_extra(win->app)->display;
	depth = DefaultDepth(disp, DefaultScreen(disp));

	if ((width <= 0) || (height <= 0)) {
		b->area = rect(0,0,0,0);
		width = height = 1;
	}

	bitmap_extra(b)->handle = XCreatePixmap(disp,
			win_extra(win)->xid, width, height, depth);

	if (! app_bitmap_created(disp, bitmap_extra(b)->handle)) {
		app_free(bitmap_extra(b));
		app_free(b);
		return NULL;
	}

	bitmap_extra(b)->clipmask = app_new_clipmask(disp, width, height);

	return b;
}

/*
 *  Create a fully-opaque white bitmap.
 */
Bitmap *app_new_white_bitmap(Window *win, int width, int height)
{
	Bitmap *b;
	GC gc;
	int depth;
	Display *disp;

	b = app_zero_alloc(sizeof(struct Bitmap));
	b->win = win;
	b->area = rect(0,0,width,height);
	b->extra = app_zero_alloc(sizeof(struct BitmapExtra));

	disp = app_extra(win->app)->display;
	depth = DefaultDepth(disp, DefaultScreen(disp));

	if ((width <= 0) || (height <= 0)) {
		b->area = rect(0,0,0,0);
		width = height = 1;
	}

	bitmap_extra(b)->handle = XCreatePixmap(disp,
			win_extra(win)->xid, width, height, depth);

	if (! app_bitmap_created(disp, bitmap_extra(b)->handle)) {
		app_free(bitmap_extra(b));
		app_free(b);
		return NULL;
	}

	gc = XCreateGC(disp, bitmap_extra(b)->handle, 0, NULL);
	if (! gc) {
		XFreePixmap(disp, bitmap_extra(b)->handle);
		app_free(bitmap_extra(b));
		app_free(b);
		return NULL;
	}

	XSetForeground(disp, gc, WhitePixel(disp, DefaultScreen(disp)));
	XFillRectangle(disp, bitmap_extra(b)->handle, gc,
		0, 0, width, height);
	XFreeGC(disp, gc);

	return b;
}

/*
 *  Create a fully-transparent bitmap with depth 1 bit per pixel.
 */
Bitmap *app_new_monochrome_bitmap(Window *win, int width, int height)
{
	Bitmap *b;
	int depth;
	Display *disp;

	b = app_zero_alloc(sizeof(struct Bitmap));
	b->win = win;
	b->area = rect(0,0,width,height);
	b->extra = app_zero_alloc(sizeof(struct BitmapExtra));

	disp = app_extra(win->app)->display;
	depth = 1;

	if ((width <= 0) || (height <= 0)) {
		b->area = rect(0,0,0,0);
		width = height = 1;
	}

	bitmap_extra(b)->handle = XCreatePixmap(disp,
			DefaultRootWindow(disp), width, height, depth);

	if (! app_bitmap_created(disp, bitmap_extra(b)->handle)) {
		app_free(bitmap_extra(b));
		app_free(b);
		return NULL;
	}

	bitmap_extra(b)->clipmask = app_new_clipmask(disp, width, height);

	return b;
}

/*
 *  Delete a bitmap from memory.
 */
void app_del_bitmap(Bitmap *b)
{
	if (bitmap_extra(b)->clipmask != None)
		XFreePixmap(app_extra(b->win->app)->display,
			bitmap_extra(b)->clipmask);
	XFreePixmap(app_extra(b->win->app)->display,
		bitmap_extra(b)->handle);
	app_free(bitmap_extra(b));
	app_free(b);
}

/*
 *  Return the rectangular area used by the bitmap.
 */
Rect app_get_bitmap_area(Bitmap *b)
{
	return b->area;
}

