/*
 *  Radio-buttons.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Added proper radio groups.
 *  Version: 3.06  2001/10/30  Fixed a bug with already-checked buttons.
 *  Version: 3.09  2001/11/13  Improved disabled behaviour.
 *  Version: 3.10  2001/12/01  Added app_add_radio_button/group.
 *  Version: 3.40  2003/03/07  More Windows-like look.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.54  2005/07/21  Unhandled keys (tab) go to underlying window.
 *  Version: 3.56  2005/08/09  Silenced some size_t conversion warnings.
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

/*
 *  A dummy procedure used only when locating radio groups.
 */
static void app_radio_group_draw(Control *c, Graphics *g)
{
	/* nothing to be done */
}

/*
 *  Return a pointer to the enclosing radio group, if any.
 */
static Control *app_find_radio_group(Control *btn)
{
	int i;
	int num_children;
	Control **children;
	Control *c;
	Control *rg = NULL;

	if (btn->parent) {
		num_children = btn->parent->num_children;
		children = btn->parent->children;
	}
	else if (btn->win) {
		num_children = btn->win->num_children;
		children = btn->win->children;
	}
	else
		return NULL;

	for (i=num_children-1; i >= 0; i--) {
		c = children[i];
		if (c->redraw && (c->redraw[0] == app_radio_group_draw))
			rg = c;
		else if (c == btn)
			break;
	}
	return rg;
}

/*
 *  Find the last radio group that was created.
 */
static Control *app_last_radio_group(int num_children, Control **children)
{
	int i;
	Control *c;
	Control *rg = NULL;

	for (i=num_children-1; i >= 0; i--) {
		c = children[i];
		if (c->redraw && (c->redraw[0] == app_radio_group_draw))
			rg = c;
	}
	return rg;
}

/*
 *  Uncheck the formerly checked radio button in this group,
 *  (unless it is this button), then check the newly checked button.
 */
static void app_check_radio_button(Control *btn)
{
	Control *former;
	Control *rg;

	rg = app_find_radio_group(btn);

	if (rg == NULL) {
		app_check(btn);
		return;
	}
	if (rg->extra) {
		former = rg->extra;
		if (former == btn)
			return;	/* nothing to be done */
		if (former != NULL)
			app_uncheck(former);
	}
	rg->extra = btn;
	app_check(btn);
}

static void app_radio_button_draw(Control *c, Graphics *g)
{
	int w;
	Rect r, box, textbox;
	char *name;
	int style = (ALIGN_LEFT | VALIGN_TOP);
	Control *rg = app_find_radio_group(c);

	/* Calculate rectangles. */
	r = app_get_control_area(c);
	w = 15;
	if (w > r.width)  w = r.width;
	if (w > r.height) w = r.height;
	box = rect(r.x,r.y+1,w,w);
	textbox = rect(r.x+w+w/2,r.y,r.width-(w+w/2),r.height);

	/* 'Pressed button' effect: fill check area. */
	if (app_is_highlighted(c))
		app_set_colour(g, BACKGROUND);
	else
		app_set_colour(g, FILL_ITEM);
	app_fill_ellipse(g, app_inset_rect(box,2));

	/* Bevelled border. */
	app_set_line_width(g, 1);

	app_set_colour(g, RADIO_BUTTON_LOWER);
	app_draw_arc(g, box, 225, 45);
	if (app_is_enabled(c))
		app_set_colour(g, RADIO_BUTTON_UPPER);
	else
		app_set_colour(g, DISABLED_ITEM);
	app_draw_arc(g, box, 45, 225);

	app_set_colour(g, RADIO_BUTTON_INNER_LOWER);
	app_draw_arc(g, app_inset_rect(box,1), 225, 45);
	if (app_is_enabled(c))
		app_set_colour(g, RADIO_BUTTON_INNER_UPPER);
	else
		app_set_colour(g, DISABLED_ITEM);
	app_draw_arc(g, app_inset_rect(box,1), 45, 225);

	/* Put o in check area if checked. */
	if (app_is_checked(c)) {
		app_fill_ellipse(g, app_inset_rect(box,5));
		if (rg != NULL)
			rg->extra = c;
	}

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

static void app_radio_button_mouse_down(Control *c, int buttons, Point p)
{
	if (! app_is_enabled(c))
		return;
	app_arm(c);
	app_highlight(c);
}

static void app_radio_button_mouse_move(Control *c, int buttons, Point p)
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

static void app_radio_button_activate(Control *c)
{
	if (! app_is_checked(c)) {
		app_check_radio_button(c);
		app_activate_control(c);
	} else {
		app_check_radio_button(c);
	}
}

static void app_radio_button_mouse_up(Control *c, int buttons, Point p)
{
	if (! app_is_enabled(c))
		return;
	if (! app_is_armed(c))
		return;
	if (buttons)
		return;
	app_disarm(c);
	app_unhighlight(c);
	if (app_point_in_rect(p, app_get_control_area(c)))
		app_radio_button_activate(c);
}

static void app_radio_button_key_down(Control *c, unsigned long ch)
{
	if (! app_is_enabled(c))
		return;
	if ((ch & ~(CONTROL | SHIFT)) == '\n')
		app_radio_button_activate(c);
	else
		app_pass_event(c);
}

static Control *app_create_radio_group(Control *c)	//!!
{
	if (c == NULL)
		return NULL;

	app_hide_control(c);
	app_on_control_redraw(c, app_radio_group_draw);
	app_set_control_background(c, CLEAR);
	return c;
}

Control *app_new_radio_group(Window *win)
{
	return app_create_radio_group(app_new_control(win, rect(-1,-1,0,0)));
}

Control *app_add_radio_group(Control *parent)
{
	return app_create_radio_group(app_add_control(parent, rect(-1,-1,0,0)));
}

static Control *app_create_radio_button(Control *c, Rect r, const char *text,
	ControlFunc fn)	//!!
{
	if (c == NULL)
		return NULL;

	app_set_control_text(c, text);
	app_on_control_redraw(c, app_radio_button_draw);
	app_on_control_mouse_down(c, app_radio_button_mouse_down);
	app_on_control_mouse_move(c, app_radio_button_mouse_move);
	app_on_control_mouse_drag(c, app_radio_button_mouse_move);
	app_on_control_mouse_up(c, app_radio_button_mouse_up);
	app_on_control_key_down(c, app_radio_button_key_down);
	app_on_control_action(c, fn);
	app_set_control_background(c, CLEAR);
	app_show_control(c);

	return c;
}

Control *app_new_radio_button(Window *win, Rect r, const char *text,
	ControlFunc fn)
{
	if (! app_last_radio_group(win->num_children, win->children))
		if (app_new_radio_group(win) == NULL)
			return NULL;

	return app_create_radio_button(app_new_control(win, r), r, text, fn);
}

Control *app_add_radio_button(Control *parent, Rect r, const char *text,
	ControlFunc fn)
{
	if (! app_last_radio_group(parent->num_children, parent->children))
		if (app_add_radio_group(parent) == NULL)
			return NULL;

	return app_create_radio_button(app_add_control(parent, r), r, text, fn);
}

