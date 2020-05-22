/*
 *  Text editing box with a scrollbar.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Can now use any font.
 *  Version: 3.03  2001/10/20  Added scroll bar and improved appearance.
 *  Version: 3.04  2001/10/26  Can now be resized, improved cursor motion.
 *  Version: 3.05  2001/10/28  Added bitmap backing store, fast selection.
 *  Version: 3.06  2001/10/30  Fixed resizing.
 *  Version: 3.07  2001/11/03  Added deletion and update handlers.
 *  Version: 3.09  2001/11/13  Improved disabled behaviour.
 *  Version: 3.10  2001/12/01  Added add_text_box constructor.
 *  Version: 3.11  2001/12/12  Added support for menu shortcuts.
 *  Version: 3.12  2001/12/13  Added field restrictions.
 *  Version: 3.18  2002/01/15  Added select_text, copy_text etc functions.
 *  Version: 3.19  2002/02/15  Fixed ignoring of newline key.
 *  Version: 3.26  2002/07/31  If disabled, disallows editing but looks same.
 *  Version: 3.33  2002/11/22  Fixed maxwidth+selection overtype bug.
 *  Version: 3.37  2002/12/31  Handles CONTROL/SHIFT bits in key events.
 *  Version: 3.39  2003/03/03  Now using IS_UTF8_EXTRA_BYTE macro.
 *  Version: 3.40  2003/03/07  Now uses centralised colour settings.
 *  Version: 3.41  2003/03/15  Added focus-change (refocus) handler.
 *  Version: 3.45  2003/05/12  Supports X-Windows mouse-based copy/paste.
 *  Version: 3.47  2003/05/27  Reduced some flickering.
 *  Version: 3.48  2003/06/07  Fixed some bugs, speedier drawing/editing.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced some size_t conversion warnings.
 *  Version: 3.57  2005/08/16  CTRL+INS=copy, SHIFT+INS=paste, SHIFT+DEL=cut.
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
#include "textedit.h"

enum {
	SCROLL_SIZE = 16,
	BOX_BORDER  = SCROLL_SIZE
};

/*
 *  Find the font to use in this text box.
 */
static Font * app_get_text_box_font(Control *c)
{
	TextBox *t;

	t = c->extra;
	if (t->parent->font)
		return t->parent->font;
	return app_find_default_font(app_parent_window(t->parent)->app);
}

/*
 *  Recompute the starting byte locations for the entire text.
 */
static void app_recompute_text_lines(TextBox *t)
{
	int i, nb, total;
	int num = 32; /* an initial guess */
	int length = 0;
	long offset;
	char *text;
	int width;
	Font *f;

	if (t->lines)
		app_free(t->lines);
	t->num_lines = 0;

	text = t->parent->text;
	width = t->box->area.width - 8;

	/* start with some appropriate number of lines */
	t->lines = app_alloc(num * sizeof(t->lines[0]));
	t->lines[0] = 0;
	t->lines[1] = 0;
	t->lines[2] = 0;
	if (text)
		nb = total = t->text_length;
	else
		nb = total = num = 0;

	if (t->caret > nb)
		t->caret = nb;

	f = app_get_text_box_font(t->parent);

	/* compute other line-start positions */
	for (offset=i=0; i < num; i++)
	{
		if (i == num-2) {	/* filled up (leave two spots) */
			num *= 2;	/* double size of array */
			t->lines = app_realloc(t->lines,
				num * sizeof(t->lines[0]));
		}
		t->lines[i] = offset;
		if (nb == 0) {
			t->lines[i+1] = offset;
			break;
		}
		length = app_text_line_length(f, width, text, nb);
		offset += length;
		text += length;
		nb -= length;
	}

	/* a newline at the end of the text means an extra line */
	text = t->parent->text;
	if ((total > 0) && (text[total-1] == '\n')) {
		i++;
		t->lines[i+1] = t->lines[i];
	}
	else if (total == 0) {
		i++;
		t->lines[i] = t->lines[i+1] = 0;
	}

	/* resize to smallest array size */
	t->num_lines = i;
	t->lines = app_realloc(t->lines, (i+2) * sizeof(t->lines[0]));

	/* fix previous selection */
	if (total > t->prev_start)
		t->prev_start = total;
	if (total > t->prev_end)
		t->prev_end = total;
}

/*
 *  Recompute the starting byte locations for the text,
 *  starting from the specified line.
 */
static int app_recompute_text_lines_and_resync(TextBox *t,
		int old_first_line, long *old_lines, int old_num_lines,
		int inserted)
{
	int i, nb, total;
	int num = 32; /* an initial guess */
	int length = 0;
	long offset;
	char *text;
	int width;
	Font *f;
	int resync, first_resync;

	if (t->lines)
		app_free(t->lines);
	t->num_lines = 0;

	text = t->parent->text;
	width = t->box->area.width - 8;

	/* start with some appropriate number of lines */
	t->lines = app_alloc(num * sizeof(t->lines[0]));
	t->lines[0] = 0;
	t->lines[1] = 0;
	t->lines[2] = 0;
	if (text)
		nb = total = t->text_length;
	else
		nb = total = num = 0;

	if (t->caret > nb)
		t->caret = nb;

	f = app_get_text_box_font(t->parent);

	/* compute other line-start positions */
	resync = 0;
	first_resync = -1;
	for (offset=i=0; i < num; i++)
	{
		if (i == num-2) {	/* filled up (leave two spots) */
			num *= 2;	/* double size of array */
			t->lines = app_realloc(t->lines,
				num * sizeof(t->lines[0]));
		}
		t->lines[i] = offset;
		if (nb == 0) {
			/* No bytes left, stop here. */
			t->lines[i+1] = offset;
			break;
		}
		if (i < old_first_line) {
			/* Before first changed line, so copy from old. */
			length = old_lines[i+1] - old_lines[i];
		}
		else if ((resync < old_num_lines) &&
			 (t->lines[i] == old_lines[resync] + inserted))
		{
			/* Found a line which just shifted up or down. */
			length = old_lines[resync + 1] - old_lines[resync];
			first_resync = resync;
			resync++;
		}
		else {
			/* Changed line, recalculate line length. */
			length = app_text_line_length(f, width, text, nb);
			while ((resync < old_num_lines) &&
			  (t->lines[i] + length > old_lines[resync] + inserted))
				resync++;
		}
		offset += length;
		text += length;
		nb -= length;
	}

	/* a newline at the end of the text means an extra line */
	text = t->parent->text;
	if ((total > 0) && (text[total-1] == '\n')) {
		i++;
		t->lines[i+1] = t->lines[i];
	}
	else if (total == 0) {
		i++;
		t->lines[i] = t->lines[i+1] = 0;
	}

	/* resize to smallest array size */
	t->num_lines = i;
	t->lines = app_realloc(t->lines, (i+2) * sizeof(t->lines[0]));

	/* fix previous selection */
	if (total > t->prev_start)
		t->prev_start = total;
	if (total > t->prev_end)
		t->prev_end = total;

	if (first_resync < 0)
		first_resync = t->num_lines;
	return first_resync;
}

