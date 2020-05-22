/*
 *  Text field with an associated drop-down list.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Can now use any font.
 *  Version: 3.02  2001/10/10  Added text selection and cut/copy/paste.
 *  Version: 3.03  2001/10/20  Fixed bugs with scrolling, list selection.
 *  Version: 3.05  2001/10/28  Fixed memory bug in insert_text.
 *  Version: 3.07  2001/11/03  Added deletion and update handlers.
 *  Version: 3.09  2001/11/13  Improved disabled behaviour.
 *  Version: 3.10  2001/12/01  Added app_add_drop_field constructor.
 *  Version: 3.11  2001/12/12  Added support for menu shortcuts.
 *  Version: 3.12  2001/12/13  Added field restrictions.
 *  Version: 3.26  2002/07/31  Improved disabled behaviour.
 *  Version: 3.37  2002/12/31  Handles CONTROL/SHIFT bits in key events.
 *  Version: 3.39  2003/03/05  Reduced flicker.
 *  Version: 3.40  2003/03/07  More Windows-like look.
 *  Version: 3.41  2003/03/14  Added focus-change (refocus) handler.
 *  Version: 3.45  2003/05/05  X mouse copy/paste, t->item now c->value.
 *  Version: 3.47  2003/05/24  Reduced flicker on add, backspace, delete.
 *  Version: 3.48  2003/06/08  Text length is now kept not recalculated.
 *  Version: 3.49  2003/08/24  Fixed a selection-overtype bug.
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

typedef struct DropField  DropField;

struct DropField {
	int start;		/* first visible glyph */
	int caret;		/* insertion point */
	int selected;		/* selected length, 0=bar, else <0 >0 */
	char *allowed;		/* allowed Unicode characters, or NULL */
	char *disallowed;	/* disallowed characters, or NULL */
	int maxwidth;		/* maximum length in Chars, or 0 */
	int text_length;	/* running byte count of text length */
	char **lines;		/* the lines to display, ended by NULL */
};

/*
 *  Given the offsets to the start of text and to the caret, and
 *  the pixel width of the field, return the x-offset (in pixels)
 *  to the caret, from the start.
 */
static int app_caret_pixel_offset(Font *f, const char *s,
	int start, int caret, int width)
{
	int i, j, x;

	for (x=0, i=start; i < caret; i=j) {
		j = i+1;
		while (IS_UTF8_EXTRA_BYTE(s[j]))
			j += 1;
		x += app_font_width(f, s+i, j-i);
		if (x >= width)
			break;
	}
	return x;
}

/*
 *  Return the new start of text. This is the offset to the first
 *  character which will be displayed in the field. The old start
 *  is also passed to this function, so that if the caret is
 *  already visible, we just return the old start.
 */
static int app_start_of_text(Font *f, const char *s,
	int old_start, int caret, int width)
{
	int x, len, start;

	start = caret;
	for (x=0; (x < width) && (start > 0); ) {
		if (start == old_start)
			break;
		len = 1;
		while ((start-len > 0) &&
			IS_UTF8_EXTRA_BYTE(s[start-len]))
				len += 1;
		x += app_font_width(f, s+start-len, len);
		if (x < width)
			start -= len;
	}
	return start;
}

/*
 *  Find where the caret should go based on a mouse click x-position.
 */
static int app_caret_from_click(Font *f, const char *s, int start, int px)
{
	int x, w, len, caret;

	caret = start;
	for (x=0; x < px; ) {
		len = 0;
		if (s[caret+len] != '\0')
			len += 1;
		while (IS_UTF8_EXTRA_BYTE(s[caret+len]))
			len += 1;
		if (len == 0)
			break;
		w = app_font_width(f, s+caret, len);
		x += w;
		if (x < px+w/2)
			caret += len;
	}
	return caret;
}

/*
 *  The field is updated when the text is changed.
 */
