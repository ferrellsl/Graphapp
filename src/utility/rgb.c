/*
 *  Colours:
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.16  2002/01/06  Added rgbs_equal comparator.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"

Colour app_new_rgb(int r, int g, int b)
{
	Colour col;

	col.alpha = 0;
	col.red = r;
	col.green = g;
	col.blue = b;

	return col;
}

Colour app_new_argb(int a, int r, int g, int b)
{
	Colour col;

	col.alpha = a;
	col.red = r;
	col.green = g;
	col.blue = b;

	return col;
}

int app_rgbs_equal(Colour a, Colour b)
{
	return ((a.red == b.red) &&
		(a.green == b.green) &&
		(a.blue == b.blue) &&
		(a.alpha == b.alpha));
}
