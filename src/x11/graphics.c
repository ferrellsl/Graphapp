/*
 *  Graphics contexts and drawing.
 *
 *  Platform: X-Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Added XOR capability.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

static Graphics *app_new_graphics(App *app, XID xid)
{
	Graphics *g;
	Display *disp;

	g = app_zero_alloc(sizeof(struct Graphics));
	g->line_width = 1;
	g->colour = CLEAR;
	g->extra = app_zero_alloc(sizeof(struct GraphicsExtra));

	if (app) {
		disp = app_extra(app)->display;
		g->app = app;
		graphics_extra(g)->gc = XCreateGC(disp, xid, 0, NULL);
	}

	return g;
}

Graphics *app_get_window_graphics(Window *w)
{
	Graphics *g;

	g = app_new_graphics(w->app, win_extra(w)->xid);
	g->win = w;
	g->area = app_get_window_area(w);
	app_set_clip_region(g, NULL);
	g->copy_rect = app_window_copy_rect;
	g->fill_rect = app_window_fill_rect;
	g->draw_utf8 = app_window_draw_utf8;
	g->draw_line = app_window_draw_line;
	app_set_rgb(g, BLACK);
	return g;
}

Graphics *app_get_control_graphics(Control *c)
{
	Graphics *g;
	Window *w;

	w = app_parent_window(c);
	g = app_new_graphics(w->app, win_extra(w)->xid);
	g->win = w;
	g->ctrl = c;
	g->area = app_get_control_area(c);
	g->offset = c->offset;
	app_set_clip_region(g, NULL);
	g->copy_rect = app_window_copy_rect;
	g->fill_rect = app_window_fill_rect;
	g->draw_utf8 = app_window_draw_utf8;
	g->draw_line = app_window_draw_line;
	app_set_rgb(g, BLACK);
	return g;
}

Graphics *app_get_bitmap_graphics(Bitmap *b)
{
	Graphics *g;

	g = app_new_graphics(b->win->app, bitmap_extra(b)->handle);
	g->bmap = b;
	g->area = app_get_bitmap_area(b);
	g->copy_rect = app_bitmap_copy_rect;
	g->fill_rect = app_bitmap_fill_rect;
	g->draw_utf8 = app_bitmap_draw_utf8;
	g->draw_line = app_bitmap_draw_line;
	app_set_rgb(g, BLACK);
	return g;
}

Graphics *app_get_image_graphics(Image *img)
{
	Graphics *g;

	g = app_new_graphics(NULL, 0);
	g->img = img;
	g->area = app_get_image_area(img);
	g->copy_rect = app_image_copy_rect;
	g->fill_rect = app_image_fill_rect;
	g->draw_utf8 = app_image_draw_utf8;
	g->draw_line = app_portable_draw_line;
	app_set_rgb(g, BLACK);
	return g;
}

void app_del_graphics(Graphics *g)
{
	if (graphics_extra(g)->gc)
		XFreeGC(app_extra(g->app)->display, graphics_extra(g)->gc);
	if (g->clip)
		app_del_region(g->clip);
	app_free(graphics_extra(g));
	app_free(g);
}

/*
 *  Set the drawing colour.
 */
void app_set_rgb(Graphics *g, Colour col)
{
	Display *disp;
	long bg;
	GC gc;
	Window *win = NULL;

	if (col.alpha > 0x7F) {	/* transparent */
		g->colour = col;
		return;
	}

	if (g->win)
		win = g->win;
	else if (g->bmap)
		win = g->bmap->win;

	if (win) {
		disp = app_extra(win->app)->display;
		gc = graphics_extra(g)->gc;
		if (g->xor_mode) {
			/* retrieve previous background pixval */
			bg = graphics_extra(g)->bgpixval;

			/* find new foreground pixval */
			g->pixval = app_window_find_colour(g, win, col);

			/* use the correct XOR drawing colour */
			XSetForeground(disp, gc, g->pixval ^ bg);
		}
		else {
			/* find new foreground pixval and use it */
			g->pixval = app_window_find_colour(g, win, col);
			XSetForeground(disp, gc, g->pixval);
		}
	}
	else if (g->img) {
		if (g->img->cmap_size > 0)
			g->pixval = app_image_find_colour(g->img, col);
	}

	g->colour = col;
}

void app_set_rgbindex(Graphics *g, int index)
{
	Display *disp;
	long bg;
	GC gc;
	Window *win = NULL;

	if (g->win)
		win = g->win;
	else if (g->bmap)
		win = g->bmap->win;

	if (win) {
		disp = app_extra(win->app)->display;
		gc = graphics_extra(g)->gc;
		if (g->xor_mode) {
			/* retrieve previous background pixval */
			bg = graphics_extra(g)->bgpixval;

			/* use the correct XOR drawing colour */
			XSetForeground(disp, gc, index ^ bg);
		}
		else {
			/* use new foreground pixval */
			XSetForeground(disp, gc, index);
		}
	}
	else if (g->img) {
		if (g->img->cmap_size > 0)
			g->colour = g->img->cmap[index];
	}

	g->pixval = index;
}

