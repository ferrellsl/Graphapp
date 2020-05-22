/*
 *  Drawing functions.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/24  Updated some portable drawing functions.
 *  Version: 3.34  2001/12/18  Debugging code is now better encapsulated.
 *  Version: 3.41  2003/03/14  Added app_fill_region function.
 *  Version: 3.45  2003/05/05  Included stdlib for abs() definition.
 *  Version: 3.47  2003/05/28  Fixed round-off error in boundary_point.
 *  Version: 3.56  2005/08/09  Silenced some double to int conversions.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "app.h"

/*
 *  Some useful definitions.
 */

#define PI (3.14159265359)

#define ABS(i)    (((i)<0)?(0-(i)):(i))
#define SIGN(i)   (((i)<0)?-1:(((i)>0)?1:0))
#define SQUARE(i) ((long)(i)*(i))


#define DEGREES_TO_RADIANS(deg) ((deg)*2*PI/360)

/*
 *  Debugging code:
 */

#ifdef DRAWING_DEBUG

  static int which_rect = 0;
  static Font *small_font = NULL;
  #define MUL 14

  #define START_DEBUG() (which_rect = 0)

  static int debug_fill_rect(Graphics *g, Rect r)
  {
	char buf[40];
	int buflen;

	if (small_font == NULL)
		small_font = app_new_font(g->app, "plain", PLAIN, 8);

	if ((r.width > 0) && (r.height > 0))
	{
		app_fill_rect(g, rect(r.x*MUL, r.y*MUL,
			r.width*MUL-2, 1));
		app_fill_rect(g, rect(r.x*MUL, r.y*MUL+1,
			1, r.height*MUL-2));
		app_fill_rect(g, rect(r.x*MUL+r.width*MUL-2, r.y*MUL,
			1, r.height*MUL-2));
		app_fill_rect(g, rect(r.x*MUL+1, r.y*MUL+r.height*MUL-2,
			r.width*MUL-2, 1));

		which_rect++;
		sprintf(buf, "%d", which_rect);
		buflen = strlen(buf);
		app_set_font(g, small_font);
		app_draw_utf8(g, pt(r.x*MUL+2, r.y*MUL+1), buf, buflen);
	}
	return 1;
  }
  #undef app_fill_rect
  #define app_fill_rect debug_fill_rect

#else

  #define START_DEBUG()

#endif


/*
 *  Drawing functions:
 */

int app_draw_point(Graphics *g, Point p)
{
	Rect r;

	r.x = p.x;
	r.y = p.y;
	r.width = r.height = 1;

	return app_fill_rect(g, r);
}

int  app_draw_rect(Graphics *g, Rect r)
{
	int result = 1;
	int w = g->line_width;

	if (r.width < 0) {
		r.x += r.width;
		r.width = -r.width;
	}
	if (r.height < 0) {
		r.y += r.height;
		r.height = -r.height;
	}
	if ((w*2 >= r.width) || (w*2 >= r.height))
		return app_fill_rect(g, r);
	else {
		result &= app_fill_rect(g, rect(r.x, r.y, r.width-w, w));
		result &= app_fill_rect(g, rect(r.x, r.y+w, w, r.height-w));
		result &= app_fill_rect(g, rect(r.x+r.width-w, r.y,
				w, r.height-w));
		result &= app_fill_rect(g, rect(r.x+w, r.y+r.height-w,
				r.width-w, w));
	}
	return result;
}

int app_draw_shadow_rect(Graphics *g, Rect r, Colour c1, Colour c2)
{
	int i, result = 1;
	int w = g->line_width;
	Colour old = g->colour;

	if ((r.width == 0) || (r.height == 0))
		return 1;

	/* Draw top-left button border. */
	app_set_colour(g, c1);
	for (i=0; i < w; i++) {
		result &= app_fill_rect(g, rect(r.x+i, r.y+i,
				r.width-1-i-i, 1));
		result &= app_fill_rect(g, rect(r.x+i, r.y+1+i,
				1, r.height-2-i-i));
	}

	/* Draw bottom-right button border. */
	app_set_colour(g, c2);
	for (i=0; i < w; i++) {
		result &= app_fill_rect(g, rect(r.x+r.width-1-i, r.y+i,
				1, r.height-i-i));
		result &= app_fill_rect(g, rect(r.x+i, r.y+r.height-1-i,
				r.width-1-i-i, 1));
	}
	app_set_colour(g, old);
	return result;
}

int app_texture_rect(Graphics *g, Rect dr, Graphics *src, Rect sr)
{
	long x, y, sw, sh, sdx, sdy;
	long right, bottom;
	Rect r;
	int result = 1;

	sw = sr.width;
	sh = sr.height;
	right = dr.x + dr.width;
	bottom = dr.y + dr.height;

	for (y=dr.y; y < bottom; y+=sh)
	{
		for (x=dr.x; x < right; x+=sw)
		{
			/* reduce size of source rectangle for clipping */
			if (x+sw > right)
				sdx = right - x;
			else
				sdx = sw;

			if (y+sh > bottom)
				sdy = bottom - y;
			else
				sdy = sh;

			r = rect(sr.x, sr.y, sdx, sdy);
			result &= app_copy_rect(g, pt(x,y), src, r);
		}
	}
	return result;
}

int app_fill_region(Graphics *g, Region *reg)
{
	int i, result = 1;

	for (i=0; i < reg->num_rects; i++)
		result &= app_fill_rect(g, reg->rects[i]);
	return result;
}

/*
 *  Run-length slice line drawing:
 *
 *  This is based on Bresenham's line-slicing algorithm, which is
 *  faster than the traditional Bresenham's line drawing algorithm,
 *  and better suited for drawing filled rectangles instead of
 *  individual pixels.
 *
 *  It essentially reverses the ordinary Bresenham's logic;
 *  instead of keeping an error term which counts along the
 *  direction of travel (the major axis), it keeps an error
 *  term perpendicular to the major axis, to determine when
 *  to step to the next run of pixels.
 *
 *  See Michael Abrash's Graphics Programming Black Book on-line
 *  at http://www.ddj.com/articles/2001/0165/0165f/0165f.htm
 *  chapter 36 (and 35 and 37) for more details.
 *
 *  The algorithm can also draw lines with a thickness greater
 *  than 1 pixel. In that case, the line hangs below and to
 *  the right of the end points.
 */

