/*
 *  Delayed-deletion of windows and controls, for use in event loop.
 *
 *  Platform: Neutral.
 *
 *  Version: 3.51  2004/03/28  First release.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"

/*
 *  The problem is that deleting a window or control within
 *  a call-back may cause a crash in the event loop.
 *  The reason is that if I delete a window or control some
 *  of the pointers the event loop is following to find
 *  parents or siblings may become invalid dangling pointers.
 *
 *  To avoid this, a delayed deletion mechanism is used.
 *  Calling app_del_window or app_del_control will not
 *  immediately delete an object, but will instead move it
 *  onto a 'delayed deletion' list, for later disposal in
 *  the event loop. This avoids the problem by keeping the
 *  objects around long enough that their memory is still
 *  valid during all call-backs.
 */

/*
 *  Remember or forget a window within the list of deleted windows.
 */
int app_remember_deleted_window(App *app, Window *win)
{
	Window **newlist;
	int i, bytes;

	i = app->num_deleted_windows;
	bytes = (i+1) * sizeof(Window *);
	newlist = app_realloc(app->deleted_windows, bytes);
	if (newlist) {
		newlist[i] = win;
		app->deleted_windows = newlist;
		app->num_deleted_windows++;
		return 1;	/* success */
	}
	else
		return 0;	/* realloc failed */
}


/*
 *  Remember or forget a control within the list of deleted controls.
 */
int app_remember_deleted_control(App *app, Control *c)
{
	Control **newlist;
	int i, bytes;

	i = app->num_deleted_controls;
	bytes = (i+1) * sizeof(Control *);
	newlist = app_realloc(app->deleted_controls, bytes);
	if (newlist) {
		newlist[i] = c;
		app->deleted_controls = newlist;
		app->num_deleted_controls++;
		return 1;	/* success */
	}
	else
		return 0;	/* realloc failed */
}


/*
 *  Perform the second stage of deletion, which
 *  actually deletes windows and controls which have
 *  been put aside earlier for deletion.
 *  This function also removes them from the delayed
 *  deletion lists.
 */
void app_do_delayed_deletion(App *app)
{
	int i;

	app->deleting = 1;

	/* It is important that controls are deleted before windows. */
	if (app->num_deleted_controls > 0)
	{
		for (i=0; i < app->num_deleted_controls; i++)
			app_del_control(app->deleted_controls[i]);
		app->num_deleted_controls = 0;
		app_free(app->deleted_controls);
		app->deleted_controls = NULL;
	}

	if (app->num_deleted_windows > 0)
	{
		for (i=0; i < app->num_deleted_windows; i++)
			app_del_window(app->deleted_windows[i]);
		app->num_deleted_windows = 0;
		app_free(app->deleted_windows);
		app->deleted_windows = NULL;
	}

	app->deleting = 0;
}