/*
 *  Scroll one page up or down.
 */
static void app_text_box_scroll_page(TextBox *t, int dir)
{
	int start = t->start;
	int page, h, max;
	Rect r;

	r = app_get_control_area(t->box);
	r = app_inset_rect(r, 4);
	h = app_font_height(app_get_text_box_font(t->parent));
	page = (r.height + h - 1) / h;

	max = t->num_lines - page + 1;
	if (max < 0)
		max = 0;

	if (dir < 0)
		start -= page;	/* up */
	else if (dir > 0)
		start += page;	/* down */

	if (start > max)
		start = max;
	if (start < 0)
		start = 0;

	t->start = start;
	app_change_scroll_bar(t->vert, start, max, page);
	app_draw_control(t->box);
}

static void app_text_box_update(Control *c)
{
	int length;
	TextBox *t;

	t = c->extra;
	length = c->text ? (int) strlen(c->text) : 0;

	if (t->caret < 0)
		t->caret = 0;
	if (t->caret > length)
		t->caret = length;
	if (t->caret + t->selected < 0)
		t->selected = 0 - t->caret;
	if (t->caret + t->selected > length)
		t->selected = length - t->caret;

	t->text_length = length;
	t->caret = length;
	t->caret = 0;
	t->selected = 0;
	t->start = 0;

	app_recompute_text_lines(t);
	app_text_box_scroll_page(t, 0);
	/* app_redraw_control(c); */	//!!
}

static Region * app_text_box_selection_region(Control *c,
			int sel_start, int sel_end)
{
	int i, y, x1, x2, h, h2, temp;
	Font *f;
	Rect r, textbox;
	Region *reg;
	TextBox *t = c->extra;

	/* Find text-drawing area. */
	r = app_get_control_area(t->box);
	textbox = app_inset_rect(r, 4);

	/* Find text-drawing font. */
	f = app_get_text_box_font(t->parent);
	h = app_font_height(f);
	if (h > textbox.height)
		h = textbox.height;

	/* Ensure sel_start <= sel_end */
	if (sel_end < sel_start) {
		temp = sel_end;
		sel_end = sel_start;
		sel_start = temp;
	}

	/* Generate selection region. */
	reg = app_new_region();
	for (i=t->start, y=0;
		(i < t->num_lines) && (y < textbox.height);
		y+=h, i++)
	{
		if ((sel_start == t->lines[i+1]) &&
			(sel_end == sel_start) &&
			(i < t->num_lines-1))
		{
			/* caret at end of this line */
			/* belongs at start of the next line */
			continue;
		}
		else if (sel_start > t->lines[i+1]) {
			/* start of selection on later line */
			continue;
		}
		else if (sel_start < t->lines[i]) {
			/* start of selection on prior line */
			/* so select from start of this line */
			x1 = textbox.x;
		}
		else {
			/* start of selection on this line */
			/* so select only from that point */
			x1 = textbox.x;
			x1 += app_text_width(f, textbox.width,
				c->text + t->lines[i],
				sel_start - t->lines[i]);
		}

		if (sel_end < t->lines[i]) {
			/* sel_start <= sel_end < t->lines[i] */
			/* so the selection is on a prior line */
			continue;
		}
		else if (sel_end == sel_start) {
			/* end of selection on same line */
			/* so use a thin insertion bar */
			x2 = x1 + 1;
		}
		else if (sel_end >= t->lines[i+1]) {
			/* end of selection on later line */
			/* so select up to end of this line */
			x2 = textbox.x;
			x2 += app_text_width(f, textbox.width,
				c->text + t->lines[i],
				t->lines[i+1] - t->lines[i]);
		}
		else {
			/* end of selection on this line */
			/* so select only up to that point */
			x2 = textbox.x;
			x2 += app_text_width(f, textbox.width,
				c->text + t->lines[i],
				sel_end - t->lines[i]);
		}

		h2 = h;
		if (y + h2 > textbox.height)
			h2 = textbox.height - y;
		app_union_region_with_rect(reg,
			rect(x1,textbox.y+y,x2-x1,h2), reg);
	}
	return reg;
}

static void app_text_box_draw_selection(Control *c, Graphics *g,
		int sel_start, int sel_end)
{
	Region *reg;
	TextBox *t = c->extra;

	app_set_xor_mode(g, app_get_control_background(t->box));

	reg = app_text_box_selection_region(c, sel_start, sel_end);
	app_fill_region(g, reg);
	app_del_region(reg);

	app_set_paint_mode(g);
}

static void app_text_box_redraw_selection(Control *c, Graphics *g,
		int old_start, int old_end, int sel_start, int sel_end)
{
	Region *old_reg, *new_reg;
	TextBox *t = c->extra;

	app_set_xor_mode(g, app_get_control_background(t->box));

	old_reg = app_text_box_selection_region(c, old_start, old_end);
	new_reg = app_text_box_selection_region(c, sel_start, sel_end);
	app_xor_region(new_reg, old_reg, new_reg);
	app_del_region(old_reg);
	app_fill_region(g, new_reg);
	app_del_region(new_reg);

	app_set_paint_mode(g);
}

