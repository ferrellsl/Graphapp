/*
 *  Controls (sub-windows).
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/17  Added font capability to Controls.
 *  Version: 3.07  2001/11/03  Added deletion and update handlers.
 *  Version: 3.09  2001/11/13  Disabled controls now receive events.
 *  Version: 3.10  2001/12/01  Changed app_new_control, app_add_control.
 *  Version: 3.41  2003/03/14  Added focus-change (refocus) handler.
 *  Version: 3.47  2003/05/24  Added cursors and redraw_control_rect.
 *  Version: 3.50  2004/01/11  Events may be sent to multiple observers.
 *  Version: 3.51  2004/03/28  Supports delayed-deletion.
 *  Version: 3.57  2005/08/16  Added layout options DOCK, FLOW, AUTOSIZE.
 *  Version: 3.58  2005/08/28  Silenced a size_t conversion warning.
 *  Version: 3.60  2007/06/06  Fixed some bugs. Added tooltip support!
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"

/*
 *  Resize a visible control without re-placing others in window.
 */
static void app_try_resize_control(Control *c, int width, int height) //!!
{
	int i;

	if (c->area.width == width && c->area.height == height)
		return;
	c->area.width = width;
	c->area.height = height;
	if (c->resize)
		for (i=0; c->resize[i]; i++)
			c->resize[i](c);
}

static void app_autosize_controls(int num, Control **list,
	Rect *natural, int pad, long parent_state)
{
	int maxwidth, maxheight;
	int sumwidth, sumheight;
	Control *c;

	maxwidth = maxheight = 0;
	sumwidth = sumheight = 0;

	while (num--) {
		long edging;	//!!
		int wgap, hgap;

		c = list[num];
		edging = c->state & EDGE_ALL;	//!!

		if (c->num_children > 0) {
			app_autosize_controls(c->num_children, c->children,
				&c->natural, c->padding, c->state);

			if (c->state & AUTOSIZE)
				app_try_resize_control(c,
					c->natural.width, c->natural.height);
		}

		wgap = c->margin;
		hgap = c->margin;

		if (c->state & DOCK) {	//!!
			if (edging & (EDGE_TOP | EDGE_BOTTOM)) {
				wgap += c->natural.width;
				hgap += c->area.height;
			} else {
				wgap += c->area.width;
				hgap += c->natural.height;
			}
		} else {
			wgap += c->area.width;
			hgap += c->area.height;
		}

		if (edging != 0) {
			if (edging & (EDGE_TOP | EDGE_BOTTOM)) {
				sumheight += hgap;
				if (wgap > maxwidth)
					maxwidth = wgap;
			} else {
				sumwidth += wgap;
				if (hgap > maxheight)
					maxheight = hgap;
			}
		}
		else {
			wgap += c->area.x;
			if (wgap > maxwidth)
				maxwidth = wgap;
			hgap += c->area.y;
			if (hgap > maxheight)
				maxheight = hgap;
		}
	}

	if (natural != NULL) {
		if (! (parent_state & (FLOW | FLOW_VERT)))	//!!
			pad *= 2;
		natural->width = pad +
				(maxwidth > sumwidth ? maxwidth : sumwidth);
		natural->height = pad +
				(maxheight > sumheight ? maxheight : sumheight);
	}
}

static void app_handle_layout(Control *c,
	int parent_width, int parent_height,
	Rect *box, int pad, long flags)
{
	int w, h;

	w = c->area.width;
	h = c->area.height;

	if (flags & (DOCK | FLOW | FLOW_VERT)) {
		/* Docking */
		if (flags & DOCK) {
			switch (flags & EDGE_ALL)	//!!
			{
			  case EDGE_TOP:
				w = box->width;
				c->area.x = box->x;
				c->area.y = box->y;
				box->y += h + c->margin;
				box->height -= h + c->margin;
				break;
			  case EDGE_BOTTOM:
				w = box->width;
				box->height -= h + c->margin;
				c->area.x = box->x;
				c->area.y = box->y + box->height;
				break;
			  case EDGE_RIGHT:
				h = box->height;
				box->width -= w + c->margin;
				c->area.y = box->y;
				c->area.x = box->x + box->width;
				break;
			  case EDGE_ALL:	//!!
				c->area.x = box->x;
				c->area.y = box->y;
				w = box->width;
				h = box->height;
				break;
			  default:	/* EDGE_LEFT */
				h = box->height;
				c->area.x = box->x;
				c->area.y = box->y;
				box->x += w + c->margin;
				box->width -= w + c->margin;
				break;
			}
		}
		/* Horizontally Flowing */
		else if (flags & FLOW) {
			int margin = c->margin;	//!!

			if (box->x == pad) {
				if (box->y == pad)
					box->height = 0;
			}
			else if (box->x + w > box->width) {
				box->x = pad;
				box->y += box->height;
			}
			c->area.x = box->x;
			c->area.y = box->y;
			box->x += w + margin;
			margin += h;
			if (box->height < margin)
				box->height = margin;
		}
		/* Vertically Flowing */
		else {
			int margin = c->margin;	//!!

			if (box->y == pad) {
				if (box->x == pad)
					box->width = 0;
			}
			else if (box->y + h > box->height) {
				box->y = pad;
				box->x += box->width;
			}
			c->area.x = box->x;
			c->area.y = box->y;
			box->y += h + margin;
			margin += w;
			if (box->width < margin)
				box->width = margin;
		}
	}
	/* Anchoring */
	else {
		if (flags & EDGE_RIGHT) {
			/* stretch or move horizontally */
			if (flags & EDGE_LEFT)
				w = parent_width - c->area.x + c->corner.x;
			else
				c->area.x = parent_width - w + c->corner.x;
		}
		if (flags & EDGE_BOTTOM) {
			/* stretch or move vertically */
			if (flags & EDGE_TOP)
				h = parent_height - c->area.y + c->corner.y;
			else
				c->area.y = parent_height - h + c->corner.y;
		}
	}

	app_try_resize_control(c, w, h);	//!!
}

