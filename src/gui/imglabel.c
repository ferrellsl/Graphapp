/*
 *  Labels to display images.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Fixed image pointer problem.
 *  Version: 3.10  2001/12/01  Added app_add_image_label constructor.
 *  Version: 3.60  2007/06/06  Unified window/control adding code.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>
#include "app.h"

static void app_image_label_draw(Control *c, Graphics *g)
{
	Rect r, ir, box;
	int align;
	Image *img;

	/* Draw the image. */

	img = c->img;
	if (img) {
		r = app_get_control_area(c);
		ir = app_get_image_area(img);
		box = ir;
		align = app_get_control_value(c);

		if ((align & ALIGN_CENTER) == ALIGN_CENTER)
			box.x += (r.width - ir.width)/2;
		else if ((align & ALIGN_RIGHT) == ALIGN_RIGHT)
			box.x += (r.width - ir.width);
		else if ((align & ALIGN_JUSTIFY) == ALIGN_JUSTIFY)
			box.width += r.width;

		if ((align & VALIGN_CENTER) == VALIGN_CENTER)
			box.y += (r.height - ir.height)/2;
		else if ((align & VALIGN_BOTTOM) == VALIGN_BOTTOM)
			box.y += (r.height - ir.height);
		else if ((align & VALIGN_JUSTIFY) == VALIGN_JUSTIFY)
			box.height += r.height;

		if (app_is_enabled(c))
			app_draw_image(g, box, img, ir);
		else
			app_draw_image_greyscale(g, box, img, ir);
	}
}

static Control *app_create_image_label(Control *c, Rect r, Image *img, int align) //!!
{
	if (c == NULL)
		return NULL;

	app_set_control_image(c, img);
	app_set_control_value(c, align);
	app_set_control_background(c, CLEAR);
	app_on_control_redraw(c, app_image_label_draw);
	app_show_control(c);

	return c;
}

Control *app_new_image_label(Window *win, Rect r, Image *img,
	int align)
{
	return app_create_image_label(app_new_control(win, r), r, img, align);
}

Control *app_add_image_label(Control *parent, Rect r, Image *img,
	int align)
{
	return app_create_image_label(app_add_control(parent, r), r, img, align);
}

