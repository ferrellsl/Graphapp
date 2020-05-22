/*
 *  A scrollable list of lines of text.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Can now use any font.
 *  Version: 3.02  2001/10/10  Using copy_rect scrolling for speed.
 *  Version: 3.03  2001/10/20  Focus rectangle added.
 *  Version: 3.07  2001/11/03  Added deletion handler.
 *  Version: 3.09  2001/11/13  Improved disabled appearance and behaviour.
 *  Version: 3.10  2001/12/01  Added app_add_list_box constructor.
 *  Version: 3.24  2003/06/10  Reduced flicker during mouse scrolling.
 *  Version: 3.37  2003/12/31  Handles CONTROL/SHIFT key bits (ignores).
 *  Version: 3.40  2003/03/07  Centralised colours, less wasted space.
 *  Version: 3.41  2003/03/14  Added focus-change (refocus) handler.
 *  Version: 3.49  2003/09/07  Fixed minor visual bug with focus border.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced some size_t conversion warnings.
 *  Version: 3.60  2007/06/06  Unified window/control adding code.
 *  Version: 3.62  2010/06/29  Various enhancements.
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
	SCROLL_SIZE = 16,
	BOX_BORDER  = SCROLL_SIZE
};

typedef struct ListBox  ListBox;

struct ListBox {
	int num_lines;
	char **list;
	int left;
	int top;
	int old_value;
	int vert_max, vert_pagesize;
	int horz_max, horz_pagesize;
	Control *parent;
	Control *vert;
	Control *horz;
	Control *box;
	int    (*get_item_height)(Control *c, int i);
	int    (*get_item_width)(Control *c, int i);
	void   (*draw_item)(Control *c, Graphics *g, int x, int y, Rect r, int h, int i);
};

static int app_listbox_get_item_height(Control *c, int i)
{
	ListBox *lb = c->extra;
	return app_font_height(lb->parent->font) + 1;
}

static int app_listbox_get_item_width(Control *c, int i)
{
	ListBox *lb = c->extra;
	Font *f = lb->parent->font;
	char *item = lb->list[i];

	return app_font_width(f, item, (int) strlen(item));
}

static void app_listbox_draw_item
	(Control *c, Graphics *g, int x, int y, Rect r, int h, int i)
{
	ListBox *lb = c->extra;
	Font *f = lb->parent->font;
	char *item = lb->list[i];

	app_set_font(g, f);
	app_draw_utf8(g, pt(x,y), item, (int) strlen(item));
}

static void app_listbox_change_values(Control *c)
{
	int top, bottom, left, horzmax;
	int h, max, pagesize;
	Rect r;
	ListBox *lb;

	lb = c->extra;

	r = app_get_control_area(lb->box);
	h = lb->get_item_height(c, 0);

	top = lb->top;
	if (top < 0)
		top = 0;
	left = lb->left;
	if (left < 0)
		left = 0;
	pagesize = (r.height -8) / h;
	max = lb->num_lines - pagesize;
	if (max < 0)
		max = 0;

	bottom = top + pagesize - 1;
	if (bottom >= lb->num_lines)
		bottom = lb->num_lines - 1;

	horzmax = lb->horz_max;
	if (left > horzmax)
		left = horzmax;

	if ((lb->parent->value < 0) || (lb->parent->value >= lb->num_lines))
		lb->parent->value = -1;
	else if (lb->parent->value < top)
		top = lb->parent->value;
	else if (lb->parent->value > bottom)
		top = lb->parent->value - pagesize + 1;

	if (top < 0)
		top = 0;
	if (left < 0)
		left = 0;

	lb->top = top;
	lb->vert_max = max;
	lb->vert_pagesize = pagesize;
	app_change_scroll_bar(lb->vert, top, max, pagesize);

	lb->left = left;
	lb->horz_max = horzmax;
	lb->horz_pagesize = r.width-8;
	app_change_scroll_bar(lb->horz, left, horzmax, r.width-8);

	app_draw_control(lb->box);
}

static void app_listbox_redraw(Control *c, Graphics *g, int do_clip)
{
	int max, selection;
	int i, h, x, y;
	Colour bg, fg;
	Rect r;
	ListBox *lb;

	lb = c->extra;

	r = app_get_control_area(lb->box);

	if (app_is_enabled(lb->parent))
		fg = app_get_control_foreground(lb->box);
	else
		fg = DISABLED_ITEM;
	bg = app_get_control_background(lb->box);

	h = lb->get_item_height(c, 0);

	max = lb->top + (r.height - 8) / h;
	if (max > lb->num_lines)
		max = lb->num_lines;
	selection = lb->parent->value;

	/* draw the visible lines of text */
	if (do_clip)
		app_set_clip_rect(g, app_inset_rect(r, 4));
	x = 4 - lb->left + 1;
	for (i = lb->top; i < max; i++) {
		y = 4 + (i - lb->top) * h;
		if (i == selection)
			app_set_colour(g, fg);
		else
			app_set_colour(g, bg);
		app_fill_rect(g, rect(4, y, r.width-8, h));
		if (i == selection)
			app_set_colour(g, bg);
		else
			app_set_colour(g, fg);
		if (lb->list[i] == NULL)
			break;
		lb->draw_item(c, g, x, y, r, h, i);
	}
	if (do_clip)
		app_set_clip_rect(g, r);

	/* draw the listbox border */
	if (app_has_focus(lb->parent)) {
		app_set_line_width(g, 1);
		app_set_colour(g, FOCUS_BORDER);
		if (BOX_BORDER > SCROLL_SIZE+2) {
			/* focus border around entire area */
			app_draw_rect(g, r);

			/* shadow border within that focus border */
			app_draw_shadow_rect(g, app_inset_rect(r, 1),
					LOWER_RIGHT, UPPER_LEFT);
		}
		else {
			/* focus border only on top-left edges */
			app_fill_rect(g, rect(r.x,r.y,r.width,1));
			app_fill_rect(g, rect(r.x,r.y+1,1,r.height-1));

			/* shadow border from x+1,y+1 to lower-right */
			app_draw_shadow_rect(g, rect(r.x+1, r.y+1,
					r.width-1, r.height-1),
					LOWER_RIGHT, UPPER_LEFT);
			app_draw_shadow_rect(g, rect(r.x+1, r.y+1,
					r.width-2, r.height-2),
					CLEAR, UPPER_LEFT);
		}
	}
	else {
		app_set_line_width(g, 2);
		app_draw_shadow_rect(g, r, LOWER_RIGHT, UPPER_LEFT);
		app_set_line_width(g, 1);
	}

	/* draw a pale inner border last to separate text */
	app_set_line_width(g, 2);
	app_set_colour(g, bg);
	app_draw_rect(g, app_inset_rect(r, 2));
	app_set_line_width(g, 1);
}

