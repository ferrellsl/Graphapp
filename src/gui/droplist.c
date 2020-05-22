/*
 *  Drop-down lists displaying text in a menu.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Can now use any font.
 *  Version: 3.07  2001/11/03  Added deletion handler.
 *  Version: 3.09  2001/11/13  Improved disabled behaviour.
 *  Version: 3.10  2001/12/01  Added app_add_drop_list constructor.
 *  Version: 3.15  2002/01/05  Pop-up menus don't need mouse held down.
 *  Version: 3.40  2003/03/07  More Windows-like look.
 *  Version: 3.41  2003/03/18  Exits list shown mouse-loop if other event.
 *  Version: 3.42  2003/04/09  Simplified; control's value is hit item.
 *  Version: 3.45  2003/05/12  Now look more like menubar menus.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced some size_t conversion warnings.
 *  Version: 3.57  2005/08/16  Define USE_POPUP_WINDOW to use popups here.
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

#define USE_POPUP_WINDOW

enum {
	MARGIN = 4,		/* outside to text */
	BORDER = 3,		/* outside to selection boxes */
	BORDER_SIZE = 1,	/* width of outlining border */
	VSPACING = 1,		/* extra spacing between lines of text */
	VSEPSIZE = 6,		/* vertical gap for separator bars */
	FLASH = 1,		/* number of times to flash selection */
	FLASH_MILLISEC = 50	/* milliseconds between flashes */
};

/*
 *  Private functions:
 */

#ifdef USE_POPUP_WINDOW

  typedef struct PopupMenu  PopupMenu;
  struct PopupMenu {
	char **lines;
	int    nitem;
	int    lasti;
	Font * font;
  };

#endif

static Rect app_menu_item_rect(char *lines[], Rect r, int which,
	int font_height)
{
	int i;
	const char *text;

	if (which < 0)
		return rect(0, 0, 0, 0);
	r = app_inset_rect(r, MARGIN);

	for (i=0; i <= which; i++) {
		text = lines[i];
		if ((text[0] == '-') && (text[1] == '\0'))
			r.height = VSEPSIZE;
		else
			r.height = font_height + VSPACING;
		if (i != which)
			r.y += r.height;
	}

	return app_inset_rect(r, BORDER-MARGIN);
}

static int app_menu_selection(char *lines[], Rect r, Point p,
	int font_height)
{
	int i;
	const char *text;

	r = app_inset_rect(r, MARGIN);
	if (! app_point_in_rect(p, r))
		return -1;
	p.y -= r.y;
	for (i=0; lines[i] != NULL; i++) {
		text = lines[i];
		if ((text[0] == '-') && (text[1] == '\0'))
			p.y -= VSEPSIZE;
		else 
			p.y -= (font_height + VSPACING);
		if (p.y < 0)
			return i;
	}
	return -1;
}

static int app_draw_menu_item(Graphics *wg, Rect menur,
		const char *text, Rect r, Font *font, int selected)
{
	Point p;

	p.x = r.x + MARGIN - BORDER;
	p.y = r.y + MARGIN - BORDER;

	/* draw a separator */
	if ((text[0] == '-') && (text[1] == '\0')) {
		r.height = VSEPSIZE;

		/* draw separator background */
		app_set_rgb(wg, MENU_BACKGROUND);
		app_fill_rect(wg, r);

		/* draw separator line and shadow */
		app_draw_shadow_rect(wg, rect(menur.x + BORDER, p.y + 2,
				menur.width - BORDER*2, 2),
				LOWER_RIGHT, UPPER_LEFT);
		return r.height;
	}

	r.height = font->height + VSPACING;

	/* fill background of menu item */
	if (selected)
		app_set_rgb(wg, MENU_HILIGHT);
	else
		app_set_rgb(wg, MENU_BACKGROUND);
	app_fill_rect(wg, rect(r.x, r.y,
			r.width, r.height+(MARGIN-BORDER)*2));

	/* draw border */
	if (selected) {
		app_draw_shadow_rect(wg, rect(r.x, r.y,
			r.width, r.height+(MARGIN-BORDER)*2),
			MENU_ITEM_UPPER_BORDER,
			MENU_ITEM_LOWER_BORDER);
	}

	/* draw menu item's text */
	if (selected)
		app_set_rgb(wg, MENU_TEXT_HILIGHT);
	else
		app_set_rgb(wg, MENU_TEXT);
	app_draw_utf8(wg, pt(p.x, p.y), text, (int) strlen(text));

	return r.height;
}

