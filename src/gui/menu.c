/*
 *  Pull-down menus.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Cosmetic changes.
 *  Version: 3.11  2001/12/12  Menus can persist, and shortcuts work.
 *  Version: 3.15  2002/01/05  Menu items have fonts, colours, state.
 *  Version: 3.16  2002/01/06  Menus have fonts, colours.
 *  Version: 3.30  2002/08/25  Menus now look more three dimentional.
 *  Version: 3.37  2002/12/31  Now handles CONTROL bits for shortcuts.
 *  Version: 3.40  2003/03/07  More Windows-like look.
 *  Version: 3.41  2003/03/18  Exits menu shown mouse-loop if other event.
 *  Version: 3.44  2003/04/29  New menu font and look.
 *  Version: 3.45  2003/05/12  Menu shortcuts can use CONTROL and SHIFT.
 *  Version: 3.49  2003/08/24  Special display of some item shortcuts.
 *  Version: 3.50  2004/01/17  Menu now always appears below menubar.
 *  Version: 3.56  2005/08/09  Silenced some size_t conversion warnings.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "app.h"
#include "appgui.h"

static char ctrl[] = "Ctrl+";
static char shift[] = "Shift+";

#define CTRL_LENGTH (sizeof(ctrl))
#define SHIFT_LENGTH (sizeof(shift))

enum {
	MARGIN = 4,		/* outside to text */
	BORDER = 3,		/* outside to selection boxes */
	BORDER_SIZE = 2,	/* width of outlining border */
	VSPACING = 1,		/* extra spacing between lines of text */
	HSPACING = 12,		/* extra spacing between menu names */
	ICONSPACE = 14,		/* for small icons on left or right */
	VSEPSIZE = 6,		/* vertical gap for separator bars */
	FLASH = 2,		/* number of times to flash selection */
	FLASH_MILLISEC = 50	/* milliseconds between flashes */
};

static char * app_find_menu_item_shortcut_substitute(int ch)
{
	ch &= ~(CONTROL | SHIFT);

	switch (ch) {
		case ' ':	return "Space";
		case '\t':	return "Tab";
		case DEL:	return "Delete";
		case INS:	return "Insert";
		case HOME:	return "Home";
		case END:	return "End";
		case PGUP:	return "Page Up";
		case PGDN:	return "Page Down";
		case ESC:	return "Escape";
		case F1:	return "F1";
		case F2:	return "F2";
		case F3:	return "F3";
		case F4:	return "F4";
		case F5:	return "F5";
		case F6:	return "F6";
		case F7:	return "F7";
		case F8:	return "F8";
		case F9:	return "F9";
		case F10:	return "F10";
		default:	break;
	}
	return NULL;
}

static Font * app_find_menu_item_font(MenuItem *mi)
{
	if (mi->font)
		return mi->font;
	if (mi->submenu && mi->submenu->font)
		return mi->submenu->font;
	if (mi->parent && mi->parent->parent) {
		if (mi->parent->parent->font)
			return mi->parent->parent->font;
	}
	return NULL;
}

static Font * app_find_menu_font(Menu *m)
{
	if (m->font)
		return m->font;
	if (m->parent) {
		if (m->parent->font)
			return m->parent->font;
	}
	return NULL;
}

static char * app_find_menu_item_text(MenuItem *item)
{
	char *text = NULL;

	if (item->text)
		text = item->text;
	if (text == NULL)
		if (item->submenu)
			text = item->submenu->text;
	if (text == NULL)
		text = "-";
	return text;
}

static Rect app_menu_item_rect(Menu *menu, Rect r, int which)
{
	int i;
	MenuItem *item;
	char *text;
	Font *font;

	if ((which < 0) || (menu->num_items == 0))
		return rect(0, 0, 0, 0);
	r = app_inset_rect(r, MARGIN);

	for (i=0; i <= which; i++) {
		item = menu->items[i];
		text = app_find_menu_item_text(item);
		if ((text[0] == '-') && (text[1] == '\0'))
			r.height = VSEPSIZE;
		else {
			font = app_find_menu_item_font(item);
			r.height = font->height + VSPACING;
		}
		if (i != which)
			r.y += r.height;
	}

	return app_inset_rect(r, BORDER-MARGIN);
}

