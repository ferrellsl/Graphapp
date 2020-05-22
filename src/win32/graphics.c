/*
 *  Graphics contexts and drawing.
 *
 *  Platform: Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/17  Added XOR drawing mode.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

enum GraphicsKinds {
	NO_DC          = 0,
	BEGIN_END      = 1,
	GET_RELEASE    = 2,
	CREATE_DELETE  = 3
};

static Graphics *app_new_graphics(App *app, int kind, HDC dc)
{
	Graphics *g;

	g = app_zero_alloc(sizeof(struct Graphics));
	g->line_width = 1;
	g->colour = CLEAR;
	g->app = app;
	g->extra = app_zero_alloc(sizeof(struct GraphicsExtra));
	graphics_extra(g)->dc = dc;
	graphics_extra(g)->kind = kind;

	return g;
}

Graphics *app_get_window_redraw(HDC dc, Window *w)
{
	Graphics *g;

	g = app_new_graphics(w->app, BEGIN_END, dc);
	SetBkMode(graphics_extra(g)->dc, 1); /* transparent behind text */
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

Graphics *app_get_window_graphics(Window *w)
{
	Graphics *g;

	g = app_new_graphics(w->app, GET_RELEASE, GetDC(win_extra(w)->hwnd));
	SetBkMode(graphics_extra(g)->dc, 1); /* transparent behind text */
	g->win = w;
	if (w->pal) {
		graphics_extra(g)->oldpal =
			SelectPalette(graphics_extra(g)->dc,
				win_extra(w)->winpal, 0);
		RealizePalette(graphics_extra(g)->dc);
	}
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

	g = app_new_graphics(w->app, GET_RELEASE, GetDC(win_extra(w)->hwnd));
	SetBkMode(graphics_extra(g)->dc, 1); /* transparent behind text */
	g->win = w;
	if (w->pal) {
		graphics_extra(g)->oldpal =
			SelectPalette(graphics_extra(g)->dc,
				win_extra(w)->winpal, 0);
		RealizePalette(graphics_extra(g)->dc);
	}
	g->ctrl = c;
	g->offset = c->offset;
	g->area = app_get_control_area(c);
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
	HDC dc;
	Window *w;

	w = b->win;

	dc = GetDC(win_extra(w)->hwnd);
	g = app_new_graphics(w->app, CREATE_DELETE, CreateCompatibleDC(dc));
	ReleaseDC(win_extra(w)->hwnd, dc);
	SetBkMode(graphics_extra(g)->dc, 1); /* transparent behind text */
	g->bmap = b;
	graphics_extra(g)->oldbm = SelectObject(graphics_extra(g)->dc,
			bitmap_extra(b)->handle);
	if (w->pal) {
		graphics_extra(g)->oldpal =
			SelectPalette(graphics_extra(g)->dc,
				win_extra(w)->winpal, 0);
		RealizePalette(graphics_extra(g)->dc);
	}
	g->area = app_get_bitmap_area(b);
	g->copy_rect = app_bitmap_copy_rect;
	g->fill_rect = app_bitmap_fill_rect;
	g->draw_utf8 = app_bitmap_draw_utf8;
	g->draw_line = app_portable_draw_line;
	app_set_rgb(g, BLACK);
	return g;
}

Graphics *app_get_image_graphics(Image *img)
{
	Graphics *g;

	g = app_new_graphics(NULL, NO_DC, 0);
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
	if (graphics_extra(g)->oldbr)
		SelectObject(graphics_extra(g)->dc,
			graphics_extra(g)->oldbr);
	if (graphics_extra(g)->brush)
		DeleteObject(graphics_extra(g)->brush);
	if (graphics_extra(g)->oldpen)
		SelectObject(graphics_extra(g)->dc,
			graphics_extra(g)->oldpen);
	if (graphics_extra(g)->pen)
		DeleteObject(graphics_extra(g)->pen);
	if (graphics_extra(g)->oldbm)
		SelectObject(graphics_extra(g)->dc,
			graphics_extra(g)->oldbm);
	if (graphics_extra(g)->oldpal)
		SelectPalette(graphics_extra(g)->dc,
			graphics_extra(g)->oldpal, 0);

	switch (graphics_extra(g)->kind)
	{
	  case NO_DC:
		/* do nothing, since there is no DC */
		break;

	  case BEGIN_END:
		/* do nothing, since the event loop will call EndPaint */
		break;

	  case GET_RELEASE:
		ReleaseDC(win_extra(g->win)->hwnd,
			graphics_extra(g)->dc);
		break;

	  case CREATE_DELETE:
		DeleteDC(graphics_extra(g)->dc);
		break;
	}

	if (g->clip)
		app_del_region(g->clip);
	app_free(graphics_extra(g));
	app_free(g);
}

/*
 *  Set the drawing colour.
 */
void app_set_rgb(Graphics *g, Colour c)
{
	HBRUSH oldbr, brush;
	HPEN oldpen, pen;
	int red, green, blue;

	if (g->img) {
		if (g->img->cmap_size > 0)
			g->pixval = app_image_find_colour(g->img, c);
	}
	else {
		if (g->xor_mode) {
			red   = c.red   ^ graphics_extra(g)->bg.red;
			green = c.green ^ graphics_extra(g)->bg.green;
			blue  = c.blue  ^ graphics_extra(g)->bg.blue;
		}
		else {
			red   = c.red;
			green = c.green;
			blue  = c.blue;
		}
		g->pixval = PALETTERGB(red, green, blue);
		brush = CreateSolidBrush(g->pixval);
		oldbr = SelectObject(graphics_extra(g)->dc, brush);
		pen    = CreatePen(PS_SOLID, 1, g->pixval);
		oldpen = SelectObject(graphics_extra(g)->dc, pen);

		if (! graphics_extra(g)->oldbr)
			graphics_extra(g)->oldbr = oldbr;
		else
			DeleteObject(oldbr);
		graphics_extra(g)->brush = brush;

		if (! graphics_extra(g)->oldpen)
			graphics_extra(g)->oldpen = oldpen;
		else
			DeleteObject(oldpen);
		graphics_extra(g)->pen = pen;
	}

	g->colour = c;
}

void app_set_rgbindex(Graphics *g, int index)
{
	HBRUSH oldbr, brush;
	HPEN oldpen, pen;

	if (g->img) {
		if (g->img->cmap_size > 0)
			g->colour = g->img->cmap[index];
		g->pixval = index;
	}
	else {
		g->pixval = PALETTEINDEX(index);
		brush = CreateSolidBrush(g->pixval);
		oldbr = SelectObject(graphics_extra(g)->dc, brush);
		pen    = CreatePen(PS_SOLID, 1, g->pixval);
		oldpen = SelectObject(graphics_extra(g)->dc, pen);

		if (! graphics_extra(g)->oldbr)
			graphics_extra(g)->oldbr = oldbr;
		else
			DeleteObject(oldbr);
		graphics_extra(g)->brush = brush;

		if (! graphics_extra(g)->oldpen)
			graphics_extra(g)->oldpen = oldpen;
		else
			DeleteObject(oldpen);
		graphics_extra(g)->pen = pen;
	}
}

/*
 *  Set/remove the XOR drawing mode.
 */
void app_set_xor_mode(Graphics *g, Colour bgcol)
{
	graphics_extra(g)->bg = bgcol;
	g->xor_mode = 1;
	app_set_rgb(g, g->colour);
}

void app_set_paint_mode(Graphics *g)
{
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
	if (graphics_extra(g)->dc && font_extra(f)->fnt)
		SelectObject(graphics_extra(g)->dc, font_extra(f)->fnt);
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
