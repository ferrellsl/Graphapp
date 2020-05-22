/*
 *  Push-buttons displaying images.
 *
 *  Platform: Neutral
 *
 *  Version: 3.46  2003/05/22  First release.
 *  Version: 3.60  2007/06/06  Unified window/control adding code.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>
#include "app.h"
#include "appgui.h"

static void app_image_check_box_draw(Control *c, Graphics *g)
{
	Rect r;
	Rect ir;
	Rect box;
	Image *img;

	/* Draw the button image. */
	r = app_get_control_area(c);
	img = c->img;
	if (img) {
		ir = app_get_image_area(img);
		box = app_center_rect(ir, r);
		if (app_is_highlighted(c)) {
			box.x += 1;
			box.y += 1;
		}
		if (app_is_checked(c))
			app_draw_image_darker(g, box, img, ir);
		else if (app_is_enabled(c))
			app_draw_image(g, box, img, ir);
		else
			app_draw_image_greyscale(g, box, img, ir);
	}

	/* Draw button border. */
	app_draw_rect(g, r);
	r = app_inset_rect(r, 1);

	/* Draw button shadow. */
	if (app_has_focus(c)) {
		app_set_colour(g, FOCUS_BORDER);
		app_draw_rect(g, r);
		r = app_inset_rect(r, 1);
	}
	if (app_is_highlighted(c) || app_is_checked(c))
		app_draw_shadow_rect(g, r, LOWER_RIGHT, UPPER_LEFT);
	else
		app_draw_shadow_rect(g, r, UPPER_LEFT, LOWER_RIGHT);
}

static void app_check_box_mouse_down(Control *c, int buttons, Point p)
{
	if (! app_is_enabled(c))
		return;
	app_arm(c);
	app_highlight(c);
}

static void app_check_box_mouse_drag(Control *c, int buttons, Point p)
{
	if (! app_is_enabled(c))
		return;
	if (! app_is_armed(c))
		return;
	if (buttons && app_point_in_rect(p, app_get_control_area(c)))
		app_highlight(c);
	else
		app_unhighlight(c);
}

static void app_check_box_mouse_up(Control *c, int buttons, Point p)
{
	if (! app_is_enabled(c))
		return;
	if (! app_is_armed(c))
		return;
	if (buttons)
		return;
	app_disarm(c);
	app_unhighlight(c);
	if (app_point_in_rect(p, app_get_control_area(c))) {
		if (app_is_checked(c))
			app_uncheck(c);
		else
			app_check(c);
		app_activate_control(c);
	}
}

static void app_check_box_key_down(Control *c, unsigned long ch)
{
	if (! app_is_enabled(c))
		return;
	if (ch == '\n') {
		if (app_is_checked(c))
			app_uncheck(c);
		else
			app_check(c);
		app_activate_control(c);
	}
}

static Control *app_create_image_check_box(Control *c, Rect r, Image *img,
	ControlFunc fn)	//!!
{
	if (c == NULL)
		return NULL;

	app_set_control_image(c, img);
	app_on_control_redraw(c, app_image_check_box_draw);
	app_on_control_mouse_down(c, app_check_box_mouse_down);
	app_on_control_mouse_move(c, app_check_box_mouse_drag);
	app_on_control_mouse_drag(c, app_check_box_mouse_drag);
	app_on_control_mouse_up(c, app_check_box_mouse_up);
	app_on_control_key_down(c, app_check_box_key_down);
	app_on_control_action(c, fn);
	app_set_control_background(c, BACKGROUND);
	app_show_control(c);

	return c;
}

Control *app_new_image_check_box(Window *win, Rect r, Image *img,
	ControlFunc fn)
{
	return app_create_image_check_box(app_new_control(win, r), r, img, fn);
}

Control *app_add_image_check_box(Control *parent, Rect r, Image *img,
	ControlFunc fn)
{
	return app_create_image_check_box(app_add_control(parent, r), r, img, fn);
}