#ifndef USE_POPUP_WINDOW

static int app_menu_hit(Window *w, Font *f, char *lines[],
	int prev_value, int buttons, Point xy)
{
	int i, nitem, maxwidth, maxheight, lasti, is_mouse_event;
	int btns, displaying;
	Rect r, menur, maxrect;
	Point p;
	Bitmap *b;
	const char *item;
	Graphics *wg, *bg = NULL;
	Region *old;

	/* set up drawing so we can draw anywhere inside the window */
	maxrect = app_get_window_area(w);
	wg = app_get_window_graphics(w);
	app_set_font(wg, f);
	old = wg->clip;
	wg->clip = NULL;

	/* find the maximum pixel width of the menu */
	maxwidth = 0;
	for (nitem = 0; (item = lines[nitem]) != NULL; nitem++) {
		i = app_font_width(f, item, (int) strlen(item));
		if (i > maxwidth)
			maxwidth = i;
	}

	/* find the maximum pixel height of the menu */
	maxheight = 0;
	for (nitem = 0; (item = lines[nitem]) != NULL; nitem++) {
		if ((item[0] == '-') && (item[1] == '\0'))
			maxheight += VSEPSIZE;
		else
			maxheight += f->height + VSPACING;
	}

	if (prev_value >= nitem)
		prev_value = 0;

	/* determine the dimensions of the menu */
	r = app_inset_rect(rect(0, 0, maxwidth, maxheight), -MARGIN);
	r.x -= maxwidth/2;
	r.y -= prev_value*(f->height+VSPACING)+f->height/2;
	r.x += xy.x;
	r.y += xy.y;

	/* determine where to display the menu */
	p = pt(0, 0);
	if (r.x+r.width > maxrect.width)
		p.x = maxrect.width-r.x-r.width;
	if (r.y+r.height > maxrect.height)
		p.y = maxrect.height-r.y-r.height;
	if (r.x < 0)
		p.x = 0-r.x;
	if (r.y < 0)
		p.y = 0-r.y;
	menur = r;
	menur.x += p.x;
	menur.y += p.y;

	/* save the pixels underneath the menu */
	b = app_new_bitmap(w, menur.width, menur.height);
	if (b != NULL) {
		bg = app_get_bitmap_graphics(b);
		app_copy_rect(bg, pt(0,0), wg, menur);
	}

	/* draw the menu */
	app_set_rgb(wg, MENU_BACKGROUND);
	app_fill_rect(wg, menur);

	if (BORDER_SIZE == 1) {
		app_set_line_width(wg, BORDER_SIZE);
		app_draw_shadow_rect(wg, menur,
			MENU_UPPER_BORDER, MENU_LOWER_BORDER);
	}
	else if (BORDER_SIZE > 1) {
		app_set_line_width(wg, BORDER_SIZE);
		app_draw_shadow_rect(wg, menur,
			MENU_UPPER_BORDER, MENU_LOWER_BORDER);
		app_set_line_width(wg, 1);
		app_draw_shadow_rect(wg, menur,
			MENU_OUTER_BORDER, MENU_OUTER_BORDER);
	}
	r = rect(menur.x+BORDER, menur.y+BORDER, menur.width-BORDER*2, 5);
	for (i=0; i < nitem; i++) {
		r.y += app_draw_menu_item(wg, menur, lines[i], r, f, 0);
	}

	/* highlight the selected menu item */
	app_draw_all(w->app);
	lasti = app_menu_selection(lines, menur, xy, f->height);
	if (lasti >= 0) {
		r = app_menu_item_rect(lines, menur, lasti, f->height);
		app_draw_menu_item(wg, menur, lines[lasti], r, f, 1);
	}

	/* process mouse events only */
	displaying = 1;
	while (displaying) {
		is_mouse_event = app_get_mouse_event(w->app, &btns, &xy);

		if (! is_mouse_event) {
			displaying = 0;
			break;
		}

		if (! (btns & LEFT_BUTTON)) { /* mouse button up */
			if (displaying == 1) {
				if (! app_point_in_rect(xy, menur))
					displaying = 2;
				else
					break;
			}
			else if (displaying == 2)
				;
			else if (displaying == 3) {
				displaying = 0;
				break;
			}
		}
		else { /* mouse button down */
			if (displaying == 2)
				displaying = 3;
		}
		i = app_menu_selection(lines, menur, xy, f->height);
		if (i == lasti)
			continue;

		if (lasti >= 0)
			app_draw_menu_item(wg, menur, lines[lasti], r, f, 0);
		if (i >= 0) {
			r = app_menu_item_rect(lines, menur, i, f->height);
			app_draw_menu_item(wg, menur, lines[i], r, f, 1);
		}

		lasti = i;
	}

	if ((lasti >= 0) && (lasti < nitem) && (lines[lasti][0] != '-'))
	{
		r = app_menu_item_rect(lines, menur, lasti, f->height);
		for (i=0; i < FLASH; i++) {
			app_delay(w->app, FLASH_MILLISEC);
			app_draw_menu_item(wg, menur, lines[lasti], r, f, 0);
			app_draw_all(w->app);

			app_delay(w->app, FLASH_MILLISEC);
			app_draw_menu_item(wg, menur, lines[lasti], r, f, 1);
			app_draw_all(w->app);
		}
	}

	/* tidy up */
	if (b != NULL) {
		app_copy_rect(wg, pt(menur.x,menur.y), bg,
			rect(0, 0, menur.width, menur.height));
		app_del_graphics(bg);
		app_del_bitmap(b);
	}
	wg->clip = old;
	app_del_graphics(wg);

	if (prev_value == lasti)
		return -1;	/* no change */
	return lasti;
}