int app_portable_draw_line(Graphics *g, Point p1, Point p2)
{
	int x1, y1, x2, y2;
	int temp, adj_up, adj_down, error_term, xadvance, dx, dy;
	int whole_step, initial_run, final_run, i, run_length;
	int w = g->line_width;
	int result = 1;
	Rect r;

	START_DEBUG();

	x1 = p1.x;
	y1 = p1.y;
	x2 = p2.x;
	y2 = p2.y;

	/* Figure out whether we're going left or right, and how
	 * far we're going horizontally */

	if ((dx = x2 - x1) < 0)
	{
		xadvance = -1;
		dx = -dx;
	}
	else
	{
		xadvance = 1;
	}

	/* We'll always draw top to bottom, to reduce the number of
	 * cases we have to handle */

	if ((dy = y2 - y1) < 0) {
		temp = y1;
		y1 = y2;
		y2 = temp;
		temp = x1;
		x1 = x2;
		x2 = temp;
		xadvance = -xadvance;
		dy = -dy;
	}

	/* Special-case horizontal, vertical, and diagonal lines,
	 * for speed and to avoid nasty boundary conditions and
	 * division by 0 */

	if (dx == 0)
	{
		/* Vertical line */
		r.x = x1;
		r.y = y1;
		r.width = w;
		r.height = dy+1;
		return app_fill_rect(g, r);
	}
	if (dy == 0)
	{
		/* Horizontal line */
		if (x1 < x2)
			r.x = x1;
		else
			r.x = x2;
		r.y = y1;
		r.width = dx+1;
		r.height = w;
		return app_fill_rect(g, r);
	}
	if (dx == dy)
	{
		/* Diagonal line */

		r.x = x1;
		r.y = y1;
		r.width = w;
		r.height = 1;
		for (i=0; i < dx+1; i++)
		{
			result &= app_fill_rect(g, r);
			r.x += xadvance;
			r.y++;
		}
		return result;
	}

	/* Determine whether the line is more horizontal or vertical,
	 * and handle accordingly */

	if (dx >= dy)
	{
		/* More horizontal than vertical */

		if (xadvance < 0)
			x1++, x2++;

		/* Minimum # of pixels in a run in this line */
		whole_step = dx / dy;

		/* Error term adjust each time Y steps by 1; used to
		 * tell when one extra pixel should be drawn as part
		 * of a run, to account for fractional steps along
		 * the X axis per 1-pixel steps along Y */
		adj_up = (dx % dy) * 2;

		/* Error term adjust when the error term turns over,
		 * used to factor out the X step made at that time */
		adj_down = dy * 2;

		/* Initial error term; reflects an initial step of 0.5
		 * along the Y axis */
		error_term = (dx % dy) - (dy * 2);

		/* The initial and last runs are partial, because Y
		 * advances only 0.5 for these runs, rather than 1.
		 * Divide one full run, plus the initial pixel, between
		 * the initial and last runs */
		initial_run = (whole_step / 2) + 1;
		final_run = initial_run;

		/* If the basic run length is even and there's no
		 * fractional advance, we have one pixel that could
		 * go to either the initial or last partial run, which
		 * we'll arbitrarily allocate to the last run */
		if ((adj_up == 0) && ((whole_step & 0x01) == 0))
		{
			initial_run--;
		}

		/* If there're an odd number of pixels per run, we
		 * have 1 pixel that can't be allocated to either
		 * the initial or last partial run, so we'll add 0.5
		 * to error term so this pixel will be handled by
		 * the normal full-run loop */
		if ((whole_step & 0x01) != 0)
		{
			error_term += dy;
		}

		/* Draw the first, partial run of pixels */
		r.x = x1;
		r.y = y1;
		r.height = w;
		r.width = initial_run;
		if (xadvance < 0) {
			r.x -= r.width;
			result &= app_fill_rect(g, r);
		} else {
			result &= app_fill_rect(g, r);
			r.x += r.width;
		}
		r.y ++;

		/* Draw all full runs */
		for (i=1; i < dy; i++)
		{
			run_length = whole_step;  /* at least */

			/* Advance the error term and add an extra
			 * pixel if the error term so indicates */
			if ((error_term += adj_up) > 0)
			{
				run_length++;
				error_term -= adj_down;   /* reset */
			}

			/* Draw this scan line's run */
			r.width = run_length;
			if (xadvance < 0) {
				r.x -= r.width;
				result &= app_fill_rect(g, r);
			} else {
				result &= app_fill_rect(g, r);
				r.x += r.width;
			}
			r.y ++;
		}
		/* Draw the final run of pixels */
		r.width = final_run;
		if (xadvance < 0)
			r.x -= r.width;
		result &= app_fill_rect(g, r);
	}
	else
	{
		/* More vertical than horizontal */

		/* Minimum # of pixels in a run in this line */
		whole_step = dy / dx;

		/* Error term adjust each time X steps by 1; used to
		 * tell when 1 extra pixel should be drawn as part of
		 * a run, to account for fractional steps along the
		 * Y axis per 1-pixel steps along X */
		adj_up = (dy % dx) * 2;

		/* Error term adjust when the error term turns over,
		 * used to factor out the Y step made at that time */
		adj_down = dx * 2;

		/* Initial error term; reflects initial step of 0.5
		 * along the X axis */
		error_term = (dy % dx) - (dx * 2);

		/* The initial and last runs are partial, because
		 * X advances only 0.5 for these runs, rather than 1.
		 * Divide one full run, plus the initial pixel,
		 * between the initial and last runs */
		initial_run = (whole_step / 2) + 1;
		final_run = initial_run;

		/* If the basic run length is even and there's no
		 * fractional advance, we have 1 pixel that could
		 * go to either the initial or last partial run,
		 * which we'll arbitrarily allocate to the last run */
		if ((adj_up == 0) && ((whole_step & 0x01) == 0))
		{
			initial_run--;
		}

		/* If there are an odd number of pixels per run, we
		 * have one pixel that can't be allocated to either
		 * the initial or last partial run, so we'll add 0.5
		 * to the error term so this pixel will be handled
		 * by the normal full-run loop */
		if ((whole_step & 0x01) != 0)
		{
			error_term += dx;
		}

		/* Draw the first, partial run of pixels */
		r.x = x1;
		r.y = y1;
		r.height = initial_run;
		r.width = w;
		result &= app_fill_rect(g, r);
		r.x += xadvance;
		r.y += r.height;

		/* Draw all full runs */
		for (i=1; i < dx; i++)
		{
			run_length = whole_step;  /* at least */

			/* Advance the error term and add an extra
			 * pixel if the error term so indicates */
			if ((error_term += adj_up) > 0)
			{
				run_length++;
				error_term -= adj_down;   /* reset */
			}

			/* Draw this scan line's run */
			r.height = run_length;
			result &= app_fill_rect(g, r);
			r.x += xadvance;
			r.y += r.height;
		}
		/* Draw the final run of pixels */
		r.height = final_run;
		result &= app_fill_rect(g, r);
	}
	return result;
}

/*
 *  Draw the border of a rounded rectangle.
 */
int app_draw_round_rect(Graphics *g, Rect r)
{
	int rx, ry; /* radius in x and y directions */
	int w = g->line_width;
	int result = 1;

	if (r.width < r.height)
		rx = ry = r.width / 3;
	else
		rx = ry = r.height / 3;

	result &= app_draw_arc(g, rect(r.x,r.y,rx+rx,ry+ry), 90, 180);
	result &= app_fill_rect(g, rect(r.x,r.y+ry+1,w,r.height-ry-ry));
	result &= app_draw_arc(g, rect(r.x,r.y+r.height-ry-ry,rx+rx,ry+ry),
			180, 270);
	result &= app_fill_rect(g, rect(r.x+rx,r.y+r.height-w,
			r.width-rx-rx,w));
	result &= app_draw_arc(g, rect(r.x+r.width-rx-rx,r.y+r.height-ry-ry,
			rx+rx,ry+ry), 270, 360);
	result &= app_fill_rect(g, rect(r.x+r.width-w,r.y+ry+1,
			w,r.height-ry-ry));
	result &= app_draw_arc(g, rect(r.x+r.width-rx-rx,r.y,rx+rx,ry+ry),
			0, 90);
	result &= app_fill_rect(g, rect(r.x+rx,r.y,r.width-rx-rx,w));
	return result;
}

/*
 *  Fill a rounded rectangle with colour.
 */
int app_fill_round_rect(Graphics *g, Rect r)
{
	int rx, ry; /* radius in x and y directions */
	int result = 1;

	if (r.width < r.height)
		rx = ry = r.width / 3;
	else
		rx = ry = r.height / 3;

	result &= app_fill_rect(g, rect(r.x, r.y+ry+1,
				r.width, r.height-ry-ry));
	result &= app_fill_rect(g, rect(r.x+rx, r.y,
				r.width-rx-rx, ry+1));
	result &= app_fill_rect(g, rect(r.x+rx, r.y+r.height-ry+1,
				r.width-rx-rx, ry-1));

	result &= app_fill_arc(g, rect(r.x,r.y,rx+rx,ry+ry), 90, 180);
	result &= app_fill_arc(g, rect(r.x,r.y+r.height-ry-ry,rx+rx,ry+ry),
			180, 270);
	result &= app_fill_arc(g, rect(r.x+r.width-rx-rx,r.y+r.height-ry-ry,
			rx+rx,ry+ry), 270, 360);
	result &= app_fill_arc(g, rect(r.x+r.width-rx-rx,r.y,rx+rx,ry+ry),
			0, 90);
	return result;
}

/*
 *  To fill an axis-aligned ellipse, we use a scan-line algorithm.
 *  We walk downwards from the top Y co-ordinate, calculating
 *  the width of the ellipse using incremental integer arithmetic.
 *  To save calculation, we observe that the top and bottom halves
 *  of the ellipsoid are mirror-images, therefore we can draw the
 *  top and bottom halves by reflection. As a result, this algorithm
 *  draws rectangles inwards from the top and bottom edges of the
 *  bounding rectangle.
 *
 *  To save rendering time, draw as few rectangles as possible.
 *  Other ellipse-drawing algorithms assume we want to draw each
 *  line, using a draw_pixel operation, or a draw_horizontal_line
 *  operation. This approach is slower than it needs to be in
 *  circumstances where a fill_rect operation is more efficient
 *  (such as in X-Windows, where there is a communication overhead
 *  to the X-Server). For this reason, the algorithm accumulates
 *  rectangles on adjacent lines which have the same width into a
 *  single larger rectangle.
 *
 *  This algorithm forms the basis of the later, more complex,
 *  draw_ellipse algorithm, which renders the rectangular spaces
 *  between an outer and inner ellipse, and also the draw_arc and
 *  fill_arc operations which additionally clip drawing between
 *  a start_angle and an end_angle.
 *  
 */