static int app_menu_item_top(Menu *menu, Rect r, int which)
{
	r = app_menu_item_rect(menu, r, which);
	return r.y;
}

static int app_menu_selection(Menu *menu, Rect r, Point p)
{
	int i;
	MenuItem *item;
	char *text;
	Font *font;

	r = app_inset_rect(r, MARGIN);
	if (! app_point_in_rect(p, r))
		return -1;
	p.y -= r.y;
	for (i=0; i < menu->num_items; i++) {
		item = menu->items[i];
		text = app_find_menu_item_text(item);
		if ((text[0] == '-') && (text[1] == '\0'))
			p.y -= VSEPSIZE;
		else {
			font = app_find_menu_item_font(item);
			p.y -= (font->height + VSPACING);
		}
		if (p.y < 0)
			return (item->state & ENABLED) ? i : -1;
	}
	return -1;
}

static int app_draw_menu_item(Graphics *wg, Rect menur,
		MenuItem *item, Rect r, int shortcutwidth, int selected)
{
	char *text;
	Font *font;
	Point p;
	Point tri[3];
	unsigned long ch;
	char buf[8];
	char shortcut[CTRL_LENGTH + SHIFT_LENGTH + 10];
	char * substitute;

	p.x = r.x + MARGIN - BORDER;
	p.y = r.y + MARGIN - BORDER;

	text = app_find_menu_item_text(item);

	/* draw a separator */
	if ((text[0] == '-') && (text[1] == '\0')) {
		r.height = VSEPSIZE;

		if (selected) {
			/* draw separator background */
			app_set_rgb(wg, ENABLED_ITEM);
			app_fill_rect(wg, r);
		}
		else {
			/* draw separator background */
			app_set_rgb(wg, MENU_BACKGROUND);
			app_fill_rect(wg, r);

			/* draw tick-mark fill */
			app_set_rgb(wg, MENU_LEFT_FILL);
			app_fill_rect(wg, rect(r.x, r.y,
				ICONSPACE, r.height+(MARGIN-BORDER)*2));
		}
		/* draw separator line and shadow */
		app_draw_shadow_rect(wg, rect(menur.x + BORDER, p.y + 2,
				menur.width - BORDER*2, 2),
				LOWER_RIGHT, UPPER_LEFT);
		return r.height;
	}

	font = app_find_menu_item_font(item);
	app_set_font(wg, font);
	r.height = font->height + VSPACING;

	/* fill background of menu item */
	if (selected)
		app_set_rgb(wg, MENU_HILIGHT);
	else
		app_set_rgb(wg, MENU_BACKGROUND);
	app_fill_rect(wg, rect(r.x, r.y,
			r.width, r.height+(MARGIN-BORDER)*2));

	/* draw tick-mark fill */
	if (! selected) {
		app_set_rgb(wg, MENU_LEFT_FILL);
		app_fill_rect(wg, rect(r.x, r.y,
				ICONSPACE, r.height+(MARGIN-BORDER)*2));
	}

	/* draw border */
	if (selected) {
		app_draw_shadow_rect(wg, rect(r.x, r.y,
			r.width, r.height+(MARGIN-BORDER)*2),
			MENU_ITEM_UPPER_BORDER,
			MENU_ITEM_LOWER_BORDER);
	}

	/* draw check mark */
	if (selected)
		app_set_rgb(wg, MENU_TEXT_HILIGHT);
	else
		app_set_rgb(wg, MENU_TEXT);
	if (item->state & CHECKED) {
		/* draw tick mark */
		int x = p.x;
		int y = p.y + 1;
		app_draw_line(wg, pt(x+3, y+11), pt(x+1, y+9));
		app_draw_line(wg, pt(x+3, y+10), pt(x+1, y+8));
		app_draw_line(wg, pt(x+4, y+10), pt(x+9, y+5));
		app_draw_line(wg, pt(x+4, y+ 9), pt(x+9, y+4));
	}
	/* draw menu item's text */
	if (item->state & ENABLED) {
		if (selected)
			app_set_rgb(wg, MENU_TEXT_HILIGHT);
		else
			app_set_rgb(wg, item->fg);
	}
	else {
		app_set_rgb(wg, DISABLED_ITEM);
	}
	app_draw_utf8(wg, pt(p.x+ICONSPACE,p.y), text, (int) strlen(text));
	if (item->shortcut) {
		/* draw the shortcut text */
		strcpy(shortcut, ctrl);
		if (item->shortcut & SHIFT)
			strcat(shortcut, shift);
		ch = item->shortcut & ~(CONTROL | SHIFT);
		substitute = app_find_menu_item_shortcut_substitute(ch);
		if (substitute)
			strcat(shortcut, substitute);
		else {
			app_unicode_char_to_utf8(ch, buf);
			strcat(shortcut, buf);
		}
		app_draw_utf8(wg,
			pt(menur.x+menur.width-shortcutwidth-MARGIN, p.y),
			shortcut, (int) strlen(shortcut));
	}
	if (selected)
		app_set_rgb(wg, MENU_TEXT_HILIGHT);
	else
		app_set_rgb(wg, MENU_TEXT);
	if (item->submenu) {
		/* draw submenu triangle */
		tri[0] = pt(menur.x+menur.width-MARGIN-8,p.y+3);
		tri[1] = pt(tri[0].x,   tri[0].y+10);
		tri[2] = pt(tri[1].x+5, tri[1].y-5);
		app_fill_polygon(wg, tri, 3);
	}
	return r.height;
}

