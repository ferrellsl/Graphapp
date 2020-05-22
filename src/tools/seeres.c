/*
 *  SeeRes
 *  ------
 *  This program displays a list of the resources (files)
 *  inside a compiled program which has had resources
 *  added using the AddRes tool.
 */

/*
 *  Usage:
 *
 *  Each parameter is a filename to examine.
 *
 *  Example:
 *
 *    seeres prog1 prog2 prog3
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <app.h>

char *this_prog = "seeres";
char *resource_trailer = "\nApp Resource File Type 1\n";

void error(char *msg)
{
	fprintf(stderr, "%s: %s\n", this_prog, msg);
}

void usage(void)
{
	fprintf(stderr, "usage: %s files...\n", this_prog);
}

void skipped(char *name, char *reason)
{
	fprintf(stderr, "%s: skipped %s (%s)\n", this_prog, name, reason);
}

void viewing(char *name)
{
	fprintf(stdout, "%s\n", name);
}

void report(char *name, long bytes)
{
	fprintf(stdout, "\t%-48s %10ld bytes\n", name, bytes);
}

/*
 *  Process a file.
 */

int see_res(char *name)
{
	int i, power = 1;
	long size, bytes;
	FILE *f;
	char buf[64];
	char *block;
	char *resname;

	/* open the file */
	bytes = app_file_size(name);
	f = app_open_file(name, "rb");
	if (f == NULL) {
		error("the resource file could not be opened");
		return -1;
	}

	/* check the resource trailer is there */
	size = strlen(resource_trailer);
	if (bytes < size+2) {
		app_close_file(f);
		skipped(name, "too small for the resource file trailer");
		return -1;
	}
	fseek(f, bytes-size, SEEK_SET);
	fread(buf, 1, size, f);
	buf[size] = '\0';
	if (strcmp(buf, resource_trailer) != 0) {
		app_close_file(f);
		skipped(name, "the resource file trailer was incorrect");
		return -1;
	}
	bytes -= size;
	viewing(name);

	/* extract the size of the table of contents */
	size = 0;
	bytes -= 2;
	fseek(f, bytes, SEEK_SET);
	fread(buf, 1, 1, f);
	while (buf[0] != '\0') {
		size += (buf[0] - '0') * power;
		power *= 10;
		bytes -= 1;
		fseek(f, bytes, SEEK_SET);
		fread(buf, 1, 1, f);
	}

	/* extract the table of contents */
	fseek(f, bytes - size, SEEK_SET);
	block = app_alloc(size);
	fread(block, 1, size, f);
	app_close_file(f);

	/* print the table of contents */
	for (i=0; i < size; i++) {
		resname = block+i;
		sscanf(resname+strlen(resname)+1, "%ld", &bytes);
		report(resname, bytes);
		i += strlen(resname)+1;	/* skip file name */
		i += strlen(block+i);	/* skip file size */
	}
	app_free(block);
	return 0;
}

int main(int argc, char *argv[])
{
	int i;

	if (argc < 2) {
		usage();
		return 2;
	}

	for (i=1; i < argc; i++)
		see_res(argv[i]);

	return 0;
}

