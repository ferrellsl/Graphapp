/*
 * Monty - a simple project editor.
 *
 * File: convert.c -- convert files.
 * Platform: Neutral  Version: 2.00  Date: 2001/08/08
 *
 * Version: 1.00  Changes: Original version by L. Patrick.
 * Version: 1.10  Changes: Only files with \0 or \255 are binary now.
 */

/*  Copyright (C) 1998-2001 L. Patrick
 *
 *  This file is part of Monty, a simple project editor.
 */

/*
 *  Some variables used in conversion.
 */
	static char * CONVERT_TMP = "MontyCnv.tmp";

	enum {
		LF = 10,
		CR = 13
	};

/*
 *  Functions to convert text files.
 */

/*
 *  Conversion functions returns 0 on failure, 1 otherwise.
 */
static int convert_to_dos(FILE *fin, FILE *fout)
{
	int c, prev = 'a';

	while ((c = fgetc(fin)) != EOF)
	{
		/* if file is non-textual, abort */
		if ((c == '\0') || (c > 254))
			return 0;

		/* Carriage returns occur in Mac and DOS files */
		if (c == CR) {
			fputc(CR, fout);
			fputc(LF, fout);
		}
		/* Linefeeds happen in DOS and Unix files */
		else if (c == LF) {
			/* don't output extra CR-LF if DOS file */
			if (prev != CR) {
				fputc(CR, fout);
				fputc(LF, fout);
			}
		}
		else	/* normal output of chars */
			fputc(c, fout);

		prev = c;
	}

	return 1;
}

static int convert_to_unix(FILE *fin, FILE *fout)
{
	int c, prev = 'a';

	while ((c = fgetc(fin)) != EOF)
	{
		/* if file is non-textual, abort */
		if ((c == '\0') || (c > 254))
			return 0;

		/* Carriage returns occur in Mac and DOS files */
		if (c == CR)
			fputc(LF, fout);
		/* Linefeeds happen in DOS and Unix files */
		else if (c == LF) {
			/* don't output extra LF if DOS file */
			if (prev != CR)
				fputc(LF, fout);
		}
		else	/* normal output of chars */
			fputc(c, fout);

		prev = c;
	}

	return 1;
}

static int convert_to_mac(FILE *fin, FILE *fout)
{
	int c, prev = 'a';

	while ((c = fgetc(fin)) != EOF)
	{
		/* if file is non-textual, abort */
		if ((c == '\0') || (c > 254))
			return 0;

		/* Carriage returns occur in Mac and DOS files */
		if (c == CR)
			fputc(CR, fout);
		/* Linefeeds happen in DOS and Unix files */
		else if (c == LF) {
			/* don't output extra CR if DOS file */
			if (prev != CR)
				fputc(CR, fout);
		}
		else	/* normal output of chars */
			fputc(c, fout);

		prev = c;
	}

	return 1;
}

/*
 *  Convert a text file between formats.
 *  Return 0 on failure, 1 on success.
 */
static int convert_file_format(App *app,
		char *dir, char *file, int conversion)
{
	FILE *fin, *fout;
	char *tmp = CONVERT_TMP;
	int ok = 0;

	if (!strcmp(file,tmp)) 	/* don't convert a temporary file */
		return 0;

	if ((fin = app_open_file(file, "rb")) == NULL) {
		ask_ok_str(app, "Error",
			"Could not open source file %s\n", file);
		return 0;
	}

	if ((fout = app_open_file(tmp, "wb")) == NULL) {
		ask_ok_str(app, "Error",
			"Could not open temporary output file %s\n", tmp);
		return 0;
	}

	switch (conversion) {
	  case CONVERT_TO_DOS:	ok = convert_to_dos(fin, fout); break;
	  case CONVERT_TO_UNIX:	ok = convert_to_unix(fin, fout); break;
	  case CONVERT_TO_MAC:	ok = convert_to_mac(fin, fout); break;
	}

	if (! ok) {
		/* problem converting file - e.g. it wasn't a text file */
		app_close_file(fin);
		app_close_file(fout);
		app_remove_file(tmp);
		return 0;
	}

	if (app_close_file(fin) == 0) {
		ask_ok_str(app, "Error",
			"Could not close source file %s\n", file);
		return 0;
	}
	if (app_close_file(fout) == 0) {
		ask_ok_str(app, "Error",
			"Could not close output file %s\n", tmp);
		return 0;
	}

	if (app_remove_file(file) == 0) {
		ask_ok_str(app, "Error",
			"Could not delete source file %s\n", file);
		app_remove_file(tmp);
		return 0;
	}

	if (app_rename_file(tmp, file) == 0) {
		ask_ok_str(app, "Error",
			"Could not rename output file to %s\n", file);
		return 0;
	}

	return 1; /* success */
}

/*
 *  Convert a file name to upper or lower case.
 *  Return 0 on failure, 1 on success.
 */
static int convert_case(App *app, char *dir, char *file1, int conversion)
{
	int i, result = 1;
	char *file2;
	char *path1;
	char *path2;

	file2 = app_copy_string(file1);

	switch (conversion) {
	  case CONVERT_TO_LOWER:
		for (i=0; file2[i]; i++)
			file2[i] = tolower(file2[i]);
		break;
	  case CONVERT_TO_UPPER:
		for (i=0; file2[i]; i++)
			file2[i] = toupper(file2[i]);
		break;
	}

	path1 = add_strings(NULL, dir);
	path1 = add_strings(path1, file1);

	path2 = add_strings(NULL, dir);
	path2 = add_strings(path2, file2);

	if (strcmp(path1, path2) == 0) {
		/* nothing to be done */
		app_del_string(path1);
		app_del_string(path2);
		return 1;
	}

	if (app_rename_file(path1, path2) == 0) {
		ask_ok_str(app, "Error",
			"Could not rename file %s\n", file1);
		result = 0;
	}

	app_del_string(path1);
	app_del_string(path2);
	app_del_string(file2);

	return result;
}

/*
 *  Convert file to some other format.
 *  Return 0 on failure, 1 on success.
 */
int convert_file(char *dir, char *file, int conversion)
{
	App *app;
	int ok = 0;

	app = app_new_app(0,0);

	switch (conversion) {
	  case CONVERT_TO_DOS:
	  case CONVERT_TO_UNIX:
	  case CONVERT_TO_MAC:
		ok = convert_file_format(app, dir, file, conversion);
		break;

	  case CONVERT_TO_LOWER:
	  case CONVERT_TO_UPPER:
		ok = convert_case(app, dir, file, conversion);
		break;
	}

	app_del_app(app);

	return ok;
}