static MenuItem *app_menu_bar_hit(Window *w, Graphics *wg,
	Rect maxrect, Menu *menu, Rect parentr, Region *active,
	int btn, int *btns, Point *xy, int *displaying)
{
	int i, nitem, maxwidth, lasti, shortcutwidth, is_mouse_event;
	Rect r, menur;
	Point p;
	Bitmap *b;
	MenuItem *item;
	char *text;
	Graphics *bg = NULL;
	Font *font;
	unsigned long ch;
	char buf[8];
	char *ss;	/* shortcut substitute string */

	/* find the maximum pixel width and height of the menu */
	r = rect(0,0,0,0);
	maxwidth = shortcutwidth = 0;
	for (nitem = 0; nitem < menu->num_items; nitem++) {
		item = menu->items[nitem];
		text = app_find_menu_item_text(item);
		font = app_find_menu_item_font(item);
		i = app_font_width(font, text, (int) strlen(text));
		i += ICONSPACE; /* for tick mark on left */
		i += ICONSPACE; /* for sub-menu arrow */
		if (i > maxwidth)
			maxwidth = i;
		r.width = maxwidth;
		if ((text[0] == '-') && (text[1] == '\0')) {
			r.height += VSEPSIZE;
			continue;
		}
		else
			r.height += font->height + VSPACING;
		if (item->shortcut) {
			i = app_font_width(font, ctrl, CTRL_LENGTH-1);
			if (item->shortcut & SHIFT)
				i += app_font_width(font, shift, SHIFT_LENGTH-1);
			ch = item->shortcut & ~(CONTROL | SHIFT);
			ss = app_find_menu_item_shortcut_substitute(ch);
			if (ss)
				i += app_font_width(font, ss, (int) strlen(ss));
			else {
				app_unicode_char_to_utf8(ch, buf);
				i += app_font_width(font, buf, (int) strlen(buf));
			}
			i += ICONSPACE; /* after text, before shortcut */
			if (i > shortcutwidth)
				shortcutwidth = i;
		}
	}
	maxwidth += shortcutwidth;
	r.width = maxwidth;
	if (menu->lasthit < 0 || menu->lasthit >= nitem)
		menu->lasthit = 0;

	/* determine the dimensions of the menu */
	if (maxwidth == 0)
		r = rect(0,0,0,0);
	else
		r = app_inset_rect(r, -MARGIN);
	r.x = xy->x;
	r.y = xy->y;

	/* determine where to display the menu */
	p = pt(0, 0);
	if (r.x+r.width > maxrect.x+maxrect.width)
		p.x = maxrect.x+maxrect.width-r.x-r.width;
	if (r.y+r.height > maxrect.y+maxrect.height)
		p.y = maxrect.y+maxrect.height-r.y-r.height;
	if (r.x < 0)
		p.x = 0-r.x;
	if (r.y < 0)
		p.y = 0-r.y;
	if (p.y < 0)
		p.y = 0;
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
		r.y += app_draw_menu_item(wg, menur, menu->items[i],
				r, shortcutwidth, 0);
	}

	/* highlight the selected menu item */
	app_draw_all(w->app);
	lasti = app_menu_selection(menu, menur, *xy);
	if (lasti >= 0) {
		r = app_menu_item_rect(menu, menur, lasti);
		app_draw_menu_item(wg, menur, menu->items[lasti],
			r, shortcutwidth, 1);
	}

	/* determine the currently active region */
	active = app_copy_region(active);
	app_union_region_with_rect(active, menur, active);

	/* process mouse events only */
	item = NULL;
	while (*displaying)
	{
		is_mouse_event = app_get_mouse_event(w->app, btns, xy);

		if (! is_mouse_event) {
			*displaying = 0;
			lasti = -1;
			item = NULL;
			break;
		}

		if (! (btn & *btns)) { /* mouse button up */
			if (*displaying == 1) {
				if (app_point_in_rect(*xy, parentr))
					*displaying = 2;
				else
					break;
			}
			else if (*displaying == 2)
				;
			else if (*displaying == 3) {
				*displaying = 0;
				break;
			}
		}
		else { /* mouse button down */
			if (*displaying == 2)
				*displaying = 3;
		}

		if ((! app_point_in_rect(*xy, menur)) &&
		    (! app_point_in_rect(*xy, parentr)) &&
		    (app_point_in_region(*xy, active)))
		{
			lasti = -1;
			item = NULL;
			break;
		}
		i = app_menu_selection(menu, menur, *xy);
		if (i == lasti)
			continue;

		if (lasti >= 0)
			app_draw_menu_item(wg, menur, menu->items[lasti],
				r, shortcutwidth, 0);
		if (i >= 0) {
			r = app_menu_item_rect(menu, menur, i);
			app_draw_menu_item(wg, menur, menu->items[i],
				r, shortcutwidth, 1);
		}

		lasti = i;
		if ((lasti >= 0) && (lasti < nitem)) {
			item = menu->items[lasti];
			if (item->submenu != NULL) {
				xy->x = menur.x + menur.width - 2;
				xy->y = app_menu_item_top(menu, menur, lasti) - 3;
				item = app_menu_bar_hit(w, wg,
					maxrect, item->submenu,
					app_inset_rect(r, 0-BORDER),
					active, btn,
					btns, xy, displaying);
				if (item != NULL) {
					lasti = -1;
					break;
				}
			}
		} else {
			item = NULL;
		}
	}

	/* menu item selected, flash the item */
	if ((lasti >= 0) && (lasti < nitem) &&
	    (item == menu->items[lasti]) && (item->submenu == NULL))
	{
		r = app_menu_item_rect(menu, menur, lasti);
		for (i=0; i < FLASH; i++) {
			app_delay(w->app, FLASH_MILLISEC);
			app_draw_menu_item(wg, menur, item,
				r, shortcutwidth, 0);
			app_draw_all(w->app);

			app_delay(w->app, FLASH_MILLISEC);
			app_draw_menu_item(wg, menur, item,
				r, shortcutwidth, 1);
			app_draw_all(w->app);
		}
	}

	/* tidy up */
	app_del_region(active);
	if (b != NULL) {
		app_copy_rect(wg, pt(menur.x,menur.y), bg,
			rect(0, 0, menur.width, menur.height));
		app_draw_all(w->app);
		app_del_graphics(bg);
		app_del_bitmap(b);
	}

	if (lasti >= 0)
		menu->lasthit = lasti;
	return item;
}

