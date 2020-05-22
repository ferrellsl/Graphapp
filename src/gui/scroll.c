/*
 *  Scroll bars.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Updated.
 *  Version: 3.03  2001/10/20  More bounds checking on thumb size.
 *  Version: 3.07  2001/11/03  Added deletion handler.
 *  Version: 3.09  2001/11/13  Improved disabled behaviour.
 *  Version: 3.10  2001/12/01  Added app_add_scroll_bar constructor.
 *  Version: 3.29  2002/08/22  Reduced redundant redrawing.
 *  Version: 3.30  2002/08/25  Added timer-based scrolling.
 *  Version: 3.31  2002/08/26  Made timer-scrolling faster.
 *  Version: 3.34  2002/12/18  Timer-scrolling now occurs every 0.1s.
 *  Version: 3.39  2003/03/05  Reduced flicker, more timer failsafes.
 *  Version: 3.40  2003/03/07  More Window-like look.
 *  Version: 3.41  2003/03/12  Reduced thumb flicker, centered arrows.
 *  Version: 3.54  2005/07/21  Unhandled keys (tab) go to underlying window.
 *  Version: 3.56  2005/08/03  Right-clicks now reverse normal motion.
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
	SCROLL_WIDTH  = 14,
	SCROLL_HEIGHT = 14
};

enum {
	DIR_LEFT  = 1,
	DIR_RIGHT = 2,
	DIR_UP    = 5,
	DIR_DOWN  = 6
};

enum {
	DRAG_NOTHING = 0,
	DRAG_DEC     = 1,
	DRAG_INC     = 2,
	DRAG_THUMB   = 3,
	DRAG_UP      = 4,
	DRAG_DOWN    = 5
};

enum {
	SCROLL_MSEC = 100
};

typedef struct ScrollBar  ScrollBar;

struct ScrollBar {
	Control *ctrl;
	int minstep;
	int pagestep;
	int max;
	int dragging;
	Point click_pos;
	Timer *timer;
};

static void app_draw_thumb(Graphics *g, Rect r)
{
	if ((r.width == 0) || (r.height == 0))
		return; /* no thumb */
	app_set_rgb(g, ENABLED_ITEM);
	app_draw_rect(g, r);
	app_draw_shadow_rect(g, app_inset_rect(r, 1), UPPER_LEFT, LOWER_RIGHT);
	app_set_rgb(g, SCROLL_THUMB_FILL);
	app_fill_rect(g, app_inset_rect(r, 2));
}

static void app_draw_scroll_arrow(Graphics *g, Rect r, int direction, int pressed)
{
	Point p[3];
	Colour col1, col2, bg;
	int midx, midy;

	if ((r.width==0) || (r.height==0))
		return;

	midx = r.x + r.width / 2;
	midy = r.y + r.height / 2;

	switch (direction) {
		case DIR_UP:
			p[0] = pt(midx+4, midy+2);
			p[1] = pt(p[0].x-9, p[0].y);
			p[2] = pt(p[1].x+4, p[1].y-5);
			r = rect(r.x,r.y,r.width,16);
			break;
		case DIR_DOWN:
			p[0] = pt(midx+3, midy-3);
			p[1] = pt(p[0].x-7, p[0].y);
			p[2] = pt(p[1].x+3, p[1].y+4);
			r = rect(r.x,r.y+r.height-16,r.width,16);
			break;
		case DIR_LEFT:
			p[0] = pt(midx+2, midy+4);
			p[1] = pt(p[0].x, p[0].y-9);
			p[2] = pt(p[1].x-4, p[1].y+4);
			r = rect(r.x,r.y,16,r.height);
			break;
		case DIR_RIGHT:
			p[0] = pt(midx-3, midy+3);
			p[1] = pt(p[0].x, p[0].y-8);
			p[2] = pt(p[1].x+4, p[1].y+4);
			r = rect(r.x+r.width-16,r.y,16,r.height);
			break;
	}

	if (pressed) {
		col1 = LOWER_RIGHT;
		col2 = UPPER_LEFT;
		bg = BACKGROUND;
		p[0].x++; p[0].y++;
		p[1].x++; p[1].y++;
		p[2].x++; p[2].y++;
	}
	else {
		col1 = UPPER_LEFT;
		col2 = LOWER_RIGHT;
		bg = BACKGROUND;
	}

	app_set_line_width(g, 1);
	app_set_rgb(g, ENABLED_ITEM);
	app_draw_rect(g, r);
	app_draw_shadow_rect(g, app_inset_rect(r,1), col1, col2);
	app_set_rgb(g, bg);
	app_fill_rect(g, app_inset_rect(r,2));
	app_set_rgb(g, ENABLED_ITEM);
	app_fill_polygon(g, p, 3);
}

