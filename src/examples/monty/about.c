/*
 * Monty - a simple project editor.
 *
 * File: about.c -- an about box function.
 * Platform: Neutral  Version: 3.00  Date: 2002/08/02
 *
 * Version: 1.00  Changes: Original version by L. Patrick.
 */

/*  Copyright (C) 1998-2001 L. Patrick
 *
 *  This file is part of Monty, a simple project editor.
 */


static Window *about_box = NULL;

static char * about_string =
	"Monty\n\nversion 3.0 (2002/08/02)\n\nby Loki";

static void hide_about_box(Control *c)
{
	app_hide_window(app_parent_window(c));
}

static Window *create_about_box(App *app)
{
	Font *f = app_find_default_font(app);
	int h   = app_font_height(f) * 3 / 2;
	Rect r1 = rect(160,120,280,180);
	Rect r2 = rect(10,10,r1.width-20,r1.height-40-h);
	Rect r3 = rect((r1.width-80)/2,r1.height-20-h,80,h);

	Window *w = app_new_window(app, r1, "About Monty...",
			TITLEBAR + MODAL + CLOSEBOX + CENTERED);
	Control *l = app_new_label(w, r2, about_string,
			ALIGN_CENTER + VALIGN_CENTER);
	Control *b = app_new_button(w, r3, "Okay", hide_about_box);

	return w;
}

void handle_about_box(App *app)
{
	if (about_box == NULL)
		about_box = create_about_box(app);
	app_show_window(about_box);
}

