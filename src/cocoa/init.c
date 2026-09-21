/*
 *  Initialisation of the App.
 *
 *  Platform: macOS.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

#include <limits.h>
#include <libgen.h>
#include <sys/stat.h>
#include <mach-o/dyld.h>

App *app_the_app = NULL;	/* for callbacks that carry no window */

/*
 *  Point APP_FONT_PATH at the portable fonts shipped with the program, so
 *  that it works from an .app bundle (Contents/Resources/fonts) or from a
 *  fonts folder next to the executable, without the user setting anything.
 *  A value the user has already set is left alone.
 */
static void app_find_bundled_fonts(void)
{
	char exe[PATH_MAX], real[PATH_MAX], cand[PATH_MAX + 32];
	uint32_t size = sizeof(exe);
	struct stat st;
	char *dir;
	int i;
	static const char *where[] = { "%s/../Resources/fonts", "%s/fonts" };

	if (getenv("APP_FONT_PATH") != NULL)
		return;
	if (_NSGetExecutablePath(exe, &size) != 0 || realpath(exe, real) == NULL)
		return;
	dir = dirname(real);
	for (i=0; i < 2; i++) {
		snprintf(cand, sizeof(cand), where[i], dir);
		if (stat(cand, &st) == 0 && S_ISDIR(st.st_mode)) {
			setenv("APP_FONT_PATH", cand, 0);
			return;
		}
	}
}

App *app_new_app(int argc, char *argv[])
{
	App *app;

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

	/* Deactivate X-Windows style mouse-based copy/paste */
	app->use_X_copy_paste = 0;

	app_find_bundled_fonts();

	/* Initialise string table and similar portable things. */
	app_app_initialise(app);

	/* Windows and Macs don't use sockets to communicate events. */
	app->socket_fd = -1;

	/* Start Cocoa. */
	if (! gab_init()) {
		app->gui_available = 0;
		return app;
	}
	app->gui_available = 1;
	app_extra(app)->started = 1;
	app_the_app = app;
	app_install_bridge_callbacks(app);

	/* Initialise the screen dimensions */
	app->screen_area.x = 0;
	app->screen_area.y = 0;
	gab_screen_info(&app->screen_area.width, &app->screen_area.height,
			&app->screen_mm.width, &app->screen_mm.height);
	app->screen_mm.x = 0;
	app->screen_mm.y = 0;

	return app;
}

void app_del_app(App *app)
{
	if (app_the_app == app)
		app_the_app = NULL;
	app_app_deinitialise(app);
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
	gab_beep();
}