static int app_is_vertical_scroll_bar(Rect r)
{
	if (r.width > r.height)
		return 0;
	else
		return 1;
}

static Rect app_vert_scroll_thumb_rect(ScrollBar *s, Rect r)
{
	long max_pixels, thumb_pixels, thumb_min;
	Rect thumb;

	if (! app_is_enabled(s->ctrl))
		return rect(0,0,0,0);

	if (r.height >= 3*SCROLL_HEIGHT) { /* show thumb */
		max_pixels = r.height - 2*SCROLL_HEIGHT;
		thumb_pixels = max_pixels * s->pagestep /
				(s->max + s->pagestep);
		if (thumb_pixels < SCROLL_HEIGHT)
			thumb_pixels = SCROLL_HEIGHT;
		if (thumb_pixels > max_pixels)
			thumb_pixels = max_pixels;
		if (s->max)
			thumb_min = (max_pixels - thumb_pixels) *
				s->ctrl->value / s->max;
		else
			thumb_min = 0;
		thumb = rect(r.x, r.y + thumb_min + SCROLL_HEIGHT,
				r.width, thumb_pixels);
	}
	else
		thumb = rect(0,0,0,0);

	return thumb;
}

static Rect app_horiz_scroll_thumb_rect(ScrollBar *s, Rect r)
{
	long max_pixels, thumb_pixels, thumb_min;
	Rect thumb;

	if (! app_is_enabled(s->ctrl))
		return rect(0,0,0,0);

	if (r.width >= 3*SCROLL_WIDTH) { /* show thumb */
		max_pixels = r.width - 2*SCROLL_WIDTH;
		thumb_pixels = max_pixels * s->pagestep /
				(s->max + s->pagestep);
		if (thumb_pixels < SCROLL_WIDTH)
			thumb_pixels = SCROLL_WIDTH;
		if (thumb_pixels > max_pixels)
			thumb_pixels = max_pixels;
		if (s->max)
			thumb_min = (max_pixels - thumb_pixels) *
				s->ctrl->value / s->max;
		else
			thumb_min = 0;
		thumb = rect(r.x + thumb_min + SCROLL_WIDTH, r.y,
				thumb_pixels, r.height);
	}
	else
		thumb = rect(0,0,0,0);

	return thumb;
}

static Rect app_up_scroll_arrow(ScrollBar *s, Rect r)
{
	if (r.height >= 2*SCROLL_HEIGHT) /* show arrows */
		return rect(r.x, r.y, r.width, SCROLL_HEIGHT);
	else
		return rect(r.x, r.y, r.width, r.height/2);
}

static Rect app_down_scroll_arrow(ScrollBar *s, Rect r)
{
	if (r.height >= 2*SCROLL_HEIGHT) /* show arrows */
		return rect(r.x, r.y + r.height - SCROLL_HEIGHT,
				r.width, SCROLL_HEIGHT);
	else
		return rect(r.x, r.y+r.height/2, r.width, (r.height+1)/2);
}

static Rect app_left_scroll_arrow(ScrollBar *s, Rect r)
{
	if (r.width >= 2*SCROLL_WIDTH) /* show arrows */
		return rect(r.x, r.y, SCROLL_WIDTH, r.height);
	else
		return rect(r.x, r.y, r.width/2, r.height);
}

static Rect app_right_scroll_arrow(ScrollBar *s, Rect r)
{
	if (r.width >= 2*SCROLL_WIDTH) /* show arrows */
		return rect(r.x + r.width - SCROLL_WIDTH, r.y,
				SCROLL_WIDTH, r.height);
	else
		return rect(r.x+r.width/2, r.y, (r.width+1)/2, r.height);
}