static int app_find_menu_bar_text_direction(MenuBar *menubar)
{
	return menubar->align;
}

static Rect app_menu_name_rect(MenuBar *menubar, int which)
{
	int i, len, width, align;
	Rect r;
	char *text;
	Font *font;

	if (which < 0)
		return rect(0, 0, 0, 0);
	r = app_get_control_area(menubar->ctrl);
	r = app_inset_rect(r, MARGIN);

	align = app_find_menu_bar_text_direction(menubar);
	if (align & RL_TB)
		r.x += r.width; /* start on right edge */
	r.width = 0;

	for (i=0; i < menubar->num_menus; i++)
	{
		text = menubar->menus[i]->text;
		len = (int) strlen(text);
		font = app_find_menu_font(menubar->menus[i]);
		width = app_font_width(font, text, len);
		if (align & RL_TB)
			r.x -= width; /* step left before */
		r.width = width;
		if (i == which)
			return r;
		if (align & RL_TB) {
			r.x -= HSPACING;
		}
		else {
			r.x += width; /* step right after */
			r.x += HSPACING;
		}
	}
	return rect(0, 0, 0, 0);
}

static int app_menu_bar_selection(MenuBar *menubar, Point p)
{
	int i, len, width, align;
	Rect r;
	char *text;

	r = app_get_control_area(menubar->ctrl);
	r = app_inset_rect(r, MARGIN);
	if (! app_point_in_rect(p, r))
		return -1;

	align = app_find_menu_bar_text_direction(menubar);
	if (align & RL_TB)
		r.x += r.width; /* start on right edge */
	r.width = 0;

	for (i=0; i < menubar->num_menus; i++)
	{
		text = menubar->menus[i]->text;
		len = (int) strlen(text);
		width = app_font_width(menubar->font, text, len);
		r.width = width;
		if (align & RL_TB)
			r.x -= width; /* step left before */
		if (app_point_in_rect(p, r))
			return i;
		if (align & RL_TB) {
			r.x -= HSPACING;
		}
		else {
			r.x += width; /* step right after */
			r.x += HSPACING;
		}
	}
	return -1;
}

