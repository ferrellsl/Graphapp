/*
 *  Standard file/folder functions.
 *
 *  Platform: Linux/Unix.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Added some folder functions.
 *  Version: 3.32  2002/09/04  Updated the fix_path_string function.
 *  Version: 3.43  2003/04/25  Passing NULL to close_file/folder is legal.
 *  Version: 3.50  2003/11/25  Both / and \ are now folder separators.
 *  Version: 3.53  2004/05/08  Better handling of root folder.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

/*
 *  On Linix, change '\' into '/'.
 */

char * app_to_native_path(const char *filepath)
{
	int i, length;
	char *path;

	length = strlen(filepath);
	path = app_alloc(length+1);
	for (i=0; i < length; i++) {
		if (filepath[i] == '\\')
			path[i] = '/';
		else
			path[i] = filepath[i];
	}
	path[length] = '\0';
	return path;
}

char * app_to_portable_path(const char *filepath)
{
	int i, length;
	char *path;

	length = strlen(filepath);
	path = app_alloc(length+1);
	for (i=0; i < length; i++) {
		if (filepath[i] == '\\')
			path[i] = '/';
		else
			path[i] = filepath[i];
	}
	path[length] = '\0';
	return path;
}

/*
 *  Handle "./" and "../" entries by removing redundant folder names
 *  from a path string. Modifies the string in-place.
 *  Should only be used on portable paths.
 */
static void app_fix_portable_path(char *path)
{
	long len;
	char *s;
	char *start = path;

	/* turn all // into single / */
	while ((s = strstr(start, "//")) != NULL)
		strcpy(s, s+1);

	/* remove all redundant ./ within path */
	while ((s = strstr(start, "/./")) != NULL)
		strcpy(s, s+2);

	/* remove any number of ./ from start of path */
	while ((s = strstr(start, "./")) == start)
		strcpy(s, s+2);

	/* remove all redundant /../ from start of path */
	while ((s = strstr(start, "/../")) == start)
		strcpy(s, s+3);

	/* leave all ../ at the start of path */
	while ((s = strstr(start, "../")) == start)
		start = s+3;

	/* turn trailing /. into / */
	len = strlen(start);
	if ((len > 1) && (strcmp(&start[len-2], "/.") == 0)) {
		start[len-1] = '\0';
		len--;
	}

	/* remove all redundant xyz/../ within path */
	while ((s = strstr(start, "/")) != NULL) {
		if (strncmp(s, "/../", 4) == 0)
			strcpy(start, s+4);
		else if (memcmp(s, "/..", 4) == 0) /* end of path */
			strcpy(start, s+3);
		else
			start = s+1;
	}
}

/*
 *  Open files, translating file names to whatever the operating
 *  system requires.
 */

FILE * app_open_file(const char *filepath, const char *mode)
{
	FILE *f;
	char *path;

	path = app_to_native_path(filepath);
	f = fopen(path, mode);
	app_free(path);
	return f;
}

int app_close_file(FILE *f)
{
	if (f == NULL)
		return 1;
	else if (fclose(f) == 0)
		return 1;
	else
		return 0;
}

/*
 *  Remove a file from the file system.
 *  Returns 1 on success, 0 if there it could not be removed.
 */
int app_remove_file(const char *filepath)
{
	int result;
	char *path = app_to_native_path(filepath);

	if (remove(path) == 0)
		result = 1;
	else
		result = 0;
	app_free(path);
	return result;
}

/*
 *  Rename a file. Returns 1 on success, 0 on error.
 */
int app_rename_file(const char *oldpath, const char *newpath)
{
	int result;
	char *old_path = app_to_native_path(oldpath);
	char *new_path = app_to_native_path(newpath);

	if (rename(old_path, new_path) == 0)
		result = 1;
	else
		result = 0;
	app_free(old_path);
	app_free(new_path);
	return result;
}

/*
 *  Calibrate the readfolder function, return offset to d_name field.
 *  This is an internal function used to compensate for problems
 *  associated with using the UCB lib on some Unix machines.
 *  It is harmless to use on other machines.
 *
 *  Ideally, we should just use struct dirent->d_name to access
 *  directory filenames, but some versions of Unix include
 *  two different definitions of struct dirent, with the d_name
 *  field in different spots. This is not a problem if you
 *  include the correct header file, but it's easy to get wrong.
 *  Hence, this ugly but useful function which pokes around in
 *  memory to find the actual byte offset.
 */