static void app_text_box_redraw(Control *c, Graphics *g, int do_clip)
{
	Rect r;
	Rect textbox;
	int h, i, y, lb;
	int sel_start, sel_end;
	Font *f;
	TextBox *t;

	t = c->extra;
	c = t->parent;

	/* find text-drawing area */
	r = app_get_control_area(t->box);
	textbox = app_inset_rect(r, 4);


	/* Blank the text area of the text_box first. */
	app_set_colour(g, app_get_control_background(t->box));
	if (app_has_focus(t->parent))
		app_fill_rect(g, app_inset_rect(r, 3));
	else
		app_fill_rect(g, app_inset_rect(r, 2));

	/* Set up for drawing text. */
	f = app_get_text_box_font(t->parent);
	app_set_font(g, f);
	app_set_colour(g, app_get_control_foreground(c));

	/* Find line and caret height */
	h = app_font_height(f);
	if (h > textbox.height)
		h = textbox.height;

	/* Draw the visible text a line at a time. */
	if (do_clip)
		app_set_clip_rect(g, textbox);

	for (i=t->start, y=0;
		(i < t->num_lines) && (y < textbox.height); y+=h, i++)
	{
		lb = t->lines[i+1] - t->lines[i];

		app_draw_text(g,
			rect(textbox.x, textbox.y+y, textbox.width, h),
			ALIGN_LEFT+VALIGN_TOP,
			c->text + t->lines[i], lb);
	}
	if (do_clip)
		app_set_clip_rect(g, r);

	/* Draw the bevelled border. */
	app_draw_shadow_rect(g, r, LOWER_RIGHT, UPPER_LEFT);
	r = app_inset_rect(r, 1);
	app_draw_shadow_rect(g, r, ENABLED_ITEM, BACKGROUND);

	/* Draw caret if it has focus. */
	if (app_has_focus(c)) {
		/* Draw the caret (insertion bar) or selection box. */
		if (app_is_enabled(c))
			app_set_colour(g, app_get_control_foreground(c));
		else
			app_set_colour(g, DISABLED_ITEM);

		/* Find start and end of selection, so start <= end. */
		if (t->selected == 0)
			sel_start = sel_end = t->caret;
		else if (t->selected > 0) {
			sel_start = t->caret;
			sel_end = t->caret + t->selected;
		}
		else {
			sel_start = t->caret + t->selected;
			sel_end = t->caret;
		}

		app_text_box_draw_selection(c, g, sel_start, sel_end);

		t->prev_start = sel_start;
		t->prev_end = sel_end;
	}
	return;

#if 0	//!!
	/* Draw text box shadow border, and caret if it has focus. */
	if (app_has_focus(t->parent)) {
		app_set_colour(g, FOCUS_BORDER);
		r.width += 1;	/* avoid doubling the scrollbar border */
		app_draw_rect(g, r);

		r = app_inset_rect(r, 1);

		/* Draw the caret (insertion bar) or selection box. */
		app_set_colour(g, app_get_control_foreground(c));

		/* Find start and end of selection, so start <= end. */
		if (t->selected == 0)
			sel_start = sel_end = t->caret;
		else if (t->selected > 0) {
			sel_start = t->caret;
			sel_end = t->caret + t->selected;
		}
		else {
			sel_start = t->caret + t->selected;
			sel_end = t->caret;
		}

		app_text_box_draw_selection(c, g, sel_start, sel_end);

		t->prev_start = sel_start;
		t->prev_end = sel_end;
	}
	app_set_line_width(g, 2);
	app_draw_shadow_rect(g, r, LOWER_RIGHT, UPPER_LEFT);
	app_set_colour(g, app_get_control_background(t->box));
	app_set_line_width(g, 1);
	app_draw_rect(g, app_inset_rect(r, 2));
#endif
}

static void app_text_box_draw(Control *c, Graphics *g)
{
	app_text_box_redraw(c, g, 1);
}

static void app_text_box_refocus(Control *c)
{
	TextBox *t;
	Rect r;
	Region *clip, *inside;
	Graphics *g;

	t = c->extra;
	r = app_get_control_area(t->box);
	clip = app_new_rect_region(r);

	inside = app_new_rect_region(app_inset_rect(r,4));
	app_subtract_region(clip, inside, clip);
	app_del_region(inside);

	inside = app_text_box_selection_region(c,
			t->prev_start, t->prev_end);
	app_union_region(clip, inside, clip);
	app_del_region(inside);

	inside = app_text_box_selection_region(c,
			t->caret, t->caret + t->selected);
	app_union_region(clip, inside, clip);
	app_del_region(inside);

	g = app_get_control_graphics(t->box);
	app_set_clip_region(g, clip);
	app_del_region(clip);
	app_text_box_redraw(t->box, g, 0); /* do our own clipping */
	app_del_graphics(g);
}

/*
 *  Scroll to make the caret visible.
 */
static void app_text_box_scroll_to_caret(TextBox *t, int redraw)
{
	int caret = t->caret;
	int start = t->start;
	int page;
	int h, num, max;
	int sel_start, sel_end;
	Rect r;
	Graphics *g;
	Font *f;

	f = app_get_text_box_font(t->parent);
	r = app_get_control_area(t->box);
	r = app_inset_rect(r, 4);
	h = app_font_height(f);
	num = page = (r.height + h - 1) / h;
	if (start + num > t->num_lines)
		num = t->num_lines - start;

	if (t->lines[start] > caret)
		while (t->lines[start] > caret)
			start--;
	else if (t->lines[start + num] < caret)
		while (t->lines[start + num] < caret)
			start++;

	max = t->num_lines - page + 1;
	if (max < 0)
		max = 0;
	if (start > max)
		start = max;
	if (start < 0)
		start = 0;

	if (! redraw) {
		/* Find start and end of selection, so start <= end. */
		if (t->selected == 0)
			sel_start = sel_end = t->caret;
		else if (t->selected > 0) {
			sel_start = t->caret;
			sel_end = t->caret + t->selected;
		}
		else {
			sel_start = t->caret + t->selected;
			sel_end = t->caret;
		}

		/* Remove old selection and draw the new selection. */
		g = app_get_control_graphics(t->box);
		app_set_font(g, f);

		app_text_box_redraw_selection(t->parent, g,
				t->prev_start, t->prev_end,
				sel_start, sel_end);
		app_del_graphics(g);

		t->prev_start = sel_start;
		t->prev_end = sel_end;
	}

	app_change_scroll_bar(t->vert, start, max, page);
	app_activate_control(t->vert);

	if (redraw) {
		t->start = start;
		app_draw_control(t->box);
	}
}