static void app_draw_scrollbar(Control *c, Graphics *g)
{
	Rect r;
	Colour bg;
	ScrollBar *s;
	Rect thumb_rect, arrow1, arrow2;

	r = app_get_control_area(c);
	s = c->extra;

	/* draw border of scrollbar */
	app_set_rgb(g, ENABLED_ITEM);
	app_draw_rect(g, r);
	app_draw_shadow_rect(g, app_inset_rect(r, 1), LOWER_RIGHT, UPPER_LEFT);

	/* fill interior with background colour */
	bg = app_get_control_background(c);
	app_set_rgb(g, bg);
	app_fill_rect(g, app_inset_rect(r, 2));

	if (app_is_vertical_scroll_bar(r))
	{
		thumb_rect = app_vert_scroll_thumb_rect(s, r);
		arrow1 = app_up_scroll_arrow(s, r);
		arrow2 = app_down_scroll_arrow(s, r);

		app_draw_thumb(g, thumb_rect);
		app_draw_scroll_arrow(g, arrow1, DIR_UP,
			s->dragging == DRAG_DEC ? 1 : 0);
		app_draw_scroll_arrow(g, arrow2, DIR_DOWN,
			s->dragging == DRAG_INC ? 1 : 0);
	}
	else {
		thumb_rect = app_horiz_scroll_thumb_rect(s, r);
		arrow1 = app_left_scroll_arrow(s, r);
		arrow2 = app_right_scroll_arrow(s, r);

		app_draw_thumb(g, thumb_rect);
		app_draw_scroll_arrow(g, arrow1, DIR_LEFT,
			s->dragging == DRAG_DEC ? 1 : 0);
		app_draw_scroll_arrow(g, arrow2, DIR_RIGHT,
			s->dragging == DRAG_INC ? 1 : 0);
	}
}

static void app_draw_scrollbar_arrows(Control *c)
{
	Rect r;
	Colour bg;
	ScrollBar *s;
	Graphics *g;
	Rect arrow1, arrow2;

	g = app_get_control_graphics(c);
	r = app_get_control_area(c);
	s = c->extra;

	/* fill appropriate arrow with background colour */
	bg = app_get_control_background(c);
	app_set_rgb(g, bg);

	if (app_is_vertical_scroll_bar(r)) {
		arrow1 = app_up_scroll_arrow(s, r);
		arrow2 = app_down_scroll_arrow(s, r);

		if (s->dragging == DRAG_DEC) {
			app_fill_rect(g, arrow1);
			app_draw_scroll_arrow(g, arrow1, DIR_UP, 1);
		}
		else if (s->dragging == DRAG_INC) {
			app_fill_rect(g, arrow2);
			app_draw_scroll_arrow(g, arrow2, DIR_DOWN, 1);
		}
		else {
			app_fill_rect(g, arrow1);
			app_fill_rect(g, arrow2);
			app_draw_scroll_arrow(g, arrow1, DIR_UP, 0);
			app_draw_scroll_arrow(g, arrow2, DIR_DOWN, 0);
		}
	}
	else {
		arrow1 = app_left_scroll_arrow(s, r);
		arrow2 = app_right_scroll_arrow(s, r);

		if (s->dragging == DRAG_DEC) {
			app_fill_rect(g, arrow1);
			app_draw_scroll_arrow(g, arrow1, DIR_LEFT, 1);
		}
		else if (s->dragging == DRAG_INC) {
			app_fill_rect(g, arrow2);
			app_draw_scroll_arrow(g, arrow2, DIR_RIGHT, 1);
		}
		else {
			app_fill_rect(g, arrow1);
			app_fill_rect(g, arrow2);
			app_draw_scroll_arrow(g, arrow1, DIR_LEFT, 0);
			app_draw_scroll_arrow(g, arrow2, DIR_RIGHT, 0);
		}
	}

	app_del_graphics(g);
}