static int app_calibrate_readdir(void)
{
	char *dirname[3] = { "/bin", "C:\\Dos", "C:\\Windows" };
	int offsets[4] = { 8, 10 };
	struct dirent *d;
	DIR *dir;
	char *name;
	int i;
	int dirent_offset = 0; /* offset to d_name field */

	/* Just assume it's correctly built and linked. */
	dirent_offset = ((char *)&d->d_name - (char *)d);
	return dirent_offset;

	/* Find a directory to use as reference. */
	d = NULL;
	dir = NULL;
	for (i=0; i < (sizeof(dirname)/sizeof(dirname[0])); i++) {
		/* Open a reference directory: */
		if ((dir = opendir(dirname[i])) == NULL)
			continue;
		/* Skip the "." directory entry: */
		if ((d = readdir(dir)) == NULL) {
			if (dir != NULL)
				closedir(dir);
			continue;
		}
		/* Use the ".." entry to calibrate on: */
		if ((d = readdir(dir)) == NULL) {
			if (dir != NULL)
				closedir(dir);
			continue;
		}
		/* Exit the loop, check this directory: */
		break;
	}

	if (d == NULL)
		return -1;	/* error */

	/* Check if normal dirent structure works: */
	if (! strcmp(d->d_name, "..")) {
		dirent_offset = ((char *)&d->d_name - (char *)d);
		if (dir != NULL)
			closedir(dir);
		return dirent_offset;
	}

	/* No, didn't work. Must be using wrong readdir function. */

	/* Find an offset that yields ".." as the filename: */
	for (i=0; i < (sizeof(offsets)/sizeof(offsets[0])); i++) {
		name = ((char *) d) + offsets[i];
		if (! strcmp(name, "..") ) {
			dirent_offset = offsets[i];
			break;
		}
	}

	/* Clean up: */
	if (dir)
		closedir(dir);
	return dirent_offset;
}

/*
 *  Open a folder. Return NULL on failure.
 */
Folder * app_open_folder(const char *foldername)
{
	char *path;
	Folder *f;

	if (foldername[0] == '\0')	/* empty path */
		foldername = "/";	/* use root directory instead */

	path = app_to_native_path(foldername);
	f = (Folder *) opendir(path);
	app_free(path);
	return f;
}

/*
 *  Read the next file name from an open folder.
 *  Return NULL if there are no more entries.
 */
char * app_read_folder(Folder *f)
{
	struct dirent *d;
	int offset;

	d = readdir((DIR *)f);
	if (d == NULL)
		return NULL;

	offset = app_calibrate_readdir();
	if (offset < 0)
		return NULL;

	return ((char *) d) + offset;
}

/*
 *  Close a folder. Return zero on failure.
 */
int app_close_folder(Folder *f)
{
	if (f == NULL)
		return 1;
	else if (closedir((DIR *) f) == 0)
		return 1;
	else
		return 0;
}

/*
 *  Create a new folder, using mkdir().
 *  Return 1 on success, 0 otherwise.
 */
int app_make_folder(const char *foldername, int mode)
{
	int result;
	char *path = app_to_native_path(foldername);

	if (mkdir(path, mode) == 0)
		result = 1;
	else
		result = 0;
	app_free(path);
	return result;
}

/*
 *  Remove a folder from the file system.
 *  Returns 1 on success, 0 if there it could not be removed.
 */
int app_remove_folder(const char *folderpath)
{
	int result;
	char *path = app_to_native_path(folderpath);

	if (rmdir(path) == 0)
		result = 1;
	else
		result = 0;
	app_free(path);
	return result;
}

/*
 *  Rename a folder. Returns 1 on success, 0 on error.
 */
int app_rename_folder(const char *oldpath, const char *newpath)
{
	int result;
	char *old_path = app_to_native_path(oldpath);
	char *new_path = app_to_native_path(newpath);

	if (rename(old_path, new_path) == 0)
		result = 1;
	else
		result = 0;
	app_free(old_path);
	app_free(new_path);
	return result;
}

/*
 *  Return some information about a file.
 *  The returned value is a bit-field.
 */
int app_file_info(const char *filepath)
{
	struct stat s;
	char *path;
	int info = 0;

	if (filepath[0] == '\0')
		return IS_FOLDER | IS_READ;

	path = app_to_portable_path(filepath);
	app_fix_portable_path(path);

	if (stat(path, &s) == 0) {
		info += ((s.st_mode & S_IFDIR)  ? IS_FOLDER : 0);
		info += ((s.st_mode & S_IFREG)  ? IS_FILE   : 0);
		info += ((s.st_mode & S_IEXEC)  ? IS_APP    : 0);
		info += ((s.st_mode & S_IREAD)  ? IS_READ   : 0);
		info += ((s.st_mode & S_IWRITE) ? IS_WRITE  : 0);
	}

	app_free(path);

	return info;
}

