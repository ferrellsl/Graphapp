/*
 *  Clipboard (text only).
 *
 *  Platform: macOS (NSPasteboard, via the bridge).
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

int app_set_clipboard_text(App *app, const char *text)
{
	return gab_clipboard_set(text);
}

/*
 *  The caller frees the result with app_free, so copy it onto the
 *  App allocator.
 */
char *app_get_clipboard_text(App *app)
{
	char *text, *copy;

	text = gab_clipboard_get();
	if (text == NULL)
		return NULL;
	copy = app_copy_string(text);
	free(text);
	return copy;
}
