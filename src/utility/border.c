/*
 *  Draw border of controls.
 *
 *  Platform: Neutral
 *
 *  Version: 3.57  2005/08/16  First released version.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"

static void app_border_draw(Control *c, Graphics *g)
{
	long border;
	Rect r;

	border = c->state & BORDER_ALL;
	if (! border)
		return;
	app_set_rgb(g, c->fg);
	app_set_line_width(g, 1);
	r = app_get_control_area(c);
	if (border == BORDER_ALL) {
		app_draw_rect(g, r);
		return;
	}
	if (border & BORDER_TOP)
		app_fill_rect(g, rect(0,0,r.width,1));
	if (border & BORDER_BOTTOM)
		app_fill_rect(g, rect(0,r.height-1,r.width,r.height-1));
	if (border & BORDER_LEFT)
		app_fill_rect(g, rect(0,0,1,r.height));
	if (border & BORDER_RIGHT)
		app_fill_rect(g, rect(r.width-1,0,r.width-1,r.height));
}

void app_set_control_border(Control *c, long flags)
{
	if (!((c->state & BORDER_ALL) ^ flags))
		return;
	c->state &= ~ BORDER_ALL;
	c->state |= flags;
	c->redraw = (DrawFunc *) ((flags)
		? app_add_array_element((void **) c->redraw, app_border_draw)
		: app_del_array_element((void **) c->redraw, app_border_draw));
	app_redraw_control(c);
}
