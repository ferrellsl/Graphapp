/*
 *  Notebook-pane buttons displaying text.
 *
 *  Platform: Neutral
 *
 *  Version: 3.57  2005/08/16  First release in main distribution.
 *  Version: 3.58  2005/08/28  Silenced a size_t conversion warning.
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

static void app_note_button_draw(Control *c, Graphics *g)
{
	int is_checked;
	Rect r;

	r = app_get_control_area(c);
	is_checked = app_is_checked(c);

	/* Draw the button name. */
	if (app_is_enabled(c))
		app_set_colour(g, app_get_control_foreground(c));
	else
		app_set_colour(g, DISABLED_ITEM);
	if (c->font)
		app_set_font(g, c->font);
	else
		app_set_default_font(g);
	app_draw_text(g, r, ALIGN_CENTER | VALIGN_CENTER,
		c->text, (int) strlen(c->text));

	/* Draw button border except at upper edge. */
	app_set_colour(g, (is_checked != 0) ? ENABLED_ITEM : DISABLED_ITEM);
	app_fill_rect(g, rect(r.x,r.y,1,r.height));
	app_fill_rect(g, rect(r.x+r.width-1,r.y,1,r.height));
	app_fill_rect(g, rect(r.x+1,r.y+r.height-1,r.width-1,1));
	app_set_colour(g, UPPER_LEFT);
	app_fill_rect(g, rect(r.x+1,r.y,1,r.height-1));
}

static void app_note_button_mouse_down(Control *c, int buttons, Point p)
{
	if (! app_is_enabled(c))
		return;
	app_activate_control(c);
}

static Control *app_create_note_button(Control *c, Rect r, const char *text,
	ControlFunc fn)	//!!
{
	if (c == NULL)
		return NULL;

	app_set_control_text(c, text);
	app_on_control_redraw(c, app_note_button_draw);
	app_on_control_mouse_down(c, app_note_button_mouse_down);
	app_on_control_action(c, fn);
	app_set_control_background(c, BACKGROUND);
	app_show_control(c);

	return c;
}

Control *app_new_note_button(Window *win, Rect r, const char *text,
	ControlFunc fn)
{
	return app_create_note_button(app_new_control(win, r), r, text, fn);
}

Control *app_add_note_button(Control *parent, Rect r, const char *text,
	ControlFunc fn)
{
	return app_create_note_button(app_add_control(parent, r), r, text, fn);
}