int app_fill_ellipse(Graphics *g, Rect r)
{
	/* e(x,y) = b*b*x*x + a*a*y*y - a*a*b*b */

	int a = r.width / 2;
	int b = r.height / 2;
	int x = 0;
	int y = b;
	long a2 = a*a;
	long b2 = b*b;
	long xcrit = (3 * a2 / 4) + 1;
	long ycrit = (3 * b2 / 4) + 1;
	long t = b2 + a2 - 2*a2*b;	/* t = e(x+1,y-1) */
	long dxt = b2*(3+x+x);
	long dyt = a2*(3-y-y);
	int d2xt = b2+b2;
	int d2yt = a2+a2;
	Rect r1, r2;
	int result = 1;

	START_DEBUG();

	if ((r.width <= 2) || (r.height <= 2))
		return app_fill_rect(g, r);

	r1.x = r.x + a;
	r1.y = r.y;
	r1.width = r.width & 1; /* i.e. if width is odd */
	r1.height = 1;

	r2 = r1;
	r2.y = r.y + r.height - 1;

	while (y > 0)
	{
		if (t + a2*y < xcrit) { /* e(x+1,y-1/2) <= 0 */
			/* move outwards to encounter edge */
			x += 1;
			t += dxt;
			dxt += d2xt;

			/* move outwards */
			r1.x -= 1; r1.width += 2;
			r2.x -= 1; r2.width += 2;
		}
		else if (t - b2*x >= ycrit) { /* e(x+1/2,y-1) > 0 */
			/* drop down one line */
			y -= 1;
			t += dyt;
			dyt += d2yt;

			/* enlarge rectangles */
			r1.height += 1;
			r2.height += 1; r2.y -= 1;
		}
		else {
			/* drop diagonally down and out */
			x += 1;
			y -= 1;
			t += dxt + dyt;
			dxt += d2xt;
			dyt += d2yt;

			if ((r1.width > 0) && (r1.height > 0))
			{
				/* draw rectangles first */

				if (r1.y+r1.height < r2.y) {
					/* distinct rectangles */
					result &= app_fill_rect(g, r1);
					result &= app_fill_rect(g, r2);
				}

				/* move down */
				r1.y += r1.height; r1.height = 1;
				r2.y -= 1;         r2.height = 1;
			}
			else {
				/* skipped pixels on initial diagonal */

				/* enlarge, rather than moving down */
				r1.height += 1;
				r2.height += 1; r2.y -= 1;
			}

			/* move outwards */
			r1.x -= 1; r1.width += 2;
			r2.x -= 1; r2.width += 2;
		}
	}
	if (r1.y < r2.y) {
		/* overlap */
		r1.x = r.x;
		r1.width = r.width;
		r1.height = r2.y+r2.height-r1.y;
		result &= app_fill_rect(g, r1);
	}
	else if (x <= a) {
		/* crossover, draw final line */
		r1.x = r.x;
		r1.width = r.width;
		r1.height = r1.y+r1.height-r2.y;
		r1.y = r2.y;
		result &= app_fill_rect(g, r1);
	}
	return result;
}

/*
 *  Drawing an ellipse with a certain line thickness.
 *  Use an inner and and outer ellipse and fill the spaces between.
 *  The inner ellipse uses all UPPERCASE letters, the outer lowercase.
 *
 *  This algorithm is based on the fill_ellipse algorithm presented
 *  above, but uses two ellipse calculations, and some fix-up code
 *  to avoid pathological cases where the inner ellipse is almost
 *  the same size as the outer (in which case the border of the
 *  elliptical curve might otherwise have appeared broken).
 */
int app_draw_ellipse(Graphics *g, Rect r)
{
	/* Outer ellipse: e(x,y) = b*b*x*x + a*a*y*y - a*a*b*b */

	int a = r.width / 2;
	int b = r.height / 2;
	int x = 0;
	int y = b;
	long a2 = a*a;
	long b2 = b*b;
	long xcrit = (3 * a2 / 4) + 1;
	long ycrit = (3 * b2 / 4) + 1;
	long t = b2 + a2 - 2*a2*b;	/* t = e(x+1,y-1) */
	long dxt = b2*(3+x+x);
	long dyt = a2*(3-y-y);
	int d2xt = b2+b2;
	int d2yt = a2+a2;

	int w = g->line_width;

	/* Inner ellipse: E(X,Y) = B*B*X*X + A*A*Y*Y - A*A*B*B */

	int A = a-w > 0 ? a-w : 0;
	int B = b-w > 0 ? b-w : 0;
	int X = 0;
	int Y = B;
	long A2 = A*A;
	long B2 = B*B;
	long XCRIT = (3 * A2 / 4) + 1;
	long YCRIT = (3 * B2 / 4) + 1;
	long T = B2 + A2 - 2*A2*B;	/* T = E(X+1,Y-1) */
	long DXT = B2*(3+X+X);
	long DYT = A2*(3-Y-Y);
	int D2XT = B2+B2;
	int D2YT = A2+A2;

	int movedown, moveout;
	int innerX = 0, prevx, prevy, W;
	Rect r1, r2;
	int result = 1;

	START_DEBUG();

	if ((r.width <= 2) || (r.height <= 2))
		return app_fill_rect(g, r);

	r1.x = r.x + a;
	r1.y = r.y;
	r1.width = r.width & 1; /* i.e. if width is odd */
	r1.height = 1;

	r2 = r1;
	r2.y = r.y + r.height - 1;

	prevx = r1.x;
	prevy = r1.y;

	while (y > 0)
	{
		while (Y == y)
		{
			innerX = X;

			if (T + A2*Y < XCRIT) /* E(X+1,Y-1/2) <= 0 */
			{
				/* move outwards to encounter edge */
				X += 1;
				T += DXT;
				DXT += D2XT;
			}
			else if (T - B2*X >= YCRIT) /* e(x+1/2,y-1) > 0 */
			{
				/* drop down one line */
				Y -= 1;
				T += DYT;
				DYT += D2YT;
			}
			else {
				/* drop diagonally down and out */
				X += 1;
				Y -= 1;
				T += DXT + DYT;
				DXT += D2XT;
				DYT += D2YT;
			}
		}

		movedown = moveout = 0;

		W = x - innerX;
		if (r1.x + W < prevx)
			W = prevx - r1.x;
		if (W < w)
			W = w;

		if (t + a2*y < xcrit) /* e(x+1,y-1/2) <= 0 */
		{
			/* move outwards to encounter edge */
			x += 1;
			t += dxt;
			dxt += d2xt;

			moveout = 1;
		}
		else if (t - b2*x >= ycrit) /* e(x+1/2,y-1) > 0 */
		{
			/* drop down one line */
			y -= 1;
			t += dyt;
			dyt += d2yt;

			movedown = 1;
		}
		else {
			/* drop diagonally down and out */
			x += 1;
			y -= 1;
			t += dxt + dyt;
			dxt += d2xt;
			dyt += d2yt;

			movedown = 1;
			moveout = 1;
		}

		if (movedown) {
			if (r1.width == 0) {
				r1.x -= 1; r1.width += 2;
				r2.x -= 1; r2.width += 2;
				moveout = 0;
			}

			if (r1.x < r.x)
				r1.x = r2.x = r.x;
			if (r1.width > r.width)
				r1.width = r2.width = r.width;
			if (r1.y == r2.y-1) {
				r1.x = r2.x = r.x;
				r1.width = r2.width = r.width;
			}

			if ((r1.y < r.y+w) || (r1.x+W >= r1.x+r1.width-W))
			{
				result &= app_fill_rect(g, r1);
				result &= app_fill_rect(g, r2);

				prevx = r1.x;
				prevy = r1.y;
			}
			else if (r1.y+r1.height < r2.y)
			{
				/* draw distinct rectangles */
				result &= app_fill_rect(g, rect(r1.x,r1.y,
						W,1));
				result &= app_fill_rect(g, rect(
						r1.x+r1.width-W,r1.y,W,1));
				result &= app_fill_rect(g, rect(r2.x,
						r2.y,W,1));
				result &= app_fill_rect(g, rect(
						r2.x+r2.width-W,r2.y,W,1));

				prevx = r1.x;
				prevy = r1.y;
			}

			/* move down */
			r1.y += 1;
			r2.y -= 1;
		}

		if (moveout) {
			/* move outwards */
			r1.x -= 1; r1.width += 2;
			r2.x -= 1; r2.width += 2;
		}
	}
	if ((x <= a) && (prevy < r2.y)) {
		/* draw final line */
		r1.height = r1.y+r1.height-r2.y;
		r1.y = r2.y;

		W = w;
		if (r.x + W != prevx)
			W = prevx - r.x;
		if (W < w)
			W = w;

		if (W+W >= r.width) {
			result &= app_fill_rect(g, rect(r.x, r1.y,
				r.width, r1.height));
			return result;
		}

		result &= app_fill_rect(g, rect(r.x, r1.y, W, r1.height));
		result &= app_fill_rect(g, rect(r.x+r.width-W, r1.y,
			W, r1.height));
	}
	return result;
}