#else /* USE_POPUP_WINDOW */

static void app_popup_draw_menu(Window *w, Graphics *wg)
{
	Rect r, menur;
	PopupMenu *pm;
	char **lines;
	int i, nitem, lasti;
	Font *f;

	pm = app_get_window_data(w);
	lines = pm->lines;
	nitem = pm->nitem;
	lasti = pm->lasti;
	f = pm->font;

	menur = app_get_window_area(w);
	app_set_rgb(wg, MENU_BACKGROUND);
	app_fill_rect(wg, menur);

	if (BORDER_SIZE == 1) {
		app_set_line_width(wg, BORDER_SIZE);
		app_draw_shadow_rect(wg, menur,
			MENU_UPPER_BORDER, MENU_LOWER_BORDER);
	}
	else if (BORDER_SIZE > 1) {
		app_set_line_width(wg, BORDER_SIZE);
		app_draw_shadow_rect(wg, menur,
			MENU_UPPER_BORDER, MENU_LOWER_BORDER);
		app_set_line_width(wg, 1);
		app_draw_shadow_rect(wg, menur,
			MENU_OUTER_BORDER, MENU_OUTER_BORDER);
	}
	r = rect(menur.x+BORDER, menur.y+BORDER, menur.width-BORDER*2, 5);
	for (i=0; i < nitem; i++)
		r.y += app_draw_menu_item(wg, menur, lines[i], r, f, lasti==i);
}

static void app_popup_draw_item(Window *w, Font *f, Rect menur,
	char **lines, int lasti, int i)
{
	Rect r;
	Graphics *wg;

	wg = app_get_window_graphics(w);

	if (lasti >= 0) {
		r = app_menu_item_rect(lines, menur, lasti, f->height);
		app_draw_menu_item(wg, menur, lines[lasti], r, f, 0);
	}
	if (i >= 0) {
		r = app_menu_item_rect(lines, menur, i, f->height);
		app_draw_menu_item(wg, menur, lines[i], r, f, 1);
	}

	app_del_graphics(wg);
}

