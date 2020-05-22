/*
 *  Standard file/folder functions.
 *
 *  Platform: Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/08/13  Added some folder functions.
 *  Version: 3.10  2001/12/01  Updated to use only stdio and Win32.
 *  Version: 3.27  2002/08/14  Can now access other drive letters.
 *  Version: 3.32  2002/09/04  Floppy drive checks are now more silent.
 *  Version: 3.43  2003/04/25  Passing NULL to close_file/folder is legal.
 *  Version: 3.46  2003/05/22  Fixed some bugs.
 *  Version: 3.50  2003/11/25  Both / and \ are now folder separators.
 *  Version: 3.53  2004/05/08  Better handling of root and drive letters.
 *  Version: 3.56  2005/08/09  Silenced some size_t conversion warnings.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

/*
 *  To change a file path into a canonical DOS path,
 *  change each '/' into '\'.
 */

char * app_to_native_path(const char *filepath)
{
	long i, length;
	char *path;

	length = (long) strlen(filepath);
	path = app_alloc(length+1);
	for (i=0; i < length; i++) {
		if (filepath[i] == '/')
			path[i] = '\\';
		else
			path[i] = filepath[i];
	}
	path[length] = '\0';
	return path;
}

/*
 *  To change a file path into a canonical Linux path,
 *  change each '\' into '/'.
 */