static void app_scrollbar_update_thumb(Control *c, int value)
{
	Rect r, r1, r2, r1a, r1b, r2a, r2b;
	ScrollBar *s;
	Graphics *g;
	Region *clip1, *clip2;

	s = c->extra;

	if (value == s->ctrl->value)	/* nothing changed */
		return;

	r = app_get_control_area(c);
	if (app_is_vertical_scroll_bar(r)) {
		r1 = app_vert_scroll_thumb_rect(s, r);
		r1a = r1b = r1; /* top and bottom edge-shadows */
		r1b.y += r1b.height - 2;
		r1a.height = r1b.height = 2;
	}
	else {
		r1 = app_horiz_scroll_thumb_rect(s, r);
		r1a = r1b = r1; /* left and right edge-shadows */
		r1b.x += r1b.width - 2;
		r1a.width = r1b.width = 2;
	}

	s->ctrl->value = value;

	if (app_is_vertical_scroll_bar(r)) {
		r2 = app_vert_scroll_thumb_rect(s, r);
		r2a = r2b = r2; /* top and bottom edge-shadows */
		r2b.y += r2b.height - 2;
		r2a.height = r2b.height = 2;
	}
	else {
		r2 = app_horiz_scroll_thumb_rect(s, r);
		r2a = r2b = r2; /* left and right edge-shadows */
		r2b.x += r2b.width - 2;
		r2a.width = r2b.width = 2;
	}

	/* Draw to the difference of thumb rectangles (what's changed). */
	clip1 = app_new_rect_region(r1);
	clip2 = app_new_rect_region(r2);
	app_xor_region(clip1, clip2, clip1);
	app_del_region(clip2);

	/* Always draw to the shadow-edges (union of these regions). */
	app_union_region_with_rect(clip1, r1a, clip1);
	app_union_region_with_rect(clip1, r1b, clip1);
	app_union_region_with_rect(clip1, r2a, clip1);
	app_union_region_with_rect(clip1, r2b, clip1);

	/* Draw the scrollbar within the selected clipping region. */
	g = app_get_control_graphics(c);
	app_set_clip_region(g, clip1);
	app_draw_scrollbar(c, g);
	app_del_graphics(g);
	app_del_region(clip1);
	app_activate_control(c);
}

static void app_key_scrollbar(Control *c, unsigned long key)
{
	ScrollBar *s;
	int value;

	if (! app_is_enabled(c))
		return;

	s = c->extra;
	value = s->ctrl->value;

	switch (key) {
		case PGUP:	value -= s->pagestep; break;
		case PGDN:	value += s->pagestep; break;
		case LEFT:
		case UP:	value -= s->minstep; break;
		case RIGHT:
		case DOWN:	value += s->minstep; break;
		default:	app_pass_event(c); break;
	}
	if (value < 0)
		value = 0;
	if (value > s->max)
		value = s->max;
	app_scrollbar_update_thumb(c, value);
}

static void app_timeout_scrollbar(Timer *t)
{
	Control *c = t->data;
	int value, halt = 0;
	ScrollBar *s;

	if (! app_is_enabled(c))
		return;

	s = c->extra;
	value = s->ctrl->value;

	if (s->dragging == DRAG_DEC) {
		value -= s->minstep;
	}
	else if (s->dragging == DRAG_INC) {
		value += s->minstep;
	}
	else if (s->dragging == DRAG_UP) {
		value -= s->pagestep;
	}
	else if (s->dragging == DRAG_DOWN) {
		value += s->pagestep;
	}

	if (value < 0) {
		value = 0;
		halt = 1;
	}
	if (value > s->max) {
		value = s->max;
		halt = 1;
	}
	app_scrollbar_update_thumb(c, value);

	if (halt)
		if (s->timer) {
			app_del_timer(s->timer);
			s->timer = NULL;
			s->dragging = DRAG_NOTHING;
			app_draw_control(c);
		}
}