/*
 *  Where would the caret be if the mouse was clicked here?
 */
static int app_text_box_caret_from_click(TextBox *t, Point p)
{
	int i, h, w1, w2, line;
	Font *f;
	char *text;
	Rect textbox;

	f = app_get_text_box_font(t->parent);
	if (f == NULL)
		return 0;
	h = app_font_height(f);

	textbox = app_get_control_area(t->box);
	textbox = app_inset_rect(textbox, 4);

	/* First, find which line it was. */
	if (p.y < textbox.y) {
		/* Clicked above the text box, use prior line. */
		if (t->start > 0)
			line = t->start - 1;
		else
			return 0;	/* start of text */
	}
	else if (p.y >= textbox.y + textbox.height) {
		/* Clicked below text box. */
		line = t->start + textbox.height / h + 1;
		if (line >= t->num_lines)
			return t->lines[t->num_lines];	/* end of text */
	}
	else {
		/* Clicked inside text box. */
		line = t->start + (p.y-textbox.y) / h;
		if (line >= t->num_lines)
			return t->lines[t->num_lines];	/* end of text */
	}

	/* Now find the character offset within that line. */

	text = t->parent->text + t->lines[line];
	p.x -= textbox.x;
	for (i=w1=0; i < t->lines[line+1] - t->lines[line]; i++)
	{
		if (text[i] == '\n')
			break;
		w2 = app_text_width(f, textbox.width, text, i);
		if (w2 > p.x) {
			if (w2 - p.x > p.x - w1) {
				while ((i>0) && IS_UTF8_EXTRA_BYTE(text[i-1]))
					i--; /* exclude UTF-8 bytes */
				if (i>0)
					i--; /* UTF-8 start byte */
			}
			break;
		}
		w1 = w2;
		if (text[i] & 0x80) /* UTF-8 start byte */
			while (IS_UTF8_EXTRA_BYTE(text[i+1]))
				i++; /* include UTF-8 continuation bytes */
	}
/*
	if ((i > 0) && (i == t->lines[line+1]))
		if ((text[i-1] == '\n') ||
			(text[i-1] == '\t') ||
			(text[i-1] == ' '))
			i--;
*/
	return t->lines[line] + i;
}

/*
 *  Recompute all text lines and update (redraw) only those lines
 *  which have changed. This may involve scrolling some lines up or down
 *  to handle text insertion or deletion, and redrawing certain
 *  rectangular regions. The selection is emptied as a result.
 */