/*
 *  Draw an arc of an ellipse from start_angle anti-clockwise to
 *  end_angle. If the angles coincide, draw nothing; if they
 *  differ by 360 degrees or more, draw a full ellipse.
 *  The shape is drawn with the current line thickness,
 *  completely within the bounding rectangle. The shape is also
 *  axis-aligned, so that the ellipse would be horizontally and
 *  vertically symmetric is it was complete.
 *
 *  The draw_arc algorithm is based on draw_ellipse, but unlike
 *  that algorithm is not symmetric in the general case, since
 *  an angular portion is clipped from the shape. 
 *  This clipping is performed by keeping track of two hypothetical
 *  lines joining the centre point to the enclosing rectangle,
 *  at the angles start_angle and end_angle, using a line-intersection
 *  algorithm. Essentially the algorithm just fills the spaces
 *  which are within the arc and also between the angles, going
 *  in an anti-clockwise direction from start_angle to end_angle.
 *  In the top half of the ellipse, this amounts to drawing
 *  to the left of the start_angle line and to the right of
 *  the end_angle line, while in the bottom half of the ellipse,
 *  it involves drawing to the right of the start_angle and to
 *  the left of the end_angle.
 */

/*
 *  Fill a rectangle within an arc, given the centre point p0,
 *  and the two end points of the lines corresponding to the
 *  start_angle and the end_angle. This function takes care of
 *  the logic needed to swap the fill direction below
 *  the central point, and also performs the calculations
 *  needed to intersect the current Y value with each line.
 */
static int app_fill_arc_rect(Graphics *g, Rect r,
	Point p0, Point p1, Point p2, int start_angle, int end_angle)
{
	int x1, x2;
	int start_above, end_above;
	long rise1, run1, rise2, run2;

	rise1 = p1.y - p0.y;
	run1  = p1.x - p0.x;
	rise2 = p2.y - p0.y;
	run2  = p2.x - p0.x;

	if (r.y <= p0.y) {
		/* in top half of arc ellipse */

		if (p1.y <= r.y) {
			/* start_line is in the top half and is */
			/* intersected by the current Y scan line */
			if (rise1 == 0)
				x1 = p1.x;
			else
				x1 = p0.x + (r.y-p0.y)*run1/rise1;
			start_above = 1;
		}
		else if ((start_angle >= 0) && (start_angle <= 180)) {
			/* start_line is above middle */
			x1 = p1.x;
			start_above = 1;
		}
		else {
			/* start_line is below middle */
			x1 = r.x + r.width;
			start_above = 0;
		}
		if (x1 < r.x)
			x1 = r.x;
		if (x1 > r.x+r.width)
			x1 = r.x+r.width;

		if (p2.y <= r.y) {
			/* end_line is in the top half and is */
			/* intersected by the current Y scan line */
			if (rise2 == 0)
				x2 = p2.x;
			else
				x2 = p0.x + (r.y-p0.y)*run2/rise2;
			end_above = 1;
		}
		else if ((end_angle >= 0) && (end_angle <= 180)) {
			/* end_line is above middle */
			x2 = p2.x;
			end_above = 1;
		}
		else {
			/* end_line is below middle */
			x2 = r.x;
			end_above = 0;
		}
		if (x2 < r.x)
			x2 = r.x;
		if (x2 > r.x+r.width)
			x2 = r.x+r.width;

		if (start_above && end_above) {
			if (start_angle > end_angle) {
				/* fill outsides of wedge */
				if (! app_fill_rect(g, rect(r.x, r.y,
					x1-r.x, r.height)))
					return 0;
				return app_fill_rect(g, rect(x2, r.y,
					r.x+r.width-x2, r.height));
			}
			else {
				/* fill inside of wedge */
				r.width = x1-x2;
				r.x = x2;
				return app_fill_rect(g, r);
			}
		}
		else if (start_above) {
			/* fill to the left of the start_line */
			r.width = x1-r.x;
			return app_fill_rect(g, r);
		}
		else if (end_above) {
			/* fill right of end_line */
			r.width = r.x+r.width-x2;
			r.x = x2;
			return app_fill_rect(g, r);
		}
		else {
			if (start_angle > end_angle)
				return app_fill_rect(g,r);
			else
				return 1;
		}
	}
	else {
		/* in lower half of arc ellipse */

		if (p1.y >= r.y) {
			/* start_line is in the lower half and is */
			/* intersected by the current Y scan line */
			if (rise1 == 0)
				x1 = p1.x;
			else
				x1 = p0.x + (r.y-p0.y)*run1/rise1;
			start_above = 0;
		}
		else if ((start_angle >= 180) && (start_angle <= 360)) {
			/* start_line is below middle */
			x1 = p1.x;
			start_above = 0;
		}
		else {
			/* start_line is above middle */
			x1 = r.x;
			start_above = 1;
		}
		if (x1 < r.x)
			x1 = r.x;
		if (x1 > r.x+r.width)
			x1 = r.x+r.width;

		if (p2.y >= r.y) {
			/* end_line is in the lower half and is */
			/* intersected by the current Y scan line */
			if (rise2 == 0)
				x2 = p2.x;
			else
				x2 = p0.x + (r.y-p0.y)*run2/rise2;
			end_above = 0;
		}
		else if ((end_angle >= 180) && (end_angle <= 360)) {
			/* end_line is below middle */
			x2 = p2.x;
			end_above = 0;
		}
		else {
			/* end_line is above middle */
			x2 = r.x + r.width;
			end_above = 1;
		}
		if (x2 < r.x)
			x2 = r.x;
		if (x2 > r.x+r.width)
			x2 = r.x+r.width;

		if (start_above && end_above) {
			if (start_angle > end_angle)
				return app_fill_rect(g,r);
			else
				return 1;
		}
		else if (start_above) {
			/* fill to the left of end_line */
			r.width = x2-r.x;
			return app_fill_rect(g,r);
		}
		else if (end_above) {
			/* fill right of start_line */
			r.width = r.x+r.width-x1;
			r.x = x1;
			return app_fill_rect(g,r);
		}
		else {
			if (start_angle > end_angle) {
				/* fill outsides of wedge */
				if (! app_fill_rect(g, rect(r.x, r.y,
					x2-r.x, r.height)))
					return 0;
				return app_fill_rect(g, rect(x1, r.y,
					r.x+r.width-x1, r.height));
			}
			else {
				/* fill inside of wedge */
				r.width = x2-x1;
				r.x = x1;
				return app_fill_rect(g, r);
			}
		}
	}
}