/* Correct size of docking and flowing autosize control */	//!!
static void app_autosize_dock_flow(Control *c, Rect *box, long flags)
{
	int w, h, dw, dh;

	w = c->area.width;
	h = c->area.height;

	/* transition point of flowing children */
	dw = c->natural.width;
	dh = c->natural.height;
	app_autosize_controls(c->num_children,
		c->children, &c->natural, c->padding, c->state);
	dw = c->natural.width - dw;
	dh = c->natural.height - dh;
	if ((dw != 0) || (dh != 0)) {
		if ((flags == EDGE_BOTTOM) || (flags == EDGE_TOP)) {
			if (flags == EDGE_BOTTOM)
				c->area.y -= dh;
			else
				box->y += dh;
			h += dh;
			box->height -= dh;
		}
		else {
			if (flags == EDGE_RIGHT)
				c->area.x -= dw;
			else
				box->x += dw;
			w += dw;
			box->width -= dw;
		}
	}

	/* roll up control into box */
	if (box->width < 0) {
		if (flags == EDGE_RIGHT)
			c->area.x -= box->width;
		w += box->width;
		box->width = 0;
	}
	if (box->height < 0) {
		if (flags == EDGE_BOTTOM)
			c->area.y -= box->height;
		h += box->height;
		box->height = 0;
	}

	app_try_resize_control(c, w, h);	//!!
}

/*
 *  Recursive function to re-calculate layout and size of all controls
 *  and all children of those controls.
 */
static void app_layout_controls(int num, Control **list,
	int parent_width, int parent_height, int pad, long parent_state)
{
	const int parent_not_manager = (parent_state & MANAGER) == 0L;
	const long flow_flag = parent_state & (FLOW | FLOW_VERT);
	long layout_flags = 0L;
	Rect box;
	Control *c;

	/* new docking/flowing area for children */
	box = rect(pad, pad, parent_width-pad*2, parent_height-pad*2);

	while (num--) {
		c = list[num];

		if (! (c->state & VISIBLE))
			continue;

		if (parent_not_manager) {
			layout_flags = (flow_flag != 0L)
				? flow_flag : c->state & LAYOUT_MASK;
		if (layout_flags != 0)
				app_handle_layout(c,
					parent_width, parent_height,
				&box, pad, layout_flags);
		}

		/* recursively call this function on all children */
		if (c->num_children > 0) {
			app_layout_controls(c->num_children, c->children,
				c->area.width, c->area.height,
				c->padding, c->state);

			/* correct autosize of docking and flowing control */
			if ((c->state & (DOCK | AUTOSIZE)) == (DOCK | AUTOSIZE)
			    && (c->state & (FLOW | FLOW_VERT))	//!!
			    && ((layout_flags &= EDGE_ALL) != EDGE_ALL))
				app_autosize_dock_flow(c, &box, layout_flags);
				}

		/* call resize manager to process adding/deleting of children */
		if ((c->state & MANAGER) && c->resize) {	//!!
			int i;
			for (i=0; c->resize[i]; i++)
				c->resize[i](c);
		}
	}
}

/*
 *  Recursive function to place all controls and all children of
 *  those controls. Creates or updates each control's visible region,
 *  expressed in window co-ordinates. Also sets each control's offset,
 *  which is its top-left point expressed in window co-ordinates.
 */
static void app_place_controls(Region *parent, int parent_x, int parent_y,
	int num, Control **list, int is_visible)
{
	int i, x, y, w, h;
	Control *c;

	for (i=0; i<num; i++) {
		c = list[i];

		/* find rectangle in window co-ordinates */
		x = c->area.x + parent_x;
		y = c->area.y + parent_y;
		w = c->area.width;
		h = c->area.height;

		/* remember control's offset from window co-ordinates */
		c->offset.x = x;
		c->offset.y = y;

		/* delete the control's visible region, if any */
		if (c->visible)
			app_del_region(c->visible);

		/* handle invisible controls specially */
		if ((is_visible == 0) || ((c->state & VISIBLE) == 0)) {
			c->visible = app_new_region();
			if (c->num_children > 0)
				app_place_controls(c->visible, x, y,
					c->num_children, c->children, 0);
			continue;
		}

		/* determine control's visible rectangle */
		c->visible = app_new_rect_region(rect(x, y, w, h));

		/* clip visible region against parent's boundary */
		app_intersect_region(c->visible, parent, c->visible);

		/* remove this control's region from parent's region */
		/* unless the background of this control is transparent */
		if (c->bg.alpha <= 0x7F)
			app_subtract_region(parent, c->visible, parent);

		/* recursively call this function on all children */
		if (c->num_children > 0)
			app_place_controls(c->visible, x, y,
				c->num_children, c->children, 1);
	}
}