static void app_click_scrollbar(Control *c, int buttons, Point p)
{
	Rect r;
	Rect thumb, arrow1, arrow2, jumpup, jumpdown;
	int value;
	ScrollBar *s;

	if (! app_is_enabled(c))
		return;

	r = app_get_control_area(c);
	s = c->extra;

	value = s->ctrl->value;

	if (app_is_vertical_scroll_bar(r)) {
		thumb = app_vert_scroll_thumb_rect(s, r);
		arrow1 = app_up_scroll_arrow(s, r);
		arrow2 = app_down_scroll_arrow(s, r);
		if (thumb.height == 0) {
			jumpup = jumpdown = rect(0,0,0,0);
		}
		else {
			jumpup = rect(0,arrow1.height, r.width,
					thumb.y - arrow1.height);
			jumpdown = rect(0,thumb.y + thumb.height, r.width,
					arrow2.y - thumb.y - thumb.height);
		}
	}
	else {
		thumb = app_horiz_scroll_thumb_rect(s, r);
		arrow1 = app_left_scroll_arrow(s, r);
		arrow2 = app_right_scroll_arrow(s, r);
		if (thumb.width == 0) {
			jumpup = jumpdown = rect(0,0,0,0);
		}
		else {
			jumpup = rect(arrow1.width, 0,
					thumb.x - arrow1.width, r.height);
			jumpdown = rect(thumb.x + thumb.width, 0,
					arrow2.x - thumb.x - thumb.width,
					r.height);
		}
	}

	if (app_point_in_rect(p,thumb)) {
		s->click_pos = pt(p.x - thumb.x, p.y - thumb.y);
		s->dragging = DRAG_THUMB;
	}
	else if (app_point_in_rect(p,arrow1) && (buttons & LEFT_BUTTON)) {
		value -= s->minstep;
		s->dragging = DRAG_DEC;
	}
	else if (app_point_in_rect(p,arrow2) && (buttons & LEFT_BUTTON)) {
		value += s->minstep;
		s->dragging = DRAG_INC;
	}
	else if (app_point_in_rect(p,arrow2) && (buttons & RIGHT_BUTTON)) {
		value -= s->minstep;
		s->dragging = DRAG_DEC;
	}
	else if (app_point_in_rect(p,arrow1) && (buttons & RIGHT_BUTTON)) {
		value += s->minstep;
		s->dragging = DRAG_INC;
	}
	else if (app_point_in_rect(p,jumpup) && (buttons & LEFT_BUTTON)) {
		value -= s->pagestep;
		s->dragging = DRAG_UP;
	}
	else if (app_point_in_rect(p,jumpdown) && (buttons & LEFT_BUTTON)) {
		value += s->pagestep;
		s->dragging = DRAG_DOWN;
	}
	else if (app_point_in_rect(p,jumpdown) && (buttons & RIGHT_BUTTON)) {
		value -= s->pagestep;
		s->dragging = DRAG_UP;
	}
	else if (app_point_in_rect(p,jumpup) && (buttons & RIGHT_BUTTON)) {
		value += s->pagestep;
		s->dragging = DRAG_DOWN;
	}
	else
		s->dragging = DRAG_NOTHING;

	if (value < 0)
		value = 0;
	if (value > s->max)
		value = s->max;

	/* force a redraw */
	app_scrollbar_update_thumb(c, value);

	if ((s->dragging == DRAG_DEC) || (s->dragging == DRAG_INC)) {
		app_draw_scrollbar_arrows(c);
	}

	if ((s->dragging == DRAG_DEC) || (s->dragging == DRAG_INC) ||
	    (s->dragging == DRAG_UP) || (s->dragging == DRAG_DOWN))
	{
		if (s->timer == NULL) {
			s->timer = app_new_timer(app_parent_window(c)->app,
				app_timeout_scrollbar, SCROLL_MSEC);
			if (s->timer)
				s->timer->data = c;
		}
	}
	else {
		if (s->timer) {
			app_del_timer(s->timer);
			s->timer = NULL;
			app_draw_scrollbar_arrows(c);
		}
	}
}