static void app_drop_field_update(Control *c)
{
	int length;
	DropField *t;

	t = c->extra;

	length = c->text ? (int) strlen(c->text) : 0;
	if (t->caret < 0)
		t->caret = 0;
	if (t->caret > length)
		t->caret = length;
	if (t->start > t->caret)
		t->start = t->caret;
	if (t->caret + t->selected < 0)
		t->selected = 0 - t->caret;
	if (t->caret + t->selected > length)
		t->selected = length - t->caret;

	t->text_length = length;
/*
	t->caret = length;
	t->selected = 0;
	t->start = 0;

	app_redraw_control(c);	//!!
*/
}

/*
 *  Return the rectangle of the selection (in control-relative co-ordinates).
 */
static Rect app_field_selection_rect(Control *c)
{
	Rect textbox;
	DropField *t;
	Font *f;
	int h, x1, x2, temp;

	t = c->extra;
	textbox = app_inset_rect(app_get_control_area(c), 4);
	textbox.width -= 16;

	/* Find caret height. */
	f = c->font;
	if (f == NULL)
		f = app_find_default_font(app_parent_window(c)->app);
	h = app_font_height(f);
	if (h > textbox.height)
		h = textbox.height;

	/* Find selection rectangle. */
	x1 = app_caret_pixel_offset(c->font, c->text,
			t->start, t->caret, textbox.width);
	if (t->selected != 0)
		x2 = app_caret_pixel_offset(c->font, c->text,
			t->start, t->caret + t->selected, textbox.width);
	else
		x2 = x1;
	if (x1 > x2) {
		temp = x1;
		x1 = x2;
		x2 = temp;
	}
	else if (x1 == x2)
		x2 = x1 + 1;
	if (x2-x1 > textbox.width)
		x2 = x1 + textbox.width;

	return rect(textbox.x + x1, textbox.y, x2-x1, h);
}

/*
 *  Update the selection by only redrawing the area that must be redrawn.
 *  This function assumes only the selection or caret needs to be redrawn,
 *  and that the control already had and still has focus (and thus only
 *  the selection must be modified). The control's contents must not
 *  have changed. If the start has changed, the entire field is redrawn.
 */
static void app_field_update_selection(Control *c, int start, int caret,
	int sel)
{
	Rect r1, r2;
	Graphics *g;
	DropField *t = c->extra;

	if ((start == t->start) && (caret == t->caret) && (sel == t->selected))
		return; /* nothing to be done */

	if (start != t->start) /* too different, so redraw the whole field */
	{
		t->start = start;
		t->caret = caret;
		t->selected = sel;
		app_draw_control(c);
	}
	else {
		r1 = app_field_selection_rect(c);

		t->start = start;
		t->caret = caret;
		t->selected = sel;

		r2 = app_field_selection_rect(c);

		g = app_get_control_graphics(c);
		if (app_is_enabled(c))
			app_set_colour(g, app_get_control_foreground(c));
		else
			app_set_colour(g, DISABLED_ITEM);
		app_set_xor_mode(g, app_get_control_background(c));

		if (r1.x + r1.width == r2.x + r2.width) /* right edges join */
		{
			/* just draw the difference of the two rectangles */
			if (r1.x < r2.x) /* rectangle 1 is longer and left-most */
				app_fill_rect(g, rect(r1.x,r1.y,r2.x-r1.x,r1.height));
			else /* rectangle 2 is longer and left-most */
				app_fill_rect(g, rect(r2.x,r1.y,r1.x-r2.x,r1.height));
		}
		else if (r1.x == r2.x) /* left edges join */
		{
			/* just draw the difference of the two rectangles */
			if (r1.width < r2.width) /* rectangle 2 is longer */
				app_fill_rect(g, rect(r1.x+r1.width, r1.y,
						r2.width-r1.width, r1.height));
			else /* rectangle 1 is longer */
				app_fill_rect(g, rect(r2.x+r2.width, r1.y,
						r1.width-r2.width, r1.height));
		}
		else { /* distinct: draw both selection rectangles */
			app_fill_rect(g, r1);
			app_fill_rect(g, r2);
		}
		app_set_paint_mode(g);
		app_del_graphics(g);
	}
}