static void app_popup_mouse_move(Window *w, int btns, Point xy)
{
	Rect menur;
	PopupMenu *pm;
	char **lines;
	int i, lasti;
	Font *f;

	pm = app_get_window_data(w);
	lines = pm->lines;
	lasti = pm->lasti;
	f = pm->font;

	menur = app_get_window_area(w);

	i = app_menu_selection(lines, menur, xy, f->height);
	if (i != lasti)
		app_popup_draw_item(w, f, menur, pm->lines, lasti, i);
	pm->lasti = i;
}

static void app_popup_mouse_down(Window *w, int btns, Point xy)
{
	Rect r, menur;
	PopupMenu *pm;
	char **lines;
	int i, lasti;
	Font *f;
	Graphics *wg;

	pm = app_get_window_data(w);
	lines = pm->lines;
	lasti = pm->lasti;
	f = pm->font;

	wg = app_get_window_graphics(w);
	menur = app_get_window_area(w);

	if ((lasti >= 0) && (lines[lasti][0] != '-'))
	{
		r = app_menu_item_rect(lines, menur, lasti, f->height);
		for (i=0; i < FLASH; i++) {
			app_delay(w->app, FLASH_MILLISEC);
			app_draw_menu_item(wg, menur, lines[lasti], r, f, 0);

			app_delay(w->app, FLASH_MILLISEC);
			app_draw_menu_item(wg, menur, lines[lasti], r, f, 1);
		}
		app_hide_window(w);
	}

	app_del_graphics(wg);
}

static void app_popup_key_down(Window *w, unsigned long key)
{
	PopupMenu *pm;

	if (key == ESC) {
		pm = app_get_window_data(w);
		pm->lasti = -1;
	}

	app_hide_window(w);
}

static void app_popup_key_action(Window *w, unsigned long key)
{
	PopupMenu *pm;
	int lasti;

	pm = app_get_window_data(w);
	lasti = pm->lasti;

	if (key == UP) {
		if (--lasti < 0)
			lasti = pm->nitem - 1;
	} else if (key == DOWN) {
		if (++lasti >= pm->nitem)
			lasti = 0;
	} else
		return;

	if (pm->lasti != lasti)
		app_popup_draw_item(w, pm->font, app_get_window_area(w),
			pm->lines, pm->lasti, lasti);
	pm->lasti = lasti;
}

static int app_handle_popup_menu(Window *w, int prev_value)
{
	PopupMenu *pm;

	pm = app_get_window_data(w);

	app_show_window(w);
	while (w->state & VISIBLE)
		app_wait_event(w->app);

	if (prev_value == pm->lasti)
		return -1;	/* no change */
	return pm->lasti;
}

static int app_menu_hit(Window *w, Font *f, char *lines[],
	int prev_value, int buttons, Point xy)
{
	int i, nitem, maxwidth, maxheight;
	Rect r;
	PopupMenu *pm;
	const char *item;
	Graphics *wg;

	/* find the maximum pixel width of the menu */
	maxwidth = 0;
	for (nitem = 0; (item = lines[nitem]) != NULL; nitem++) {
		i = app_font_width(f, item, (int) strlen(item));
		if (i > maxwidth)
			maxwidth = i;
	}

	/* find the maximum pixel height of the menu */
	maxheight = 0;
	for (nitem = 0; (item = lines[nitem]) != NULL; nitem++) {
		if ((item[0] == '-') && (item[1] == '\0'))
			maxheight += VSEPSIZE;
		else
			maxheight += f->height + VSPACING;
	}

	if (prev_value >= nitem)
		prev_value = 0;

	/* determine the dimensions and location of the menu */
	r = app_inset_rect(rect(0, 0, maxwidth, maxheight), -MARGIN);
	r.x -= maxwidth/2;
	r.y -= prev_value*(f->height+VSPACING)+f->height/2;
	r.x += xy.x;
	r.y += xy.y;

	/* save the position of item */
	i = app_menu_selection(lines, r, xy, f->height);

	r.x += w->area.x;
	r.y += w->area.y;

	w = app_new_window(w->app, r, NULL, BASE | POPUP);
	wg = app_get_window_graphics(w);
	app_set_font(wg, f);
	app_del_graphics(wg);

	pm = app_alloc(sizeof(PopupMenu));

	pm->lines = lines;
	pm->nitem = nitem;
	pm->lasti = i;
	pm->font = f;

	app_on_window_redraw(w, app_popup_draw_menu);
	app_on_window_mouse_move(w, app_popup_mouse_move);
	app_on_window_mouse_down(w, app_popup_mouse_down);
	app_on_window_key_down(w, app_popup_key_down);
	app_on_window_key_action(w, app_popup_key_action);
	app_set_window_data(w, pm);

	i = app_handle_popup_menu(w, prev_value);

	app_free(pm);
	app_del_window(w);
	return i;
}

