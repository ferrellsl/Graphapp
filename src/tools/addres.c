/*
 *  AddRes
 *  ------
 *  This program adds resources (files) to a compiled program.
 *  The resources can be any kind of file.
 *  Many files can be added to a program.
 *  An App program has functions to read these resources as
 *  if they were ordinary files.
 *
 *  The main purpose of resources of this kind is to add
 *  fonts and images onto a program, to create a single
 *  executable file which can be distributed without needing
 *  an installation program to reconstruct a font tree
 *  or other directory structure.
 *
 *  In essence this program is similar to the Linux 'cat' program
 *  except that it also writes a table of contents structure
 *  at the end of the file to allow the resources to be extracted.
 *  Also, each of the constituent files is separated by a zero byte.
 *
 *  The main reason for having the table of contents and an
 *  identifying trailer string at the end of the file rather
 *  than the start is that this way an executable program
 *  still works; the resources become invisible additions to
 *  the program.
 */

/*
 *  Usage:
 *
 *  The first parameter must be the name of the compiled program
 *  to which resources are to be added.
 *
 *  All files to be added as resources must be specified next.
 *  It is currently not possible to incrementally add files.
 *  For this reason, a script may be needed to run AddRes.
 *  Alternatively, the -l flag can be used.
 *  The -l flag is followed by a file name.
 *  This file lists resource file name, one per line.
 *  No wild card characters can be used withinin this file;
 *  each file name must be correct and fully specified.
 *
 *  Files can be added using either their absolute path name or
 *  a relative path name.
 *
 *  The -o flag is followed by the output file name.
 *  If no output file name is given in this manner, an error results.
 *
 *  Example:
 *
 *   addres prog -o prog2 fonts/unifont/16.txt fonts/unifont/16/*.png
 *
 *  The above example adds the Unifont metrics file and all of the
 *  Unifont sub-font images into a program. This means prog2 now
 *  contains the entire Unifont, and will be able to use and
 *  display text in Unifont without needing any other files.
 *  If your program only requires ISO Latin-1 (the first sub-font)
 *  you could use the following example instead:
 *
 *   addres prog -o prog3 fonts/unifont/16.txt fonts/unifont/16/00000000.png
 *
 *  The 00000000.png file contains just the ISO Latin-1 sub-font
 *  (have a look at it now in a web browser or image viewer).
 *  You can use this process to just add the fonts and sub-fonts
 *  you desire, but remember, you'll always need the metrics file
 *  also, which in the above case is 16.txt (the 16 refers to the
 *  pixel height of the font, and also names the directory to search
 *  for the associated sub-fonts).
 *
 *  To name resource files using a listing file, you might do this:
 *
 *   addres prog -o prog4 -l listfile.txt
 *
 *  Where listfile.txt might contain:
 *
 *   fonts/unifont/16.txt
 *   fonts/unifont/16/00000000.png
 *   fonts/unifont/16/00000100.png
 *
 *  This example adds the first two sub-fonts only. Since wild card
 *  characters cannot be used, you'll have to spell out all the files.
 *  You could even put the input "prog" parameter as the first
 *  resource in the list file, and omit it from the command-line parameters.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <app.h>

char *this_prog = "addres";
char *resource_trailer = "\nApp Resource File Type 1\n";

void error(char *msg)
{
	fprintf(stderr, "%s: %s\n", this_prog, msg);
}

void usage(void)
{
	fprintf(stderr, "usage: %s infile -o outfile [-l listfile] files...\n", this_prog);
}

void skipped(char *name)
{
	fprintf(stderr, "%s: skipped %s\n", this_prog, name);
}

void creating(char *name)
{
	fprintf(stderr, "%s\n", name);
}

void adding(char *name, long bytes)
{
	fprintf(stdout, "    +   %-48s %10ld bytes\n", name, bytes);
}

void add_to_contents(char *name, long bytes, char **contents, long *len)
{
	long namelen, numlen;
	char num[64];

	/* append NUL-terminated name string and length string */
	sprintf(num, "%ld", bytes);
	namelen = strlen(name) + 1;
	numlen = strlen(num) + 1;
	if (*len == 0)
		*contents = app_alloc(namelen+numlen);
	else
		*contents = app_realloc(*contents, *len +namelen +numlen);
	strcpy(*contents + *len, name);
	strcpy(*contents + *len + namelen, num);
	*len += namelen + numlen;
}

