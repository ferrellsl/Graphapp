/*
 *  App utility functions:
 *
 *  Platform: Neutral
 *
 *  Version: 3.35  2002/12/23  First release.
 *  Version: 3.36  2002/12/30  Fixed crash if key does not exist.
 *  Version: 3.41  2003/03/18  Fixed a typo.
 *  Version: 3.43  2003/04/25  Now deletes all cursors when deinitialised.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced some size_t conversion warnings.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"

/*
 *  Complete initialising the App structure.
 */
void app_app_initialise(App *app)
{
	app->string_table = app_new_string_table();

	app_set_string(app, "Yes",	"Yes");
	app_set_string(app, "No",	"No");
	app_set_string(app, "Cancel",	"Cancel");
	app_set_string(app, "OK",	"OK");
	app_set_string(app, "Quit",	"Quit");
	app_set_string(app, "Create",	"Create");
	app_set_string(app, "Overwrite","Overwrite");
	app_set_string(app, "Error",	"Error");
	app_set_string(app, "File:",	"File:");
	app_set_string(app, "Files:",	"Files:");
	app_set_string(app, "Path:",	"Path:");
	app_set_string(app, "Folder:",	"Folder:");
	app_set_string(app, "Folders:",	"Folders:");

	app_set_string(app, "Create File", "Create File");
	app_set_string(app, "Error: Create File", "Error: Create File");
	app_set_string(app, "Create file?",
		"That file does not exist. Create the file?");
	app_set_string(app, "Already a folder!",
		"That file name is already in use as a folder.");

	app_set_string(app, "Overwrite File", "Overwrite File");
	app_set_string(app, "Error: Save File", "Error: Save File");
	app_set_string(app, "Overwrite file?",
		"That file already exists. Overwrite the file?");
}

/*
 *  Delete whatever was initialised above, and whatever can be
 *  portably deleted.
 */
void app_app_deinitialise(App *app)
{
	int i;

	app_hide_all_windows(app);
	app_del_all_windows(app);
	for (i = app->num_cursors - 1; i >= 0; i--)
		app_del_cursor(app->cursors[i]);
	for (i = app->num_fonts - 1; i >= 0; i--)
		app_del_font(app->fonts[i]);
	while (app->num_timers)
		app_del_timer(app->timers[0]);
	app_free(app->program_name);
	app_free(app->open_folder);
	app_free(app->save_folder);
	app_del_string_table(app->string_table);
	app->string_table = NULL;
}

/*
 *  We just define two functions here, for using strings
 *  stored with the App structure. These functions are used
 *  by the dialogs, amongst others, to find the appropriate
 *  string to display.
 */

void app_set_string(App *app, const char *key, char *value)
{
	StringNode *n;

	n = app_insert_node(app->string_table, key, (int) strlen(key));
	n->value = value;
}

char * app_get_string(App *app, const char *key)
{
	StringNode *n;

	n = app_locate_node(app->string_table, key, (int) strlen(key));
	if (n == NULL)
		return NULL;
	return n->value;
}
