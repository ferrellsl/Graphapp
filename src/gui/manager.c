/*
 *  Resizing managers.	//!!
 *
 *  Platform: Neutral
 *
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"

void app_manager_hbox(Control *parent)
{
	int num, nfills, is_prev_fill;
	int x, wgap, pad, parent_height;
	long edging;
	Control *c;

	x = wgap = pad = parent->padding;
	parent_height = parent->area.height - 2 * pad;
	nfills = 0;

	/* calculate width of fill gap */
	num = parent->num_children;
	is_prev_fill = 0;
	while (num--) {
		c = parent->children[num];
		edging = c->state & (EDGE_LEFT | EDGE_RIGHT);
		if (edging || is_prev_fill) {
			if ((edging & EDGE_RIGHT) || is_prev_fill)
				++nfills;
			is_prev_fill = (edging & EDGE_LEFT) != 0L;
		}
		wgap += c->margin;
		if (! (c->state & DOCK))
			wgap += c->area.width;
	}
	nfills += is_prev_fill ? 1 : 0;
	if (nfills != 0) {
		wgap = parent->area.width - pad - wgap;
		if (wgap > 1)
			wgap /= nfills;
		else
			nfills = 0;
	}

	num = parent->num_children;
	is_prev_fill = 0;
	while (num--) {
		int w, h;

		c = parent->children[num];
		w = c->area.width;
		h = c->area.height;

		if (nfills != 0) {
			edging = c->state & (EDGE_LEFT | EDGE_RIGHT);
			if (edging || is_prev_fill) {
				if ((edging & EDGE_RIGHT) || is_prev_fill) {
					--nfills;
					if (c->state & DOCK)
						w = wgap;
					else
						x += wgap;
				}
				is_prev_fill = (edging & EDGE_LEFT) != 0L;
			}
		}

		c->area.x = x;
		c->area.y = pad;
		x += w + c->margin;

		edging = c->state & (EDGE_TOP | EDGE_BOTTOM);
		if (edging == (EDGE_TOP | EDGE_BOTTOM))
			h = parent_height;
		else if (! (edging & EDGE_TOP)) {
			int hgap = parent_height - c->area.height;
			if (hgap > 0)
				c->area.y += edging ? hgap : hgap / 2;
		}

		if ((w != c->area.width) || (h != c->area.height))
			app_size_control(c, rect(0,0,w,h));
	}
}

void app_manager_vbox(Control *parent)
{
	int num, nfills, is_prev_fill;
	int y, hgap, pad, parent_width;
	long edging;
	Control *c;

	y = hgap = pad = parent->padding;
	parent_width = parent->area.width - 2 * pad;
	nfills = 0;

	/* calculate width of fill gap */
	num = parent->num_children;
	is_prev_fill = 0;
	while (num--) {
		c = parent->children[num];
		edging = c->state & (EDGE_TOP | EDGE_BOTTOM);
		if (edging || is_prev_fill) {
			if ((edging & EDGE_BOTTOM) || is_prev_fill)
				++nfills;
			is_prev_fill = (edging & EDGE_TOP) != 0L;
		}
		hgap += c->margin;
		if (! (c->state & DOCK))
			hgap += c->area.height;
	}
	nfills += is_prev_fill ? 1 : 0;
	if (nfills != 0) {
		hgap = parent->area.height - pad - hgap;
		if (hgap > 1)
			hgap /= nfills;
		else
			nfills = 0;
	}

	num = parent->num_children;
	is_prev_fill = 0;
	while (num--) {
		int w, h;

		c = parent->children[num];
		w = c->area.width;
		h = c->area.height;

		if (nfills != 0) {
			edging = c->state & (EDGE_TOP | EDGE_BOTTOM);
			if (edging || is_prev_fill) {
				if ((edging & EDGE_BOTTOM) || is_prev_fill) {
					--nfills;
					if (c->state & DOCK)
						h = hgap;
					else
						y += hgap;
				}
				is_prev_fill = (edging & EDGE_TOP) != 0L;
			}
		}

		c->area.x = pad;
		c->area.y = y;
		y += h + c->margin;

		edging = c->state & (EDGE_LEFT | EDGE_RIGHT);
		if (edging == (EDGE_LEFT | EDGE_RIGHT))
			w = parent_width;
		else if (! (edging & EDGE_LEFT)) {
			int wgap = parent_width - c->area.width;
			if (wgap > 0)
				c->area.x += edging ? wgap : wgap / 2;
		}

		if ((w != c->area.width) || (h != c->area.height))
			app_size_control(c, rect(0,0,w,h));
	}
}