static int app_draw_menu_name(Graphics *g, Menu *menu, Rect r,
		int align, int selected)
{
	char *text;
	int len;
	Font *font;
	int width;

	text = menu->text;
	len = (int) strlen(text);
	font = app_find_menu_font(menu);
	width = app_font_width(font, text, len);
	if (align & RL_TB)
		r.x -= width; /* step left then draw text */
	r.width = width;

	/* fill menu background */
	app_set_font(g, font);
	if (selected) {
		app_set_rgb(g, MENU_BAR_HILIGHT);
		app_fill_rect(g, rect(r.x-2,r.y-2,r.width+4,r.height+3));
	}
	else {
		app_set_rgb(g, MENU_BAR_BACKGROUND);
		app_fill_rect(g, rect(r.x-2,r.y-2,r.width+4,r.height+3));
	}

	/* draw menu name's text */
	app_set_rgb(g, menu->fg);
	app_draw_text(g, r, align, text, len);

	/* draw border if selected */
	if (selected) {
		app_draw_shadow_rect(g, rect(r.x-2,r.y-2,r.width+4,r.height+3),
				MENU_OUTER_BORDER, MENU_OUTER_BORDER);
	}

	/* step to next name */
	if (align & RL_TB) {
		r.x -= HSPACING;
	}
	else {
		r.x += width; /* step right after drawing */
		r.x += HSPACING;
	}
	return r.x;
}

