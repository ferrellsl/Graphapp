/*
 * Monty - a simple project editor.
 *
 * File: copyfile.c -- a file copying utility.
 * Platform: Neutral  Version: 2.00  Date: 1998/02/02
 *
 * Version: 1.00  Changes: Original version by L. Patrick.
 */

/* Copyright (C) 1998 L. Patrick

   This file is part of Monty, a simple project editor.
*/


/*
 *  The file copying function:
 */

static char * file_copy_string (char *src, char *dest)
{
	char *str = NULL;

	str = add_strings(NULL, "Source:       ");
	str = add_strings(str,  src);
	str = add_strings(str,  "\n");
	str = add_strings(str,  "Destination:  ");
	str = add_strings(str,  dest);
	str = add_strings(str,  "\n");

	return str;
}

void copy_file(char *src, char *dest)
{
	int ch;
	FILE *f1;
	FILE *f2;
	long BLOCK_SIZE = 1024*16;
	char *block = NULL;
	long size, i, total = 0L;
	char *message;
	Window *p;
	App *app;

	app = app_new_app(0,0);

	if ((f1 = app_open_file(src, "rb")) == NULL) {
		ask_ok_str(app, "Error",
			"Couldn't open the file \"%s\" for reading.", src);
		return;
	}
	if ((f2 = app_open_file(dest, "wb")) == NULL) {
		ask_ok_str(app, "Error",
			"Couldn't open the file \"%s\" for writing.", dest);
		return;
	}

	size = app_file_size(src);
	if (size >= 1024)
		block = array(BLOCK_SIZE, char);

	if (block == NULL) {
		/* copy file char by char */
		while ((ch = getc(f1)) != EOF)
			putc(ch, f2);
	} else {
		/* copy file in 16K blocks */
		message = file_copy_string(src, dest);
		p = new_progress_window(app, "File Copy", message);
		while ((i = fread(block, 1, BLOCK_SIZE, f1)) != 0) {
			fwrite(block, 1, i, f2);
			total += i;
			update_progress_window(p, total*100/size);
		}
		discard(block);
		app_del_window(p);
		app_del_string(message);
	}

	app_close_file(f1);
	app_close_file(f2);

	app_del_app(app);
}