/*
 *  Set/remove the XOR drawing mode.
 */
void app_set_xor_mode(Graphics *g, Colour bgcol)
{
	long bg;
	GC gc;
	Display *disp;
	Window *win = NULL;

	if (g->win)
		win = g->win;
	else if (g->bmap)
		win = g->bmap->win;

	if (win) {
		bg = app_window_find_colour(g, win, bgcol);
		graphics_extra(g)->bgpixval = bg;
		disp = app_extra(win->app)->display;
		gc = graphics_extra(g)->gc;
		XSetForeground(disp, gc, g->pixval ^ bg);
		XSetFunction(disp, gc, GXxor);
	}

	g->xor_mode = 1;
}

void app_set_paint_mode(Graphics *g)
{
	GC gc;
	Display *disp;
	Window *win = NULL;

	if (g->win)
		win = g->win;
	else if (g->bmap)
		win = g->bmap->win;

	if (win) {
		disp = app_extra(win->app)->display;
		gc = graphics_extra(g)->gc;
		XSetForeground(disp, gc, g->pixval);
		XSetFunction(disp, gc, GXcopy);
	}

	g->xor_mode = 0;
}

/*
 *  Set pixel width of newly drawn lines.
 */
void app_set_line_width(Graphics *g, int width)
{
	g->line_width = width;
}

/*
 *  Set direction of drawing text (default is LR_TB).
 */
void app_set_text_direction(Graphics *g, int direction)
{
	g->text_direction = direction;
}

/*
 *  Set the drawing font.
 */
void app_set_font(Graphics *g, Font *f)
{
	g->font = f;
	if (g->app && (f->style & NATIVE_FONT))
		XSetFont(app_extra(g->app)->display,
			graphics_extra(g)->gc,
			font_extra(f)->fnt->fid);
}

/*
 *  Set the clipping region to be a new region based on the
 *  given rectangle.
 */
void app_set_clip_rect(Graphics *g, Rect r)
{
	Region *temp;

	temp = app_new_rect_region(r);
	if (temp) {
		app_set_clip_region(g, temp);
		app_del_region(temp);
	}
}

/*
 *  Make a copy of the clipping region to be used during drawing,
 *  and clip that region against the visible portion of the
 *  destination.
 *
 *  Passing NULL as the region deletes the old one and forces no
 *  clipping to occur during drawing, except, as usual, clipping
 *  against the boundaries of the destination window, control or bitmap.
 */
void app_set_clip_region(Graphics *g, Region *rgn)
{
	Rect r;

	/* determine the enclosing rectangle */
	if (g->ctrl)
		r = app_get_control_area(g->ctrl);
	else if (g->win)
		r = app_get_window_area(g->win);
	else if (g->bmap)
		r = app_get_bitmap_area(g->bmap);
	else if (g->img)
		r = app_get_image_area(g->img);
	else
		r = rect(0,0,0,0);	/* should never happen */

	/* remove any existing clipping region */
	if (g->clip)
		app_del_region(g->clip);

	/* take a copy of the given region, if one was given */
	if (rgn)
		g->clip = app_copy_region(rgn);

	/* or else enable drawing to the entire destination */
	else
		g->clip = app_new_rect_region(r);

	if (g->ctrl) {
		/* translate region to window co-ordinates */
		app_move_region(g->clip, g->offset.x, g->offset.y);

		/* clip to the control's visible boundary */
		if (g->ctrl->visible)
			app_intersect_region(g->clip, g->ctrl->visible,
				g->clip);
		else
			app_intersect_region_with_rect(g->clip, r, g->clip);

		/* clip to the window's current redraw region also */
		if (g->win && g->win->redraw_rgn)
			app_intersect_region(g->clip, g->win->redraw_rgn,
				g->clip);
	}
	else if (g->win) {
		/* no translation needed; top-left point must be (0,0) */

		/* clip to the window's visible boundary */
		if (g->win->visible)
			app_intersect_region(g->clip, g->win->visible,
				g->clip);
		else
			app_intersect_region_with_rect(g->clip, r, g->clip);

		/* clip to the window's current redraw region also */
		if (g->win->redraw_rgn)
			app_intersect_region(g->clip, g->win->redraw_rgn,
				g->clip);
	}
	else {
		/* no translation needed; top-left point must be (0,0) */

		/* clip to the destination's visible boundary */
		app_intersect_region_with_rect(g->clip, r, g->clip);
	}
}