static void app_recompute_and_update(TextBox *t, int inserted, int new_caret)
{
	int i, h, final_line;
	int old_num_lines;
	long *old_lines;
	int first, last;
	int old_first_line, first_line;
	int x, y;
	Font *f;
	Rect textbox;
	Region *update, *extra;

	/* Determine some settings. */
	f = app_get_text_box_font(t->parent);
	if (f == NULL)
		return;
	h = app_font_height(f);

	textbox = app_get_control_area(t->box);
	textbox = app_inset_rect(textbox, 4);

	/* Determine updated text slice. */
	first = t->caret;
	last = t->caret;

	/*
	 * Check what selection was removed, if any.
	 * All lines where the selection was valid
	 * must be redrawn in full or in part
	 */
	if (t->selected < 0)
		first += t->selected;
	else if (t->selected > 0)
		last += t->selected;

	/* Add inserted to current caret to expand update region. */
	if (inserted < 0) {
		if (first > t->caret + inserted)
			first = t->caret + inserted;
	}
	else if (inserted > 0) {
		if (last < t->caret + inserted)
			last = t->caret + inserted;
	}

	/* Save old lines array. */
	old_num_lines = t->num_lines;
	old_lines = t->lines;
	t->num_lines = 0;
	t->lines = NULL;

	/* Find line on which first change occurred. */
	old_first_line = 0;
	for (i=0; i < old_num_lines; i++) {
		if (first < old_lines[i])
			break;
		old_first_line = i;
	}

	/* Update caret and selection. */
	t->caret = new_caret;
	t->selected = 0;

	/* Recompute where each line begins. */
	app_recompute_text_lines_and_resync(t, old_first_line,
			old_lines, old_num_lines, inserted);

	/* Find line on which first change will be seen. */
	first_line = 0;
	for (i=0; i < t->num_lines; i++) {
		if (first < t->lines[i])
			break;
		first_line = i;
	}

	/* Redraw everything if it's too difficult to work out. */
	final_line = t->start + textbox.height / h;
	if ((first_line < t->start) || (first_line > final_line))
	{
		app_free(old_lines);
		app_text_box_scroll_to_caret(t, 1);
		return;
	}

	/* Accumulate the lines to update in a region. */
	update = app_new_region();

	/* Scroll text if possible. */
	#ifdef DO_NOT_COMPILE
	for (i=t->start; i < t->num_lines; i++)
	{
		int j;
		int found = 0;

		if (t->lines[i] < last)
			continue;

		for (j=i; j < old_num_lines; j++) {
			if (t->lines[i] == old_lines[j] + inserted) {
				found = 1;
				break;
			}
		}
		if (! found)
			continue;

		/* Else, found a resync spot. */
		if (i < j)
		{
			/* scrolled up */
		}
		else if (i > j)
		{
			/* scrolled down */
		}
		break;
	}
	#endif

	/* Redraw whatever changed from old to new lines array. */
	if (first_line > old_first_line)
	{
		/* Perhaps a word wrapped onto a later line; if so
		 * redraw the end of old_first_line and every
		 * visible line thereafter.
		 */
		i = old_first_line;

		if ((i >= t->start) && (i <= final_line) && (i < t->num_lines))
		{
			x = app_font_width(f,
					t->parent->text + t->lines[i],
					t->lines[i+1] - t->lines[i]);
			y = (i - t->start) * h;
			extra = app_new_rect_region(rect(textbox.x + x,
					textbox.y + y, textbox.width - x, h));
			app_union_region(update, extra, update);
			app_del_region(extra);
		}

		i = old_first_line + 1;

		if ((i >= t->start) && (i <= final_line))
		{
			y = (i - t->start) * h;
			extra = app_new_rect_region(rect(textbox.x,
					textbox.y + y, textbox.width,
					textbox.height - y));
			app_union_region(update, extra, update);
			app_del_region(extra);
		}
	}
	else if (first_line < old_first_line)
	{
		/* Perhaps a word wrapped onto an earlier line
		 * due to hitting the backspace or delete key; if so
		 * redraw the new first_line and every line thereafter.
		 */
		i = first_line;

		if ((i >= t->start) && (i <= final_line))
		{
			y = (i - t->start) * h;
			extra = app_new_rect_region(rect(textbox.x,
					textbox.y + y, textbox.width,
					textbox.height - y));
			app_union_region(update, extra, update);
			app_del_region(extra);
		}
	}
	else {
		/* First changed line is the same in both old and new
		 * lines lists. Perhaps we just typed one character.
		 * So, redraw from the first change onwards.
		 */
		i = first_line;

		if ((i >= t->start) && (i <= final_line) && (i < t->num_lines))
		{
			x = app_font_width(f,
					t->parent->text + t->lines[i],
					first - t->lines[i]);
			y = (i - t->start) * h;
			extra = app_new_rect_region(rect(textbox.x + x,
					textbox.y + y, textbox.width - x, h));
			app_union_region(update, extra, update);
			app_del_region(extra);
		}

		/* If the last change was also on that first_line, stop. */

		i = first_line + 1;

		if (i > final_line) {
			/* After last visible line; redraw nothing extra. */
		}
		else if ((i < t->num_lines) && (i < old_num_lines)
			&& (t->lines[i] == old_lines[i] + inserted))
		{
			/* Changes on one line only; redraw nothing extra. */
		}
		else if (i >= t->start)
		{
			y = (i - t->start) * h;
			extra = app_new_rect_region(rect(textbox.x,
					textbox.y + y, textbox.width,
					textbox.height - y));
			app_union_region(update, extra, update);
			app_del_region(extra);
		}
	}
	/* Redraw changed lines. */
	#ifdef DO_NOT_COMPILE
	for (i=t->start; i < t->num_lines; i++)
	{
		/* If first changed text is after this line, keep looking. */
		if (i < first_line)
			continue;

		/* If found a resync spot, stop here. */
		if (t->lines[i] >= last) {
			if (i < old_num_lines)
				if (t->lines[i] == old_lines[i] + inserted)
					break;
		}

		/* If first bit of changed text is on this line, */
		/* just redraw the line from the change onwards. */
		if (i == first_line)
		{
			x = app_font_width(f,
					t->parent->text + t->lines[i],
					first - t->lines[i]);
			extra = app_new_rect_region(rect(textbox.x+x,
					textbox.y+(i-t->start)*h,
					textbox.width-x, h));
			app_union_region(update, extra, update);
			app_del_region(extra);
		}
		/* Else if after the first change, redraw this line */
		else if (i > first_line) {
			extra = app_new_rect_region(rect(textbox.x,
					textbox.y+(i-t->start)*h,
					textbox.width, h));
			app_union_region(update, extra, update);
			app_del_region(extra);
		}
	}
	#endif

	if (update) {
		Graphics *g = app_get_control_graphics(t->box);
		app_set_clip_region(g, update);
		app_set_colour(g, app_get_control_background(t->box));
		app_fill_rect(g, app_get_control_area(t->box));
		app_set_colour(g, BLACK);
		app_text_box_redraw(t->box, g, 0); /* do our own clipping */
		app_del_graphics(g);
		app_del_region(update);
	}
	app_free(old_lines);
}

/*
 *  Where is the caret located now? Return the central point of the caret.
 */
static Point app_text_box_caret_point(TextBox *t)
{
	int i, h;
	Font *f;
	char *text;
	Rect textbox;

	textbox = app_inset_rect(app_get_control_area(t->box), 4);
	f = app_get_text_box_font(t->parent);
	if (f == NULL)
		return pt(textbox.x, textbox.y);
	h = app_font_height(f);
	text = t->parent->text;

	for (i=0; i < t->num_lines; i++) {
		if ((i == t->num_lines - 1) ||
			((t->lines[i] <= t->caret) && (t->caret < t->lines[i+1])))
			return pt(textbox.x +
				app_text_width(f, textbox.width,
					text + t->lines[i],
					t->caret - t->lines[i]),
				textbox.y + h * (i - t->start) + h/2);
	}
	return pt(textbox.x + textbox.width,
		textbox.y + h * (t->num_lines-1) + h/2);
}

static void app_text_box_mouse_down(Control *c, int buttons, Point p)
{
	int i;
	TextBox *t = c->extra;
	int caret, sel;

	app_arm(t->parent);

	caret = app_text_box_caret_from_click(t, p);
	sel = 0;

	if (app_parent_window(c)->app->use_X_copy_paste) {
		if (buttons == MIDDLE_BUTTON) { /* paste text */
			c = t->parent;
			app_set_focus(c);
			t->prev_start = caret;
			t->prev_end = t->prev_start;
			t->caret = caret;
			t->selected = 0;
			if (c->key_action)
				for (i=0; c->key_action[i]; i++)
					c->key_action[i](c, CONTROL + 'V');
			return;
		}
	}

	if (! app_has_focus(t->parent)) {
		t->prev_start = caret;
		t->prev_end = t->prev_start;
		t->caret = caret;
		t->selected = sel;
		app_set_focus(t->parent);
	}

	if ((caret != t->caret) || (sel != t->selected)) {
		t->caret = caret;
		t->selected = sel;
		app_text_box_scroll_to_caret(t, 0);
	}
}

