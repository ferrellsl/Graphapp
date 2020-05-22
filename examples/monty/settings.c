/*
 * Monty - a simple project editor.
 *
 * File: settings.c -- loading, saving, and using settings.
 * Platform: Neutral  Version: 2.00  Date: 2001/08/08
 *
 * Version: 1.00  Changes: Original version by L. Patrick.
 */

/* Copyright (C) 1998 L. Patrick

   This file is part of Monty, a simple project editor.
*/


/*
 *  Default actions:
 */

	typedef struct FileType {
		char *	extension;
		char * 	filetype;
	} FileType;

	typedef struct ProgList {
		char *	filetype;
		char *	prog;
	} ProgList;

	FileType DEFAULT_TYPES [] =
	{
		{ ".txt",	"text/plain" },
		{ ".htm",	"text/html" },
		{ ".html",	"text/html" },

		{ ".txt.gz",	"text/plain-gzip-compressed" },

		{ ".c", 	"text/source" },
		{ ".h", 	"text/source" },
		{ ".cpp",	"text/source" },
		{ ".java",	"text/source" },
		{ ".jav",	"text/source" },
		{ ".py",	"text/source" },
		{ ".bat",	"text/MS-batch" },
		{ ".ini",	"text/MS-ini" },
		{ ".sh",	"text/sh" },

		{ ".pif",	"application/MS-pif" },
		{ ".exe", 	"application/octet-stream" },
		{ ".dll", 	"application/octet-stream" },

		{ ".o", 	"application/octet-stream" },
		{ ".obj", 	"application/octet-stream" },
		{ ".lib", 	"application/octet-stream" },
	/*	{ ".a", 	"application/octet-stream" }, */
		{ ".so", 	"application/octet-stream" },
		{ ".class", 	"application/java" },
		{ ".jar",	"application/jar" },

		{ ".ps", 	"application/postscript" },
		{ ".ps.gz", 	"application/postscript" },
		{ ".eps", 	"application/postscript" },
		{ ".rtf", 	"application/rtf" },
		{ ".pdf", 	"application/pdf" },
		{ ".prj", 	"application/c++project" },

		{ ".wri", 	"application/MS-write" },

		{ ".zip", 	"application/zip" },
		{ ".gz", 	"application/gzip" },
		{ ".tar", 	"application/tar" },
		{ ".tgz", 	"application/tar-gzip" },
		{ ".sit", 	"application/stuffit" },

		{ ".gif",	"image/gif" },
		{ ".jpg",	"image/jpeg" },
		{ ".jpeg",	"image/jpeg" },
		{ ".png",	"image/png" },
		{ ".tiff",	"image/tiff" },
		{ ".tif",	"image/tiff" },
		{ ".xbm",	"image/xbitmap" },
		{ ".xpm",	"image/xpixmap" },
		{ ".fig",	"image/xfig" },
		{ ".bmp",	"image/MS-bmp" },

		{ ".snd",	"audio/basic" },
		{ ".au",	"audio/basic" },
		{ ".wav",	"audio/wav" },

		{ ".mpg",	"video/mpeg" },
		{ ".mpe",	"video/mpeg" },
		{ ".mpeg",	"video/mpeg" },
		{ ".avi",	"video/avi" },
	};

	#define DEFAULT_FILETYPE	"text/unknown"

#ifdef __WIN32__

	#define DEFAULT_SMALL_EDITOR	"notepad.exe"
	#define DEFAULT_EDITOR		"write.exe"
	#define DEFAULT_RUN		""

	ProgList DEFAULT_EDITORS [] = {
		{ "text/html",			DEFAULT_EDITOR },
		{ "text/source",		DEFAULT_EDITOR },
		{ "text/plain-gzip-compressed",	"zless" },
		{ "text/*",			DEFAULT_EDITOR },
		{ "image/jpeg",			"/apps/viewjpeg/viewjpeg" },
		{ "image/*",			"/apps/lview/lview31" },

		{ "application/zip",		"/apps/stuffit/expander" },
		{ "application/gzip",		"/apps/stuffit/expander" },
		{ "application/tar",		"/apps/winzip/winzip" },
		{ "application/tar-gzip",	"/apps/winzip/winzip" },
		{ "application/stuffit",	"/apps/stuffit/expander" },

		{ "application/postscript",	"monty -hex" },
		{ "application/MS-write",	"write.exe" },
		{ "application/c++project",	"/borlandc/bin/bcw.exe" },
		{ "application/*",		"monty -hex" },
		{ "audio/*",			"monty -hex" },
		{ "video/*",			"monty -hex" },
	};

	ProgList DEFAULT_EXECUTORS [] = {
		{ "text/html",			DEFAULT_EDITOR },
		{ "text/source",		DEFAULT_EDITOR },
		{ "text/MS-batch",		DEFAULT_RUN },
		{ "text/plain-gzip-compressed",	"zless" },
		{ "text/*",			DEFAULT_EDITOR },
		{ "image/*",			"/apps/lview/lview31" },
		{ "application/zip",		"/apps/stuffit/expander" },
		{ "application/gzip",		"/apps/stuffit/expander" },
		{ "application/tar-gzip",	"/apps/winzip/winzip" },
		{ "application/tar",		"/apps/winzip/winzip" },
		{ "application/stuffit",	"/apps/stuffit/expander" },
		{ "application/*",		DEFAULT_RUN },
	};
