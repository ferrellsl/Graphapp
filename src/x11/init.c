/*
 *  Initialisation routines.
 *
 *  Platform: X-Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.10  2001/12/01  App structure now remembers the program name.
 *  Version: 3.17  2002/01/08  Added support for timers.
 *  Version: 3.25  2002/07/07  Added file dialog persistence.
 *  Version: 3.35  2002/12/23  Added portable App initialisation.
 *  Version: 3.45  2003/05/12  Supports X-Windows mouse copy and paste.
 *  Version: 3.48  2003/06/05  Better support for non-graphical Apps.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.57  2005/08/16  Reports X11 socket file descriptor in App.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

/*
 *  Routines:
 */

static int app_x_error_handler(Display *disp, XErrorEvent *err)
{
	char msg[256];

	if ((err->error_code == BadAlloc) &&
		(err->request_code == X_CreatePixmap))
	{
		/* XCreatePixmap errors are allowed; we check for
		   them separately in the app_new_bitmap function
		   so here we just ignore any X error which occurs
		   prior to that check. */
	}
	else {
		XGetErrorText(disp, err->error_code, msg, sizeof(msg));
		fprintf(stderr, "Error detected:\n");
		fprintf(stderr, "  %s\n", msg);
		fprintf(stderr, "  Protocol request:   %d\n",
			(int) err->request_code);
		fprintf(stderr, "  Minor request code: %d\n",
			(int) err->minor_code);
		fprintf(stderr, "  Resource ID:        0x%lx\n",
			(unsigned long) err->resourceid);
		exit(1);
	}
	return 0;
}

static int app_x_io_error_handler(Display *disp)
{
	return 0;
}

App *app_new_app(int argc, char *argv[])
{
	App *app;
	Display *disp;
	char *display_name;

	/* Create the App structure */
	app = app_zero_alloc(sizeof(struct App));
	if (app == NULL)
		return NULL;
	app->visible_windows = 0;
	app->extra = app_zero_alloc(sizeof(struct AppExtra));
	if (app->extra == NULL) {
		app_free(app);
		return NULL;
	}

	/* Copy the program name from argv[0] */
	if (argv && argv[0]) {
		if (argv[0][0] != '/') {
			app->program_name = app_form_file_path(
					app_current_folder(), argv[0]);
		}
		else {
			app->program_name = app_form_file_path("", argv[0]);
		}
		app->has_resources = app_file_has_resources(app->program_name);
	}
	else {
		app->program_name = app_copy_string("");
		app->has_resources = 0; /* no resources, since no name */
	}

	/* Activate X-Windows style mouse-based copy and paste */
	app->use_X_copy_paste = 1;

	/* Initialise string table and similar portable things. */
	app_app_initialise(app);

	/* Is the GUI available? */
	display_name = getenv("DISPLAY");
	if (display_name == NULL) {
		app->gui_available = 0;
		return app;
	}

	/* The GUI should be available, excellent */
	app->gui_available = 1;
	XSetErrorHandler(app_x_error_handler);
	XSetIOErrorHandler(app_x_io_error_handler);
	app_extra(app)->display = disp = XOpenDisplay(display_name);

	/* Whoops, the display variable (or something else) is wrong */
	if (disp == NULL) {
		app->gui_available = 0;
		return app;
	}

	/* Get the X11 socket file descriptor. */
	app->socket_fd = ConnectionNumber(disp);

	/* Initialise the screen dimensions */
	app->screen_area.x = 0;
	app->screen_area.y = 0;
	app->screen_area.width  = DisplayWidth(disp, DefaultScreen(disp));
	app->screen_area.height = DisplayHeight(disp, DefaultScreen(disp));

	app->screen_mm.x = 0;
	app->screen_mm.y = 0;
	app->screen_mm.width  = DisplayWidthMM(disp, DefaultScreen(disp));
	app->screen_mm.height = DisplayHeightMM(disp, DefaultScreen(disp));

	return app;
}

void app_del_app(App *app)
{
	app_app_deinitialise(app);
	if (app_extra(app)->display)
		XCloseDisplay(app_extra(app)->display);
	app_free(app_extra(app));
	app_free(app);
}

int app_exec(App *app, const char *cmd)
{
	if (system(cmd) == 0)
		return 1;
	return 0;
}

void app_beep(App *app)
{
	XBell(app_extra(app)->display, 0);
}
