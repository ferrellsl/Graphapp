/*
 * Monty - a simple project editor.
 *
 * File: split.c -- a utility to split a file into 1.4 MB blocks.
 * Platform: Neutral  Version: 2.00  Date: 1999/10/10
 *
 * Version: 1.00  Changes: Original version by L. Patrick.
 */

/* Copyright (C) 1998 L. Patrick

   This file is part of Monty, a simple project editor.
*/


#define SPLIT_SIZE 1400000	/* 1.4 MB blocks */
#define BLOCK_SIZE 14000
#define BUFFER_SIZE 140

/*
 *  The file copying function:
 */

static char * file_split_string (char *src)
{
	char *str;

	str = add_strings(NULL, "Splitting: ");
	str = add_strings(str,  src);

	return str;
}

static char * file_split_question (char *src, long size)
{
	long pieces = (size + SPLIT_SIZE - 1) / SPLIT_SIZE;
	char buffer[30];
	char *str;

	sprintf(buffer, "%ld", pieces);
	str = add_strings(NULL, "This operation will split the file");
	str = add_strings(str,  " into ");
	str = add_strings(str,  buffer);
	str = add_strings(str,  " files, each of a size which will fit onto ");
	str = add_strings(str,  " a typical floppy disk.\n\nContinue?");

	return str;
}

static char * remove_suffix(App *app, char *src)
{
	long i, len;
	char *basename;

	len = strlen(src);
	basename = array(len+1, char);
	if (basename == NULL) {
		app_ask_ok(app, "Error", "Ran out of memory.");
		return NULL;
	}
	strcpy(basename, src);
	for (i=len-1; i >= 0; i--) {
		if (basename[i] == '.') {
			basename[i] = '\0';
			break;
		}
		else if ((basename[i] == '\\') || (basename[i] == '/')) {
			break;
		}
	}
	return basename;
}

static void split_file_by_blocks(App *app, FILE *f, char *src, long size,
		char *block, long block_size)
{
	int filecount;
	char *basename;
	char *dest;
	FILE *outfile = NULL;
	long i, total, count, len;
	char *message;
	Window *p;

	message = file_split_string(src);
	p = new_progress_window(app, "File Split", message);

	/* Find base filename by stripping off the suffix. */
	basename = remove_suffix(app, src);

	/* Make a buffer to store each filename. */
	len = strlen(basename) + 10;
	dest = array(len, char);
	if (dest == NULL) {
		app_ask_ok(app, "Error", "Out of memory.");
		app_del_window(p);
		app_del_string(basename);
		app_del_string(message);
		return;
	}

	/* Split the file. */
	filecount = 1;
	total = count = 0;
	while ((i = fread(block, 1, block_size, f)) != 0) {
		total += i;
		count += i;
		if (outfile == NULL) {
			sprintf(dest, "%s.s%02.2d", basename, filecount);
			outfile = app_open_file(dest, "wb");
			if (outfile == NULL) {
				ask_ok_str(app, "Error",
				"Couldn't open the file \"%s\" for writing\n", dest);
				break;
			}
		}
		fwrite(block, 1, i, outfile);
		update_progress_window(p, total*100/size);
		if (count >= SPLIT_SIZE) {
			count = 0;
			app_close_file(outfile);
			outfile = NULL;
			filecount++;
		}
	}
	if (outfile != NULL) {
		app_close_file(outfile);
	}
	discard(dest);
	discard(basename);
	app_del_window(p);
	app_del_string(message);
}

void split_file(App *app, char *src)
{
	FILE *f;
	int result;
	char *block = NULL;
	long size;
	char *question;
	char buffer[BUFFER_SIZE];

	if ((f = app_open_file(src, "rb")) == NULL) {
		ask_ok_str(app, "Error",
			"Couldn't open the file \"%s\" for reading.", src);
		return;
	}

	size = app_file_size(src);
	if (size <= SPLIT_SIZE) {
		app_ask_ok(app, "Error",
			"That file is already less than 1.4MB in size.");
		return;
	}

	question = file_split_question(src, size);
	result = app_ask_ok_cancel(app, "Continue?", question);
	app_del_string(question);
	if (result == CANCEL)
		return;

	/* create block for splitting file */
	block = array(BLOCK_SIZE, char);

	if (block == NULL) {
		/* copy using small blocks */
		split_file_by_blocks(app, f, src, size, buffer, BUFFER_SIZE);
	} else {
		/* copy file in 14K blocks */
		split_file_by_blocks(app, f, src, size, block, BLOCK_SIZE);
		discard(block);
	}

	app_close_file(f);
}

long concat_files(App *app, FILE *src, FILE *dest, long total, long size,
		char *block, long block_size)
{
	long i;
	long srcsize = 0;
	Window *p;

	p = new_progress_window(app, "File Join", "Joining files:");

	while ((i = fread(block, 1, block_size, src)) != 0) {
		srcsize += i;
		total += i;
		fwrite(block, 1, i, dest);
		update_progress_window(p, total*100/size);
	}

	app_del_window(p);

	return srcsize;
}

void join_files(App *app, char *src)
{
	long len, size, total;
	char *basename;
	char *filename;
	char *block;
	long block_size;
	FILE *f1;
	FILE *f2;
	int filecount, result;
	char buffer[BUFFER_SIZE];

	/* Check it's a valid filename. */
	len = strlen(src);
	if ((len < 4) || (src[len-4] != '.') || (src[len-3] != 's')
		|| (! isdigit(src[len-2])) || (! isdigit(src[len-1]))) {
		app_ask_ok(app, "Error",
			"That file was not produced by the split operation.");
		return;
	}
	result = app_ask_ok_cancel(app, "Error", "Join the split files together?");
	if (result == CANCEL)
		return;

	/* Find base filename by stripping off the suffix. */
	basename = remove_suffix(app, src);
	len = strlen(basename) + 10;
	filename = array(len, char);

	/* Count the number and size of source files. */
	size = 0;
	filecount = 1;
	while (1) {
		sprintf(filename, "%s.s%02.2d", basename, filecount);
		f1 = app_open_file(filename, "rb");
		if (f1 == NULL)
			break;
		app_close_file(f1);
		size += app_file_size(filename);
		filecount++;
	}

	/* Open the destination file. */
	sprintf(filename, "%s.tmp", basename);
	f2 = app_open_file(filename, "wb");
	if (f2 == NULL) {
		ask_ok_str(app, "Error",
			"Couldn't open the file \"%s\" for writing.", filename);
		discard(filename);
		return;
	}

	/* Step through the source files, finding and concatenating. */
	total = 0;
	filecount = 1;

	block = array(BLOCK_SIZE, char);
	block_size = BLOCK_SIZE;
	if (block == NULL) {
		block = buffer;
		block_size = BUFFER_SIZE;
	}

	while (1) {
		sprintf(filename, "%s.s%02.2d", basename, filecount);
		f1 = app_open_file(filename, "rb");
		if (f1 == NULL)
			break;
		total += concat_files(app, f1, f2, total, size, block, block_size);
		app_close_file(f1);
		filecount++;
	}

	/* Tidy up. */
	if (block != buffer)
		discard(block);
	app_close_file(f2);
	discard(filename);
}