/*
 *  Create or update a window's visible region, and the visible regions
 *  of all child controls, recursively. This is a depth-first pre-order
 *  traversal of the hierarchy tree. Depth-first, since if control A
 *  is a child of control B, which is a child of the window, and control
 *  C is also a child of the window, but is behind B, then A (which is
 *  deeper in the hierarchy) is in front of C. Pre-order traversal,
 *  since we wish to determine B's visible region before A's, so that
 *  we can later subtract A's region from B's.
 */
void app_place_window_controls(Window *w, int recalculate)
{
	/* handle autosize and layout of all children */
	if ((recalculate != 0) && (w->num_children > 0)) {
		/* to avoid recursive calling hell */
		if (w->state & CHECKED)
			return;	//!!
		w->state |= CHECKED;

		app_autosize_controls(w->num_children, w->children,
			NULL, 0, 0L);
		app_layout_controls(w->num_children, w->children,
			w->area.width, w->area.height, 0, 0);

		w->state &= ~ CHECKED;
	}

	/* delete the window's visible region, if any */
	if (w->visible)
		app_del_region(w->visible);

	/* handle invisible windows specially */
	if ((w->state & VISIBLE) == 0) {
		w->visible = app_new_region();
		if (w->num_children > 0)
			app_place_controls(w->visible, 0, 0,
				w->num_children, w->children, 0);
		return;
	}

	/* determine window's visible rectangle */
	w->visible = app_new_rect_region(app_get_window_area(w));

	/* call the recursive function to place all children */
	if (w->num_children > 0)
		app_place_controls(w->visible, 0, 0,
			w->num_children, w->children, 1);
}

/*
 *  Recursive function to find which control a mouse click
 *  occurred within. This is a post-order traversal of the tree.
 */
static Control *app_locate_controls(int num, Control **list, Point p)
{
	int i;
	Rect r;
	Control *c;
	Control *child = NULL;

	for (i=0; i<num; i++) {
		c = list[i];

		/* if this control is not visible, events pass through */
		if ((c->state & VISIBLE) == 0)
			continue;

		/* if the point is outside this control's rect, pass */
		r.x = c->offset.x;
		r.y = c->offset.y;
		r.width = c->area.width;
		r.height = c->area.height;
		if (! app_point_in_rect(p, r))
			continue;

		/* if this control is not enabled, events pass through */
		/*
		if ((c->state & ENABLED) == 0)
			continue;
		*/
		/* this is now handled on a per-control basis */

		/* recursively call this function on all children */
		if (c->num_children > 0)
			child = app_locate_controls(c->num_children,
				c->children, p);
		if (child)
			return child;

		/* if control has no mouse handlers and transparent, pass */
		if ((c->mouse_down == NULL) &&
			(c->mouse_up == NULL) &&
			(c->mouse_move == NULL) &&
			(c->mouse_drag == NULL) &&
			(c->bg.alpha > 0x7F))	//!!
				continue;

		/* otherwise, all tests were passed */
		return c;
	}
	return NULL;
}

/*
 *  Find which control a mouse click is directed to.
 *  Calls a recursive function (above) to do most of the work.
 *  Assumes app_place_window_controls has been called, so all the
 *  visible regions are correctly set, and also assumes that
 *  this window is visible. Returns NULL if no child control can
 *  handle the mouse click.
 */
Control *app_locate_control(Window *w, Point p)
{
	if (w->mouse_grab)
		return w->mouse_grab;

	if ((w->state & VISIBLE) == 0)
		return NULL;

	/* call the recursive function to check all children */
	if (w->num_children > 0)
		return app_locate_controls(w->num_children, w->children, p);
	return NULL;
}

/*
 *  Recursive function to draw all child controls in a list.
 *  It draws the control first, then all of its children from
 *  front to back. This is acceptible, since the clipping regions
 *  for each control will be distinct.
 *
 *  This function re-uses a supplied Graphics object, which
 *  already contains a valid Window ID, so all we have to do is
 *  change the clipping region and the offset to make it appear
 *  to be a Control's Graphics object, rather than the Window's.
 *  This speeds thing up a little, since we don't have to create
 *  and destroy many Graphics objects for each redraw event, plus
 *  we inherit whatever clipping has already been set on the
 *  Window due to the redraw event itself.
 */
