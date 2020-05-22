/*
 *  2-dimensional points.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"

Point app_new_point(int x, int y)
{
	Point p;

	p.x = x;
	p.y = y;

	return p;
}

int app_points_equal(Point p1, Point p2)
{
	if ((p1.x == p2.x) && (p1.y == p2.y))
		return 1;
	else
		return 0;
}

int app_point_in_rect(Point p, Rect r)
{
	if ((p.x >= r.x) && (p.x < r.x+r.width) &&
	    (p.y >= r.y) && (p.y < r.y+r.height))
		return 1;
	else
		return 0;
}