static void app_listbox_draw(Control *c, Graphics *g)
{
	app_listbox_redraw(c, g, 1);
}

static void app_listbox_mouse_drag(Control *c, int buttons, Point p)
{
	int h, new_value;
	int top, max, bottom, must_scroll;
	Rect r;
	ListBox *lb;

	lb = c->extra;

	if (! app_is_enabled(lb->parent))
		return;
	if (! app_is_armed(lb->parent))
		return;

	h = lb->get_item_height(c, 0);

	r = app_get_control_area(lb->box);

	top = lb->top;
	max = top + (r.height -8) / h;
	bottom = max - 1;
	if (bottom >= lb->num_lines)
		bottom = lb->num_lines - 1;

	if (p.y <= 2)
		p.y -= h;
	new_value = top + (p.y - 3) / h;

	if ((new_value < top) && (top == 0)) {
		must_scroll = 0;
		new_value = -1;
	}
	else if ((new_value > bottom) && (max >= lb->num_lines)) {
		must_scroll = 0;
		new_value = -1;
	}
	else if ((new_value < top) || (new_value > bottom)) {
		/* scroll to display new selection */
		if (new_value < top) {
			new_value = top - 1;
			if (new_value < 0)
				new_value = -1;
			else
				lb->top = top - 1;
		} else {
			new_value = bottom + 1;
			if (new_value >= lb->num_lines)
				new_value = -1;
			else
				lb->top = top + 1;
		}
		must_scroll = 1;
	}
	else
		must_scroll = 0;

	if (lb->parent->value == new_value)
		return;	/* nothing to be done */

	if (must_scroll) {
		lb->parent->value = new_value;
		app_listbox_change_values(c);
	}
	else {
		app_set_list_box_item(c, new_value);
	}
}

static void app_listbox_mouse_down(Control *c, int buttons, Point p)
{
	ListBox *lb;
	Rect r;

	lb = c->extra;
	r = app_get_control_area(lb->box);

	if (! app_is_enabled(lb->parent))
		return;
	if (! app_point_in_rect(p, r))
		return;
	app_arm(lb->parent);
	app_set_focus(lb->parent);
	lb->old_value = lb->parent->value;
	app_listbox_mouse_drag(c, buttons, p);
}

