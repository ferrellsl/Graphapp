/*
 *  Push-buttons displaying text.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Can now use any font.
 *  Version: 3.09  2001/11/13  Improved disabled behaviour.
 *  Version: 3.10  2001/12/01  Added app_add_button constructor.
 *  Version: 3.40  2003/03/07  More Windows-like look.
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

static void app_button_draw(Control *c, Graphics *g)
{
	Rect r;
	Rect textbox;

	r = app_get_control_area(c);

	/* Draw the button name. */
	if (app_is_enabled(c))
		app_set_colour(g, app_get_control_foreground(c));
	else
		app_set_colour(g, DISABLED_ITEM);
	textbox = app_inset_rect(r,3);
	if (app_is_highlighted(c)) {
		textbox.x += 1;
		textbox.y += 1;
	}
	if (c->font)
		app_set_font(g, c->font);
	else
		app_set_default_font(g);
	app_draw_text(g, textbox, ALIGN_CENTER | VALIGN_CENTER,
		c->text, (int) strlen(c->text));

	/* Draw button border. */
	app_draw_rect(g, r);
	r = app_inset_rect(r, 1);

	/* Draw button shadow. */
	if (app_has_focus(c)) {
		app_set_colour(g, FOCUS_BORDER);
		app_draw_rect(g, r);
		r = app_inset_rect(r, 1);
	}
	if (app_is_highlighted(c))
		app_draw_shadow_rect(g, r, LOWER_RIGHT, UPPER_LEFT);
	else
		app_draw_shadow_rect(g, r, UPPER_LEFT, LOWER_RIGHT);
}

static void app_button_mouse_down(Control *c, int buttons, Point p)
{
	if (! app_is_enabled(c))
		return;
	app_arm(c);
	app_highlight(c);
}

static void app_button_mouse_drag(Control *c, int buttons, Point p)
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

static void app_button_mouse_up(Control *c, int buttons, Point p)
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
		app_activate_control(c);
}

static void app_button_key_down(Control *c, unsigned long ch)
{
	if (! app_is_enabled(c))
		return;
	if ((ch & ~(CONTROL | SHIFT)) == '\n') {
		app_flash_control(c);
		app_activate_control(c);
	}
	else {
		app_pass_event(c);
	}
}

static Control *app_create_button(Control *c, Rect r, const char *text,
	ControlFunc fn)	//!!
{
	if (c == NULL)
		return NULL;

	app_set_control_text(c, text);
	app_on_control_redraw(c, app_button_draw);
	app_on_control_mouse_down(c, app_button_mouse_down);
	app_on_control_mouse_drag(c, app_button_mouse_drag);
	app_on_control_mouse_up(c, app_button_mouse_up);
	app_on_control_key_down(c, app_button_key_down);
	app_on_control_action(c, fn);
	app_set_control_background(c, BACKGROUND);
	app_show_control(c);

	return c;
}

Control *app_new_button(Window *win, Rect r, const char *text,
	ControlFunc fn)
{
	return app_create_button(app_new_control(win, r), r, text, fn);
}

Control *app_add_button(Control *parent, Rect r, const char *text,
	ControlFunc fn)
{
	return app_create_button(app_add_control(parent, r), r, text, fn);
}