static void app_drag_scrollbar(Control *c, int buttons, Point p)
{
	Rect r;
	Rect thumb, arrow1, arrow2, jumpup, jumpdown;
	long max_pixels, thumb_pixels, thumb_min;
	int value, diff;
	ScrollBar *s;

	if (! app_is_enabled(c))
		return;

	r = app_get_control_area(c);
	s = c->extra;
	value = s->ctrl->value;

	if (app_is_vertical_scroll_bar(r)) {
		thumb = app_vert_scroll_thumb_rect(s, r);
		arrow1 = app_up_scroll_arrow(s, r);
		arrow2 = app_down_scroll_arrow(s, r);
		if (thumb.height == 0) {
			jumpup = jumpdown = rect(0,0,0,0);
		}
		else {
			jumpup = rect(0,arrow1.height, r.width,
					thumb.y - arrow1.height);
			jumpdown = rect(0,thumb.y + thumb.height, r.width,
					arrow2.y - thumb.y - thumb.height);
		}
	}
	else {
		thumb = app_horiz_scroll_thumb_rect(s, r);
		arrow1 = app_left_scroll_arrow(s, r);
		arrow2 = app_right_scroll_arrow(s, r);
		if (thumb.width == 0) {
			jumpup = jumpdown = rect(0,0,0,0);
		}
		else {
			jumpup = rect(arrow1.width, 0,
					thumb.x - arrow1.width, r.height);
			jumpdown = rect(thumb.x + thumb.width, 0,
					arrow2.x - thumb.x - thumb.width,
					r.height);
		}
	}

	if (s->dragging == DRAG_DEC) {
	/*	value -= s->minstep;	// replaced by timer*/
	}
	else if (s->dragging == DRAG_INC) {
	/*	value += s->minstep;	// replaced by timer*/
	}
	else if (s->dragging == DRAG_UP) {
	/*	value -= s->pagestep;	// replaced by timer*/
	}
	else if (s->dragging == DRAG_DOWN) {
	/*	value += s->pagestep;	// replaced by timer*/
	}
	else if (s->dragging == DRAG_THUMB) {
		if (app_is_vertical_scroll_bar(r)) {
			/* set thumb position based on mouse y pos */
			diff = p.y - s->click_pos.y - thumb.y;

			max_pixels = r.height - 2*SCROLL_HEIGHT;
			thumb_pixels = max_pixels * s->pagestep /
				(s->max + s->pagestep);
			if (thumb_pixels < SCROLL_HEIGHT)
				thumb_pixels = SCROLL_HEIGHT;
			if (s->max)
				thumb_min = (max_pixels - thumb_pixels) *
					s->ctrl->value / s->max;
			else
				thumb_min = 0;

			value = s->max * (thumb_min + diff) /
				(max_pixels - thumb_pixels + 1);
		}
		else {
			/* set thumb position based on mouse x pos */
			diff = p.x - s->click_pos.x - thumb.x;

			max_pixels = r.width - 2*SCROLL_WIDTH;
			thumb_pixels = max_pixels * s->pagestep /
				(s->max + s->pagestep);
			if (thumb_pixels < SCROLL_WIDTH)
				thumb_pixels = SCROLL_WIDTH;
			if (s->max)
				thumb_min = (max_pixels - thumb_pixels) *
					s->ctrl->value / s->max;
			else
				thumb_min = 0;

			value = s->max * (thumb_min + diff) /
				(max_pixels - thumb_pixels + 1);
		}
	}

	if (value < 0)
		value = 0;
	if (value > s->max)
		value = s->max;
	app_scrollbar_update_thumb(c, value);
}

static void app_release_scrollbar(Control *c, int buttons, Point p)
{
	ScrollBar *s;

	s = c->extra;
	if ((s->dragging == DRAG_DEC) || (s->dragging == DRAG_INC)) {
		s->dragging = DRAG_NOTHING;
		app_draw_scrollbar_arrows(c);
	}
	else
		s->dragging = DRAG_NOTHING;

	if (s->timer) {
		app_del_timer(s->timer);
		s->timer = NULL;
	}
}

static void app_scrollbar_del(Control *c)
{
	app_free(c->extra);
}


void app_change_scroll_bar(Control *c, int pos, int max, int pagesize)
{
	ScrollBar *s;

	s = c->extra;

	if (pos < 0)
		pos = 0;
	if (pos > max)
		pos = max;

	if ((s->max == max) && (s->ctrl->value == pos)
	 && (s->pagestep == pagesize))
		return;

	s->max = max;
	s->ctrl->value = pos;
	s->pagestep = pagesize;

	if (app_is_visible(c))
		app_draw_control(c);
}

static Control *app_create_scroll_bar(Control *c, Rect r, int max,
	int pagesize, ControlFunc fn)	//!!
{
	ScrollBar *s;

	if (c == NULL)
		return c;

	s = app_zero_alloc(sizeof(ScrollBar));
	c->extra = s;

	s->ctrl = c;
	s->max = max;
	s->ctrl->value = 0;
	s->minstep = 1;
	s->pagestep = pagesize;
	s->dragging = DRAG_NOTHING;

	app_set_control_background(c, SCROLL_BACKGROUND);
	app_on_control_redraw(c, app_draw_scrollbar);
	app_on_control_key_action(c, app_key_scrollbar);
	app_on_control_mouse_down(c, app_click_scrollbar);
	app_on_control_mouse_drag(c, app_drag_scrollbar);
	app_on_control_mouse_up(c, app_release_scrollbar);
	app_on_control_mouse_move(c, app_release_scrollbar);
	app_on_control_action(c, fn);
	app_on_control_deletion(c, app_scrollbar_del);

	return c;
}

Control *app_new_scroll_bar(Window *win, Rect r, int max,
		int pagesize, ControlFunc fn)
{
	return app_create_scroll_bar(app_new_control(win, r), r, max,
				pagesize, fn);
}

Control *app_add_scroll_bar(Control *parent, Rect r, int max,
	int pagesize, ControlFunc fn)
{
	return app_create_scroll_bar(app_add_control(parent, r), r, max,
				pagesize, fn);
}