void app_do_draw_controls(Graphics *g, int num, Control **list, int clear)
{
	int n, i;
	Control *c;

	for (n=num-1; n >= 0; n--) {
		c = list[n];

		if ((c->state & VISIBLE) == 0)
			continue;	/* skip invisible controls */

		/* draw this control */
		g->ctrl = c;
		g->offset = c->offset;
		app_set_clip_region(g, NULL);
		if ((clear) && (c->bg.alpha <= 0x7F)) {
			app_set_rgb(g, c->bg);
			app_fill_rect(g, app_get_control_area(c));
		}
		app_set_rgb(g, BLACK);
		app_set_line_width(g, 1);
		if (c->redraw)
			for (i=0; c->redraw[i]; i++)
				c->redraw[i](c, g);

		/* draw children of this control */
		if (c->children)
			app_do_draw_controls(g, c->num_children, c->children, clear);
	}
}

/*
 *  Draw this control.
 */
void app_draw_control(Control *c)
{
	int i;
	Graphics *g;
	Window *w;

	if ((c->state & VISIBLE) == 0)
		return;
	w = app_parent_window(c);
	if ((w == NULL) || ((w->state & VISIBLE) == 0))
		return;

	g = app_get_control_graphics(c);
	app_set_rgb(g, BLACK);
	if (c->redraw)
		for (i=0; c->redraw[i]; i++)
			c->redraw[i](c, g);

	/* draw children of this control */
	if (c->children)
		app_do_draw_controls(g, c->num_children, c->children, 0);
	app_del_graphics(g);
}

void app_redraw_control(Control *c)
{
	int i;
	Graphics *g;
	Window *w;

	if ((c->state & VISIBLE) == 0)
		return;
	w = app_parent_window(c);
	if ((w == NULL) || ((w->state & VISIBLE) == 0))
		return;

	g = app_get_control_graphics(c);
	if (c->bg.alpha <= 0x7F) {
		app_set_rgb(g, c->bg);
		app_fill_rect(g, app_get_control_area(c));
	}
	app_set_rgb(g, BLACK);
	if (c->redraw)
		for (i=0; c->redraw[i]; i++)
			c->redraw[i](c, g);

	/* draw children of this control */
	if (c->children)
		app_do_draw_controls(g, c->num_children, c->children, 1);
	app_del_graphics(g);
}

#if 0	//!!
/*
 *  Which window is a control on?
 */
Window *app_parent_window(Control *c)
{
	while (c) {
		if (c->win)
			return c->win;
		else
			c = c->parent;
	}
	return NULL;
}
#endif

/*
 *  Pass an event up the control hierarchy.
 */
void app_pass_event(Control *c)
{
	Window *win;

	win = app_parent_window(c);
	if (win)
		win->pass_event = 1;
}

/*
 *  Move a control on its parent.
 */
void app_move_control(Control *c, Rect r)
{
	int i;

	c->area.x = r.x;
	c->area.y = r.y;
	if (c->state & VISIBLE) {
		app_place_window_controls(app_parent_window(c), 1);
		if (c->resize)
			for (i=0; c->resize[i]; i++)
				c->resize[i](c);
	}
}

/*
 *  Change the size of a control.
 */
void app_size_control(Control *c, Rect r)
{
	int i;

	c->area.width = r.width;
	c->area.height = r.height;
	if (c->state & VISIBLE) {
		app_place_window_controls(app_parent_window(c), 1);
		if (c->resize)
			for (i=0; c->resize[i]; i++)
				c->resize[i](c);
	}
}

/*
 *  Move and resize a control on its parent.
 */
void app_set_control_area(Control *c, Rect r)
{
	int i;

	c->area = r;
	if (c->state & VISIBLE) {
		app_place_window_controls(app_parent_window(c), 1);
		if (c->resize)
			for (i=0; c->resize[i]; i++)
				c->resize[i](c);
	}
}

/*
 *  Return the control's rectangle, in its own co-ordinate system.
 */
Rect app_get_control_area(Control *c)
{
	return rect(0, 0, c->area.width, c->area.height);
}

/*
 *  Does the control's state say it's visible?
 */
int app_is_visible(Control *c)
{
	return (c->state & VISIBLE) ? 1 : 0;
}

/*
 *  Make a control visible, if it isn't already.
 */
void app_show_control(Control *c)
{
	if ((c->state & VISIBLE) == 0) {
		c->state |= VISIBLE;
		app_place_window_controls(app_parent_window(c), 1);
	}
}

/*
 *  Make a control invisible, if it isn't already.
 */
void app_hide_control(Control *c)
{
	if (c->state & VISIBLE) {
		c->state &= ~ VISIBLE;
		app_place_window_controls(app_parent_window(c), 1);
	}
}

/*
 *  Set window to a control and its children.
 */
static void app_set_parent_window(Window *w, Control *c)
{
	int num = c->num_children;

	c->win = w;
	while (num--)
		app_set_parent_window(w, c->children[num]);
}

/*
 *  Attach a control to a window's child control list.
 *  The control is attached at the front of the list.
 *  The control placements are only recomputed if the window
 *  is currently visible, otherwise this will happen later,
 *  during app_show_window.
 */