static void app_listbox_mouse_up(Control *c, int buttons, Point p)
{
	ListBox *lb;

	lb = c->extra;

	if (! app_is_enabled(lb->parent))
		return;
	if (! app_is_armed(lb->parent))
		return;
	app_disarm(lb->parent);
	/*if (lb->old_value != lb->parent->value)*/
		app_activate_control(lb->parent);
}

static void app_listbox_scroll_vertical(Control *c)
{
	ListBox *lb;
	Graphics *g;
	int top, h;
	Rect box, clip;
	Point dp;

	lb = c->parent->extra;
	if (lb->top == lb->vert->value)
		return;

	top = lb->top;
	lb->top = lb->vert->value;
	h = lb->get_item_height(c->parent, 0);
	box = app_get_control_area(lb->box);
	box = app_inset_rect(box, 4);
	box.height = (box.height / h) * h;
	clip = box;

	if (top < lb->top) {	/* scrolling down = copy upwards */
		clip.height = (lb->top - top) * h;
		dp = pt(box.x, box.y);
		box.height -= clip.height;
		clip.y = box.y + box.height;
		box.y += clip.height;
	}
	else {	/* scrolling up = copy downwards */
		clip.height = (top - lb->top) * h;
		dp = pt(box.x, box.y + clip.height);
		box.height -= clip.height;
	}

	if (box.height <= 0) {	/* major change = complete redraw */
		app_draw_control(lb->box);
		return;
	}

	g = app_get_control_graphics(lb->box);
	app_copy_rect(g, dp, g, box);
	app_set_clip_rect(g, clip);
	app_listbox_redraw(lb->box, g, 0); /* doing our own clipping */
	app_del_graphics(g);
}

static void app_listbox_scroll_horizontal(Control *c)
{
	ListBox *lb;
	Graphics *g;
	int left, h;
	Rect box, clip;
	Point dp;

	lb = c->parent->extra;
	if (lb->left == lb->horz->value)
		return;

	left = lb->left;
	lb->left = lb->horz->value;
	h = lb->get_item_height(c->parent, 0);
	box = app_get_control_area(lb->box);
	box = app_inset_rect(box, 4);
	box.height = (box.height / h) * h;
	clip = box;

	if (left < lb->left) {	/* scrolling right = copy left */
		clip.width = (lb->left - left);
		dp = pt(box.x, box.y);
		box.width -= clip.width;
		clip.x = box.x + box.width;
		box.x += clip.width;
	}
	else {	/* scrolling left = copy right */
		clip.width = (left - lb->left);
		dp = pt(box.x + clip.width, box.y);
		box.width -= clip.width;
	}

	if (box.width <= 0) {	/* major change = complete redraw */
		app_draw_control(lb->box);
		return;
	}

	g = app_get_control_graphics(lb->box);
	app_copy_rect(g, dp, g, box);
	app_set_clip_rect(g, clip);
	app_listbox_redraw(lb->box, g, 0); /* doing our own clipping */
	app_del_graphics(g);
}

static void app_listbox_key_action(Control *c, unsigned long key)
{
	int dx=0, dy=0;
	int new_left, new_value, top, max, h;
	Rect r;
	ListBox *lb;

	lb = c->extra;
	lb->old_value = lb->parent->value;

	switch (key & ~(CONTROL | SHIFT)) {
	  case UP:	dy = -1; break;
	  case DOWN:	dy = +1; break;
	  case PGUP:	dy = - lb->vert_pagesize; break;
	  case PGDN:	dy = + lb->vert_pagesize; break;
	  case LEFT:	dx = -5; break;
	  case RIGHT:	dx = +5; break;
	  case HOME:	dx = - lb->left; break;
	  case END:	dx = + lb->horz_max; break;
	  default:	app_pass_event(c); return;
	}

	if (dx) {
		new_left = lb->left + dx;
		if (new_left > lb->horz_max)
			new_left = lb->horz_max;
		if (new_left < 0)
			new_left = 0;
		if (new_left == lb->left)
			return;
		lb->left = new_left;
	}
	else if (dy) {
		if (! app_is_enabled(lb->parent))
			return;
		new_value = lb->parent->value + dy;
		if (new_value >= lb->num_lines)
			new_value = lb->num_lines - 1;
		if (new_value < 0)
			new_value = 0;
		if (new_value == lb->parent->value)
			return;

		r = app_get_control_area(lb->box);
		h = lb->get_item_height(c, 0);
		top = lb->top;
		max = top + (r.height -8) / h;
		if (max > lb->num_lines)
			max = lb->num_lines;
		//max -= 1;

		if ((new_value >= top) && (new_value < max)) {
			app_set_list_box_item(c, new_value);
			app_activate_control(lb->parent);
			return;
		}

		lb->parent->value = new_value;
	}

	if (lb->old_value != lb->parent->value) {
		app_listbox_change_values(lb->parent);
		app_activate_control(lb->parent);
	}
}