/*
 *  Draw the entire control.
 */
static void app_drop_field_redraw(Control *c, Graphics *g, int do_clip)
{
	Rect r;
	Rect textbox;
	Point p[3];
	DropField *t;

	t = c->extra;
	r = app_get_control_area(c);
	r.width -= 16; /* leave space for triangle icon */
	textbox = app_inset_rect(r, 4);

	/* Blank the text area of the field first. */
	app_set_colour(g, app_get_control_background(c));
	app_fill_rect(g, app_inset_rect(textbox,-2));

	/* Draw the field text. */
	app_set_font(g, c->font);
	if (app_is_enabled(c))
		app_set_colour(g, app_get_control_foreground(c));
	else
		app_set_colour(g, DISABLED_ITEM);

	if (do_clip)
		app_set_clip_rect(g, textbox);
	app_draw_utf8(g, pt(textbox.x, textbox.y), c->text + t->start,
			t->text_length - t->start);
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
		app_set_xor_mode(g, app_get_control_background(c));
		app_fill_rect(g, app_field_selection_rect(c));
		app_set_paint_mode(g);
	}

#if 0
	/* Draw field border, and caret if it has focus. */
	if (app_has_focus(c)) {
		/* Draw the focus border. */
		app_set_colour(g, FOCUS_BORDER);
		app_draw_rect(g, r);
		r = app_inset_rect(r, 1);

		/* Draw the caret (insertion bar) or selection box. */
		if (app_is_enabled(c))
			app_set_colour(g, app_get_control_foreground(c));
		else
			app_set_colour(g, DISABLED_ITEM);
		app_set_xor_mode(g, app_get_control_background(c));
		app_fill_rect(g, app_field_selection_rect(c));
		app_set_paint_mode(g);
	}
	/* Draw the bevelled border. */
	app_set_line_width(g, 2);
	app_draw_shadow_rect(g, r, LOWER_RIGHT, UPPER_LEFT);
#endif

	/* Draw 'drop' triangle */
	r = app_get_control_area(c);
	r.x += r.width - 17;
	r.width = 17;
	if (do_clip)
		app_set_clip_rect(g, r);

	/* Blank the triangle area */
	app_set_colour(g, BACKGROUND);
	app_fill_rect(g, r);

	/* Draw a border */
	app_set_line_width(g, 1);
	app_set_colour(g, FOCUS_BORDER);
	app_draw_rect(g, r);
	app_draw_shadow_rect(g, app_inset_rect(r, 1), UPPER_LEFT, LOWER_RIGHT);

	/* Draw a triangle to signify dropping the menu */
	app_set_line_width(g, 1);
	app_set_colour(g, ENABLED_ITEM);
	p[0] = pt(r.x+r.width-5, r.y+r.height/2-2);
	p[1] = pt(p[0].x-7, p[0].y);
	p[2] = pt(p[1].x+3, p[1].y+4);
	app_fill_polygon(g, p, 3);
}

static void app_drop_field_draw(Control *c, Graphics *g)
{
	app_drop_field_redraw(c, g, 1);
}

static void app_drop_field_refocus(Control *c)
{
	DropField *t;
	Rect r;
	Region *clip, *inside;
	Graphics *g;

	t = c->extra;
	r = app_get_control_area(c);
	r.width -= 16; /* leave space for triangle icon */
	clip = app_new_rect_region(r);

	inside = app_new_rect_region(app_inset_rect(r,4));
	app_subtract_region(clip, inside, clip);
	app_del_region(inside);

	r = app_field_selection_rect(c);
	app_union_region_with_rect(clip, r, clip);

	g = app_get_control_graphics(c);
	app_set_clip_region(g, clip);
	app_del_region(clip);
	app_drop_field_redraw(c, g, 0); /* doing our own clipping */
	app_del_graphics(g);
}

/*
 *  Set start to 0, caret to the start, and select the whole field.
 */