int app_attach_to_window(Window *w, Control *c)
{
	int num;
	Control **list;

	num = w->num_children;
	list = app_realloc(w->children, (num+1) * sizeof(Control *));
	if (list == NULL)
		return 0;
	w->children = list;
	w->num_children++;
	if (num > 0)
		memmove(&list[1], list, num * sizeof(Control *));
	list[0] = c;
	c->parent = NULL;
	if (c->win != w)	//!!
		app_set_parent_window(w, c);
	if (c->state & VISIBLE)	//!!
		app_place_window_controls(w, 1);
	return 1;
}

/*
 *  Attach a control to another control's child control list.
 *  The control is attached at the front of the list.
 *  The control placements are only recomputed if the window
 *  is currently visible, otherwise this will happen later,
 *  during app_show_window.
 */
int app_attach_to_control(Control *parent, Control *c)
{
	int num;
	Control **list;
	Window *w;

	num = parent->num_children;
	list = app_realloc(parent->children, (num+1) * sizeof(Control *));
	if (list == NULL)
		return 0;
	parent->children = list;
	parent->num_children++;
	if (num > 0)
		memmove(&list[1], list, num * sizeof(Control *));
	list[0] = c;
	c->parent = parent;
	w = app_parent_window(parent);
	if (c->win != w)	//!!
		app_set_parent_window(w, c);
	if (c->state & VISIBLE)	//!!
		app_place_window_controls(w, 1);
	return 1;
}

/*
 *  Bring a control to the front of the stacking order.
 *  If the control is not attached to anything, this
 *  function does nothing, returning zero.
 *  The control placements are only recomputed if the window
 *  is currently visible, otherwise this will happen later,
 *  during app_show_window.
 */
int app_bring_control_to_front(Control *c)
{
	int num = 0;
	int pos;
	Control **list = NULL;
	Window *w;

	if (c->parent) {
		num = c->parent->num_children;
		list = c->parent->children;
	}
	else if (c->win) {
		num = c->win->num_children;
		list = c->win->children;
	}
	if (list == NULL)
		return 0;
	for (pos=0; pos < num; pos++)
		if (list[pos] == c)
			break;
	if ((pos > 0) && (pos < num))
		memmove(&list[1], list, pos * sizeof(Control *));
	list[0] = c;
	w = app_parent_window(c);
	if (w && (c->state & VISIBLE))	//!!
		app_place_window_controls(w, 1);
	return 1;
}

/*
 *  Send a control to the end of the stacking order.
 *  If the control is not attached to anything, this
 *  function does nothing, returning zero.
 *  The control placements are only recomputed if the window
 *  is currently visible, otherwise this will happen later,
 *  during app_show_window.
 */
int app_send_control_to_back(Control *c)
{
	int num = 0;
	int pos;
	Control **list = NULL;
	Window *w;

	if (c->parent) {
		num = c->parent->num_children;
		list = c->parent->children;
	}
	else if (c->win) {
		num = c->win->num_children;
		list = c->win->children;
	}
	if (list == NULL)
		return 0;
	for (pos=0; pos < num; pos++)
		if (list[pos] == c)
			break;
	if ((pos >= 0) && (pos < num-1))
		memmove(&list[pos], &list[pos+1],
			(num-pos-1) * sizeof(Control *));
	list[num-1] = c;
	w = app_parent_window(c);
	if (w && (c->state & VISIBLE))	//!!
		app_place_window_controls(w, 1);
	return 1;
}

/*
 *  Remove a control from a window's child control list.
 *  The control placements are only recomputed if the window
 *  is currently visible, otherwise this will happen later,
 *  during app_show_window.
 */
static int app_remove_window_control(Window *w, Control *c)
{
	int i, num, found;
	Control **list;

	if (w->key_focus == c)
		w->key_focus = NULL;
	if (w->mouse_grab == c)
		w->mouse_grab = NULL;
	/* c->win = NULL; */

	num = w->num_children;
	found = 0;
	for (i=0; i < num; i++) {
		if (w->children[i] == c)
			found++;
		if ((found > 0) && (i + found < num))
			w->children[i] = w->children[i+found];
	}
	w->num_children = num - found;
	if (num == found) {
		if (w->children) {
			app_free(w->children);
			w->children = NULL;
		}
	}
	else {
		num -= found;
		list = app_realloc(w->children, num * sizeof(Control *));
		if (list == NULL)
			return 0;
		w->children = list;
	}
	/* if (w->state & VISIBLE) */	//!!
		app_place_window_controls(w, 1);
	return 1;
}

/*
 *  Remove a control from another control's child control list.
 *  The control placements are only recomputed if the window
 *  is currently visible, otherwise this will happen later,
 *  during app_show_window.
 */
