/*
 *  2-dimensional rectangles.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/07/19  Added a function.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"

Rect app_new_rect(int x, int y, int width, int height)
{
	Rect r;

	r.x = x;
	r.y = y;
	r.width = width;
	r.height = height;

	return r;
}

Rect app_center_rect(Rect r1, Rect r2) /* center r1 on r2 */
{
	Rect r;

	r.x = r2.x + (r2.width-r1.width)/2;
	r.y  = r2.y + (r2.height-r1.height)/2;
	r.width = r1.width;
	r.height = r1.height;

	return r;
}

Rect app_inset_rect(Rect r, int width)
{
	r.x += width;
	r.y += width;
	r.width -= width*2;
	r.height -= width*2;
	return r;
}

int app_rect_in_rect(Rect r1, Rect r2)
{
	if ((r1.x >= r2.x) && (r1.y >= r2.y) &&
	    (r1.x+r1.width <= r2.x+r2.width) &&
	    (r1.y+r1.height <= r2.y+r2.height))
		return 1;
	else
		return 0;
}

int app_rect_intersects_rect(Rect r1, Rect r2)
{
	if ((r1.x < r2.x+r2.width) &&
	    (r2.x < r1.x+r1.width) &&
	    (r1.y < r2.y+r2.height) &&
	    (r2.y < r1.y+r1.height))
		return 1;
	else
		return 0;
}

int app_rects_equal(Rect r1, Rect r2)
{
	if ((r1.x == r2.x) && (r1.width == r2.width) &&
	    (r1.y == r2.y) && (r1.height == r2.height))
		return 1;
	else
		return 0;
}

Rect app_clip_rect(Rect r1, Rect r2)
{
	if ((r1.x >= r2.x+r2.width) ||
	    (r2.x >= r1.x+r1.width) ||
	    (r1.y >= r2.y+r2.height) ||
	    (r2.y >= r1.y+r1.height))
	{
		r1.x = r1.y = r1.width = r1.height = 0;
		return r1; /* no overlap */
	}

	if (r1.x < r2.x) {
		r1.width -= (r2.x - r1.x);
		r1.x = r2.x;
	}
	if (r1.y < r2.y) {
		r1.height -= (r2.y - r1.y);
		r1.y = r2.y;
	}
	if (r1.x + r1.width > r2.x + r2.width)
		r1.width = r2.x + r2.width - r1.x;
	if (r1.y + r1.height > r2.y + r2.height)
		r1.height = r2.y + r2.height - r1.y;
	return r1; /* they do overlap */
}

Rect app_rect_abs(Rect r)
{
	if (r.width < 0) {
		r.x += r.width;
		r.width = -r.width;
	}
	if (r.height < 0) {
		r.y += r.height;
		r.height = -r.height;
	}
	return r;
}