static Point app_boundary_point(Rect r, int angle)
{
	int cx, cy;
	double tangent;

	cx = r.width;
	cx /= 2;
	cx += r.x;

	cy = r.height;
	cy /= 2;
	cy += r.y;

	if (angle == 0)
		return pt(r.x+r.width, cy);
	else if (angle == 45)
		return pt(r.x+r.width, r.y);
	else if (angle == 90)
		return pt(cx, r.y);
	else if (angle == 135)
		return pt(r.x, r.y);
	else if (angle == 180)
		return pt(r.x, cy);
	else if (angle == 225)
		return pt(r.x, r.y+r.height);
	else if (angle == 270)
		return pt(cx, r.y+r.height);
	else if (angle == 315)
		return pt(r.x+r.width, r.y+r.height);

	tangent = tan(DEGREES_TO_RADIANS(angle));

	if ((angle > 45) && (angle < 135))
		return pt((int)(cx+r.height/tangent/2), r.y);
	else if ((angle > 225) && (angle < 315))
		return pt((int)(cx-r.height/tangent/2), r.y+r.height);
	else if ((angle > 135) && (angle < 225))
		return pt(r.x, (int)(cy+r.width*tangent/2));
	else
		return pt(r.x+r.width, (int)(cy-r.width*tangent/2));
}

int app_draw_arc(Graphics *g, Rect r, int start_angle, int end_angle)
{
	/* Outer ellipse: e(x,y) = b*b*x*x + a*a*y*y - a*a*b*b */

	int a = r.width / 2;
	int b = r.height / 2;
	int x = 0;
	int y = b;
	long a2 = a*a;
	long b2 = b*b;
	long xcrit = (3 * a2 / 4) + 1;
	long ycrit = (3 * b2 / 4) + 1;
	long t = b2 + a2 - 2*a2*b;	/* t = e(x+1,y-1) */
	long dxt = b2*(3+x+x);
	long dyt = a2*(3-y-y);
	int d2xt = b2+b2;
	int d2yt = a2+a2;

	int w = g->line_width;

	/* Inner ellipse: E(X,Y) = B*B*X*X + A*A*Y*Y - A*A*B*B */

	int A = a-w > 0 ? a-w : 0;
	int B = b-w > 0 ? b-w : 0;
	int X = 0;
	int Y = B;
	long A2 = A*A;
	long B2 = B*B;
	long XCRIT = (3 * A2 / 4) + 1;
	long YCRIT = (3 * B2 / 4) + 1;
	long T = B2 + A2 - 2*A2*B;	/* T = E(X+1,Y-1) */
	long DXT = B2*(3+X+X);
	long DYT = A2*(3-Y-Y);
	int D2XT = B2+B2;
	int D2YT = A2+A2;

	/* arc rectangle calculations */
	int movedown, moveout;
	int innerX = 0, prevx, prevy, W;
	Rect r1, r2;
	int result = 1;

	/* line descriptions */
	Point p0, p1, p2;

	START_DEBUG();

	/* if angles differ by 360 degrees or more, close the shape */
	if ((start_angle + 360 <= end_angle) ||
	    (start_angle - 360 >= end_angle))
	{
		return app_draw_ellipse(g, r);
	}

	/* make start_angle >= 0 and <= 360 */
	while (start_angle < 0)
		start_angle += 360;
	start_angle %= 360;

	/* make end_angle >= 0 and <= 360 */
	while (end_angle < 0)
		end_angle += 360;
	end_angle %= 360;

	/* draw nothing if the angles are equal */
	if (start_angle == end_angle)
		return 1;

	/* find arc wedge line end points */
	p0 = pt(r.x + r.width/2, r.y + r.height/2);
	p1 = app_boundary_point(r, start_angle);
	p2 = app_boundary_point(r, end_angle);

	/* determine ellipse rectangles */
	r1.x = r.x + a;
	r1.y = r.y;
	r1.width = r.width & 1; /* i.e. if width is odd */
	r1.height = 1;

	r2 = r1;
	r2.y = r.y + r.height - 1;

	prevx = r1.x;
	prevy = r1.y;

	while (y > 0)
	{
		while (Y == y)
		{
			innerX = X;

			if (T + A2*Y < XCRIT) /* E(X+1,Y-1/2) <= 0 */
			{
				/* move outwards to encounter edge */
				X += 1;
				T += DXT;
				DXT += D2XT;
			}
			else if (T - B2*X >= YCRIT) /* e(x+1/2,y-1) > 0 */
			{
				/* drop down one line */
				Y -= 1;
				T += DYT;
				DYT += D2YT;
			}
			else {
				/* drop diagonally down and out */
				X += 1;
				Y -= 1;
				T += DXT + DYT;
				DXT += D2XT;
				DYT += D2YT;
			}
		}

		movedown = moveout = 0;

		W = x - innerX;
		if (r1.x + W < prevx)
			W = prevx - r1.x;
		if (W < w)
			W = w;

		if (t + a2*y < xcrit) /* e(x+1,y-1/2) <= 0 */
		{
			/* move outwards to encounter edge */
			x += 1;
			t += dxt;
			dxt += d2xt;

			moveout = 1;
		}
		else if (t - b2*x >= ycrit) /* e(x+1/2,y-1) > 0 */
		{
			/* drop down one line */
			y -= 1;
			t += dyt;
			dyt += d2yt;

			movedown = 1;
		}
		else {
			/* drop diagonally down and out */
			x += 1;
			y -= 1;
			t += dxt + dyt;
			dxt += d2xt;
			dyt += d2yt;

			movedown = 1;
			moveout = 1;
		}

		if (movedown) {
			if (r1.width == 0) {
				r1.x -= 1; r1.width += 2;
				r2.x -= 1; r2.width += 2;
				moveout = 0;
			}

			if (r1.x < r.x)
				r1.x = r2.x = r.x;
			if (r1.width > r.width)
				r1.width = r2.width = r.width;
			if (r1.y == r2.y-1) {
				r1.x = r2.x = r.x;
				r1.width = r2.width = r.width;
			}

			if ((r1.y < r.y+w) || (r1.x+W >= r1.x+r1.width-W))
			{
				result &= app_fill_arc_rect(g, r1,
						p0, p1, p2,
						start_angle, end_angle);
				result &= app_fill_arc_rect(g, r2,
						p0, p1, p2,
						start_angle, end_angle);

				prevx = r1.x;
				prevy = r1.y;
			}
			else if (r1.y+r1.height < r2.y)
			{
				/* draw distinct rectangles */
				result &= app_fill_arc_rect(g, rect(
						r1.x,r1.y,W,1),
						p0, p1, p2,
						start_angle, end_angle);
				result &= app_fill_arc_rect(g, rect(
						r1.x+r1.width-W,r1.y,W,1),
						p0, p1, p2,
						start_angle, end_angle);
				result &= app_fill_arc_rect(g, rect(
						r2.x,r2.y,W,1),
						p0, p1, p2,
						start_angle, end_angle);
				result &= app_fill_arc_rect(g, rect(
						r2.x+r2.width-W,r2.y,W,1),
						 p0, p1, p2,
						start_angle, end_angle);

				prevx = r1.x;
				prevy = r1.y;
			}

			/* move down */
			r1.y += 1;
			r2.y -= 1;
		}

		if (moveout) {
			/* move outwards */
			r1.x -= 1; r1.width += 2;
			r2.x -= 1; r2.width += 2;
		}
	}
	if ((x <= a) && (prevy < r2.y)) {
		/* draw final lines */
		r1.height = r1.y+r1.height-r2.y;
		r1.y = r2.y;

		W = w;
		if (r.x + W != prevx)
			W = prevx - r.x;
		if (W < w)
			W = w;

		if (W+W >= r.width) {
			while (r1.height > 0) {
				result &= app_fill_arc_rect(g, rect(r.x,
					r1.y, r.width, 1), p0, p1, p2,
					start_angle, end_angle);
				r1.y += 1;
				r1.height -= 1;
			}
			return result;
		}

		while (r1.height > 0) {
			result &= app_fill_arc_rect(g, rect(r.x, r1.y,
					W, 1), p0, p1, p2,
					start_angle, end_angle);
			result &= app_fill_arc_rect(g, rect(r.x+r.width-W,
					r1.y, W, 1), p0, p1, p2,
					start_angle, end_angle);
			r1.y += 1;
			r1.height -= 1;
		}
	}

	return result;
}