#else
	#define DEFAULT_SMALL_EDITOR	"/home/loki/bin/edittext"
	#define DEFAULT_EDITOR		"/home/loki/bin/edittext"
	#define DEFAULT_RUN_OLD		"xterm +ls -T \"xterm\" -e"
	#define DEFAULT_RUN_OLD2	"xterm -T \"xterm\" -e"
	#define DEFAULT_RUN_OLD3	"/usr/bin/gnome-terminal --login"
	#define DEFAULT_RUN		"gnome-terminal -t \"terminal\" -e"

	ProgList DEFAULT_EDITORS [] = {
		{ "text/html",			DEFAULT_EDITOR },
		{ "text/source",		DEFAULT_EDITOR },
		{ "text/plain-gzip-compressed",	"gnome-terminal -t \"zless\" -e zless" },
		{ "text/plain-gzip-compressed",	"xterm -T \"xterm\" -e zless" },
		{ "text/*",			DEFAULT_EDITOR },
		{ "image/xfig",			"xfig" },
		{ "image/*",			"/home/loki/bin/imagine" },
		{ "application/jar",		"jar xf" },
		{ "application/postscript",	"ggv" },
		{ "application/pdf",		"gpdf" },
		{ "application/*",		"khexedit" },
		{ "audio/*",			"khexedit" },
		{ "video/*",			"mplayer" },
	};

	ProgList DEFAULT_EXECUTORS [] = {
		{ "text/html",			"appletviewer" },
		{ "text/plain-gzip-compressed",	"gnome-terminal -t \"zless\" -e zless" },
		{ "text/plain-gzip-compressed",	"xterm -T \"xterm\" -e zless" },
		{ "text/plain",			"sh" },
		{ "image/xfig",			"xfig" },
		{ "image/*",			"xv" },
		{ "video/*",			"mplayer" },
		{ "application/jar",		"java -jar" },
		{ "application/java",		"java" },
		{ "application/zip",		"unzip" },
	};
#endif

	#define NUM_TYPES     (sizeof(DEFAULT_TYPES)/sizeof(DEFAULT_TYPES[0]))
	#define NUM_EDITORS   (sizeof(DEFAULT_EDITORS)/sizeof(DEFAULT_EDITORS[0]))
	#define NUM_EXECUTORS (sizeof(DEFAULT_EXECUTORS)/sizeof(DEFAULT_EXECUTORS[0]))

	FileType *type_list	= DEFAULT_TYPES;
	int     num_types	= NUM_TYPES;
	ProgList *editor_list	= DEFAULT_EDITORS;
	int     num_editors	= NUM_EDITORS;
	ProgList *executor_list	= DEFAULT_EXECUTORS;
	int     num_executors	= NUM_EXECUTORS;

/* Default settings: */

	#ifdef __WIN32__
	char *	ini_file	= "monty.ini";
	char *	search_path []	= { ".", "", "c:\\windows", NULL };
	#else
	char *	ini_file	= ".monty.ini";
	char *	search_path []	= { ".", "", "$HOME", NULL };
	#endif

/* Finding settings: */

/*
 *  Determine which editor to use, based on the filename:
 */
char *get_file_type(char *filename)
{
	int i;
	char *extension;
	char *filetype = NULL;

	if (filename == NULL)
		return DEFAULT_FILETYPE;
	for (i=0; i < num_types; i++) {
		extension = type_list[i].extension;
		filetype  = type_list[i].filetype;
		if (first_word_ends_with(filename, extension))
			break;
		filetype = NULL;
	}
	if (filetype == NULL)
		filetype = DEFAULT_FILETYPE;

	return filetype;
}

char *get_editor_by_type(char *filetype)
{
	int i;
	char *type;
	char *editor = NULL;

	if (filetype == NULL)
		filetype = DEFAULT_FILETYPE;

	for (i=0; i < num_editors; i++) {
		type = editor_list[i].filetype;
		editor = editor_list[i].prog;
		if (app_regex_match(type, filetype))
			break;
		editor = NULL;
	}
	if (editor == NULL)
		editor = DEFAULT_EDITOR;
	return editor;
}

char *get_executor_by_type(char *filetype)
{
	int i;
	char *type;
	char *executor = NULL;

	if (filetype == NULL)
		filetype = DEFAULT_FILETYPE;

	for (i=0; i < num_executors; i++) {
		type = executor_list[i].filetype;
		executor = executor_list[i].prog;
		if (app_regex_match(type, filetype))
			break;
		executor = NULL;
	}
	if (executor == NULL)
		executor = DEFAULT_RUN;
	return executor;
}

char *get_editor(char *filename)
{
	char *prog = get_editor_by_type(get_file_type(filename));
	if (!strcmp(prog, DEFAULT_EDITOR)) {
		if ((filename != NULL) && (app_file_size(filename) < 32000))
			prog = DEFAULT_SMALL_EDITOR; 
	}
	return prog;
}

/*
 *  Determine how to run a file:
 */
char *get_run(char *filename)
{
	char *prog = get_executor_by_type(get_file_type(filename));
	if (!strcmp(prog, DEFAULT_EDITOR)) {
		if ((filename != NULL) && (app_file_size(filename) < 32000))
			prog = DEFAULT_SMALL_EDITOR;
	}
	return prog;
}


/*
 *  Load the settings from ini file. If the file cannot be loaded,
 *  the current settings in memory are used.
 */
void load_settings(void)
{
}

/*
 *  Save settings to ini file.
 */
void save_settings(void)
{
}

/*
 *  Allow editing of file name extension settings.
 */
void edit_extensions(void)
{
}

/*
 *  Allow editing of file type settings.
 */
void edit_types(void)
{
}