static int app_remove_sub_control(Control *parent, Control *c)
{
	int i, num, found;
	Control **list;
	Window *w;

	w = app_parent_window(parent);
	if (w->key_focus == c)
		w->key_focus = NULL;
	if (w->mouse_grab == c)
		w->mouse_grab = NULL;
	/* c->parent = NULL; */

	num = parent->num_children;
	found = 0;
	for (i=0; i < num; i++) {
		if (parent->children[i] == c)
			found++;
		if ((found > 0) && (i + found < num))
			parent->children[i] = parent->children[i+found];
	}
	parent->num_children = num - found;
	if (num == found) {
		if (parent->children) {
			app_free(parent->children);
			parent->children = NULL;
		}
	}
	else {
		num -= found;
		list = app_realloc(parent->children, num * sizeof(Control *));
		if (list == NULL)
			return 0;
		parent->children = list;
	}
	/* if (w->state & VISIBLE) */	//!!
		app_place_window_controls(w, 1);
	return 1;
}

/*
 *  Remove a control from its parent object (window or control).
 */
int app_remove_control(Control *c)
{
	if (c->parent)
		return app_remove_sub_control(c->parent, c);
	else if (c->win)
		return app_remove_window_control(c->win, c);
	return 0;
}

/*
 *  Delete a control and all child controls, recursively.
 */
static void app_del_control_tree(Control *c)
{
	int i;

	if (c->del)
		for (i=0; c->del[i]; i++)
			c->del[i](c);
	for (i = c->num_children - 1; i >= 0; i--)
		app_del_control_tree(c->children[i]);
	app_remove_control(c);
	if (c->visible)
		app_del_region(c->visible);
	if (c->text)
		app_del_string(c->text);
	if (c->state & TIP_MASK)	//!!
		app_set_control_tip(c, 0L, NULL);
	/* Discard arrays of function pointers. */
	app_free(c->resize);
	app_free(c->redraw);
	app_free(c->mouse_down);
	app_free(c->mouse_up);
	app_free(c->mouse_drag);
	app_free(c->mouse_move);
	app_free(c->key_down);
	app_free(c->key_action);
	app_free(c->action);
	app_free(c->update);
	app_free(c->refocus);
	app_free(c->del);
	/* Discard this control. */
	app_free(c);
}

void app_del_control(Control *c)
{
	Window *w;
	App *app;

	w = app_parent_window(c);
	app = w->app;

	if (! app->deleting)
	{
		app_remove_control(c); /* remove this control now */
		app_remember_deleted_control(app, c);
		return;
	}

	app_del_control_tree(c); /* remove this control and children */
#if 0	//!! already called in app_remove_control() ...
	if (w->state & VISIBLE)
		app_place_window_controls(w, 1);
#endif
}

/*
 *  Set the control's natural size.
 */
static int app_set_control_natural(Control *c)
{
	if ((c->text != NULL) || (c->img != NULL)) {
		int w, h;
		Window *win;

		win = app_parent_window(c);

		w = c->natural.x + c->padding * 2;
		h = c->natural.y + c->padding * 2;

		if (c->text) {
			int len, width, height;	//!!
			Font *f;

			f = c->font;
			if (f == NULL)
				f = app_find_default_font(win->app);

			len = (int) strlen(c->text);
			width = app_text_width(f, 9999, c->text, len);
			w += width + 16;
			height = app_text_height(f, 9999, c->text, len);
			h += height + f->height/2;
		}
		if (c->img) {
			w += c->img->width;
			h += c->img->height;
		}

		c->natural.width = w;
		c->natural.height = h;

		/* is natural size not equal to current? */
		return ((w != c->area.width) || (h != c->area.height));
	}
	return 0;
}

/*
 *  Update a control's internal state after a change
 *  and redraw the control.	//!!
 */
static void app_update_control(Control *c)
{
	if (c->update) {
		int i;

		for (i=0; c->update[i]; i++)
			c->update[i](c);
	}
	if ((c->state & AUTOSIZE) != 0 && app_set_control_natural(c) != 0) {
		Control *parent;

		app_size_control(c, c->natural);

		/* find top non-autosizing parent */
		while ((parent = c->parent) != NULL) {
			c = parent;
			if (! (parent->state & AUTOSIZE))
				break;
		}
		if (! parent) {
			app_redraw_window(app_parent_window(c));
			return;
		}
	}
	app_redraw_control(c);
}

/*
 *  Set event handlers.
 */
void app_on_control_resize(Control *c, ControlFunc resize)
{
	c->resize = (ControlFunc *)
		app_add_array_element((void **) c->resize, resize);
}

void app_on_control_redraw(Control *c, DrawFunc redraw)
{
	c->redraw = (DrawFunc *)
		app_add_array_element((void **) c->redraw, redraw);
}

void app_on_control_mouse_down(Control *c, MouseFunc mouse_down)
{
	c->mouse_down = (MouseFunc *)
		app_add_array_element((void **) c->mouse_down, mouse_down);
}

void app_on_control_mouse_up(Control *c, MouseFunc mouse_up)
{
	c->mouse_up = (MouseFunc *)
		app_add_array_element((void **) c->mouse_up, mouse_up);
}

void app_on_control_mouse_drag(Control *c, MouseFunc mouse_drag)
{
	c->mouse_drag = (MouseFunc *)
		app_add_array_element((void **) c->mouse_drag, mouse_drag);
}

