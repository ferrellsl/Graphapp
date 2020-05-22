/*
 *  Splitter of docking controls.
 *
 *  Platform: Neutral
 *
 *  Version: 3.57  2005/08/16  First release in main distribution.
 *  Version: 3.60  2007/06/06  Unified window/control adding code.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>
#include <string.h>
#include "app.h"
#include "appgui.h"

static void app_splitter_draw(Control *c, Graphics *g)
{
	Rect r;

	r = app_get_control_area(c);

	if (app_is_highlighted(c))
		app_draw_shadow_rect(g, r, LOWER_RIGHT, UPPER_LEFT);
	else
		app_draw_shadow_rect(g, r, UPPER_LEFT, LOWER_RIGHT);
}

static void app_splitter_mouse_down(Control *c, int buttons, Point p)
{
	if (! app_is_enabled(c))
		return;
	app_arm(c);
	app_highlight(c);
	c->corner = p;
}

static void app_splitter_mouse_drag(Control *c, int buttons, Point p)
{
	long layout;
	int delta;
	Rect r, *area;
	Control *sticky;

	if (! app_is_armed(c))
		return;
	sticky = c->extra;
	layout = sticky->state;
	r = app_get_control_area(sticky);

	if (c->parent)
		area = &c->parent->area;
	else
		area = &app_parent_window(c)->area;

	if (layout & (EDGE_TOP | EDGE_BOTTOM)) {
		delta = p.y - c->corner.y;
		r.height += (layout & EDGE_TOP) ? delta : -delta;
		if (! delta || (r.height < 0)
			|| (((layout & EDGE_TOP) ? sticky->area.y : 0)
				+ r.height + c->area.height > area->height))
			return;
	}
	else {
		delta = p.x - c->corner.x;
		r.width += (layout & EDGE_LEFT) ? delta : -delta;
		if (! delta || (r.width < 0)
			|| (((layout & EDGE_LEFT) ? sticky->area.x : 0)
				+ r.width + c->area.width > area->width))
			return;
	}
	app_set_control_area(sticky, r);
	if (c->parent)
		app_redraw_control(c->parent);
	else
		app_redraw_window(app_parent_window(c));
}

static void app_splitter_mouse_up(Control *c, int buttons, Point p)
{
	if (! app_is_armed(c))
		return;
	app_disarm(c);
	app_unhighlight(c);
}

static Control *app_create_splitter(Control *c, Rect r, Control *sticky)
{
	if (c == NULL)	//!!
		return NULL;

	c->extra = sticky;
	c->state |= sticky->state & LAYOUT_MASK;

	app_set_control_cursor(c,
		app_get_standard_cursor(app_parent_window(c)->app,
			(c->state & (EDGE_LEFT | EDGE_RIGHT))
				? SIZELR_CURSOR : SIZETB_CURSOR));

	app_on_control_redraw(c, app_splitter_draw);
	app_on_control_mouse_down(c, app_splitter_mouse_down);
	app_on_control_mouse_drag(c, app_splitter_mouse_drag);
	app_on_control_mouse_up(c, app_splitter_mouse_up);
	app_set_control_background(c, BACKGROUND);
	app_show_control(c);

	return c;
}

Control *app_new_splitter(Window *win, Rect r)
{
	Control *sticky;

	/* sticky brother control required */
	if (! (win->children && (sticky = win->children[0])
		&& (sticky->state & DOCK)))
		return NULL;

	return app_create_splitter(app_new_control(win, r), r, sticky);
}

Control *app_add_splitter(Control *parent, Rect r)
{
	Control *sticky;

	/* sticky brother control required */
	if (! (parent->children && (sticky = parent->children[0])
		&& (sticky->state & DOCK)))
		return NULL;

	return app_create_splitter(app_add_control(parent, r), r, sticky);
}