static void app_select_drop_field(Control *c)
{
	int len;
	DropField *t = c->extra;

	len = (int) strlen(t->lines[c->value]);
	t->text_length = len;
	t->selected = -len;
	t->caret = len;
	t->start = 0;

	app_redraw_control(c);
}

static void app_drop_field_mouse_down(Control *c, int buttons, Point p)
{
	int i;
	Rect r;
	Window *w;
	int caret;
	DropField *t = c->extra;

	r = app_get_control_area(c);
	r.x += r.width - 16;
	r.width = 16;
	if (app_point_in_rect(p, r)) {
		if (! app_is_enabled(c))
			return;
		w = app_parent_window(c);
		p = c->offset;	//!!
		p.x += c->area.width/2;
		/* p.y += c->area.height; */
		c->value = app_pop_up_list(w, c->font, t->lines, buttons, p);
		if (c->value >= 0) {
			app_set_focus(c);
			app_set_control_text(c, t->lines[c->value]);
			app_select_drop_field(c);
		}
		return;
	}

	app_arm(c);
	caret = app_caret_from_click(c->font, c->text,
			t->start, p.x - 4);

	if (app_parent_window(c)->app->use_X_copy_paste) {
		if (buttons == MIDDLE_BUTTON) { /* paste text */
			app_set_focus(c);
			t->caret = caret;
			t->selected = 0;
			if (c->key_action)
				for (i=0; c->key_action[i]; i++)
					c->key_action[i](c, CONTROL + 'V');
			app_draw_control(c);
			return;
		}
	}

	if (! app_has_focus(c)) {
		app_set_focus(c);
		t->caret = caret;
		t->selected = 0;
		app_draw_control(c);
	}
	else
		app_field_update_selection(c, t->start, caret, 0);
}

static void app_drop_field_mouse_drag(Control *c, int buttons, Point p)
{
	int start, caret, sel;
	Rect textbox;
	DropField *t = c->extra;

	if (! app_is_armed(c))
		return;

	textbox = app_inset_rect(app_get_control_area(c), 4);
	textbox.width -= 16;
	p.x -= textbox.x;

	if (p.x < 0) {
		caret = t->start - 1;
		while ((caret > 0) && IS_UTF8_EXTRA_BYTE(c->text[caret]))
			caret -= 1;
		if (caret < 0)
			caret = 0;
	} else {
		caret = app_caret_from_click(c->font, c->text,
				t->start, p.x);
	}

	if (caret != t->caret) {
		sel = t->selected + t->caret - caret;
		start = t->start;
		if (caret < start)
			start = caret;
		else
			start = app_start_of_text(c->font, c->text,
				start, caret, textbox.width);
		app_field_update_selection(c, start, caret, sel);
	}
}

static void app_drop_field_mouse_up(Control *c, int buttons, Point p)
{
	int i;

	if (! app_is_armed(c))
		return;
	if (buttons)
		return;
	app_disarm(c);

	if (app_parent_window(c)->app->use_X_copy_paste) {
		if (c->key_action)
			for (i=0; c->key_action[i]; i++) /* copy text */
				c->key_action[i](c, CONTROL + 'C');
	}
}

/*
 *  Redraw a portion of the text from a position to the right end.
 *  This should only occur when the start of the text hasn't changed,
 *  and you just want to update the end of the line of text.
 */
static void app_redraw_end_of_drop_field(Control *c, int from_pos)
{
	int x;
	Rect textbox;
	DropField *t = c->extra;

	if (from_pos < t->start)
		from_pos = t->start;

	textbox = app_get_control_area(c);
	textbox.width -= 16;
	textbox = app_inset_rect(textbox, 4);
	x = app_font_width(c->font, c->text+t->start, from_pos-t->start);
	textbox.x += x;
	textbox.width -= x;
	app_redraw_control_rect(c, textbox);
}

/*
 *  Insert some text at the current insertion point in a field.
 *  If there is text already selected, replace it.
 */
