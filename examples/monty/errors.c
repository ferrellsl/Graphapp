/*
 * Monty - a simple project editor.
 *
 * File: errors.c -- a GraphApp error reporting utility.
 * Platform: Neutral  Version: 2.00  Date: 1998/02/02
 *
 * Version: 1.00  Changes: Original version by L. Patrick.
 */

/* Copyright (C) 1998 L. Patrick

   This file is part of Monty, a simple project editor.
*/


/*
 *  Error reporting utility:
 */

void ask_ok_str(App *app, char *title, char *fmt, char *str)
{
	char *buffer = NULL;
	int length;

	length = strlen(fmt) + strlen(str) + 1;
	buffer = array(length, char);

	sprintf(buffer, fmt, str);
	app_ask_ok(app, title, buffer);
	discard(buffer);
}
