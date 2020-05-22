/*
 *  Clip a line to a rectangle.
 *
 *  Platform: Neutral
 *
 *  Version: 3.01  2001/09/15  First release.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "app.h"

#define	XYswap(p)	t=(p)->x, (p)->x=(p)->y, (p)->y=t
#define	Swap(x, y)	t=x, x=y, y=t

typedef struct LineDesc
{
	int	x0;
	int	y0;
	char	xmajor;
	char	slopeneg;
	long	dminor;
	long	dmajor;
} LineDesc;

static long lfloor(long x, long y)	/* first integer <= x/y */
{
	if (y <= 0) {
		if (y == 0)
			return x;
		y = -y;
		x = -x;
	}
	if (x < 0) {	/* be careful; C division is undefined */
		x = -x;
		x += y-1;
		return -(x/y);
	}
	return x/y;
}

static long lceil(long x, long y)	/* first integer >= x/y */
{
	if (y <= 0) {
		if (y == 0)
			return x;
		y = -y;
		x = -x;
	}
	if (x < 0) {
		x = -x;
		return -(x/y);
	}
	x += y-1;
	return x/y;
}

static int find_height(long x, LineDesc *ld)
{
	long y;

	y = 2*(x - ld->x0)*ld->dminor + ld->dmajor;
	y = lfloor(y, 2*ld->dmajor) + ld->y0;
	return ld->slopeneg ? -y : y;
}

static int find_width(long y, LineDesc *ld)
{
	long x;

	x = 2*((ld->slopeneg ? -y : y) - ld->y0)*ld->dmajor - ld->dminor;
	x = lceil(x, 2*ld->dminor) + ld->x0;
	if (ld->dminor)
		while (find_height(x-1, ld) == y)
			x--;
	return x;
}

static void init_line_desc(Point *pp0, Point *pp1, LineDesc *ld)
{
	long dx, dy, t;
	int swapped;
	Point p0, p1;

	swapped = 0;
	p0 = *pp0;
	p1 = *pp1;
	ld->xmajor = 1;
	ld->slopeneg = 0;
	dx = p1.x - p0.x;
	dy = p1.y - p0.y;
	if (abs(dy) > abs(dx)) {	/* steep */
		ld->xmajor = 0;
		XYswap(&p0);
		XYswap(&p1);
		Swap(dx, dy);
	}
	if (dx < 0) {
		swapped++;
		Swap(p0.x, p1.x);
		Swap(p0.y, p1.y);
		dx = -dx;
		dy = -dy;
	}
	if (dy < 0) {
		ld->slopeneg = 1;
		dy = -dy;
		p0.y = -p0.y;
	}
	ld->dminor = dy;
	ld->dmajor = dx;
	ld->x0 = p0.x;
	ld->y0 = p0.y;
	p1.x = swapped? p0.x : p1.x;
	p1.y = find_height(p1.x, ld);
	if (ld->xmajor == 0) {
		XYswap(&p0);
		XYswap(&p1);
	}
	if (pp0->x > pp1->x) {
		*pp1 = *pp0;
		*pp0 = p1;
	} else
		*pp1 = p1;
}

/*
 *  Cohen-Sutherland clip line to rectangle algorithm.
 */

static int app_intersection_code(Point *p, Rect *r)
{
	return ( (p->x < r->x ? 1 : p->x >= r->x + r->width  ? 2 : 0) |
		 (p->y < r->y ? 4 : p->y >= r->y + r->height ? 8 : 0));
}

int app_clip_line_to_rect(Rect r, Point *p0, Point *p1)
{
	int c0, c1, n;
	long t, result;
	Point temp;
	int swapped;
	LineDesc ld;

	if (p0->x == p1->x && p0->y == p1->y) {
		if ((p0->x >= r.x) && (p0->x < r.x+r.width) &&
		    (p0->y >= r.y) && (p0->y < r.y+r.height))
			return 1;
		else
			return 0;
	}

	init_line_desc(p0, p1, &ld);

	if (ld.xmajor == 0) {
		XYswap(p0);
		XYswap(p1);
		t = r.x; r.x = r.y; r.y = t;
		t = r.width; r.width = r.height; r.height = t;
	}
	c0 = app_intersection_code(p0, &r);
	c1 = app_intersection_code(p1, &r);
	result = 1;
	swapped = 0;
	n = 0;
	while (c0 | c1) {
		if (c0 & c1) {	/* no point of line in r */
			result = 0;
			break;
		}
		if (++n > 10) {	/* horrible points; overflow etc. etc. */
			result = 0;
			break;
		}
		if (c0 == 0) {	/* swap points */
			temp = *p0;
			*p0 = *p1;
			*p1 = temp;
			Swap(c0, c1);
			swapped ^= 1;
		}
		if (c0 == 0)
			break;
		if (c0 & 1) {		/* push towards left edge */
			p0->x = r.x;
			p0->y = find_height(p0->x, &ld);
		}
		else if (c0 & 2) {	/* push towards right edge */
			p0->x = r.x+r.width-1;
			p0->y = find_height(p0->x, &ld);
		}
		else if (c0 & 4) {	/* push towards top edge */
			p0->y = r.y;
			p0->x = find_width(p0->y, &ld);
		}
		else if (c0 & 8) {	/* push towards bottom edge */
			p0->y = r.y+r.height-1;
			p0->x = find_width(p0->y, &ld);
		}
		c0 = app_intersection_code(p0, &r);
	}

	if (ld.xmajor == 0) {
		XYswap(p0);
		XYswap(p1);
	}
	if (swapped) {
		temp = *p0;
		*p0 = *p1;
		*p1 = temp;
	}
	return result;
}
