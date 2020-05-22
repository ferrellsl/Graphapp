/*
 *  Tool tips.	//!!
 *
 *  Platform: Neutral
 *
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

#define TIP_FGCOLOR		BLACK
#define TIP_BGCOLOR		YELLOW
#define TIP_SHOW_INTERVAL	1000	/* milliseconds to Show */
#define TIP_HIDE_INTERVAL	3000	/* milliseconds to Hide */

typedef struct TipsControl  TipsControl;

struct TipsControl {
	Timer *timer;
	Window *w;	/* timer triggered window */
	Control *c;	/* timer triggered control */
	int displaying;
	Rect area;

	int num_tips;
	void **tips;		/* list of texts or tip handlers */
	Control **owners;	/* list of tip owner controls */
};


static int app_find_tip_index(TipsControl *tc, Control *c)
{
	int i;

	for (i=0; i<tc->num_tips; i++)
		if (tc->owners[i] == c)
			return i;
	return -1;
}

static void app_draw_tip(Graphics *wg, Font *f, Rect *tipr, const char *text)
{
	int len, maxwidth, maxheight;
	Rect r;

	len = strlen(text);

	/* find the maximum pixel width */
	maxwidth = app_text_width(f, 9999, text, len) + 16;

	/* find the maximum pixel height */
	maxheight = app_text_height(f, 9999, text, len);
	maxheight += f->height/2;

	/* determine the dimensions and position of the tip */
	r = *tipr;
	if (r.width > maxwidth) {
		int x = r.width - maxwidth;
		if (r.x > x)
			r.x = x;
		r.width = maxwidth;
	} else
		r.x = 0;
	if (r.height > maxheight) {
		int y = r.height - maxheight;
		if (r.y > y)
			r.y = y;
		r.height = maxheight;
	} else
		r.y = 0;
	*tipr = r;	/* invalidated area of window */

	/* draw the tip */
	app_set_rgb(wg, TIP_BGCOLOR);
	app_fill_rect(wg, r);
	app_set_rgb(wg, TIP_FGCOLOR);
	app_draw_rect(wg, r);
	app_draw_text(wg, r, ALIGN_CENTER | VALIGN_CENTER, text, len);
}

static void app_show_tip(TipsControl *tc, Point p)
{
	int i;
	const char *text;
	Region *old, *old_visible;
	Window *win;
	Graphics *wg;
	Font *f;

	tc->displaying = 1;

	/* set up drawing so we can draw anywhere inside the window */
	win = tc->w;
	wg = app_get_window_graphics(win);
	f = app_find_default_font(win->app);
	app_set_font(wg, f);
	old = wg->clip;
	wg->clip = NULL;
	old_visible = win->visible;
	win->visible = NULL;

	/* get the text of tip */
	i = app_find_tip_index(tc, tc->c);
	if (i == -1)
		text = tc->c->text;
	else {
		void *p = tc->tips[i];
		if (tc->c->state & TIP_HANDLER)
			text = ((TipFunc) p)(tc->c, wg, &tc->area);
		else
			text = p;
	}

	/* if tip not handled by owner then draw the text */
	if (text) {
		tc->area = app_get_window_area(win);
		tc->area.x = p.x;
		tc->area.y = p.y + 16;
		app_draw_tip(wg, f, &tc->area, text);
	}

	win->visible = old_visible;
	wg->clip = old;
	app_del_graphics(wg);
}

static void app_hide_tip(TipsControl *tc, Window *win)
{
	tc->displaying = 0;

	/* Is window alive and visible? */
	if (win != tc->w) {
		App *app = tc->timer->app;
		int i;

		for (i=app->num_windows-1; i >= 0; i--) {
			win = app->windows[i];
			if (win == tc->w) {
				if (! (win->state & VISIBLE))
					return;
				break;
			}
		}
		if (i < 0)
			return;
	}
	app_redraw_rect(win, tc->area);
}