static void app_text_box_mouse_drag(Control *c, int buttons, Point p)
{
	TextBox *t = c->extra;
	int caret, sel;

	if (! app_is_armed(t->parent))
		return;
	caret = app_text_box_caret_from_click(t, p);
	sel = t->selected + t->caret - caret;
	if ((caret != t->caret) || (sel != t->selected)) {
		t->caret = caret;
		t->selected = sel;
		app_text_box_scroll_to_caret(t, 0);
	}
}

static void app_text_box_mouse_up(Control *c, int buttons, Point p)
{
	int i;
	TextBox *t = c->extra;

	if (! app_is_armed(t->parent))
		return;
	if (buttons)
		return;
	app_disarm(t->parent);

	if (app_parent_window(c)->app->use_X_copy_paste) {
		c = t->parent;
		if (c->key_action)
			for (i=0; c->key_action[i]; i++) /* copy text */
				c->key_action[i](c, CONTROL + 'C');
	}
}

/*
 *  Insert some text at the current insertion point in a text box.
 *  If there is text already selected, replace it.
 */
static void app_text_box_insert_text(Control *c, const char *newtext, int len2)
{
	int len, caret, selected;
	char *text;
	TextBox *t = c->extra;

	/* find current size of text */
	text = c->text;
	len = t->text_length;
	caret = t->caret;
	selected = t->selected;

	/* delete any current selection first */
	if (selected > 0) {
		/* selection runs from caret to caret+selected */
		/* 0 <= caret < caret+selected < len */
		/* number of bytes following selection is len-(caret+selected) */
		memmove(text+caret, text+caret+selected, len-caret-selected+1);
		len -= selected;
		t->text_length -= selected;
		if (caret >= len)
			caret = len;
	}
	else if (selected < 0) {
		/* selection runs from caret+selected to caret */
		/* 0 <= caret+selected < caret < len */
		/* number of bytes following selection is len-caret */
		memmove(text+caret+selected, text+caret, len-caret+1);
		len += selected;
		caret += selected;
		t->text_length += selected;
		if (caret >= len)
			caret = len;
	}

	/* if nothing is being inserted, just update */
	if (len2 == 0) {
		app_recompute_and_update(t, len2, caret);
		return;
	}

	/* adjust size of stored string */
	text = app_realloc(text, len+len2+1);
	if (text == NULL)
		return; /* error */

	/* copy new text into resized string */
	c->text = text;
	memmove(text+caret+len2, text+caret, len-caret+1);
	memcpy(text+caret, newtext, len2);
	t->text_length += len2;

	/* fix caret and start offset */
	caret += len2;
	app_recompute_and_update(t, len2, caret);
}

/*
 *  Handle ordinary key events, and cut, copy, paste, backspace, delete.
 */
static void app_text_box_key_down(Control *c, unsigned long ch)
{
	int i, len;
	char *text;
	char *s;
	char buffer[8];
	Window *w;
	TextBox *t = c->extra;

	c = t->parent;

	if (t->disallowed) {
		app_unicode_char_to_utf8(ch, buffer);
		if (strstr(t->disallowed, buffer) != NULL) {
			app_pass_event(c);
			return;
		}
	}

	if (ch == ESC) {
		/* pass focus events up */
		app_pass_event(c);
	}
	else if (ch == '\b')	/* backspace one character, if any */
	{
		if (! app_is_enabled(c)) {
			app_pass_event(c);
			return;
		}
		if (t->selected) {
			app_text_box_insert_text(c, NULL, 0);
			return;
		}
		text = c->text;
		len = t->text_length;
		i = t->caret;
		while ((i > 0) && IS_UTF8_EXTRA_BYTE(text[i-1]))
			i--; /* remove UTF-8 continuation bytes */
		if (i > 0)
			i--; /* remove first UTF-8 byte */
		memmove(text+i, text+t->caret, len-t->caret+1);
		t->text_length -= (t->caret - i);
		app_recompute_and_update(t, i - t->caret, i);
	}
	else if (ch == DEL)	/* delete next character, if any */
	{
		if (! app_is_enabled(c)) {
			app_pass_event(c);
			return;
		}
		if (t->selected) {
			app_text_box_insert_text(c, NULL, 0);
			return;
		}
		text = c->text;
		len = t->text_length;
		i = t->caret;
		if (i < len)
			i++; /* remove first UTF-8 byte */
		while ((i < len) && IS_UTF8_EXTRA_BYTE(text[i]))
			i++; /* remove UTF-8 continuation bytes */
		memmove(text+t->caret, text+i, len-i+1);
		t->text_length -= (i - t->caret);
		app_recompute_and_update(t, t->caret - i, t->caret);
	}
	else if ((ch == (CONTROL + 'X')) ||	/* Ctrl-X is Cut */
		 (ch == (CONTROL + 'C')))	/* Ctrl-C is Copy */
	{
		if (t->selected > 0) {
			len = t->selected;
			s = app_alloc(len+1);
			if (! s)
				return;
			memmove(s, c->text+t->caret, len);
			s[len] = '\0';
			w = app_parent_window(c);
			app_set_clipboard_text(w->app, s);
			app_free(s);
		}
		else if (t->selected < 0) {
			len = 0 - t->selected;
			s = app_alloc(len+1);
			if (! s)
				return;
			memmove(s, c->text+t->caret-len, len);
			s[len] = '\0';
			w = app_parent_window(c);
			app_set_clipboard_text(w->app, s);
			app_free(s);
		}
		if (ch == (CONTROL + 'X')) {	/* Ctrl-X is Cut */
			if (! app_is_enabled(c)) {
				app_pass_event(c);
				return;
			}
			app_text_box_insert_text(c, NULL, 0);
		}
	}
	else if (ch == (CONTROL + 'V'))	/* Ctrl-V is Paste */
	{
		if (! app_is_enabled(c)) {
			app_pass_event(c);
			return;
		}
		w = app_parent_window(c);
		s = app_get_clipboard_text(w->app);
		if (s) {
			app_text_box_insert_text(c, s, (int) strlen(s));
			app_free(s);
		}
	}
	else if (ch & CONTROL)	/* menu shortcut key */
	{
		app_pass_event(c);
		return;
	}
	else	/* otherwise, add the character to the text */
	{
		if (! app_is_enabled(c)) {
			app_pass_event(c);
			return;
		}
		if ((t->maxwidth > 0) && (t->selected == 0)) {
			if (app_utf8_length(c->text) >= t->maxwidth) {
				app_pass_event(c);
				return;
			}
		}
		app_unicode_char_to_utf8(ch, buffer);
		if (t->allowed) {
			if (strstr(t->allowed, buffer) == NULL) {
				app_pass_event(c);
				return;
			}
		}
		app_text_box_insert_text(c, buffer, (int) strlen(buffer));
	}
}