/*
 *  Return the file's size.
 */
long app_file_size(const char *filepath)
{
	char *path;
	struct stat s;
	long size = -1;

	if (filepath[0] == '\0')
		return size;

	path = app_to_native_path(filepath);
	if (stat(path, &s) == 0) {
		size = s.st_size;
	}
	app_free(path);

	return size;
}

/*
 *  Return the file's last modifcation time in seconds.
 */
long app_file_time(const char *filepath)
{
	char *path;
	struct stat s;
	long mtime = -1;

	if (filepath[0] == '\0')
		return mtime;

	path = app_to_native_path(filepath);
	if (stat(path, &s) == 0) {
		mtime = s.st_mtime;
	}
	app_free(path);

	return mtime;
}

/*
 *  Return the current folder name.
 *  Uses the getcwd() function to find the current working directory.
 *  Ensures '/' is at the end of the string.
 *  The string must later be freed using app_free.
 */
char * app_current_folder(void)
{
	char *path;
	int i;

	path = app_zero_alloc(512);
	if (! path)
		return NULL;

	if (! getcwd(path, 510))
		return NULL;

	i = strlen(path) - 1;
	if (i >= 0) {
		if (path[i] != '/') {
			path[i+1] = '/';
			path[i+2] = '\0';
		}
		path = app_realloc(path, i+3);
	} else {
		path[0] = '/';
		path[1] = '\0';
		path = app_realloc(path, 2);
	}

	return path;
}

/*
 *  Changes the current working directory using chdir()
 *  to the specified folder.
 */
int app_set_current_folder(const char *folderpath)
{
	int result;
	char *path;

	if (folderpath[0] == '\0')	/* empty path */
		folderpath = "/";	/* use root directory instead */

	path = app_to_native_path(folderpath);

	if (chdir(path) == 0)
		result = 1;
	else
		result = 0;
	app_free(path);
	return result;
}

/*
 *  Return foldername + filename, add the directory separator char
 *  between the two strings if necessary. The returned string must
 *  be destroyed by the calling function, using app_free().
 */
char * app_form_file_path(const char *foldername, const char *filename)
{
	int length, dirlen;
	char *full_name = NULL;
	char *folder_name = app_to_portable_path(foldername);
	char *file_name = app_to_portable_path(filename);

	dirlen = strlen(folder_name);
	length = dirlen + strlen(file_name) + 2;
	full_name = app_alloc(length);
	if (! full_name)
		return NULL;

	strcpy(full_name, folder_name);
	if (dirlen > 0)
		if (full_name[dirlen - 1] != '/') {
			full_name[dirlen] = '/';
			full_name[dirlen + 1] = '\0';
		}
	strcat(full_name, file_name);
	app_fix_portable_path(full_name);

	app_free(file_name);
	app_free(folder_name);

	return full_name;
}

/*
 *  Return just the filename from a full path string.
 *  This returns a pointer to where the file name begins
 *  within the filepath string. As such, there is no need to
 *  free the string returned by this function.
 */
char * app_get_file_name(const char *filepath)
{
	char *path;
	int i, offset = -1;

	if (! filepath)
		return (char *) filepath; /* cast away const */

	path = app_to_native_path(filepath);

	for (i=0; path[i]; i++) {
		if (path[i] == '/')
			offset = i;
	}
	app_free(path);

	return (char *) (filepath + offset + 1); /* cast away const */
}


#ifdef DO_NOT_COMPILE

/*
 *  Print a little, unix-like file description string.
 */
static void print_file_name(const char *foldername, const char *filename)
{
	char mode[5];
	char *path;
	int info;

	path = app_form_file_path(foldername, filename);
	info = app_file_info(path);
	app_free(path);

	strcpy(mode, "s---");

	if (info & IS_FILE)   mode[0] = '-';
	if (info & IS_FOLDER) mode[0] = 'd';
	if (info & IS_READ)   mode[1] = 'r';
	if (info & IS_WRITE)  mode[2] = 'w';
	if (info & IS_APP)    mode[3] = 'x';

	printf("%s %s\n", mode, filename);
}

/*
 *  Scan a folder, print out its contents.
 */ 
static void scanfolder(const char *foldername)
{
	Folder *f;
	char *filename;
 
	if ((f = app_open_folder(foldername)) == NULL)
	{
		perror("Unable to open folder");
		exit(1);
	}

	while ((filename = app_read_folder(f)) != NULL) {
		print_file_name(foldername, filename);
	}

	if (! app_close_folder(f))
		perror("Unable to close folder");
}

void main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("usage: scanfolder foldername\n");
		exit(1);
	}
	scanfolder(argv[1]);
	exit(0);
}

#endif