#endif /* USE_POPUP_WINDOW */

static void app_drop_list_draw(Control *c, Graphics *g)
{
	Rect r;
	Rect textbox;
	Point p[3];
	const char *text;
	char **lines = c->extra;

	r = app_get_control_area(c);

	/* Draw the drop list's current item. */
	if (app_is_enabled(c))
		app_set_colour(g, app_get_control_foreground(c));
	else
		app_set_colour(g, DISABLED_ITEM);
	textbox = app_inset_rect(r,3);
	if (app_is_highlighted(c)) {
		textbox.x += 1;
		textbox.y += 1;
	}
	if (c->font)
		app_set_font(g, c->font);
	text = lines[c->value];
	app_draw_text(g, textbox, ALIGN_LEFT | VALIGN_CENTER,
		text, (int) strlen(text));

	/* Blank the triangle area */
	app_set_colour(g, app_get_control_background(c));
	app_fill_rect(g, rect(r.x+r.width-15,r.y+3,11,r.height-6));

	/* Draw a triangle to signify dropping the menu */
	app_set_colour(g, ENABLED_ITEM);
	p[0] = pt(r.x+r.width-5, r.y+r.height/2-2);
	p[1] = pt(p[0].x-7, p[0].y);
	p[2] = pt(p[1].x+3, p[1].y+4);
	app_fill_polygon(g, p, 3);

	/* Draw drop list border. */
	app_draw_rect(g, r);
	r = app_inset_rect(r, 1);

	/* Draw drop list shadowed border. */
	if (app_has_focus(c)) {
		app_set_colour(g, FOCUS_BORDER);
		app_draw_rect(g, r);
		r = app_inset_rect(r, 1);
	}
	if (app_is_highlighted(c))
		app_draw_shadow_rect(g, r, LOWER_RIGHT, UPPER_LEFT);
	else
		app_draw_shadow_rect(g, r, UPPER_LEFT, LOWER_RIGHT);
}

static void app_drop_list_mouse_down(Control *c, int buttons, Point p)
{
	Window *w;
	Font *f;
	int i;

	if (! app_is_enabled(c))
		return;
	w = app_parent_window(c);
	p = c->offset;	//!!
	p.x += c->area.width/2;
	p.y += c->area.height;

	f = c->font;
	if (! f)
		f = app_find_default_font(w->app);

	i = app_menu_hit(w, f, c->extra, c->value, buttons, p);
	if (i >= 0) {
		app_set_control_value(c, i);
		app_redraw_control(c);
		app_activate_control(c);
	}
}

int app_pop_up_list(Window *win, Font *f, char *lines[],
	int buttons, Point p)
{
	if (! f)
		f = app_new_font(win->app, "menufont", PLAIN, 16);
	if (! f)
		f = app_find_default_font(win->app);
	return app_menu_hit(win, f, lines, -1, buttons, p);
}

static Control *app_create_drop_list(Control *c, Rect r, char *lines[],
	ControlFunc fn)	//!!
{
	if (c == NULL)
		return NULL;

	c->value = 0;
	c->extra = lines;
	app_on_control_redraw(c, app_drop_list_draw);
	app_on_control_mouse_down(c, app_drop_list_mouse_down);
	app_on_control_action(c, fn);
	app_set_control_background(c, BACKGROUND);
	app_show_control(c);

	return c;
}

Control *app_new_drop_list(Window *win, Rect r, char *lines[],
	ControlFunc fn)
{
	return app_create_drop_list(app_new_control(win, r), r, lines, fn);
}

Control *app_add_drop_list(Control *parent, Rect r, char *lines[],
	ControlFunc fn)
{
	return app_create_drop_list(app_add_control(parent, r), r, lines, fn);
}