static void app_listbox_refocus(Control *c)
{
	ListBox *lb;
	Rect r;
	Region *clip, *inside;
	Graphics *g;

	lb = c->extra;
	r = app_get_control_area(lb->box);
	clip = app_new_rect_region(r);
	inside = app_new_rect_region(app_inset_rect(r,2));
	app_subtract_region(clip, inside, clip);
	app_del_region(inside);
	g = app_get_control_graphics(lb->box);
	app_set_clip_region(g, clip);
	app_del_region(clip);
	app_listbox_redraw(lb->box, g, 0); /* doing our own clipping */
	app_del_graphics(g);
}

static void app_listbox_resize(Control *c)	//!!
{
	ListBox *lb = c->extra;
	int i, h, max, pagesize, w;
	char ** list;
	Rect r;

	/* fix up scroll bars */
	r = app_get_control_area(c);
	h = lb->get_item_height(c, 0);

	/* count items in list */
	list = lb->list;
	if (list != NULL)
		for (max=0; list[max] != NULL; max++)
			continue;
	else
		max = 0;

	/* determine vertical scrollbar placement */
	pagesize = (r.height -BOX_BORDER -8) / h;
	max = max - pagesize;
	if (max < 0)
		max = 0;
	if (lb->top > max)
		lb->top = max;
	lb->vert_max = max;
	lb->vert_pagesize = pagesize;

#if 0	//!!
	app_set_control_area(lb->vert, rect(r.width-SCROLL_SIZE, 0,
				SCROLL_SIZE, r.height-BOX_BORDER));
#endif
	app_change_scroll_bar(lb->vert, lb->top, max, pagesize);

	/* find maximum item width */
	if (list != NULL) {
		for (i=max=0; list[i] != NULL; i++) {
			w = lb->get_item_width(c, i);
			if (max < w)
				max = w;
		}
	}
	else
		max = 0;

	/* determine horizontal scrollbar placement */
	pagesize = r.width -BOX_BORDER -8;
	max = max - pagesize;
	if (max < 0)
		max = 0;
	if (lb->left > max)
		lb->left = max;
	lb->horz_max = max;
	lb->horz_pagesize = pagesize;

#if 0	//!!
	app_set_control_area(lb->horz, rect(0, r.height-SCROLL_SIZE,
				r.width-BOX_BORDER, SCROLL_SIZE));
#endif
	app_change_scroll_bar(lb->horz, lb->left, max, pagesize);

#if 0	//!!
	/* resize the list box itself */
	app_set_control_area(lb->box, rect(0, 0,
				r.width-BOX_BORDER, r.height-BOX_BORDER));
#endif
}

static void app_list_box_del(Control *c)
{
	app_free(c->extra);
}

void app_set_list_box_item(Control *c, int index)
{
	ListBox *lb;
	int old_value;
	Graphics *g;
	Rect r;
	char **list;
	Colour fg, bg;
	int x, y, h, i, top, left, max;

	/* modify selected list index */
	lb = c->extra;
	old_value = lb->parent->value;

	if (index >= lb->num_lines)
		index = lb->num_lines - 1;
	if (index <= -1)
		index = -1;
	lb->parent->value = index;

	if (index == old_value)
		return;

	/* obtain graphics object to allow selective update */
	if (app_is_visible(lb->box))
		g = app_get_control_graphics(lb->box);
	else
		return;

	h = lb->get_item_height(c, 0);

	r = app_get_control_area(lb->box);

	fg = app_get_control_foreground(lb->box);
	bg = app_get_control_background(lb->box);

	list = lb->list;
	top = lb->top;
	left = lb->left;
	max = top + (r.height - 8) / h;
	if (max > lb->num_lines)
		max = lb->num_lines;

	/* draw the changed (visible) lines of text */
	app_set_clip_rect(g, app_inset_rect(r, 4));
	x = 4-left+1;
	if (((i = old_value) >= top) && (i < max)) {
		y = 4+(i-top)*h;
		app_set_colour(g, bg);
		app_fill_rect(g, rect(4, y, r.width-8, h));
		app_set_colour(g, fg);
		lb->draw_item(c, g, x, y, r, h, i);
	}
	if (((i = index) >= top) && (i < max)) {
		y = 4+(i-top)*h;
		app_set_colour(g, fg);
		app_fill_rect(g, rect(4, y, r.width-8, h));
		app_set_colour(g, bg);
		lb->draw_item(c, g, x, y, r, h, i);
	}
	app_del_graphics(g);
}