static void app_text_box_key_action(Control *c, unsigned long ch)
{
	int i, len, start, caret, sel;
	Rect textbox;
	Point p;
	Font *f;
	TextBox *t = c->extra;

	c = t->parent;

	start = t->start;
	caret = t->caret;
	sel = t->selected;

	textbox = app_inset_rect(app_get_control_area(c), 4);
	f = app_get_text_box_font(t->parent);

	switch (ch & ~(CONTROL | SHIFT)) {
		case HOME:
			for (i=0; i < t->num_lines; i++) {
				if (caret < t->lines[i])
					break;	/* gone past the line */
				if (caret == t->lines[i])
					break;	/* already at start */
				/* last line, no newline, special */
				if (i == t->num_lines-1)
					if (caret <= t->lines[i+1]) {
						caret = t->lines[i];
						break;
					}
				if (caret < t->lines[i+1]) {
					caret = t->lines[i];
					break;
				}
			}
			sel = 0;
			break;
		case END:
			for (i=0; i < t->num_lines; i++) {
				if (caret < t->lines[i])
					break;	/* gone past the line */
				if (caret < t->lines[i+1]) {
					caret = t->lines[i+1];
					if (caret == 0)
						break;
					if ((c->text[caret-1] == '\n') ||
						(c->text[caret-1] == '\t') ||
						(c->text[caret-1] == ' '))
						caret--;
					break;
				}
			}
			sel = 0;
			break;
		case LEFT:
			caret -= 1;
			while ((caret > 0) &&
				IS_UTF8_EXTRA_BYTE(c->text[caret]))
					caret -= 1;
			if (caret < 0)
				caret = 0;
			if (caret < start)
				start = caret;
			if (ch & SHIFT)
				sel += t->caret - caret;
			else
				sel = 0;
			break;
		case RIGHT:
			len = t->text_length;
			caret += 1;
			while ((caret < len) &&
				IS_UTF8_EXTRA_BYTE(c->text[caret]))
					caret += 1;
			if (caret > len)
				caret = len;
			if (ch & SHIFT)
				sel += t->caret - caret;
			else
				sel = 0;
			if (caret + sel > len)
				sel = len - caret;
			break;
		case UP:
			p = app_text_box_caret_point(t);
			p.y -= app_font_height(f);
			caret = app_text_box_caret_from_click(t, p);
			if (ch & SHIFT)
				sel += t->caret - caret;
			else
				sel = 0;
			break;
		case DOWN:
			p = app_text_box_caret_point(t);
			p.y += app_font_height(f);
			caret = app_text_box_caret_from_click(t, p);
			if (ch & SHIFT)
				sel += t->caret - caret;
			else
				sel = 0;
			break;
		case PGUP:
			app_text_box_scroll_page(t, -1);
			return;
		case PGDN:
			app_text_box_scroll_page(t, +1);
			return;
		case INS:
			/* remap this key and handle it elsewhere */
			/* Ctrl+INS       => Ctrl+'C' (copy)
			 * Ctrl+Shift+INS => Ctrl+'V' (paste)
			 * Shift+INS      => Ctrl+'V' (paste) */
			if (ch & (CONTROL | SHIFT))
				app_text_box_key_down(c, CONTROL
					+ ((ch & SHIFT) ? 'V' : 'C'));
			break;
		case DEL:
			/* remap this key and handle it elsewhere */
			if (ch & SHIFT)
				ch = CONTROL + 'X';  /* Cut */
			else
				ch = DEL;
			app_text_box_key_down(c, ch);
			return;
		default:
			if (ch & CONTROL) {
				/* handle this key elsewhere, thanks */
				app_text_box_key_down(c, ch);
				return;
			}
			sel = 0;
			break;
	}
	if ((start != t->start) || (caret != t->caret) || (sel != t->selected))
	{
		t->start = start;
		t->caret = caret;
		t->selected = sel;
		app_text_box_scroll_to_caret(t, 0);
	}
}

static void app_text_box_resize(Control *c)
{
	TextBox *t = c->extra;

#if 0	//!!
	Rect r = app_get_control_area(c);
	Rect br = r;

	/* resize both scroll bars */
	if (t->vert && t->horz) {
		app_set_control_area(t->vert,
			rect(r.width-SCROLL_SIZE, 0,
				SCROLL_SIZE, r.height-BOX_BORDER));
		br.width -= BOX_BORDER;

		app_set_control_area(t->horz,
			rect(0, r.height-SCROLL_SIZE,
				r.width-BOX_BORDER, SCROLL_SIZE));
		br.height -= BOX_BORDER;
	}

	/* resize vertical scroll bar */
	else if (t->vert) {
		app_set_control_area(t->vert,
			rect(r.width-SCROLL_SIZE, 0, SCROLL_SIZE, r.height));
		br.width -= BOX_BORDER;
	}

	/* resize horizontal scroll bar */
	else if (t->horz) {
		app_set_control_area(t->horz,
			rect(0, r.height-SCROLL_SIZE, r.width, SCROLL_SIZE));
		br.height -= BOX_BORDER;
	}

	/* resize text box */
	app_set_control_area(t->box, br);
#endif

	/* reset scroll values */
	app_recompute_text_lines(t);
	app_text_box_scroll_page(t, 0);
}