void app_on_control_mouse_move(Control *c, MouseFunc mouse_move)
{
	c->mouse_move = (MouseFunc *)
		app_add_array_element((void **) c->mouse_move, mouse_move);
}

void app_on_control_key_down(Control *c, KeyFunc key_down)
{
	c->key_down = (KeyFunc *)
		app_add_array_element((void **) c->key_down, key_down);
}

void app_on_control_key_action(Control *c, KeyFunc key_action)
{
	c->key_action = (KeyFunc *)
		app_add_array_element((void **) c->key_action, key_action);
}

void app_on_control_action(Control *c, ControlFunc action)
{
	c->action = (ControlFunc *)
		app_add_array_element((void **) c->action, action);
}

void app_on_control_update(Control *c, ControlFunc update)
{
	c->update = (ControlFunc *)
		app_add_array_element((void **) c->update, update);
}

void app_on_control_refocus(Control *c, ControlFunc refocus)
{
	c->refocus = (ControlFunc *)
		app_add_array_element((void **) c->refocus, refocus);
}

void app_on_control_deletion(Control *c, ControlFunc del)
{
	c->del = (ControlFunc *)
		app_add_array_element((void **) c->del, del);
}

void app_set_control_layout(Control *c, long flags)
{
	Rect *area;

	if (c->parent)
		area = &c->parent->area;
	else if (c->win)
		area = &c->win->area;
	else return;

	c->state &= ~ LAYOUT_MASK;
	c->state |= flags;
	if (flags & EDGE_RIGHT) {
		c->corner.x = c->area.x + c->area.width - area->width;
	}
	if (flags & EDGE_BOTTOM) {
		c->corner.y = c->area.y + c->area.height - area->height;
	}
	if (c->state & VISIBLE)
		app_place_window_controls(app_parent_window(c), 1);
}

void app_set_control_autosize(Control *c, int autosizable)
{
	if (! autosizable) {
		c->state &= ~ AUTOSIZE;
		return;
	}
	c->state |= AUTOSIZE;

	if (app_set_control_natural(c) && (c->state & VISIBLE))
		app_place_window_controls(app_parent_window(c), 1);
}

void app_set_control_gap(Control *c, int padding, int margin)
{
	c->padding = padding;
	c->margin = margin;

	if (c->state & AUTOSIZE)	//!!
		app_set_control_natural(c);
	if (c->state & VISIBLE)
		app_place_window_controls(app_parent_window(c), 1);
}

/*
 *  Set or get control's background colour.
 *  The window (or possibly just the control) should probably be
 *  redrawn after this function is called.
 */
void app_set_control_background(Control *c, Colour col)
{
	int alpha;

	alpha = c->bg.alpha;
	c->bg = col;

	if (c->state & VISIBLE) {
		if ((alpha & 0x80) != (col.alpha & 0x80)) {
			/* change in transparency of background */
			app_place_window_controls(app_parent_window(c), 0);
		}
		app_redraw_control(c);
	}
}

Colour app_get_control_background(Control *c)
{
	return c->bg;
}

/*
 *  Set or get control's foreground colour.
 *  The control should probably be redrawn after this function is called.
 *  The control's foreground colour is used by many controls when
 *  drawing text, borders, or other features of the control, but
 *  might also be ignored.
 */
void app_set_control_foreground(Control *c, Colour col)
{
	c->fg = col;
	app_redraw_control(c);
}

Colour app_get_control_foreground(Control *c)
{
	return c->fg;
}

/*
 *  Set or get a '\0' terminated text pointer associated with the control.
 */
void app_set_control_text(Control *c, const char *text)
{
	if (c->text)
		app_del_string(c->text);
	c->text = app_copy_string(text);

	app_update_control(c);
}

char * app_get_control_text(Control *c)
{
	return c->text;
}

/*
 *  Set or get a data pointer associated with the control.
 */
void app_set_control_data(Control *c, void *data)
{
	c->data = data;
}

void * app_get_control_data(Control *c)
{
	return c->data;
}

/*
 *  Set or get a long int value associated with the control.
 */
void app_set_control_value(Control *c, long value)
{
	c->value = value;
	app_update_control(c);
}

long app_get_control_value(Control *c)
{
	return c->value;
}

/*
 *  Set or get an image associated with the control.
 */
void app_set_control_image(Control *c, Image *img)
{
	c->img = img;
	app_update_control(c);
}

Image *app_get_control_image(Control *c)
{
	return c->img;
}

/*
 *  Set or get a font associated with the control.
 */
void app_set_control_font(Control *c, Font *f)
{
	c->font = f;
	app_update_control(c);
}

Font *app_get_control_font(Control *c)
{
	return c->font;
}

/*
 *  Set or get a cursor associated with the control.
 */
static int app_cursor_is_within_control(Control *c)
{
	Control *parent;
	Window *w;
	Point p;
	Rect r;

	w = app_parent_window(c);
	if (! w) return 0;	//!!
	p = app_get_cursor_position(w->app);
	r = c->area;
	for (parent = c->parent; parent != NULL; parent = parent->parent)
	{
		r.x += parent->area.x;
		r.y += parent->area.y;
	}
	r.x += w->area.x;
	r.y += w->area.y;
	return app_point_in_rect(p, r);
}