int app_fill_arc(Graphics *g, Rect r, int start_angle, int end_angle)
{
	/* e(x,y) = b*b*x*x + a*a*y*y - a*a*b*b */

	int a = r.width / 2;
	int b = r.height / 2;
	int x = 0;
	int y = b;
	long a2 = a*a;
	long b2 = b*b;
	long xcrit = (3 * a2 / 4) + 1;
	long ycrit = (3 * b2 / 4) + 1;
	long t = b2 + a2 - 2*a2*b;	/* t = e(x+1,y-1) */
	long dxt = b2*(3+x+x);
	long dyt = a2*(3-y-y);
	int d2xt = b2+b2;
	int d2yt = a2+a2;
	Rect r1, r2;
	int movedown, moveout;
	int result = 1;

	/* line descriptions */
	Point p0, p1, p2;

	START_DEBUG();

	/* if angles differ by 360 degrees or more, close the shape */
	if ((start_angle + 360 <= end_angle) ||
	    (start_angle - 360 >= end_angle))
	{
		return app_fill_ellipse(g, r);
	}

	/* make start_angle >= 0 and <= 360 */
	while (start_angle < 0)
		start_angle += 360;
	start_angle %= 360;

	/* make end_angle >= 0 and <= 360 */
	while (end_angle < 0)
		end_angle += 360;
	end_angle %= 360;

	/* draw nothing if the angles are equal */
	if (start_angle == end_angle)
		return 1;

	/* find arc wedge line end points */
	p0 = pt(r.x + r.width/2, r.y + r.height/2);
	p1 = app_boundary_point(r, start_angle);
	p2 = app_boundary_point(r, end_angle);

	/* initialise rectangles to be drawn */
	r1.x = r.x + a;
	r1.y = r.y;
	r1.width = r.width & 1; /* i.e. if width is odd */
	r1.height = 1;

	r2 = r1;
	r2.y = r.y + r.height - 1;

	while (y > 0)
	{
		moveout = movedown = 0;

		if (t + a2*y < xcrit) { /* e(x+1,y-1/2) <= 0 */
			/* move outwards to encounter edge */
			x += 1;
			t += dxt;
			dxt += d2xt;

			moveout = 1;
		}
		else if (t - b2*x >= ycrit) { /* e(x+1/2,y-1) > 0 */
			/* drop down one line */
			y -= 1;
			t += dyt;
			dyt += d2yt;

			movedown = 1;
		}
		else {
			/* drop diagonally down and out */
			x += 1;
			y -= 1;
			t += dxt + dyt;
			dxt += d2xt;
			dyt += d2yt;

			moveout = 1;
			movedown = 1;
		}

		if (movedown) {
			if (r1.width == 0) {
				r1.x -= 1; r1.width += 2;
				r2.x -= 1; r2.width += 2;
				moveout = 0;
			}

			if (r1.x < r.x)
				r1.x = r2.x = r.x;
			if (r1.width > r.width)
				r1.width = r2.width = r.width;
			if (r1.y == r2.y-1) {
				r1.x = r2.x = r.x;
				r1.width = r2.width = r.width;
			}

			if ((r1.width > 0) && (r1.y+r1.height < r2.y)) {
				/* distinct rectangles */
				result &= app_fill_arc_rect(g, r1,
						p0, p1, p2,
						start_angle, end_angle);
				result &= app_fill_arc_rect(g, r2,
						p0, p1, p2,
						start_angle, end_angle);
			}

			/* move down */
			r1.y += 1;
			r2.y -= 1;
		}

		if (moveout) {
			/* move outwards */
			r1.x -= 1; r1.width += 2;
			r2.x -= 1; r2.width += 2;
		}
	}
	if (r1.y < r2.y) {
		/* overlap */
		r1.x = r.x;
		r1.width = r.width;
		r1.height = r2.y+r2.height-r1.y;
		while (r1.height > 0) {
			result &= app_fill_arc_rect(g,
				rect(r1.x, r1.y, r1.width, 1),
				p0, p1, p2, start_angle, end_angle);
			r1.y += 1;
			r1.height -= 1;
		}
	}
	else if (x <= a) {
		/* crossover, draw final line */
		r1.x = r.x;
		r1.width = r.width;
		r1.height = r1.y+r1.height-r2.y;
		r1.y = r2.y;
		while (r1.height > 0) {
			result &= app_fill_arc_rect(g, 
				rect(r1.x, r1.y, r1.width, 1),
				p0, p1, p2, start_angle, end_angle);
			r1.y += 1;
			r1.height -= 1;
		}
	}
	return result;
}

/*
 *  Polylines are like polygons, except the final line closing
 *  the shape is not drawn. So we just draw each point connecting
 *  to the next point in the array.
 */
int app_draw_polyline(Graphics *g, Point *p, int n)
{
	int i;
	int result = 1;

	for (i=0; i < n-1; i++)
		result &= app_draw_line(g, p[i], p[i+1]);
	return result;
}

/*
 *  Polygons are drawn simply by using app_draw_line.
 *  The final point is automatically connected back to the first
 *  point, to close the figure.
 */
int app_draw_polygon(Graphics *g, Point *p, int n)
{
	int i;
	int result = 1;

	for (i=0; i < n-1; i++)
		result &= app_draw_line(g, p[i], p[i+1]);
	result &= app_draw_line(g, p[n-1], p[0]); /* close the shape */
	return result;
}

/*
 *  Definitions for polygon-filling code.
 *  This code comes from Michael Abrash's Graphics Programming
 *  Black Book. He was one of the designers of Quake.
 */


/*
 *  Describes beginning and ending X coordinates of one horizontal line
 */
struct HLine {
	int startx;	/* X coordinate of leftmost pixel in line */
	int endx;	/* X coordinate of rightmost pixel in line */
};

/*
 *  Describes a length-long series of horizontal lines, all assumed to
 *  be on contiguous scan lines starting at starty and proceeding
 *  downward (used to describe scan-converted polygon to low-level
 *  hardware-dependent drawing code)
 */
struct HLineList {
	int starty;		/* Y coordinate of topmost line */
	int length;		/* number of horizontal lines */
	struct HLine *lines;	/* pointer to list of horz lines */
};


/*
 *  Draw a list of horizontal lines.
 *  We don't need to swap the start and end X co-ordinates here
 *  if they are around the wrong way, since our fill_rect function
 *  automatically does that if the width of a rectangle is negative.
 */
static int app_draw_horizontal_line_list(Graphics *g, struct HLineList *list)
{
	int i, result = 1;
	Rect r;

	r.height = 1;

	for (i=0, r.y=list->starty; i < list->length; i++, r.y++) {
		r.x = list->lines[i].startx;
		r.width = list->lines[i].endx - r.x;
		result &= app_fill_rect(g, r);
	}
	return result;
}

/*
 *  Returns 1 if polygon described by passed-in vertex list is
 *  monotone with respect to a vertical line, 0 otherwise.
 *  Doesn't matter if polygon is self-intersecting or not.
 *  This function is useful because monotone-vertical polygons
 *  can be drawn very quickly, so this test is used to speed up
 *  the polygon drawing code.
 */
static int app_polygon_is_monotone_vertical(Point *p, int n)
{
	int i, dy_sign, prev_dy_sign;
	int y_reversals = 0;

	/* Three or fewer points must be a monotone-vertical polygon */
	if (n < 4)
		return 1;

	/* Scan to the first non-horizontal edge */
	prev_dy_sign = SIGN(p[n-1].y - p[0].y);
	i = 0;
	while ((prev_dy_sign == 0) && (i < (n-1))) {
		prev_dy_sign = SIGN(p[i].y - p[i+1].y);
		i++;
	}

	if (i == (n-1))
		return 1;  /* polygon is a flat line */

	/* Now count Y reversals. Might miss one reversal, at the
	 * last vertex, but because reversal counts must be even,
	 * being off by one isn't a problem */
	do {
		if ((dy_sign = SIGN(p[i].y - p[i+1].y)) != 0) {
			if (dy_sign != prev_dy_sign) {
				/* Switched Y direction; not
				 * monotone-vertical if reversed Y
				 * direction as many as three times */
				if (++y_reversals > 2)
					return 0;
				prev_dy_sign = dy_sign;
			}
		}
	} while (i++ < (n-1));
	return 1;  /* it's a monotone-vertical polygon */
}

/*
 *  Scan converts an edge from (x1,y1) to (x2,y2), not including the
 *  point at (x2,y2). This avoids overlapping the end of one line with
 *  the start of the next, and causes the bottom scan line of the
 *  polygon not to be drawn. If skip_first != 0, the point at (x1,y1)
 *  isn't drawn. For each scan line, the pixel closest to the scanned
 *  line without being to the left of the scanned line is chosen
 */