static void app_menu_bar_mouse_down(Control *c, int buttons, Point xy)
{
	int i, btn, displaying, align, is_mouse_event;
	Window *w;
	MenuBar *menubar;
	Menu *menu;
	MenuItem *item;
	Graphics *g;
	Graphics *wg;
	Rect r;
	Rect maxrect;
	Region *active;
	Region *old;

	w = app_parent_window(c);
	menubar = c->extra;
	active = app_new_rect_region(c->area);
	btn = buttons;

	/* set up drawing so we can draw anywhere inside the window */
	wg = app_get_window_graphics(w);
	app_set_font(wg, menubar->font);
	old = wg->clip;
	wg->clip = NULL;
	maxrect = app_get_window_area(w);
	maxrect.y = menubar->ctrl->area.y + menubar->ctrl->area.height - 2;
	maxrect.height -= maxrect.y;

	align = app_find_menu_bar_text_direction(menubar);

	displaying = 1;

	while (displaying)
	{
		i = app_menu_bar_selection(menubar, xy);
		if (i < 0) {
			is_mouse_event = app_get_mouse_event(w->app, &buttons, &xy);

			if (! is_mouse_event) {
				displaying = 0;
				break;
			}

			if (! buttons) { /* mouse button up */
				if (displaying == 1)
					break;
				else if (displaying == 3) {
					displaying = 0;
					break;
				}
			}
			else { /* mouse button down */
				if (displaying == 2)
					displaying = 3;
			}

			continue;
		}

		menu = menubar->menus[i];

		/* invert menu name */
		r = app_menu_name_rect(menubar, i);
		g = app_get_control_graphics(c);
		app_draw_menu_name(g, menu, r, align, 1);
		app_del_graphics(g);

		/* show menu */
		xy.x = r.x - 2;
		xy.y = r.y + r.height;
		item = app_menu_bar_hit(w, wg, maxrect, menu,
				app_inset_rect(r,-2), active,
				btn, &buttons, &xy, &displaying);

		/* restore original name */
		g = app_get_control_graphics(c);
		app_draw_menu_name(g, menu, r, align, 0);
		app_del_graphics(g);

		/* call menu item's function, if any */
		if (item != NULL) {
			if (item->action)
				item->action(item);
			break;
		}
	}

	/* tidy up */
	wg->clip = old;
	app_del_graphics(wg);
	app_del_region(active);
}

static void app_menu_bar_draw(Control *c, Graphics *g)
{
	int i, align;
	Rect r;
	MenuBar *menubar;
	Menu *menu;

	r = app_get_control_area(c);
	menubar = c->extra;

	/* Draw the menubar's menu names. */
	app_set_font(g, menubar->font);
	if (app_is_enabled(c))
		app_set_colour(g, app_get_control_foreground(c));
	else
		app_set_colour(g, DISABLED_ITEM);

	align = app_find_menu_bar_text_direction(menubar);
	r = app_inset_rect(r, MARGIN);
	if (align & RL_TB)
		r.x += r.width; /* start on right edge */
	r.width = 0;

	for (i=0; i < menubar->num_menus; i++)
	{
		menu = menubar->menus[i];
		r.x = app_draw_menu_name(g, menu, r, align, 0);
	}

	/* Draw menubar border. */
	app_set_colour(g, app_get_control_foreground(c));
	r = app_get_control_area(c);
	app_draw_rect(g, rect(r.x-1, r.y-1, r.width+2, r.height+1));
}

static void app_menu_bar_del(Control *c)
{
	app_free(c->extra);
}

static int app_activate_menu_short_cut(Menu *m, unsigned long ch)
{
	int i;
	MenuItem *mi;

	for (i=0; i < m->num_items; i++) {
		mi = m->items[i];
		if (mi->submenu) {
			if (app_activate_menu_short_cut(mi->submenu, ch))
				return 1;
		}
		else if (! (mi->state & ENABLED)) {
			continue; /* ignore disabled menu items */
		}
		else if (mi->text && mi->text[0] == '-') {
			continue; /* ignore separators */
		}
		else if ((mi->shortcut | CONTROL) == ch) {
			if (mi->action) {
				mi->action(mi);
				return 1;
			}
		}
	}
	return 0;
}

