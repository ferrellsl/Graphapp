/*
 *  Separator of controls.
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

enum {
	SEPARATOR_SIZE = 9,	/* default size (width | height) */
};

static void app_separator_draw(Control *c, Graphics *g)
{
	Rect r;

	r = app_get_control_area(c);

	if (app_get_control_value(c) == VALIGN_CENTER) {
		r.y = r.height / 2 - 1;
		r.height = 3;
		r.width -= 4;
		r.x += 2;
	} else {
		r.x = r.width / 2 - 1;
		r.width = 3;
		r.height -= 4;
		r.y += 2;
	}
	app_draw_shadow_rect(g, r, LOWER_RIGHT, UPPER_LEFT);
}

static Control *app_create_separator(Control *c, Rect r, int align)	//!!
{
	if (align == VALIGN_CENTER) {
		if (r.height == -1)
			r.height = SEPARATOR_SIZE;
	}
	else
		if (r.width == -1)
			r.width = SEPARATOR_SIZE;

	if (c == NULL)
		return NULL;

	app_set_control_value(c, align);
	app_on_control_redraw(c, app_separator_draw);
	app_set_control_background(c, CLEAR);
	app_show_control(c);

	return c;
}

Control *app_new_separator(Window *win, Rect r, int align)
{
	return app_create_separator(app_new_control(win, r), r, align);
}

Control *app_add_separator(Control *parent, Rect r, int align)
{
	return app_create_separator(app_add_control(parent, r), r, align);
}