static void app_scan_edge(int x1, int y1, int x2, int y2, int set_x_start,
	int skip_first, struct HLine **edge_point_ptr)
{
	int dx, height, width, adv, err, i;
	int err_adv, x_major_adv;
	struct HLine *ep;

	ep = *edge_point_ptr; /* avoid double dereference */
	adv = ((dx = x2 - x1) > 0) ? 1 : -1;
			/* direction in which X moves (y2 is
			   always > y1, so Y always counts up) */

	if ((height = y2 - y1) <= 0)  /* Y length of the edge */
		return;     /* prevent zero length and horizontal edges */

	/* Figure out whether the edge is vertical, diagonal, X-major
	   (mostly horizontal), or Y-major (mostly vertical) and handle
	   appropriately */
	if ((width = abs(dx)) == 0) {
		/* The edge is vertical; special-case by just storing
		 * the same X coordinate for every scan line */

		/* Scan the edge for each scan line in turn */
		for (i = height - skip_first; i-- > 0; ep++) {
			/* Store the X coordinate in correct edge list */
			if (set_x_start == 1)
				ep->startx = x1;
			else
				ep->endx = x1;
		}
	} else if (width == height) {
		/* The edge is diagonal; special-case by advancing the X
		   coordinate 1 pixel for each scan line */

		if (skip_first) /* skip the first point if so indicated */
			x1 += adv; /* move 1 pixel left or right */

		/* Scan the edge for each scan line in turn */
		for (i = height - skip_first; i-- > 0; ep++) {
			/* Store the X coordinate in correct edge list */
			if (set_x_start == 1)
				ep->startx = x1;
			else
				ep->endx = x1;
			x1 += adv; /* move 1 pixel left or right */
		}
	} else if (height > width) {
		/* Edge is closer to vertical than horizontal (Y-major) */
		if (dx >= 0)
			err = 0; /* initial error term going left->right */
		else
			err = -height + 1; /* going right->left */

		if (skip_first) { /* skip the first point if told to */
			/* Is it time for the X coord to advance? */
			if ((err += width) > 0) {
				x1 += adv;     /* move X one more */
				err -= height; /* fix err to correspond */
			}
		}

		/* Scan the edge for each scan line in turn */
		for (i = height - skip_first; i-- > 0; ep++) {
			/* Store the X coordinate in correct edge list */
			if (set_x_start == 1)
				ep->startx = x1;
			else
				ep->endx = x1;

			/* Is it time for the X coord to advance? */
			if ((err += width) > 0) {
				x1 += adv;     /* move X one more */
				err -= height; /* fix err to correspond */
			}
		}
	} else {
		/* Edge is closer to horizontal than vertical (X-major) */
		/* Minimum distance to advance X each time */
		x_major_adv = (width / height) * adv;

		/* Error term advance: track when to advance X 1 extra */
		err_adv = width % height;
		if (dx >= 0)
			err = 0; /* initial err term going left->right */
		else
			err = -height + 1;  /* going right->left */

		if (skip_first) { /* skip the first point if told to */
			x1 += x_major_adv;  /* move X minimum distance */
			/* Is it time for X to advance one extra? */
			if ((err += err_adv) > 0) {
				x1 += adv;     /* move X one more */
				err -= height; /* fix err to correspond */
			}
		}

		/* Scan the edge for each scan line in turn */
		for (i = height - skip_first; i-- > 0; ep++) {
			/* Store the X coordinate in correct edge list */
			if (set_x_start == 1)
				ep->startx = x1;
			else
				ep->endx = x1;
			x1 += x_major_adv;  /* move X minimum distance */
			/* Is it time for X to advance one extra? */
			if ((err += err_adv) > 0) {
				x1 += adv;     /* move X one more */
				err -= height; /* fix err to correspond */
			}
		}
	}

	*edge_point_ptr = ep;   /* advance caller's ptr */
}

/*
 *  Fill a polygon which is "monotone with respect to a vertical line";
 *  that is, every horizontal line drawn through the polygon at
 *  any point would cross exactly two active edges (neither
 *  horizontal lines nor zero-length edges count as active edges;
 *  both are acceptable anywhere in the polygon). Right & left edges
 *  may cross (polygons may be nonsimple). Polygons that do not
 *  fall within this definition won't be drawn properly.
 *
 *  NOTE: the low-level drawing routine, app_draw_horizontal_line_list,
 *  must be able to reverse the edges, if necessary to make the
 *  correct edge the left edge. It must also expect right edge to be
 *  specified in +1 format (the X coordinate is 1 past highest
 *  coordinate to draw).
 *
 *  Returns 1 for success, 0 if memory allocation failed.
 */
static int app_fill_monotone_vertical_polygon(Graphics *g, Point *p, int n)
{
	int i, min_index, max_index, min_y, max_y;
	int index, prev_index;
	struct HLineList line_list;
	struct HLine *edge_point;

	/* Scan the list to find the top and bottom of the polygon */
	if (n == 0)
		return 1;  /* reject null polygons */

	max_y = min_y = p[min_index = max_index = 0].y;
	for (i = 1; i < n; i++) {
		if (p[i].y < min_y)
			min_y = p[min_index = i].y; /* new top */
		else if (p[i].y > max_y)
			max_y = p[max_index = i].y; /* new bottom */
	}

	/* Set # of scan lines in the polygon, skipping the bottom edge */
	if ((line_list.length = max_y - min_y) <= 0)
		return 1;  /* there's nothing to draw, so we're done */
	line_list.starty = min_y;

	/* Get memory in which to store the line list we generate */
	if ((line_list.lines = app_alloc(sizeof(struct HLine) *
			line_list.length)) == NULL)
		return 0;  /* couldn't get memory for the line list */

	/* Scan first edge and store the boundary points in the list */
	/* Initial pointer for storing scan converted first-edge coords */
	edge_point = line_list.lines;
	/* Start from the top of the first edge */
	prev_index = index = min_index;
	/* Scan convert each line in the first edge from top to bottom */
	do {
		index = (index - 1 + n) % n;
		app_scan_edge(p[prev_index].x,
			p[prev_index].y,
			p[index].x,
			p[index].y, 1, 0, &edge_point);
		prev_index = index;
	} while (index != max_index);

	/* Scan second edge and store the boundary points in the list */
	edge_point = line_list.lines;
	prev_index = index = min_index;
	/* Scan convert the second edge, top to bottom */
	do {
		index = (index + 1) % n;
		app_scan_edge(p[prev_index].x,
			p[prev_index].y,
			p[index].x,
			p[index].y, 0, 0, &edge_point);
		prev_index = index;
	} while (index != max_index);

	/* Draw the line list representing the scan converted polygon */
	if (! app_draw_horizontal_line_list(g, &line_list)) {
		app_free(line_list.lines);
		return 0;
	}

	/* Release the line list's memory and we're successfully done */
	app_free(line_list.lines);
	return 1;
}

/*
 *  Describe a linked list of edges, used in the general-purpose
 *  polygon-filling algorithm below.
 */
typedef struct EdgeState  EdgeState;

struct EdgeState {
	EdgeState *next;
	int x;
	int starty;
	int whole_pixel_x_move;
	int x_direction;
	int err;
	int err_adj_up;
	int err_adj_down;
	int count;
};

/*
 *  Create a global edge table (a GET) in the buffer pointed to by
 *  NextFreeEdge from the vertex list. Edge endpoints are flipped,
 *  if necessary, to guarantee all edges go top to bottom. The GET is
 *  sorted primarily by ascending Y start coordinate, and secondarily
 *  by ascending X start coordinate within edges with common Y
 *  coordinates
 */