int app_get_list_box_item(Control *c)
{
	ListBox *lb;

	lb = c->extra;
	return lb->parent->value;
}

static int app_string_list_length(char *list[])
{
	int i = 0;

	if (list == NULL)
		return 0;
	for (i=0; list[i] != NULL; i++)
		continue;
	return i;
}

static char ** app_new_string_list(char *list[])
{
	int i = 0;
	char ** new_list;

	if (list == NULL)
		return NULL;
	for (i=0; list[i] != NULL; i++)
		continue;

	new_list = app_alloc((i+1)*sizeof(char*));
	new_list[i] = NULL;
	while (i-- > 0)
		new_list[i] = app_copy_string(list[i]);
	return new_list;
}

static void app_del_string_list(char *list[])
{
	int i;

	if (list == NULL)
		return;
	for (i=0; list[i] != NULL; i++)
		app_del_string(list[i]);
	app_free(list);
}

void app_change_list_box(Control *c, char **list)
{
	ListBox *lb;
	char ** new_list;

	lb = c->extra;

	/* copy list values and redraw */
	new_list = app_new_string_list(list);
	lb->num_lines = app_string_list_length(list);
	app_del_string_list(lb->list);
	lb->list = new_list;
	if (lb->parent->value >= lb->num_lines)
		lb->parent->value = lb->num_lines-1;

	app_listbox_resize(c);	/* fix scroll bars */
	app_redraw_control(lb->box);
}

void app_reset_list_box(Control *c)
{
	ListBox *lb;

	lb = c->extra;
	lb->parent->value = -1;
	lb->top = 0;
	lb->left = 0;

	app_listbox_resize(c);	/* fix scroll bars */
	app_redraw_control(lb->box);
}

static Control *app_create_list_box(Control *c, Rect r, char *list[],
	ControlFunc fn)	//!!
{
	Control *box;		/* the list box itself */
	Control *vert, *horz;	/* the scroll bars */
	ListBox *lb;

	/* enclosing area */
	if (c == NULL)
		return NULL;

	app_set_control_background(c, CLEAR);
	c->font = app_find_default_font(app_parent_window(c)->app);

	/* create scrollbars within the enclosing region */
	vert = app_add_scroll_bar(c, rect(r.width-SCROLL_SIZE, 0,
			SCROLL_SIZE, r.height-BOX_BORDER),
			100, 25, app_listbox_scroll_vertical);
	app_set_control_layout(vert, EDGE_TOP | EDGE_RIGHT | EDGE_BOTTOM);

	horz = app_add_scroll_bar(c, rect(0, r.height-SCROLL_SIZE,
			r.width-BOX_BORDER, SCROLL_SIZE),
			100, 25, app_listbox_scroll_horizontal);
	app_set_control_layout(horz, EDGE_LEFT | EDGE_RIGHT | EDGE_BOTTOM);

	/* create the list box canvas */
	box = app_add_control(c, rect(0, 0,
			r.width-BOX_BORDER, r.height-BOX_BORDER));
	app_set_control_background(box, FILL_ITEM);
	app_set_control_layout(box, EDGE_ALL);

	/* associate listbox data with scrollbars */
	lb = app_zero_alloc(sizeof(ListBox));
	lb->parent = c;
	lb->vert = vert;
	lb->horz = horz;
	lb->box = box;
	
	/* set item callbacks */
	lb->get_item_height = app_listbox_get_item_height;
	lb->get_item_width  = app_listbox_get_item_width;
	lb->draw_item       = app_listbox_draw_item;
	c->extra = box->extra = lb;

	/* initialise list */
	app_change_list_box(c, list);

	/* set callbacks */
	app_on_control_resize(c, app_listbox_resize);
	app_on_control_redraw(box, app_listbox_draw);
	app_on_control_mouse_down(box, app_listbox_mouse_down);
	app_on_control_mouse_drag(box, app_listbox_mouse_drag);
	app_on_control_mouse_up(box, app_listbox_mouse_up);
	app_on_control_key_action(c, app_listbox_key_action);
	app_on_control_refocus(c, app_listbox_refocus);
	app_on_control_deletion(c, app_list_box_del);
	app_on_control_action(c, fn);

	app_show_control(c);

	return c;
}

Control *app_new_list_box(Window *win, Rect r, char *list[], ControlFunc fn)
{
	return app_create_list_box(app_new_control(win, r), r, list, fn);
}

Control *app_add_list_box(Control *parent, Rect r, char *list[], ControlFunc fn)
{
	return app_create_list_box(app_add_control(parent, r), r, list, fn);
}