static void app_text_box_scroll_vertical(Control *c)
{
	TextBox *t;
	Graphics *g;
	int start, h, height;
	Rect box, clip;
	Point dp;

	t = c->parent->extra;
	if (t->start == app_get_control_value(t->vert))
		return;

	start = t->start;
	t->start = app_get_control_value(t->vert);
	h = app_font_height(app_get_text_box_font(t->parent));
	box = app_get_control_area(t->box);
	box = app_inset_rect(box, 4);
	clip = box;

	if (start < t->start) {	/* scrolling down = copy upwards */
		height = (t->start - start) * h;
		dp = pt(box.x, box.y);
		box.y += height;
		box.height = ((box.height - height) / h) * h;
		clip.y += box.height;
		clip.height -= box.height;
	}
	else {	/* scrolling up = copy downwards */
		clip.height = (start - t->start) * h;
		dp = pt(box.x, box.y + clip.height);
		box.height -= clip.height;
	}

	if (box.height <= 0) {	/* major change = complete redraw */
		app_draw_control(t->box);
		return;
	}

	g = app_get_control_graphics(t->box);
	app_copy_rect(g, dp, g, box);
	app_set_clip_rect(g, clip);
	app_text_box_redraw(t->box, g, 0); /* do our own clipping */
	app_del_graphics(g);
}

static void app_text_box_del(Control *c)
{
	TextBox *t = c->extra;

	if (t->lines)
		app_free(t->lines);
	app_free(c->extra);
}

static Control *app_create_text_box(Control *c, Rect r, const char *text) //!!
{
	Control *box;		/* the text box itself */
	Control *vert;		/* the scroll bar */
	TextBox *t;
	App *app;

	/* enclosing area */
	if (c == NULL)
		return NULL;
	app = app_parent_window(c)->app;

	c->font = app_find_default_font(app);
	app_set_control_background(c, CLEAR);

	/* create scrollbar within the enclosing region */
	vert = app_add_scroll_bar(c, rect(r.width-SCROLL_SIZE, 0,
			SCROLL_SIZE, r.height),
			0, 1, app_text_box_scroll_vertical);
	app_set_control_layout(vert, EDGE_TOP | EDGE_RIGHT | EDGE_BOTTOM);

	/* create the text box canvas */
	box = app_add_control(c, rect(0, 0, r.width-BOX_BORDER, r.height));
	app_set_control_background(box, FILL_ITEM);
	app_set_control_cursor(box,
			app_get_standard_cursor(app, CARET_CURSOR));
	app_set_control_layout(box, EDGE_ALL);

	/* associate text box data with enclosing area */
	t = app_zero_alloc(sizeof(TextBox));

	if (text)
		app_set_control_text(c, text);
	else
		app_set_control_text(c, "");
	t->start = 0;
	t->text_length = (int) strlen(c->text);
	t->caret = t->text_length;
	t->parent = c;
	t->vert = vert;
	t->horz = NULL;
	t->box = box;
	c->extra = box->extra = t;

	/* set callbacks */
	app_on_control_resize(c, app_text_box_resize);
	app_on_control_redraw(box, app_text_box_draw);
	app_on_control_mouse_down(box, app_text_box_mouse_down);
	app_on_control_mouse_drag(box, app_text_box_mouse_drag);
	app_on_control_mouse_up(box, app_text_box_mouse_up);
	app_on_control_key_down(c, app_text_box_key_down);
	app_on_control_key_action(c, app_text_box_key_action);
	app_on_control_update(c, app_text_box_update);
	app_on_control_refocus(c, app_text_box_refocus);
	app_on_control_deletion(c, app_text_box_del);
	app_show_control(c);

	app_recompute_text_lines(t);
	app_text_box_scroll_to_caret(t, 1);

	return c;
}

Control *app_new_text_box(Window *win, Rect r, const char *text)
{
	return app_create_text_box(app_new_control(win, r), r, text);
}

Control *app_add_text_box(Control *parent, Rect r, const char *text)
{
	return app_create_text_box(app_add_control(parent, r), r, text);
}

void app_cut_text(Control *c)
{
	int i;

	if (c->key_down)
		for (i=0; c->key_down[i]; i++) /* Ctrl-X is Cut */
			c->key_down[i](c, (CONTROL + 'X'));
}

void app_copy_text(Control *c)
{
	int i;

	if (c->key_down)
		for (i=0; c->key_down[i]; i++) /* Ctrl-C is Copy */
			c->key_down[i](c, (CONTROL + 'C'));
}

void app_clear_text(Control *c)
{
	app_insert_text(c, "");
}

void app_paste_text(Control *c)
{
	int i;

	if (c->key_down)
		for (i=0; c->key_down[i]; i++) /* Ctrl-V is Paste */
			c->key_down[i](c, (CONTROL + 'V'));
}

void app_insert_text(Control *c, const char *text)
{
	if (c->key_down && (c->key_down[0] == app_text_box_key_down))
		app_text_box_insert_text(c, text, (int) strlen(text));
}

void app_text_selection(Control *c, long *start, long *end)
{
	TextBox *t = c->extra;

	if (start) {
		if (t->selected < 0)
			*start = t->caret + t->selected;
		else
			*start = t->caret;
	}
	if (end) {
		if (t->selected < 0)
			*end = t->caret;
		else
			*end = t->caret + t->selected;
	}
}

void app_select_text(Control *c, long start, long end)
{
	TextBox *t = c->extra;
	long length;

	if (((start == 0) && (end == 0)) || (c->text == NULL))
		t->start = t->caret = t->selected = 0;
	else {
		length = t->text_length;

		if (start == -1)
			start = length;
		else if (start < 0)
			start = 0;
		else if (start > length)
			start = length;

		if (end == -1)
			end = length;
		else if (end < 0)
			end = 0;
		else if (end > length)
			end = length;

		t->caret = end;
		t->selected = start - end;
		t->start = (start < end) ? start : end;
	}
	if (c->key_down && (c->key_down[0] == app_text_box_key_down)) {
		app_recompute_text_lines(t);
		app_text_box_scroll_page(t, 0);
		app_redraw_control(c);
	}
	else {
		app_redraw_control(c);
	}
}