int app_activate_menu_bar_short_cut(MenuBar *mb, unsigned long ch)
{
	int i;
	Menu *m;
	unsigned long modifiers;

	if (ch == 0)
		return 0;

	modifiers = (ch & (CONTROL | SHIFT));
	ch &= ~(CONTROL | SHIFT);
	if ((ch >= 'a') && (ch <= 'z'))
		ch = ch + 'A' - 'a';	/* Unicode => toupper dodgy */
	ch |= modifiers;

	for (i=0; i < mb->num_menus; i++) {
		m = mb->menus[i];
		if (app_activate_menu_short_cut(m, ch))
			return 1;
	}
	return 0;
}

MenuBar *app_new_menu_bar(Window *win)
{
	Control *c;
	MenuBar *menubar;
	Rect wr, mbr;

	mbr = rect(0, 0, win->area.width, 16+MARGIN*2);
	c = app_new_control(win, mbr);
	if (c == NULL)
		return NULL;

	wr = app_get_window_area(win);
	wr.height += mbr.height;
	app_size_window(win, wr);

	menubar  = app_zero_alloc(sizeof(MenuBar));
	if (menubar == NULL) {
		app_del_control(c);
		return NULL;
	}
	menubar->ctrl = c;
	menubar->font = app_new_font(win->app, "menufont", PLAIN, 16);
	if (! menubar->font)
		menubar->font = app_find_default_font(win->app);
	menubar->align = ALIGN_LEFT | VALIGN_CENTER | LR_TB;
	c->extra = menubar;
	win->menubar = menubar;

	app_on_control_deletion(c, app_menu_bar_del);
	app_on_control_redraw(c, app_menu_bar_draw);
	app_on_control_mouse_down(c, app_menu_bar_mouse_down);
	app_set_control_foreground(c, ENABLED_ITEM);
	app_set_control_background(c, MENU_BAR_BACKGROUND);
	app_show_control(c);

	return menubar;
}

void app_del_menu_bar(MenuBar *menubar)
{
	app_del_control(menubar->ctrl);
}

Menu *app_new_menu(MenuBar *menubar, const char *name)
{
	int num;
	Menu *menu;
	Menu **list;

	menu = app_zero_alloc(sizeof(Menu));
	if (menu == NULL)
		return NULL;
	num = menubar->num_menus;
	list = app_realloc(menubar->menus, (num+1) * sizeof(Menu *));
	if (list == NULL) {
		app_free(menu);
		return NULL;
	}
	list[num] = menu;
	menubar->menus = list;
	menubar->num_menus = num+1;
	menu->parent = menubar;
	menu->text = app_copy_string(name);
	return menu;
}

MenuItem *app_new_menu_item(Menu *menu, const char *name,
	unsigned long key, MenuAction fn)
{
	int num;
	MenuItem *mi;
	MenuItem **list;
	unsigned long modifiers;

	mi = app_zero_alloc(sizeof(MenuItem));
	if (mi == NULL)
		return NULL;
	num = menu->num_items;
	list = app_realloc(menu->items, (num+1) * sizeof(MenuItem *));
	if (list == NULL) {
		app_free(mi);
		return NULL;
	}
	list[num] = mi;
	menu->items = list;
	menu->num_items = num+1;
	mi->parent = menu;
	if (name) {
		mi->text = app_copy_string(name);
		if ((name[0] != '-') || (name[1] != '\0'))
			mi->state = ENABLED;
	}
	modifiers = (key & SHIFT) | CONTROL;
	key &= ~(CONTROL | SHIFT);
	if (! key)
		modifiers = 0UL;
	else if ((key >= 'a') && (key <= 'z'))
		key = key + 'A' - 'a'; /* Unicode => toupper dodgy */
	mi->shortcut = key | modifiers;
	mi->action = fn;
	return mi;
}

