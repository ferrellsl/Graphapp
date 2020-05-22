/*
 *  GetRes
 *  ------
 *  This program extracts a named resource from a
 *  file which contains resources. Resources are
 *  added to a file using the AddRes tool.
 */

/*
 *  Usage:
 *
 *     getres resource_file name_of_resource
 *
 *  Examples:
 *
 *     getres prog1 "source.c"
 *     getres prog1 "*source.c*"
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <app.h>

char *this_prog = "getres";
char *resource_trailer = "\nApp Resource File Type 1\n";

void error(char *msg)
{
	fprintf(stderr, "%s: %s\n", this_prog, msg);
}

void usage(void)
{
	fprintf(stderr, "usage: %s resource_file resource_name\n", this_prog);
}

void skipped(char *name, char *reason)
{
	fprintf(stderr, "%s: skipped %s (%s)\n", this_prog, name, reason);
}

/*
 *  Process a file.
 */

void write_bytes(FILE *f, long bytes)
{
	long count;
	int ch;

	count = 0;
	while ((ch = getc(f)) != EOF) {
		putc(ch, stdout);
		count++;
		if (count == bytes)
			break;
	}
}

int get_res(char *name, char *resource)
{
	int i, power = 1;
	long size, bytes;
	FILE *f;
	char buf[64];
	char *block;
	char *resname;
	long offset;

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
	block = app_alloc(size);
	fseek(f, bytes - size, SEEK_SET);
	fread(block, 1, size, f);

	/* search the table of contents for the named resource */
	offset = 0;
	for (i=0; i < size; i++) {
		resname = block+i;
		sscanf(resname+strlen(resname)+1, "%ld", &bytes);
		if (app_regex_match(resource, resname)) {
			fseek(f, offset, SEEK_SET);
			write_bytes(f, bytes);
			app_close_file(f);
			break;
		}
		offset += bytes + 1;
		i += strlen(resname)+1;	/* skip file name */
		i += strlen(block+i);	/* skip file size */
	}
	app_free(block);
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		usage();
		return 2;
	}
	return get_res(argv[1], argv[2]);
}
