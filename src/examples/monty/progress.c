/*
 * Monty - a simple project editor.
 *
 * File: progress.c -- a progress indicator window.
 * Platform: Neutral  Version: 2.00  Date: 2001/08/08
 *
 * Version: 1.00  Changes: Original version by L. Patrick.
 */

/*  Copyright (C) 1998-2001 L. Patrick
 *
 *  This file is part of Monty, a simple project editor.
 */


typedef struct ProgressData  ProgressData;

struct ProgressData {
	Control *display;
	Control *percent;
	long percentage;
};


/*
 *  A progress indicator window:
 */
static void draw_progress_window(Window *w, Graphics *g)
{
	long value;
	Rect r, r1, r2;
	ProgressData *pd;

	pd = app_get_window_data(w);

	r = app_get_window_area(w);
	r = rect(r.x+10,r.height-26,r.width-20,16);

	value = pd->percentage;

	r1 = rect(r.x,r.y,r.width*value/100,r.height);
	r2 = rect(r1.x+r1.width,r.y,r.width-r1.width,r.height);

	app_set_colour(g, BLACK); app_draw_rect(g, app_inset_rect(r,-1));
	app_set_colour(g, BLUE);  app_fill_rect(g, r1);
	app_set_colour(g, WHITE); app_fill_rect(g, r2);
	app_set_colour(g, BLACK);
}

void update_progress_window(Window *w, int percentage)
{
	char buffer[20];
	Control *percent;
	ProgressData *pd;

	pd = app_get_window_data(w);

	if (percentage == pd->percentage)
		return;
	pd = app_get_window_data(w);
	percent = pd->percent;
	if (percentage < 0)   percentage = 0;
	if (percentage > 100) percentage = 100;
	pd->percentage = percentage;
	sprintf(buffer, "%d%%", percentage);
	app_set_control_text(percent, buffer);
	app_draw_window(w);
	while (app_peek_event(w->app))
		app_do_event(w->app);
}

Window *new_progress_window(App *app, char *name, char *str)
{
	Window *progress;
	ProgressData *pd;
	Font *f;
	Rect r1, r2, r3;
	int h, w = 500;

	f = app_find_default_font(app);
	h = app_font_height(f) * 3 / 2;
	r1 = rect(10,10,w,h*3);
	r2 = rect(20,20,r1.width+20,r1.height+56+h);
	r3 = rect((w-60)/2,20+h*3,60,h);

	progress = app_new_window(app, r2, name, MODAL+TITLEBAR+CENTERED);
	pd = app_zero_alloc(sizeof(ProgressData));
	app_set_window_data(progress, pd);
	pd->display = app_new_label(progress, r1, str, ALIGN_LEFT);
	pd->percent = app_new_label(progress, r3, "0%", ALIGN_CENTER);
	pd->percentage = 0;
	app_on_window_redraw(progress, draw_progress_window);
	app_show_window(progress);
	return progress;
}