char * app_to_portable_path(const char *filepath)
{
	long i, length;
	char *path;

	length = (long) strlen(filepath);
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
 *  Should only be used on a portable path not a native path.
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
	len = (long) strlen(start);
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
	char *dos_path;

	dos_path = app_to_native_path(filepath);
	f = fopen(dos_path, mode);
	app_free(dos_path);
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
	char *dos_path = app_to_native_path(filepath);

	if (remove(dos_path) == 0)
		result = 1;
	else
		result = 0;
	app_free(dos_path);
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
 *  Open a folder. Return NULL on failure.
 */
Folder * app_open_folder(const char *foldername)
{
	FolderExtra *fe;
	long length;
	char *search;
	char *dos_path;

	/* special case: opening "" accesses root */
	if (strcmp(foldername, "") == 0) {
		fe = app_zero_alloc(sizeof(FolderExtra));
		if (fe != NULL)
			fe->root = 'A';
		return (Folder *) fe;
	}

	/* build search path from folder name */
	length = (long) strlen(foldername);
	search = app_alloc(length+4+1);
	strcpy(search, foldername);
	if (length > 0)
		if ((search[length-1] != '/') && (search[length-1] != '\\'))
			strcat(search, "/");
	strcat(search, "*.*");

	/* search for first matching file */
	dos_path = app_to_native_path(search);
	fe = app_zero_alloc(sizeof(FolderExtra));
	if (fe != NULL) {
		fe->dots = 1; /* ensure "." and ".." appear */
		fe->hnd = FindFirstFile(dos_path, &fe->data);
		if (fe->hnd == INVALID_HANDLE_VALUE)
			fe->hnd = 0;
		else
			fe->first = 1; /* already read the first entry */
	}
	app_free(dos_path);
	app_free(search);
	return (Folder *) fe;
}

/*
 *  Read the next file name from an open folder.
 *  Return NULL if there are no more entries.
 */
char * app_read_folder(Folder *f)
{
	int result;
	FolderExtra *fe;

	fe = folder_extra(f);

	/* special case: handle root drive letters */
	if (fe->root != 0) {
		while (fe->root <= 'Z') {
			strcpy(fe->temp, "A:\\"); /* note backslash */
			fe->temp[0] = fe->root; /* A,B,C...Z */
			fe->root += 1;
			result = GetDriveType(fe->temp);
			if ((result == 0) || (result == 1))
				continue; /* drive/folder does not exist */
			fe->temp[2] = '\0'; /* remove backslash */
			return fe->temp;
		}
		return NULL;
	}

	/* ensure "." and ".." appear before anything else */
	if (fe->dots == 1) {
		fe->dots = 2;
		strcpy(fe->temp, ".");
		return fe->temp;
	}
	else if (fe->dots == 2) {
		fe->dots = 0;
		strcpy(fe->temp, "..");
		return fe->temp;
	}

	/* fetch the next file name, ignoring "." and ".." entries */
	while (1) {
		if (fe->first) {
			/* already loaded first matching file name */
			fe->first = 0;
			break; /* return fe->data.cFileName */
		}
		else if (fe->hnd == 0)
			return NULL;
		else if (FindNextFile(fe->hnd, &fe->data) == FALSE)
			return NULL;

		/* skip entry we've already returned */
		if (strcmp(fe->data.cFileName, ".") == 0)
			continue;
		if (strcmp(fe->data.cFileName, "..") == 0)
			continue;

		break; /* return fe->data.cFileName */
	}

	return fe->data.cFileName;
}

/*
 *  Close a folder. Return zero on failure.
 */
int app_close_folder(Folder *f)
{
	FolderExtra *fe;

	if (f == NULL)
		return 1;

	fe = folder_extra(f);

	if ((fe->hnd == 0) || (FindClose(fe->hnd) == TRUE)) {
		app_free(fe);
		return 1;
	}
	else
		return 0;
}

/*
 *  Create a new folder.
 *  Currently, the DOS implementation ignores the mode.
 *  Return 1 on success, 0 otherwise.
 */
int app_make_folder(const char *foldername, int mode)
{
	int result;
	char *dos_path;
	SECURITY_ATTRIBUTES sec;

	dos_path = app_to_native_path(foldername);

	sec.nLength = sizeof(sec);
	sec.lpSecurityDescriptor = NULL;
	sec.bInheritHandle = FALSE;
	if (CreateDirectory(dos_path, &sec) == TRUE)
		result = 1;
	else
		result = 0;
	app_free(dos_path);
	return result;
}

/*
 *  Remove a folder from the file system.
 *  Returns 1 on success, 0 if there it could not be removed.
 */
int app_remove_folder(const char *folderpath)
{
	int result;
	char *dos_path = app_to_native_path(folderpath);

	if (RemoveDirectory(dos_path) == TRUE)
		result = 1;
	else
		result = 0;
	app_free(dos_path);
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
	DWORD attrib;
	int info = 0;
	char *path;
	char *dos_path;
	long i, length;
	char *lower;

	/* the root desktop is special */
	if (filepath[0] == '\0')
		return IS_FOLDER | IS_READ;

	/* floppy disk drive letters are special */
	if ( (((filepath[0] >= 'a') && (filepath[0] <= 'b'))
	   || ((filepath[0] >= 'A') && (filepath[0] <= 'B')))
	  && (filepath[1] == ':') && (filepath[2] == '\0') )
	{
		return IS_FOLDER | IS_READ | IS_WRITE;
	}

	/* handle a DOS pathname by converting to a portable path */
	path = app_to_portable_path(filepath);

	/* fix pathname to account for . and .. entries */
	app_fix_portable_path(path);

	/* now convert back to a DOS pathname */
	dos_path = app_to_native_path(path);
	app_free(path);

	/* the master root directory is special */
	if (dos_path[0] == '\0') {
		app_free(dos_path);
		return IS_FOLDER | IS_READ;
	}

	/* check obvious attributes */
	attrib = GetFileAttributes(dos_path);
	if (attrib != 0xFFFFFFFF) {
	  info |= ((attrib & FILE_ATTRIBUTE_DIRECTORY)  ? IS_FOLDER : IS_FILE);
	  info |= IS_READ;
	  info |= ((attrib & FILE_ATTRIBUTE_READONLY)   ? 0 : IS_WRITE);
	}

	/* form lowercase version of the path string */
	length = (long) strlen(dos_path);
	lower = app_alloc(length+1);
	for (i=0; i <= length; i++)
		lower[i] = tolower(dos_path[i]);

	/* is it an application? */
	if (info & IS_FILE) {
		if ((length > 4) && (strcmp(&lower[length-4], ".exe") == 0))
			info |= IS_APP;
		else if ((length > 4) && (strcmp(&lower[length-4], ".com") == 0))
			info |= IS_APP;
	}

	/* tidy up */
	app_free(dos_path);
	app_free(lower);

	return info;
}

/*
 *  Return the file's size.
 */
long app_file_size(const char *filepath)
{
	long size = -1L;
	DWORD low, high;
	char *dos_path;
	HFILE hfile;
	OFSTRUCT data;

	/* the root desktop is special */
	if (filepath[0] == '\0')
		return size;

	/* floppy disk drive letters are special */
	if ( (((filepath[0] >= 'a') && (filepath[0] <= 'b'))
	   || ((filepath[0] >= 'A') && (filepath[0] <= 'B')))
	  && (filepath[1] == ':') && (filepath[2] == '\0') )
	{
		return size;
	}

	dos_path = app_to_native_path(filepath);
	hfile = OpenFile(dos_path, &data, OF_READ);
	low = GetFileSize((HANDLE)hfile, &high);
	if ((low == 0xFFFFFFFF) && (GetLastError() != NO_ERROR))
		size = -1L;
	else
		size = low + (high << sizeof(DWORD));
	CloseHandle((HANDLE)hfile);
	app_free(dos_path);

	return size;
}

/*
 *  Return the file's last modifcation time in seconds.
 */

#define EPOCH 1970
#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_HOUR   (SECONDS_PER_MINUTE * 60)
#define SECONDS_PER_DAY    (SECONDS_PER_HOUR * 24)
#define SECONDS_PER_YEAR   (SECONDS_PER_DAY * 365)

static int month_days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int is_leap_year(int year)
{
	if (year % 4 != 0)
		return 0;
	if (year % 400 == 0)
		return 1;
	if (year % 100 == 0)
		return 0;
	return 1;
}

static long windows_system_time_to_epoch_time(SYSTEMTIME st)
{
	long t;
	int year, month, day;

	t = 0;
	/* go through previous years */
	for (year=EPOCH; year < st.wYear; year++) {
		t += SECONDS_PER_YEAR; /* add the year */
		if (is_leap_year(year))
			t += SECONDS_PER_DAY; /* add the leap day */
	}
	/* now we're at the current year */
	if (st.wMonth > 2) /* after february in a leap year? */
		if (is_leap_year(st.wYear))
			t += SECONDS_PER_DAY; /* yes, so add the leap day */
	/* skip previous months */
	for (month=0; month < st.wMonth-1; month++)
		t += SECONDS_PER_DAY * month_days[month];
	/* now we're at the current month */
	/* skip previous days */
	for (day=0; day < st.wDay-1; day++)
		t += SECONDS_PER_DAY;
	/* now we're at the current day */
	t += SECONDS_PER_HOUR * st.wHour;
	t += SECONDS_PER_MINUTE * st.wMinute;
	t += st.wSecond;
	return t;
}

long app_file_time(const char *filepath)
{
	long mtime = -1L;
	char *dos_path;
	HFILE hfile;
	OFSTRUCT data;
	FILETIME made;
	FILETIME access;
	FILETIME modify;
	SYSTEMTIME st;

	/* the root desktop is special */
	if (filepath[0] == '\0')
		return mtime;

	/* floppy disk drive letters are special */
	if ( (((filepath[0] >= 'a') && (filepath[0] <= 'b'))
	   || ((filepath[0] >= 'A') && (filepath[0] <= 'B')))
	  && (filepath[1] == ':') && (filepath[2] == '\0') )
	{
		return mtime;
	}

	dos_path = app_to_native_path(filepath);
	hfile = OpenFile(dos_path, &data, OF_READ);
	if (GetFileTime((HANDLE)hfile, &made, &access, &modify) == TRUE) {
		if (FileTimeToSystemTime(&modify, &st) == TRUE)
			mtime = windows_system_time_to_epoch_time(st);
	}
	CloseHandle((HANDLE)hfile);
	app_free(dos_path);

	return mtime;
}

/*
 *  Return the current working directory.
 *  Ensures '/' is at the end of the string.
 *  The string must later be freed using app_free.
 */
char * app_current_folder(void)
{
	char *dos_path;
	char *path;
	long i, size = 512;

	dos_path = app_zero_alloc(size);
	if (! dos_path)
		return NULL;

	size = GetCurrentDirectory(size-2, dos_path);
	if (size == 0) { /* failed */
		app_free(dos_path);
		return NULL;
	}
	else if (size > 510) { /* wasn't large enough */
		dos_path = app_realloc(dos_path, size+2);
		GetCurrentDirectory(size, dos_path);
	}

	i = (long) strlen(dos_path) - 1;
	if (i >= 0) {
		if (dos_path[i] != '\\') {
			dos_path[i+1] = '\\';
			dos_path[i+2] = '\0';
		}
	} else {
		dos_path[0] = '\\';
		dos_path[1] = '\0';
	}

	path = app_to_portable_path(dos_path);
	app_free(dos_path);
	return path;
}

/*
 *  Changes the current working directory
 *  to the specified folder.
 */
int app_set_current_folder(const char *folderpath)
{
	int result;
	char *dos_path;

	if (folderpath[0] == '\0')
		return 1;

	dos_path = app_to_native_path(folderpath);

	if (SetCurrentDirectory(dos_path) == TRUE)
		result = 1;
	else
		result = 0;
	app_free(dos_path);
	return result;
}

/*
 *  Return foldername + filename, add the directory separator char
 *  between the two strings if necessary. The returned string must
 *  be destroyed by the calling function, using app_free().
 */
char * app_form_file_path(const char *foldername, const char *filename)
{
	long length, dirlen;
	char *full_name = NULL;
	char *folder_name = app_to_portable_path(foldername);
	char *file_name = app_to_portable_path(filename);

	dirlen = (long) strlen(folder_name);
	length = dirlen + (long) strlen(file_name) + 2;
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

	path = app_to_portable_path(filepath);

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