static void app_drop_field_insert_text(Control *c,
	const char *newtext, int len2)
{
	int len, caret, selected, start;
	char *text;
	Rect textbox;
	DropField *t = c->extra;

	/* find current size of text */
	text = c->text;
	len = (int) strlen(text);
	caret = t->caret;
	selected = t->selected;

	/* delete any current selection first */
	if (selected > 0) {
		/* selection runs from caret to caret+selected */
		/* 0 <= caret < caret+selected < len */
		/* number of bytes after selection is len-(caret+selected) */
		memmove(text+caret, text+caret+selected, len-caret-selected+1);
		len -= selected;
		t->text_length -= selected;
	}
	else if (selected < 0) {
		/* selection runs from caret+selected to caret */
		/* 0 <= caret+selected < caret < len */
		/* number of bytes after selection is len-caret */
		memmove(text+caret+selected, text+caret, len-caret+1);
		len += selected;
		t->caret += selected;
		t->text_length += selected;
	}
	t->selected = 0;
	if (t->caret >= len)
		t->caret = len;
	caret = t->caret;

	if (len2 == 0) {
		app_draw_control(c);
		return;
	}

	/* adjust size of stored string */
	text = app_realloc(text, len+len2+1);
	if (text == NULL)
		return; /* error */

	/* copy new string into resized string */
	c->text = text;
	memmove(text+caret+len2, text+caret, len-caret+1);
	memcpy(text+caret, newtext, len2);
	t->text_length += len2;

	/* determine maximum text space */
	textbox = app_get_control_area(c);
	textbox.width -= 16;
	textbox = app_inset_rect(textbox, 4);

	/* fix caret and start offset */
	t->caret += len2;
	start = t->start;
	t->start = app_start_of_text(c->font, text,
			t->start, t->caret, textbox.width);

	/* special case: fast redraw if start has not moved */
	if (start == t->start) {
		app_redraw_end_of_drop_field(c, t->caret-len2);
		return;
	}

	/* major changes require drawing the entire control */
	app_draw_control(c);
}

