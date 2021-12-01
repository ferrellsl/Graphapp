/*
 * Monty - a simple project editor.
 *
 * File: monty.h -- the full Monty header file.
 * Platform: Neutral  Version: 2.00  Date: 2001/08/08
 *
 * Version: 1.00  Changes: Original version by L. Patrick.
 * Version: 2.00  Changes: Updated to use the App package.
 */

/*  Copyright (C) 1998-2001 L. Patrick
 *
 *  This file is part of Monty, a simple file manager.
 */

/* Standard definitions. */

	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>
	#include <time.h>
	#include <ctype.h>

/* Graphics definitions. */

	#include <app.h>
	#include "array.h"

/* Built-in functions. */

	void	handle_about_box(App *app);
	void	ask_ok_str(App *app, char *title, char *fmt, char *str);
	void	hexdump(int num_files, char *files[]);
	void	copy_file(char *src, char *dest);
	void	split_file(App *app, char *src);
	void	join_files(App *app, char *src);

/* File conversion functions. */

	enum {
		CONVERT_TO_DOS	 = 1,
		CONVERT_TO_UNIX	 = 2,
		CONVERT_TO_MAC	 = 3,
		CONVERT_TO_LOWER = 8,
		CONVERT_TO_UPPER = 9
	};

	int 	convert_file(char *dir, char *file, int c);

/* Settings. */

	char *	get_file_type(char *filename);
	char *	get_editor_by_type(char *filetype);
	char *	get_editor(char *filename);
	char *	get_run(char *filename);
	void	load_settings(void);
	void	save_settings(void);
	void	edit_extensions(void);
	void	edit_types(void);

/* Progress bar functions. */

	Window *new_progress_window(App *app, char *name, char *str);
	void	update_progress_window(Window *w, int percentage);

/* String utility functions. */

	int 	first_word_ends_with(char *name, char *ending);
	int 	string_ends_with(char *name, char *ending);
	int 	string_starts_with(char *name, char *start);
	char *	add_strings(char *modify, char *const_string);