void app_set_control_cursor(Control *c, Cursor *cursor)
{
	c->cursor = cursor;
	if (app_cursor_is_within_control(c))
		app_set_window_temp_cursor(app_parent_window(c), cursor); //!!
}

Cursor *app_get_control_cursor(Control *c)
{
	return c->cursor;
}

/*
 *  Enable/disable the control.
 */

int app_is_enabled(Control *c)
{
	return (c->state & ENABLED) ? 1 : 0;
}

void app_enable(Control *c)
{
	if ((c->state & ENABLED) == 0) {
		c->state |= ENABLED;
		app_redraw_control(c);
	}
}

void app_disable(Control *c)
{
	if (c->state & ENABLED) {
		c->state &= ~ ENABLED;
		app_redraw_control(c);
	}
}

/*
 *  Check/uncheck the control.
 */

int app_is_checked(Control *c)
{
	return (c->state & CHECKED) ? 1 : 0;
}

void app_check(Control *c)
{
	if ((c->state & CHECKED) == 0) {
		c->state |= CHECKED;
		app_redraw_control(c);
	}
}

void app_uncheck(Control *c)
{
	if (c->state & CHECKED) {
		c->state &= ~ CHECKED;
		app_redraw_control(c);
	}
}

/*
 *  Highlight the control.
 */

int app_is_highlighted(Control *c)
{
	return (c->state & HIGHLIGHTED) ? 1 : 0;
}

void app_highlight(Control *c)
{
	if ((c->state & HIGHLIGHTED) == 0) {
		c->state |= HIGHLIGHTED;
		app_redraw_control(c);
	}
}

void app_unhighlight(Control *c)
{
	if (c->state & HIGHLIGHTED) {
		c->state &= ~ HIGHLIGHTED;
		app_redraw_control(c);
	}
}

/*
 *  Arm/disarm the control.
 */

int app_is_armed(Control *c)
{
	return (c->state & ARMED) ? 1 : 0;
}

void app_arm(Control *c)
{
	c->state |= ARMED;
}

void app_disarm(Control *c)
{
	c->state &= ~ ARMED;
}

/*
 *  Change focus of the control.
 */

int app_has_focus(Control *c)
{
	Window *w;

	w = app_parent_window(c);
	if (w) {
		if (w->key_focus == c)
			return 1;
	}
	return 0;
}

void app_set_focus(Control *c)
{
	int i;
	Window *w;
	Control *prev;

	w = app_parent_window(c);
	if (w) {
		prev = w->key_focus;
		if (prev != c) {
			w->key_focus = c;
			if (prev) {
				if (prev->refocus)
					for (i=0; prev->refocus[i]; i++)
						prev->refocus[i](prev);
				else
					app_redraw_control(prev);
			}
			if (c) {
				if (c->refocus)
					for (i=0; c->refocus[i]; i++)
						c->refocus[i](c);
				else
					app_redraw_control(c);
			}
		}
	}
}

/*
 *  Simulate user actions on a control.
 */

void app_flash_control(Control *c)
{
	App *app;

	app = app_parent_window(c)->app;

	app_highlight(c);
	app_delay(app, 150);
	app_unhighlight(c);
}

void app_activate_control(Control *c)
{
	int i;

	if (app_is_enabled(c))
		if (c->action)
			for (i=0; c->action[i]; i++)
				c->action[i](c);
}

/*
 *  Redraw a portion of a control.
 *  The given rectangle is in control-relative co-ordinates.
 */
void app_redraw_control_rect(Control *c, Rect r)
{
	int i;
	Graphics *g;

	g = app_get_control_graphics(c);
	app_set_clip_rect(g, r);
	if (c->redraw)
		for (i=0; c->redraw[i]; i++)
			c->redraw[i](c, g);
	app_del_graphics(g);
}

/*
 *  Create a control and place it in the given parent window.
 */
Control *app_new_control(Window *parent, Rect r)
{
	Control *c;

	c = app_zero_alloc(sizeof(Control));
	if (c == NULL)
		return NULL;
	c->area = r;		/* in parent's co-ordinate system */
	c->state = VISIBLE | ENABLED;
	if ((r.width == -1) && (r.height == -1))
		c->state |= AUTOSIZE;
	c->bg.red = c->bg.green = c->bg.blue = 255;	/* white */
	app_attach_to_window(parent, c);

	return c;
}

/*
 *  Create a control and place it in the given parent control.
 */
Control *app_add_control(Control *parent, Rect r)
{
	Control *c;

	c = app_zero_alloc(sizeof(Control));
	if (c == NULL)
		return NULL;
	c->area = r;		/* in parent's co-ordinate system */
	c->state = VISIBLE | ENABLED;
	if ((r.width == -1) && (r.height == -1))
		c->state |= AUTOSIZE;
	c->bg.red = c->bg.green = c->bg.blue = 255;	/* white */
	app_attach_to_control(parent, c);

	return c;
}