static void app_tip_timer(Timer *t)
{
	Window *win;
	TipsControl *tc;
	App *app;

	app = t->app;
	tc = app->tip;
	if (! tc)
		return;

	/*
	 * Is the windows under mouse cursor and tip are same?
	 * (Window of tip may be already deleted.)
	 */
	win = app_get_window_under_cursor(app);

	if (tc->displaying)
		app_hide_tip(tc, win);
	else if (win == tc->w) {
		Point p;

		p = app_get_cursor_position(app);
		p.x -= win->area.x;
		p.y -= win->area.y;

		if (app_locate_control(win, p) == tc->c) {
			app_show_tip(tc, p);
			tc->timer->milliseconds = TIP_HIDE_INTERVAL;
			return;
		}
	}
	app_del_timer(tc->timer);
	tc->timer = NULL;
}

void app_handle_tip(Control *c)
{
	TipsControl *tc;
	Window *w;
	App *app;

	w = app_parent_window(c);
	app = w->app;
	tc = app->tip;
	if (! tc)
		return;

	if (tc->displaying)
		app_hide_tip(tc, w);
	if (tc->timer)
		app_reset_timer(tc->timer, TIP_SHOW_INTERVAL);
	else
		tc->timer = app_new_timer(app, app_tip_timer, TIP_SHOW_INTERVAL);

	tc->w = w;
	tc->c = c;
}

static int app_add_tip(TipsControl *tc, Control *c, long mode, void *p)
{
	void **list;
	int num;

	num = tc->num_tips + 1;

	list = app_realloc(tc->owners, num * sizeof(Control *));
	if (list == NULL)
		return 0;
	tc->owners = (Control **) list;
	tc->owners[num-1] = c;

	list = app_realloc(tc->tips, num * sizeof(void *));
	if (list == NULL)
		return 0;
	tc->tips = list;

	if ((mode == TIP_TEXT) && (p != NULL))
		p = app_copy_string(p);
	tc->tips[num-1] = p;

	tc->num_tips++;

	return 1;
}

static void app_del_tip(TipsControl *tc, Control *c)
{
	void **list;
	int i, num;

	i = app_find_tip_index(tc, c);
	if (i == -1)
		return;

	if (c->state & TIP_TEXT) {
		char *text = tc->tips[i];
		if (text)
			app_del_string(text);
	}

	num = --tc->num_tips;
	if (num > 0) {
		if (i != num) {
			tc->tips[i] = tc->tips[num];
			tc->owners[i] = tc->owners[num];
		}

		list = app_realloc(tc->tips, num * sizeof(void *));
		if (list != NULL)
			tc->tips = list;

		list = app_realloc(tc->owners, num * sizeof(Control *));
		if (list != NULL)
			tc->owners = (Control **) list;
	} else {
		app_free(tc->tips);
		tc->tips = NULL;

		app_free(tc->owners);
		tc->owners = NULL;
	}
}

static int app_update_tip(TipsControl *tc, Control *c, long mode, void *p)
{
	int i;

	i = app_find_tip_index(tc, c);
	if (i == -1)
		return app_add_tip(tc, c, mode, p);

	if (c->state & TIP_TEXT) {
		char *text = tc->tips[i];
		if (text)
			app_del_string(text);
	}

	if ((mode == TIP_TEXT) && (p != NULL))
		p = app_copy_string(p);
	tc->tips[i] = p;

	return 1;
}

/*
 * Control may have tip text or handler.
 * If mode != 0 and p == NULL then show c->text,
 * else store pointer in the extra array.
 */
int app_set_control_tip(Control *c, long mode, void *p)
{
	long tip_flags;
	TipsControl *tc;
	App *app;
	int result = 1;

	app = (app_parent_window(c))->app;
	tc = app->tip;

	tip_flags = c->state & TIP_MASK;
	if (mode == 0L) {
		if ((tip_flags != 0L) && (tc != NULL)) {
			app_del_tip(tc, c);
			c->state &= ~ TIP_MASK;
		}
		return 1;
	}

	if (tc == NULL) {
		tc = app_zero_alloc(sizeof(TipsControl));
		if (tc == NULL)
			return 0;
		app->tip = tc;
	}

	if (tip_flags != 0L) {
		if (p != NULL)
			result = app_update_tip(tc, c, mode, p);
		else
			app_del_tip(tc, c);
	}
	else if (p != NULL)
		result = app_add_tip(tc, c, mode, p);

	if (result) {
		c->state &= ~ TIP_MASK;
		c->state |= mode;
	}
	return result;
}