static void app_drop_field_key_down(Control *c, unsigned long ch)
{
	int i, len;
	char *text;
	char *s;
	char buffer[8];
	Window *w;
	DropField *t = c->extra;

	if (t->disallowed) {
		app_unicode_char_to_utf8(ch, buffer);
		if (strstr(t->disallowed, buffer) != NULL) {
			app_pass_event(c);
			return;
		}
	}

	if ((ch == '\n') || (ch == '\t') || (ch == ESC)) {
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
			app_drop_field_insert_text(c, NULL, 0);
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
		t->caret = i;
		if (t->caret < t->start) {
			t->start = t->caret;
			app_redraw_control(c);
		}
		else {
			app_redraw_end_of_drop_field(c, t->caret);
		}
	}
	else if (ch == DEL)	/* delete next character, if any */
	{
		if (! app_is_enabled(c)) {
			app_pass_event(c);
			return;
		}
		if (t->selected) {
			app_drop_field_insert_text(c, NULL, 0);
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
		if (t->caret > len)
			t->caret = len;
		if (t->caret < t->start) {
			t->start = t->caret;
			app_redraw_control(c);
		}
		else {
			app_redraw_end_of_drop_field(c, t->caret);
		}
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
			app_drop_field_insert_text(c, NULL, 0);
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
			app_drop_field_insert_text(c, s, (int) strlen(s));
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
		if (t->maxwidth > 0) {
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
		app_drop_field_insert_text(c, buffer, (int) strlen(buffer));
	}
}

static void app_drop_field_key_action(Control *c, unsigned long ch)
{
	int len, start, caret, sel;
	Rect textbox;
	DropField *t = c->extra;

	start = t->start;
	caret = t->caret;
	sel = t->selected;

	textbox = app_get_control_area(c);
	textbox.width -= 16;
	textbox = app_inset_rect(textbox, 4);

	switch (ch & ~(CONTROL | SHIFT)) {
		case UP:
			if (! app_is_enabled(c))
				break;
			if (c->value >= 1) {
				c->value--;
				app_set_control_text(c, t->lines[c->value]);
				app_select_drop_field(c);
			}
			return;
		case DOWN:
			if (! app_is_enabled(c))
				break;
			for (len=0; t->lines && t->lines[len]; len++)
				continue;
			if (c->value < len-1) {
				c->value++;
				app_set_control_text(c, t->lines[c->value]);
				app_select_drop_field(c);
			}
			return;
		case HOME:
			start = caret = 0;
			if (ch & SHIFT)
				sel += t->caret - caret;
			else
				sel = 0;
			break;
		case END:
			caret = t->text_length;
			start = app_start_of_text(c->font, c->text,
					start, caret, textbox.width);
			if (ch & SHIFT)
				sel += t->caret - caret;
			else
				sel = 0;
			break;
		case LEFT:
			caret -= 1;
			while ((caret > 0) &&
				IS_UTF8_EXTRA_BYTE(c->text[caret]))
					caret -= 1;
			if (caret < 0)
				caret = 0;
			start = app_start_of_text(c->font, c->text,
					start, caret, textbox.width);
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
			start = app_start_of_text(c->font, c->text,
					start, caret, textbox.width);
			if (ch & SHIFT)
				sel += t->caret - caret;
			else
				sel = 0;
			if (caret + sel > len)
				sel = len - caret;
			break;
		case INS:
			/* remap this key and handle it elsewhere */
			/* Ctrl+INS       => Ctrl+'C' (copy)
			 * Ctrl+Shift+INS => Ctrl+'V' (paste)
			 * Shift+INS      => Ctrl+'V' (paste) */
			if (ch & (CONTROL | SHIFT))
				app_drop_field_key_down(c, CONTROL
					+ ((ch & SHIFT) ? 'V' : 'C'));
			break;
		case DEL:
			/* remap this key and handle it elsewhere */
			if (ch & SHIFT)
				ch = CONTROL + 'X';  /* Cut */
			else
				ch = DEL;
			app_drop_field_key_down(c, ch);
			return;
		default:
			if (ch & CONTROL) {
				/* handle this key elsewhere, thanks */
				app_drop_field_key_down(c, ch);
				return;
			}
			sel = 0;
			break;
	}
	app_field_update_selection(c, start, caret, sel);
}

static void app_drop_field_del(Control *c)
{
	app_free(c->extra);
}

static Control *app_create_drop_field(Control *c, Rect r, char *lines[]) //!!
{
	DropField *t;
	App *app;

	if (c == NULL)
		return NULL;
	app = app_parent_window(c)->app;

	c->font = app_find_default_font(app);
#if 0
	app_set_control_cursor(c,
		app_get_standard_cursor(app, CARET_CURSOR));
#endif

	t = app_zero_alloc(sizeof(DropField));
	c->extra = t;

	c->natural.x = 16;  /* offset to correct natural size */

	if (lines && lines[0]) {
		app_set_control_text(c, lines[0]);
		c->value = 0;
	}
	else {
		app_set_control_text(c, "");
		c->value = -1;
	}
	t->start = 0;
	t->text_length = (int) strlen(c->text);
	t->caret = t->text_length;
	t->lines = lines;

	app_set_control_background(c, FILL_ITEM);
	app_on_control_redraw(c, app_drop_field_draw);
	app_on_control_mouse_down(c, app_drop_field_mouse_down);
	app_on_control_mouse_drag(c, app_drop_field_mouse_drag);
	app_on_control_mouse_up(c, app_drop_field_mouse_up);
	app_on_control_key_down(c, app_drop_field_key_down);
	app_on_control_key_action(c, app_drop_field_key_action);
	app_on_control_update(c, app_drop_field_update);
	app_on_control_refocus(c, app_drop_field_refocus);
	app_on_control_deletion(c, app_drop_field_del);

	app_show_control(c);

	return c;
}

Control *app_new_drop_field(Window *win, Rect r, char *lines[])
{
	return app_create_drop_field(app_new_control(win, r), r, lines);
}

Control *app_add_drop_field(Control *parent, Rect r, char *lines[])
{
	return app_create_drop_field(app_add_control(parent, r), r, lines);
}
