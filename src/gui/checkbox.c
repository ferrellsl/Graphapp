/*
 *  Check-boxes.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Can now use any font.
 *  Version: 3.09  2001/11/13  Improved disabled behaviour.
 *  Version: 3.10  2001/12/01  Added app_add_check_box constructor.
 *  Version: 3.40  2003/03/07  More Windows-like look.
 *  Version: 3.46  2003/05/22  Renamed move function to drag.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.54  2005/07/21  Unhandled keys (tab) go to underlying window.
 *  Version: 3.56  2005/08/09  Silenced a size_t conversion warning.
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

static void app_draw_checkmark(Graphics *g, Rect r)
{
	int len;

	if (r.width < 8) {
		r = app_inset_rect(r,1);
		app_fill_rect(g, r);
		return;
	}

	len = (r.width+1)/3;
	if (len < 3)
		len = 3;

	app_draw_line(g, pt(r.x+len, r.y+r.height-3),
			pt(r.x+2, r.y+r.height-len-1));
	app_draw_line(g, pt(r.x+len, r.y+r.height-4),
			pt(r.x+2, r.y+r.height-len-2));
	app_draw_line(g, pt(r.x+len, r.y+r.height-3),
		 	pt(r.x+r.width-2, r.y+len-1));
	app_draw_line(g, pt(r.x+len, r.y+r.height-4),
		 	pt(r.x+r.width-2, r.y+len-2));
}

static void app_check_box_draw(Control *c, Graphics *g)
{
	int w;
	Rect r, box, textbox;
	char *name;
	int style = (ALIGN_LEFT | VALIGN_TOP);

	/* Calculate rectangles. */
	r = app_get_control_area(c);
	w = 15;
	if (w > r.width)  w = r.width;
	if (w > r.height) w = r.height;
	box = rect(r.x,r.y+1,w,w);
	textbox = rect(r.x+w+w/2,r.y,r.width-(w+w/2),r.height);

	/* 'Pressed button' effect: fill in the box. */
	if (app_is_highlighted(c))
		app_set_colour(g, BACKGROUND);
	else
		app_set_colour(g, FILL_ITEM);
	app_fill_rect(g, app_inset_rect(box,2));

	/* Bevelled border. */
	app_set_line_width(g, 1);
	if (app_is_enabled(c)) {
		app_draw_shadow_rect(g, box, LOWER_RIGHT, FILL_ITEM);
		app_draw_shadow_rect(g, app_inset_rect(box,1),
					ENABLED_ITEM, BACKGROUND);
		app_set_colour(g, ENABLED_ITEM);
	}
	else {
		app_draw_shadow_rect(g, box, LOWER_RIGHT, FILL_ITEM);
		app_draw_shadow_rect(g, app_inset_rect(box,1),
					DISABLED_ITEM, BACKGROUND);
		app_set_colour(g, DISABLED_ITEM);
	}

	/* Put check-mark in box if checked. */
	if (app_is_checked(c))
		app_draw_checkmark(g, app_inset_rect(box,2));

	/* Draw the focus border (if any) around the text. */
	name = app_get_control_text(c);
	if (app_is_enabled(c)) {
		if (app_has_focus(c)) {
			app_set_colour(g, FOCUS_BORDER);
		} else {
			Colour bg = CLEAR;
			if (c->parent) {
				bg = app_get_control_background(c->parent);
			}
			if (bg.alpha > 0x7F) { /* transparent; use win's bg */
				Window *pw = app_parent_window(c);
				bg = app_get_window_background(pw);
			}
			app_set_colour(g, bg);
		}
		app_draw_rect(g, rect(textbox.x-2, textbox.y,
					textbox.width+2, textbox.height));
		app_set_colour(g, app_get_control_foreground(c));
	}

	/* Draw the text. */
	if (c->font)
		app_set_font(g, c->font);
	else
		app_set_default_font(g);
	app_draw_text(g, textbox, style, name, (int) strlen(name));
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
	if ((ch & ~(CONTROL | SHIFT)) == '\n') {
		if (app_is_checked(c))
			app_uncheck(c);
		else
			app_check(c);
		app_activate_control(c);
	}
	else {
		app_pass_event(c);
	}
}

static Control *app_create_check_box(Control *c,
	Rect r, const char *text, ControlFunc fn)	//!!
{
	if (c == NULL)
		return NULL;

	c->natural.x = 15;  /* offset to correct natural size */

	app_set_control_text(c, text);
	app_on_control_redraw(c, app_check_box_draw);
	app_on_control_mouse_down(c, app_check_box_mouse_down);
	app_on_control_mouse_move(c, app_check_box_mouse_drag);
	app_on_control_mouse_drag(c, app_check_box_mouse_drag);
	app_on_control_mouse_up(c, app_check_box_mouse_up);
	app_on_control_key_down(c, app_check_box_key_down);
	app_on_control_action(c, fn);
	app_set_control_background(c, CLEAR);
	app_show_control(c);

	return c;
}

Control *app_new_check_box(Window *win, Rect r, const char *text, ControlFunc fn)
{
	return app_create_check_box(app_new_control(win, r), r, text, fn);
}

Control *app_add_check_box(Control *parent, Rect r, const char *text, ControlFunc fn)
{
	return app_create_check_box(app_add_control(parent, r), r, text, fn);
}