/*
 *  Process a list of filenames.
 */

enum {
	BUFSIZE = 16*4096
};

int add_res(char *output_name, int n, char *filenames[])
{
	int i, count;
	char *name;
	long size, bytes;
	FILE *infile;
	FILE *outfile;
	char *block;
	char *contents = NULL;
	long length = 0;

	/* check that the output does not appear as any of the inputs */
	for (i=0; i < n; i++) {
		if (strcmp(filenames[i], output_name) == 0) {
			error("the output name cannot also be an input");
			return -1;
		}
	}

	/* open the output file */
	creating(output_name);
	outfile = app_open_file(output_name, "wb");
	if (outfile == NULL) {
		error("could not open the destination file");
		return -1;
	}

	/* create a block of memory to use in the copying */
	block = app_alloc(BUFSIZE);
	if (block == NULL) {
		error("out of memory");
		app_close_file(outfile);
		return -1;
	}

	/* then append each resource file to the end */
	for (i=count=0; i < n; i++) {
		name = filenames[i];
		bytes = app_file_size(name);
		if (bytes < 0) {
			skipped(name);
			continue;
		}
		infile = app_open_file(name, "rb");
		if (infile == NULL) {
			skipped(name);
			continue;
		}
		adding(name, bytes);
		size = BUFSIZE;
		while (size > 0) {
			size = fread(block, 1, BUFSIZE, infile);
			size = fwrite(block, 1, size, outfile);
		}
		fputc('\0', outfile);
		app_close_file(infile);
		count++;
		add_to_contents(name, bytes, &contents, &length);
	}

	/* now append the table of contents and the resource trailer */
	add_to_contents("", length, &contents, &length);
	fwrite(contents, 1, length, outfile);
	fwrite(resource_trailer, 1, strlen(resource_trailer), outfile);
	app_close_file(outfile);
	app_free(block);

	return (count == n) ? 0 : -1;
}

char ** read_lines(char *filename, int *num)
{
	FILE *f;
	char **lines;
	char *line;
	long nbytes, nchars;

	*num = 0;
	lines = NULL;
	f = app_open_file(filename, "rb");
	if (f == NULL)
		return NULL;

	while ((line = app_read_utf8_line(f, &nbytes, &nchars)) != NULL)
	{
		if ((nbytes > 0) && (line[nbytes-1] == '\n'))
			line[--nbytes] = '\0';
		if ((nbytes > 0) && (line[nbytes-1] == '\r'))
			line[--nbytes] = '\0';
		if (nbytes <= 0)
			continue;
		lines = app_realloc(lines, (*num+1) * sizeof(char *));
		lines[*num] = line;
		*num += 1;
	}

	app_close_file(f);
	return lines;
}

char ** add_lines_list(int extra, char **extras, int* num, char **lines)
{
	lines = app_realloc(lines, (*num + extra) * sizeof(char *));
	memmove(&lines[extra], lines, (*num) * sizeof(char *));
	memcpy(&lines[0], extras, extra * sizeof(char *));
	*num += extra;
	return lines;
}

int main(int argc, char *argv[])
{
	int i;
	char *output_name = NULL;
	char *list_name = NULL;
	int shift = 0;

	if (argc < 2) {
		usage();
		return 2;
	}

	for (i=1; i < argc; i++) {
		if ((output_name == NULL) && (strcmp(argv[i], "-o") == 0))
		{
			i++;
			if (i < argc) {
				output_name = argv[i];
				shift += 2;
			}
		}
		else if ((list_name == NULL) && (strcmp(argv[i], "-l") == 0))
		{
			i++;
			if (i < argc) {
				list_name = argv[i];
				shift += 2;
			}
		}
		else if (shift)
			argv[i-shift] = argv[i];
	}

	if (output_name == NULL) {
		error("no output file name was given.");
		usage();
		return 3;
	}

	if (list_name != NULL) { /* the list file contains the filenames */
		char **lines;
		int num;

		lines = read_lines(list_name, &num);
		lines = add_lines_list(argc-1-shift, &argv[1], &num, lines);
		if (lines && (num > 0))
			return add_res(output_name, num, lines);
		else {
			error("list file was empty.");
			usage();
			return 4;
		}
	}

	/* argv[1...argc-3] now contains the filenames */
	return add_res(output_name, argc-1-shift, &argv[1]);
}

