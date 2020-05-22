/*
 *  Resources:
 *
 *  Platform: Neutral
 *
 *  Version: 3.10  2001/12/01  First release.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced some size_t conversion warnings.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

/*
 *  Resources are sub-files contained within a compiled program.
 *  There is a utility program for adding resources,
 *  available in tools/addres.c.
 *
 *  This file implements a method for returning a file pointer
 *  which points to a named resource. Resources can be specified
 *  by a regular expression (similar to a shell wildcard string).
 *
 */

#include "apputils.h"

static const char *resource_trailer = "\nApp Resource File Type 1\n";

/*
 *  Open the named resource file, at the start of the named
 *  resource. The length of the resource will be given in
 *  bytes in the *length parameter. If the file cannot be
 *  opened, return NULL and set *length to zero.
 *  The returned file must later be closed with app_close_file.
 */

FILE * app_open_resource(const char *file_name, const char *resource,
	long *length)
{
	int i, power = 1;
	long size, bytes;
	FILE *f;
	char buf[64];
	char *block;
	char *resname;
	long offset;
	int found = 0;

	/* open the resource file */
	if (length)
		*length = 0;
	bytes = app_file_size(file_name);
	f = app_open_file(file_name, "rb");
	if (f == NULL)
		return NULL;

	/* check the resource trailer string is there */
	size = (long) strlen(resource_trailer);
	if (bytes < size+2) {
		app_close_file(f);
		return NULL;
	}
	fseek(f, bytes-size, SEEK_SET);
	fread(buf, 1, size, f);
	buf[size] = '\0';
	if (strcmp(buf, resource_trailer) != 0) {
		app_close_file(f);
		return NULL;
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
			found = 1;
			fseek(f, offset, SEEK_SET);
			if (length)
				*length = bytes;
			break;
		}
		offset += bytes + 1;
		i += (int) strlen(resname)+1;	/* skip file name */
		i += (int) strlen(block+i);	/* skip file size */
	}
	app_free(block);
	if (found) {
		return f;
	}
	else {
		app_close_file(f);
		return NULL;
	}
}

int app_file_has_resources(const char *file_name)
{
	long size, bytes;
	FILE *f;
	char buf[64];

	/* open the file */
	bytes = app_file_size(file_name);
	f = app_open_file(file_name, "rb");
	if (f == NULL)
		return 0;

	/* check the resource trailer string is there */
	size = (long) strlen(resource_trailer);
	if (bytes < size+2) {
		app_close_file(f);
		return 0;
	}
	fseek(f, bytes-size, SEEK_SET);
	fread(buf, 1, size, f);
	buf[size] = '\0';
	app_close_file(f);
	if (strcmp(buf, resource_trailer) != 0)
		return 0;
	return 1;
}