Menu *app_new_sub_menu(Menu *parent, const char *name)
{
	MenuItem *mi;
	Menu *menu;

	menu = app_zero_alloc(sizeof(Menu));
	if (menu == NULL)
		return NULL;
	mi = app_new_menu_item(parent, NULL, 0, NULL);
	if (mi == NULL) {
		app_free(menu);
		return NULL;
	}
	mi->submenu = menu;
	mi->state = ENABLED;
	menu->parent = parent->parent;
	menu->text = app_copy_string(name);
	return menu;
}

static void app_remove_menu_from_menu_bar(MenuBar *parent, Menu *menu)
{
	int i, num, size;
	Menu **list;

	num = parent->num_menus;
	list = parent->menus;
	for (i=0; i < num; i++) {
		if (list[i] == menu)
			break;
	}
	if (i < num-1) {
		size = (num-1-i) * sizeof(Menu *);
		memmove(&list[i], &list[i+1], size);
	}
	if (i < num) {
		list = app_realloc(list, (num-1)*sizeof(Menu *));
		num -= 1;
	}
	parent->menus = list;
	parent->num_menus = num;
}

void app_del_menu_item(MenuItem *mi)
{
	int i, num, size;
	Menu *parent;
	MenuItem **list;
	Menu *menu;

	parent = mi->parent;
	num = parent->num_items;
	list = parent->items;
	for (i=0; i < num; i++) {
		if (list[i] == mi)
			break;
	}
	if (i < num-1) {
		size = (num-1-i) * sizeof(MenuItem *);
		memmove(&list[i], &list[i+1], size);
	}
	if (i < num) {
		list = app_realloc(list, (num-1)*sizeof(MenuItem *));
		num -= 1;
	}
	parent->items = list;
	parent->num_items = num;
	if (mi->submenu) {
		menu = mi->submenu;
		for (i=menu->num_items-1; i >= 0; i--)
			app_del_menu_item(menu->items[i]);
		app_free(menu);
	}
	app_free(mi);
}

void app_del_menu(Menu *menu)
{
	int i;

	if (menu->parent)
		app_remove_menu_from_menu_bar(menu->parent, menu);
	for (i=menu->num_items-1; i >= 0; i--)
		app_del_menu_item(menu->items[i]);
	app_free(menu);
}

void app_check_menu_item(MenuItem *mi)
{
	mi->state |= CHECKED;
}

void app_uncheck_menu_item(MenuItem *mi)
{
	mi->state &= ~CHECKED;
}

int app_menu_item_is_checked(MenuItem *mi)
{
	return (mi->state & CHECKED) ? 1 : 0;
}

void app_enable_menu_item(MenuItem *mi)
{
	mi->state |= ENABLED;
}

void app_disable_menu_item(MenuItem *mi)
{
	mi->state &= ~ENABLED;
}

int app_menu_item_is_enabled(MenuItem *mi)
{
	return (mi->state & ENABLED) ? 1 : 0;
}

void app_set_menu_item_value(MenuItem *mi, int value)
{
	mi->value = value;
}

int app_get_menu_item_value(MenuItem *mi)
{
	return mi->value;
}

void app_set_menu_item_foreground(MenuItem *mi, Colour col)
{
	mi->fg = col;
}

Colour app_get_menu_item_foreground(MenuItem *mi)
{
	return mi->fg;
}

void app_set_menu_item_font(MenuItem *mi, Font *font)
{
	mi->font = font;
}

Font *app_get_menu_item_font(MenuItem *mi)
{
	return mi->font;
}

void app_set_menu_foreground(Menu *m, Colour col)
{
	m->fg = col;
}

Colour app_get_menu_foreground(Menu *m)
{
	return m->fg;
}

void app_set_menu_font(Menu *m, Font *font)
{
	m->font = font;
}

Font *app_get_menu_font(Menu *m)
{
	return m->font;
}

void app_set_menu_bar_font(MenuBar *mb, Font *font)
{
	mb->font = font;
}

Font *app_get_menu_bar_font(MenuBar *mb)
{
	return mb->font;
}

