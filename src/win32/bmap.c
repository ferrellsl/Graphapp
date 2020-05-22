/*
 *  Bitmaps.
 *
 *  Platform: Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/07/07  New bitmaps are now transparent.
 *  Version: 3.11  2001/12/12  Now guards against zero-area bitmaps.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

/*
 *  Create a fully-transparent bitmap.
 *  The bitmap is transparent because the clipmask field says so.
 *  The bitmap is initialised with 0's wherever we want it to be
 *  transparent, which in this case is everywhere.
 *  On a Windows platform, the clipmask has 1's for transparency,
 *  0's for opaque pixels, so all we do is set the entire clipmask
 *  to 1's.
 */
Bitmap *app_new_bitmap(Window *w, int width, int height)
{
	Bitmap *b;
	HDC dc;
	HDC dst_dc;

	b = app_zero_alloc(sizeof(struct Bitmap));
	b->win = w;
	b->area = rect(0,0,width,height);
	b->extra = app_zero_alloc(sizeof(struct BitmapExtra));

	if ((width <= 0) || (height <= 0)) {
		b->area = rect(0,0,0,0);
		width = height = 1;
	}

	dc = GetDC(win_extra(w)->hwnd);
	bitmap_extra(b)->handle = CreateCompatibleBitmap(dc, width, height);
	bitmap_extra(b)->clipmask = CreateBitmap(width, height, 1, 1, NULL);

	dst_dc = CreateCompatibleDC(dc);
	SelectObject(dst_dc, bitmap_extra(b)->handle);
	PatBlt(dst_dc, 0, 0, width, height, BLACKNESS); /* all 0's */
	SelectObject(dst_dc, bitmap_extra(b)->clipmask);
	PatBlt(dst_dc, 0, 0, width, height, WHITENESS); /* all 1's */
	DeleteDC(dst_dc);

	ReleaseDC(win_extra(w)->hwnd, dc);

	return b;
}

/*
 *  Create a white fully-opaque bitmap.
 *  The bitmap is opaque because the clipmask field is empty.
 */
Bitmap *app_new_white_bitmap(Window *w, int width, int height)
{
	Bitmap *b;
	HDC dc;
	HDC dst_dc;

	b = app_zero_alloc(sizeof(struct Bitmap));
	b->win = w;
	b->area = rect(0,0,width,height);
	b->extra = app_zero_alloc(sizeof(struct BitmapExtra));

	if ((width <= 0) || (height <= 0)) {
		b->area = rect(0,0,0,0);
		width = height = 1;
	}

	dc = GetDC(win_extra(w)->hwnd);
	bitmap_extra(b)->handle = CreateCompatibleBitmap(dc, width, height);

	dst_dc = CreateCompatibleDC(dc);
	SelectObject(dst_dc, bitmap_extra(b)->handle);
	PatBlt(dst_dc, 0, 0, width, height, WHITENESS);
	DeleteDC(dst_dc);

	ReleaseDC(win_extra(w)->hwnd, dc);

	return b;
}

/*
 *  Delete a bitmap.
 */
void app_del_bitmap(Bitmap *b)
{
	if (bitmap_extra(b)->clipmask)
		DeleteObject(bitmap_extra(b)->clipmask);
	DeleteObject(bitmap_extra(b)->handle);
	app_free(bitmap_extra(b));
	app_free(b);
}

/*
 *  Determine the bitmap's rectangular extent.
 */
Rect app_get_bitmap_area(Bitmap *b)
{
	return b->area;
}

