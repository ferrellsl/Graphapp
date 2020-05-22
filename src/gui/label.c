/*
 *  Labels to display text.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Can now use any font.
 *  Version: 3.10  2001/12/01  Added app_add_label constructor.
 *  Version: 3.26  2002/07/31  Added update handler for text changes.
 *  Version: 3.40  2003/03/07  Now uses centralised colour settings.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced a size_t conversion warning.
 *  Version: 3.57  2005/08/16  Handles NULL text safely, draws nothing.
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

static void app_label_draw(Control *c, Graphics *g)
{
	char *text;
	Rect r;

	/* Draw the label. */

	text = app_get_control_text(c);
	if (text == NULL)
		return;

	r = app_get_control_area(c);

	if (app_is_enabled(c))
		app_set_colour(g, app_get_control_foreground(c));
	else
		app_set_colour(g, DISABLED_ITEM);

	/* Draw the text. */
	if (c->font)
		app_set_font(g, c->font);
	else
		app_set_default_font(g);
	app_draw_text(g, r, app_get_control_value(c), text, (int) strlen(text));
}

static void app_label_update(Control *c)
{
	Window *w;
	Rect r;

	/* force an update of this control's rectangle */
	w = app_parent_window(c);
	r = c->area;
	r.x = c->offset.x;
	r.y = c->offset.y;
	app_redraw_rect(w, r);
}

static Control *app_create_label(Control *c, Rect r, const char *text, int align) //!!
{
	if (c == NULL)
		return NULL;

	app_set_control_text(c, text);
	app_set_control_value(c, align);
	app_set_control_background(c, CLEAR);
	app_on_control_redraw(c, app_label_draw);
	app_on_control_update(c, app_label_update);
	app_show_control(c);

	return c;
}

Control *app_new_label(Window *win, Rect r, const char *text, int align)
{
	return app_create_label(app_new_control(win, r), r, text, align);
}

Control *app_add_label(Control *parent, Rect r, const char *text, int align)
{
	return app_create_label(app_add_control(parent, r), r, text, align);
}