static EdgeState * app_build_GET(Point *p, int n, EdgeState *NextFreeEdge)
{
   int i, startx, starty, endx, endy, dy, dx, width, temp;
   EdgeState *new_edge;
   EdgeState *next_edge, **next_edge_ptr;
   EdgeState *GET;

   /* Scan through the vertex list and put all non-0-height edges into
      the GET, sorted by increasing Y start coordinate */

   GET = NULL;    /* initialize the global edge table to empty */
   for (i = 0; i < n; i++) {
      /* Calculate the edge height and width */
      startx = p[i].x;
      starty = p[i].y;
      /* The edge runs from the current point to the previous one */
      if (i == 0) {
         /* Wrap back around to the end of the list */
         endx = p[n-1].x;
         endy = p[n-1].y;
      } else {
         endx = p[i-1].x;
         endy = p[i-1].y;
      }
      /* Make sure the edge runs top to bottom, by swapping */
      if (starty > endy) {
         temp = startx; startx = endx; endx = temp;
         temp = starty; starty = endy; endy = temp;
      }
      /* Skip if this can't ever be an active edge (has 0 height) */
      if ((dy = endy - starty) != 0) {
         /* Allocate space for this edge's info, and fill in the
            structure */
         new_edge = NextFreeEdge++;
         new_edge->x_direction =   /* direction in which X moves */
               ((dx = endx - startx) > 0) ? 1 : -1;
         width = abs(dx);
         new_edge->x = startx;
         new_edge->starty = starty;
         new_edge->count = dy;
         new_edge->err_adj_down = dy;
         if (dx >= 0)  /* initial error term going L->R */
            new_edge->err = 0;
         else              /* initial error term going R->L */
            new_edge->err = -dy + 1;
         if (dy >= width) {     /* Y-major edge */
            new_edge->whole_pixel_x_move = 0;
            new_edge->err_adj_up = width;
         } else {                   /* X-major edge */
            new_edge->whole_pixel_x_move =
                  (width / dy) * new_edge->x_direction;
            new_edge->err_adj_up = width % dy;
         }
         /* Link the new edge into the GET so that the edge list is
            still sorted by Y coordinate, and by X coordinate for all
            edges with the same Y coordinate */
         next_edge_ptr = &GET;
         for (;;) {
            next_edge = *next_edge_ptr;
            if ((next_edge == NULL) ||
                  (next_edge->starty > starty) ||
                  ((next_edge->starty == starty) &&
                  (next_edge->x >= startx))) {
               new_edge->next = next_edge;
               *next_edge_ptr = new_edge;
               break;
            }
            next_edge_ptr = &next_edge->next;
         }
      }
   }
   return GET;
}

/*
 *  Sorts all edges currently in the active edge table into ascending
 *  order of current X coordinates
 */
static void app_x_sort_AET(EdgeState **AETPtr)
{
	EdgeState *edge, **edge_ptr, *temp;
	int swpa_occurred;

	/* Scan through the AET and swap any adjacent edges for which the
	 * second edge is at a lower current X coord than the first edge.
	 * Repeat until no further swapping is needed */

	if (*AETPtr == NULL)
		return; /* nothing to be done */

	do {
		swpa_occurred = 0;
		edge_ptr = AETPtr;
		while ((edge = *edge_ptr)->next != NULL) {
			if (edge->x > edge->next->x) {
				/* The second edge has a lower X than the
				 * first; swap them in the AET */
				temp = edge->next->next;
				*edge_ptr = edge->next;
				edge->next->next = edge;
				edge->next = temp;
				swpa_occurred = 1;
			}
			edge_ptr = &(*edge_ptr)->next;
		}
	} while (swpa_occurred != 0);
}

/*
 *  Advances each edge in the AET by one scan line.
 *   Removes edges that have been fully scanned.
 */
static void app_advance_AET(EdgeState **AETPtr)
{
	EdgeState *edge, **edge_ptr;

	/* count down and remove or advance each edge in the AET */
	edge_ptr = AETPtr;
	while ((edge = *edge_ptr) != NULL) {
		/* count off one scan line for this edge */
		if ((--(edge->count)) == 0) {
			/* This edge is done, so remove it from the AET */
			*edge_ptr = edge->next;
		} else {
			/* Advance edge's X coordinate by minimum move */
			edge->x += edge->whole_pixel_x_move;
			/* Is it time for X to advance one extra? */
			if ((edge->err += edge->err_adj_up) > 0)
			{
				edge->x += edge->x_direction;
				edge->err -= edge->err_adj_down;
			}
			edge_ptr = &edge->next;
		}
	}
}

/*
 *  Moves all edges that start at the specified Y coordinate from the
 *  GET to the AET, maintaining the X sorting of the AET.
 */
static void app_move_x_sorted_to_AET(int y, EdgeState **GETPtr,
	EdgeState **AETPtr)
{
	EdgeState *AETEdge, *temp;
	int x;

	/*
	 *  The GET is Y sorted. Any edges that start at the desired Y
	 *  coordinate will be first in the GET, so we'll move edges from
	 *  the GET to AET until the first edge left in the GET is no
	 *  longer at the desired Y coordinate. Also, the GET is X
	 *  sorted within each Y coordinate, so each successive edge
	 *  we add to the AET is guaranteed to belong later in the AET
	 *  than the one just added
	 */

	while (((*GETPtr) != NULL) && ((*GETPtr)->starty == y))
	{
		x = (*GETPtr)->x;

		/* Link the new edge into the AET so that the AET is still
		 * sorted by X coordinate */
		for (;;) {
			AETEdge = *AETPtr;
			if ((AETEdge == NULL) || (AETEdge->x >= x)) {
				temp = (*GETPtr)->next;
				/* link edge into the AET */
				*AETPtr = (*GETPtr);
				(*GETPtr)->next = AETEdge;
				AETPtr = &(*GETPtr)->next;
				/* unlink edge from the GET */
				*GETPtr = temp;
				break;
			} else {
				AETPtr = &AETEdge->next;
			}
		}
	}
}

/*
 *  Fill the scan line described by the current AET at the specified
 *  Y coordinate in the current color, using the odd/even fill rule
 */
static int app_draw_AET_line(Graphics *g, EdgeState *AET, int y)
{
	int x;
	EdgeState *edge;

	/*
	 *  Scan through the AET, drawing line segments as each pair
	 *  of edge crossings is encountered. The nearest pixel on or
	 *  to the right of left edges is drawn, and the nearest pixel
	 *  to the left of but not on right edges is drawn
	 */
	edge = AET;
	while (edge != NULL) {
		x = edge->x;
		edge = edge->next;
		if (! app_fill_rect(g, rect(x, y, edge->x - x, 1)))
			return 0; /* error during drawing */
		edge = edge->next;
	}
	return 1;
}

/*
 *  Fills an arbitrarily-shaped polygon described by a list of points.
 *  If the first and last points in the list are not the same, the path
 *  around the polygon is automatically closed.
 *  Returns 1 for success, 0 if memory allocation failed.
 *
 *  Some polygons can be filled very quickly. This function first
 *  performs a test to determine if the polygon can be filled quickly,
 *  and if it can, it does so. Otherwise it fills the polygon using
 *  a slower, more general purpose algorithm, using edge tables.
 */
int app_fill_polygon(Graphics *g, Point *p, int n)
{
	EdgeState *table, *GET, *AET;
	int y;

	/* If it is possible to draw the polygon quickly, do so */
	if (app_polygon_is_monotone_vertical(p, n))
		return app_fill_monotone_vertical_polygon(g, p, n);

	/* It takes a minimum of 3 vertices to cause any pixels to be
	   drawn; reject polygons that are guaranteed to be invisible */
	if (n < 3)
		return 1;

	/* Obtain enough memory to store the entire edge table */
	if ((table = app_alloc(sizeof(EdgeState) * n))
		== NULL)
	return 0;  /* couldn't get memory for the edge table */

	/* Build the global edge table */
	GET = app_build_GET(p, n, table);

	/* Scan down through the polygon edges, 1 scan line at a time, as
	   long as at least one edge remains in either the GET or AET */

	/* initialize the active edge table to empty */
	AET = NULL;

	/* start at the top polygon vertex */
	y = GET->starty;

	while ((GET != NULL) || (AET != NULL)) {
		/* update AET for this line */
		app_move_x_sorted_to_AET(y, &GET, &AET);

		/* draw line from AET */
		if (! app_draw_AET_line(g, AET, y)) {
			app_free(table);
			return 0; /* couldn't draw, lack of memory */
		}

		app_advance_AET(&AET);	/* advance AET edges 1 line */
		app_x_sort_AET(&AET);	/* re-sort on X */
		y++;			/* advance to the next line */
	}
	/* Release the memory we've allocated and we're done */
	app_free(table);
	return 1;
}

