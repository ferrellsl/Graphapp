/*
 * Monty - a simple project editor.
 *
 * File: main.c -- the main Monty program.
 * Platform: Neutral  Version: 3.00  Date: 2002/08/02
 *
 * Version: 1.00  Changes: Original version by L. Patrick.
 * Version: 2.00  Changes: Updated to GraphApp version 3.
 * Version: 3.00  Changes: Updated Windows version.
 * Version: 3.40  Changes: Added icon.
 */

/* Copyright (C) 1998 L. Patrick

   This file is part of Monty, a simple project editor.
*/

/*
 *  Forward declarations:
 */
		static void do_open(Control *c);

/*
 *  Window information structure:
 */
	typedef struct WindowData {
		/* General fields used by Monty: */
		Window *win;

		long	prev_click;
		int 	prev_file;
		int 	prev_folder;

		char *	result;
		Control *question;
		Control *text;
		Control *open, *run;

		/* The following are used in the file dialog: */
		MenuBar	*mbar;
		MenuItem *select_all, *select_text, *select_one;
		Control *filename, *dirname; /* text fields */
		Control *name_label, *path_label;
		Control *file_label, *folder_label;
		Control *file_list, *folder_list; /* list boxes */
	} WindowData;

	#define data(w) ((WindowData *) (app_get_window_data(w)))

/*
 *  Various measurements and settings.
 */
	enum {
		LEFT_BORDER      = 10,
		TOP_BORDER       = 10,
		MIN_BUTTON_WIDTH = 80,
		MIN_WIDTH        = 200,
		MAX_WIDTH        = 450,
		MIN_HEIGHT       = 20,
		MAX_HEIGHT       = 200,
		INTERNAL_SPACE   = 5
	};

	#define BUTTON_FONT	SystemFont
	#define TEXT_FONT	FixedFont

	#define LABEL_HEIGHT	16
	#define CONTROL_HEIGHT	24

	#define BUTTON_BG	LIGHT_GREY
	#define BUTTON_FG	BLACK

	Colour WINDOW_BG [] = {

	/*	{0, 0xff, 0xff, 0x99},	/* yellow */
		{0, 0xea, 0xad, 0x62},	/* yellow ochre */
	/*	{0, 0xff, 0x66, 0x44},	/* bold red-orange */
		{0, 0xb9, 0x1a, 0x62},	/* burgundy */
		{0, 0x9b, 0x61, 0x12},	/* chocolate */
		{0, 0x62, 0x33, 0x12},	/* dark chocolate */
	/*	{0, 0xff, 0x33, 0x99},	/* heavy magenta */

		{0, 0x66, 0xcc, 0xcc},	/* aqua */
		{0, 0x33, 0x99, 0xff},	/* bold blue */
		{0, 0x66, 0xcc, 0x99},	/* rich forest green */
		{0, 0x7f, 0x7f, 0xff},	/* bluey purpley */
		{0, 0xbe, 0x22, 0x87},	/* red purpley */
		{0, 0xcc, 0x99, 0xcc},	/* lilac */
		{0, 0x99, 0x99, 0xff},	/* purple */
		{0, 0x32, 0x30, 0x87},	/* navy blue */

		{0, 0x99, 0xcc, 0xff},	/* sky blue */
	/*	{0, 0xff, 0xcc, 0x99},	/* orange */

		{0, 0xcc, 0xcc, 0xff},	/* pale lilac */
	/*	{0, 0xd2, 0xf5, 0xee},	/* pale greeny */

	/*	{0, 0xe8, 0xee, 0xf0},	/* very pale blue grey */
	/*	{0, 0xc3, 0xd7, 0xeb},	/* pale blue */
		{0, 0xc3, 0xb0, 0xea},	/* pale bluey purpley */

	/*	{0, 0x39, 0x99, 0xa3},	/* dark blue green */
	/*	{0, 0xff, 0xe9, 0xc2},	/* parchment */
	/*	{0, 0xfa, 0xff, 0xbf},	/* golden yellow */
	};
	#define NUM_BGS (sizeof(WINDOW_BG)/sizeof(WINDOW_BG[0]))

/*
 *  Some strings to use:
 */
	static char * PROGRAM_NAME    = "Monty";

/*	static char * YES_STRING      = "Yes"; */
/*	static char * NO_STRING       = "No"; */
/*	static char * OK_STRING       = "Okay"; */
/*	static char * CANCEL_STRING   = "Cancel"; */
	static char * OPEN_STRING     = "Open";
/*	static char * SAVE_STRING     = "Save"; */
	static char * QUESTION_STRING = "Question";
	static char * ERROR_STRING    = "Error";
/*	static char * EDIT_STRING     = "Edit"; */
	static char * RUN_STRING      = "Run";
/*	static char * COMPILE_STRING  = "Compile"; */

/*
 *  File dialog Control *functions:
 */
	typedef struct FileListData {
		char *  foldername;
		char ** filenames;
		char ** descriptions;
	} FileListData;

	#define get_file_list_data(box) \
		((FileListData *) app_get_control_data(box))

static int sort_strings(const void *a, const void *b)
{
	char *s = (*(char **)a);
	char *t = (*(char **)b);

	return strcmp(s,t);
}

void set_window_name(Window *win, char *foldername)
{
	char *name;

	name = add_strings(NULL, PROGRAM_NAME);
	name = add_strings(name, ": ");
	name = add_strings(name, foldername);

	app_set_window_title(win, name);

	app_del_string(name);
}

/*
 *  Append a date string to the given string.
 *  Assume there is enough space in the string for the date.
 */
void append_date(char *str, long seconds)
{
	time_t t = (time_t) seconds;
	struct tm *date;
	char buffer[30];

	date = localtime(&t);
	strftime(buffer, 26, "%H:%M %b %d %Y %a", date);
	strcat(str, buffer);
}

void setlistfolder(Control *box, char *foldername, int flags)
{
	int i, info, num, which;
	int width, maxwidth = 14;
	long size, date;
	Folder *f;
	char * fullname;
	char * filename;
	char **filenames = NULL;
	long *sizes = NULL;
	long *dates = NULL;
	char **descriptions = NULL;
	char * desc;
	FileListData *fd;

	f = app_open_folder(foldername);
	if (! f)
		return;
	while ((filename = app_read_folder(f)) != NULL)
	{
		fullname = app_form_file_path(foldername, filename);
		info = app_file_info(fullname);
		size = app_file_size(fullname);
		date = app_file_time(fullname);
		app_del_string(fullname);

		if (!strcmp(filename, "."))
			continue;

		if (info & flags) {
			desc = app_copy_string(filename);
			append(filenames, desc);
			width = app_utf8_length(desc);
			if (width > maxwidth)
				maxwidth = width;
			append(sizes, size);
			append(dates, date);
		}
	}
	app_close_folder(f);

	if (flags == IS_FOLDER) {
	    for (num=len(filenames), i=0; i<num; i++) {
		if (strcmp(filenames[i], "..") == 0)
			desc = app_copy_string("[ Parent Folder ]");
		else
			desc = app_copy_string(filenames[i]);
		append(descriptions, desc);
	    }
	    if (len(descriptions) > 1)
		qsort(descriptions+1, len(descriptions)-1, sizeof(char*),
			sort_strings);
	    if (len(filenames) > 1)
		qsort(filenames+1, len(filenames)-1, sizeof(char*),
			sort_strings);
	} else {
	    for (num=len(filenames), i=0; i<num; i++) {
		desc = array(maxwidth+15+28, char);
		sprintf(desc, "%-*s %7ld K  ", maxwidth,
				filenames[i], (sizes[i]+1023)/1024);
		append_date(desc, dates[i]);
		append(descriptions, desc);
	    }
	    if (len(descriptions) > 0)
		qsort(descriptions, len(descriptions), sizeof(char*),
			sort_strings);
	    if (len(filenames) > 0)
		qsort(filenames, len(filenames), sizeof(char*),
			sort_strings);
	}
	discard(sizes);
	discard(dates);

	append(descriptions, NULL);
	append(filenames, NULL);

	which = app_get_control_value(box);
	app_change_list_box(box, descriptions);
	app_set_list_box_item(box, which);

	fd = get_file_list_data(box);
	if (fd) {
		app_del_string(fd->foldername);
		if (fd->filenames)
			for (i=0; fd->filenames[i]; i++)
				app_del_string(fd->filenames[i]);
		discard(fd->filenames);
		if (fd->descriptions)
			for (i=0; fd->descriptions[i]; i++)
				app_del_string(fd->descriptions[i]);
		discard(fd->descriptions);
	}
	else {
		fd = create (FileListData);
		app_set_control_data(box, fd);
	}
	fd->foldername = app_copy_string(foldername);
	fd->filenames = filenames;
	fd->descriptions = descriptions;

	set_window_name(app_parent_window(box), foldername);
}

Control *newfilelist(Window *win, Rect r, ControlFunc select_file)
{
	Control *box;

	box = app_new_list_box(win, r, NULL, select_file);
	return box;
}

Control *newfolderlist(Window *win, Rect r, ControlFunc select_folder)
{
	Control *box;

	box = app_new_list_box(win, r, NULL, select_folder);
	return box;
}

/*
 *  Dialog functions:
 */

static void add_data(Window *w)
{
	WindowData *d;

	d = create (WindowData);
	if (! d)
		return;

	d->win = w;
	d->prev_file   = -1;
	d->prev_folder = -1;

	app_set_window_data(w, d);
}

static char * get_current_path(Window *w)
{
	WindowData *d = data(w);
	app_del_string(d->result);
	if (d->filename)	/* file dialog */
		d->result = app_form_file_path(
				app_get_control_text(d->dirname),
				app_get_control_text(d->filename));
	else if (d->text)	/* question dialog */
		d->result = app_copy_string(app_get_control_text(d->text));
	return d->result;
}

static char * get_current_filename(Window *w)
{
	WindowData *d = data(w);
	return app_get_control_text(d->filename);
}

static char * get_current_folder(Window *w)
{
	WindowData *d = data(w);
	return app_get_control_text(d->dirname);
}

static int chdir_here(Window *w)
{
	int result;
	char *folder = get_current_folder(w);
	folder = app_form_file_path(folder, ".");
	result = app_set_current_folder(folder);
	app_del_string(folder);
	return result;
}

/*
 *  Handle a keypress:
 */
static void hit_key(Window *w, unsigned long key)
{
	Control *btn;
	char * name = NULL;

	if (data(w) == NULL)
		return;

	if ((btn = data(w)->open) != NULL) {
		name = app_get_control_text(btn);
		if ((key == '\n') || (tolower(name[0]) == tolower(key)))
		{
			app_flash_control(btn);
			app_activate_control(btn);
			return;
		}
	}

	if ((btn = data(w)->run) != NULL) {
		name = app_get_control_text(btn);
		if ((tolower(name[0]) == tolower(key)))
		{
			app_flash_control(btn);
			app_activate_control(btn);
			return;
		}
	}
	if (btn)
		app_pass_event(btn);
}

/*
 *  Choosing files and folders from the lists:
 */
static void choose_file(Control *box)
{
	Window *win = app_parent_window(box);
	WindowData *d;
	FileListData *fd;
	char * filename;
	int which = app_get_control_value(box);
	int double_click = 0;

	if (which < 0)
		return;

	d = data(win);
	fd = get_file_list_data(box);
	filename = fd->filenames[which];
	app_set_control_text(d->filename, filename);

	/* handle double click */
	if (which == d->prev_file) {
		if (time(NULL) - d->prev_click <= 1)
			double_click = 1;
	}
	d->prev_file   = which;
	d->prev_folder = -1;
	d->prev_click  = time(NULL);
	if (double_click == 0)
		return;
	else {
		do_open(box);
	}
}

static void change_folder(Window *w, char *path)
{
	WindowData *d = data(w);

	if (app_set_current_folder(path) == 1) {
		app_set_control_text(d->dirname, path);
		setlistfolder(d->folder_list, path, IS_FOLDER);
		setlistfolder(d->file_list, path, IS_FILE);
	}
}

static void choose_folder(Control *box)
{
	Window *win = app_parent_window(box);
	WindowData *d;
	FileListData *fd;
	char * foldername;
	char * path;
	char * dirname;
	int which = app_get_control_value(box);
	int double_click = 0;

	if (which < 0)
		return;

	d = data(win);
	fd = get_file_list_data(box);

#if 0
	if (which == d->prev_folder) {
		if (currenttime() - d->prev_click <= 500)
			double_click = 1;
	}
	d->prev_file   = -1;
	d->prev_folder = which;
	d->prev_click  = currenttime();
	if (double_click == 0)
		return;
#endif

	dirname = app_get_control_text(d->dirname);
	path = fd->filenames[which];
	foldername = app_form_file_path(dirname, path);
	path = app_form_file_path(foldername, "");

	if (app_set_current_folder(foldername) == 1) {
		/* app_set_control_text(d->filename, ""); */
		app_set_control_text(d->dirname, path);
		setlistfolder(d->folder_list, path, IS_FOLDER);
		setlistfolder(d->file_list, path, IS_FILE);
	}

	app_del_string(path);
	app_del_string(foldername);

	app_set_list_box_item(box, -1);
}

static void rescan_folder(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	WindowData *d;
	char * dirname;

	d = data(w);
	dirname = get_current_folder(w);
	setlistfolder(d->folder_list, dirname, IS_FOLDER);
	setlistfolder(d->file_list, dirname, IS_FILE);
}

/*
 *  Menu functions:
 */
static char * expand_path(char *path)
{
	char * expanded = NULL;
	char *home = getenv("HOME");

	if (home == NULL)
		return NULL;
	else if (strncmp(path, "$HOME", 5) == 0) {
		expanded = add_strings(NULL, home);
		expanded = add_strings(expanded, path+5);
	}
	else if (strncmp(path, "$(HOME)", 7) == 0) {
		expanded = add_strings(NULL, home);
		expanded = add_strings(expanded, path+7);
	}

	return expanded;
}

static int valid_folder(char *path)
{
	int info = app_file_info(path);
	if (info & IS_FOLDER)
		return 1;
	return 0;
}

static int valid_file(Window *w, char *errmsg)
{
	char * file = app_get_control_text(data(w)->filename);

	if (strcmp(file, "") == 0) {
		app_ask_ok(w->app, ERROR_STRING, errmsg);
		return 0;
	}
	return 1;
}

static void perform_function(App *app, char *cmd, char *filename)
{
	char *command = NULL;

	if (strcmp(cmd, "") != 0) {
		command = add_strings(NULL, cmd);
		command = add_strings(command, " ");
	}
	command = add_strings(command, filename);
	#ifndef __WIN32__
	command = add_strings(command, " &");
	#endif
	/* fprintf(stderr, "%s\n", command); */
	/* fflush(stderr); */
	app_exec(app, command);
	app_del_string(command);
}

static void perform_function_now(App *app, char *cmd, char *filename)
{
	char *command = NULL;

	if (strcmp(cmd, "") != 0) {
		command = add_strings(command, cmd);
		command = add_strings(command, " ");
	}
	command = add_strings(command, filename);
	app_exec(app, command);
	app_del_string(command);
}

static void perform_open(App *app, char *filename)
{
	char * editor = get_editor(filename);

	perform_function(app, editor, filename);
}

static void do_new(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);

	extern Window *init_monty_window(App *app);
	app_show_window(init_monty_window(w->app));
}

static void do_open_location(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	WindowData *d;
	char *dirname;

	dirname = app_ask_string(w->app, OPEN_STRING,
		"Open which location?", get_current_folder(w));

	if (! dirname)
		return;

	d = data(w);
	app_set_control_text(d->dirname, dirname);
	app_del_string(dirname);
	rescan_folder(m);
}

static void do_close(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);

	app_hide_window(w);
}

static void do_hexdump(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	char *arg[1];

	#ifdef __WIN32__

	arg[0] = get_current_path(w);
	hexdump(1, arg);

	#else

	perform_function(w->app, "khexedit", get_current_path(w));

	#endif
}

static void do_open(Control *c)
{
	Window *w = app_parent_window(c);

	chdir_here(w);
	if (! valid_file(w, "Choose a file to open first!"))
		return;
	perform_open(w->app, get_current_filename(w));
}

static void do_opentext(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);

	if (! valid_file(w, "Choose a file to open first!"))
		return;
	perform_function(w->app, get_editor_by_type("text/plain"),
			 get_current_filename(w));
}

static void do_openimage(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);

	if (! valid_file(w, "Choose a file to open first!"))
		return;
	perform_function(w->app, get_editor_by_type("image/*"),
			 get_current_filename(w));
}

static void do_run(Control *c)
{
	Window *w = app_parent_window(c);
	char *filename;

	chdir_here(w);
	if (! valid_file(w, "Choose a program to run first!"))
		return;
	filename = get_current_filename(w);
	perform_function(w->app, get_run(filename), filename);
}

static void do_exec(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);

	char *filename = m->text;
	perform_function(w->app, get_run(NULL), filename);
}

static void do_exec_file(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);

	if (! valid_file(w, "Choose a valid file first!"))
		return;
	perform_function_now(w->app, m->text,
			get_current_filename(w));
	rescan_folder(m);
}

static void do_exec_shell(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	char *filename;

	#ifdef __WIN32__
		filename = "dos.pif";
		perform_function(w->app, get_run(filename), filename);
	#else
		/* perform_function(w->app, get_run(NULL), "/bin/sh"); */
		perform_function(w->app, "/usr/bin/gnome-terminal -x", "/bin/bash --login");
	#endif
}

static void do_change_folder(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	char *path;

	path = app_copy_string(m->text);
	path = app_form_file_path(path, "");
	change_folder(w, path);
	app_del_string(path);
}

static void do_goto_folder(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	char *path;

	path = expand_path(m->text);
	if (valid_folder(path)) {
		path = app_form_file_path(path, "");
		change_folder(w, path);
	}
	app_del_string(path);

	/* rescan_folder(m); */
}

static void do_make_folder(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	char *this_folder = get_current_folder(w);
	char *name;
	char *new_folder = NULL;
	char *question = NULL;

	question = app_copy_string("Create a new folder called:");

	name = app_ask_string(w->app, QUESTION_STRING,
			question, NULL);
	if (name == NULL)
		return;
	new_folder = app_form_file_path(this_folder, name);
	app_del_string(name);

	if (app_make_folder(new_folder, 0755) == 0)
		app_ask_ok(w->app, ERROR_STRING,
			"Error: couldn't create the folder!");
	rescan_folder(m);

	app_del_string(new_folder);
	app_del_string(question);
}

static void do_copy_file(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	char * file1 = get_current_path(w);
	char * file2 = NULL;
	int info;

	info = app_file_info(file1);
	if (info & IS_FOLDER) {
		app_ask_ok(w->app, ERROR_STRING,
			"You must choose a valid file.");
		return;
	}
	if (! (info & IS_FILE)) {
		app_ask_ok(w->app, ERROR_STRING,
			"That is not a valid file.");
		return;
	}
	if (! (info & IS_READ)) {
		app_ask_ok(w->app, ERROR_STRING,
			"Sorry, the file isn't readable.");
		return;
	}

	file2 = app_ask_file_save(w->app, "Copy File", "Copy", file1);
	if (file2 == NULL)
		return;

	copy_file(file1, file2);
	rescan_folder(m);
}

static void do_delete_file(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	char * file;
	char * confirm;
	int info;

	if (! valid_file(w, "Choose a file to delete first!"))
		return;

	file = get_current_path(w);

	info = app_file_info(file);
	if (info & IS_FOLDER) {
		file = get_current_path(w);
		if (app_remove_folder(file) == 0)
			app_ask_ok(w->app, ERROR_STRING,
				"Error: couldn't remove that folder!");
		rescan_folder(m);
		return;
	}

	confirm = add_strings(NULL, "Delete the file \"");
	confirm = add_strings(confirm, app_get_control_text(data(w)->filename));
	confirm = add_strings(confirm, "\" now?");

	if (app_ask_yes_no(w->app, QUESTION_STRING, confirm) == YES) {
		if (app_remove_file(file) == 0)
			app_ask_ok(w->app, ERROR_STRING,
				"Error: couldn't delete that file!");
		rescan_folder(m);
	}

	app_del_string(confirm);
}

static void do_rename_file(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	char * path1 = get_current_path(w);
	char * file1;
	char * s;
	char * path2 = NULL;

	if (! valid_file(w, "Choose a file to rename first!"))
		return;

	file1 = path1;
	for (s = path1; *s; s++)
		if (*s == '\\')
			file1 = s+1;
	path2 = app_ask_file_save(w->app, "Rename File", "Rename", file1);

	if (path2 == NULL)
		return;
	if (app_rename_file(path1, path2) == 0)
		app_ask_ok(w->app, ERROR_STRING,
			"Error: couldn't rename that file!");
	rescan_folder(m);
}

static void do_split_file(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	char * file1 = get_current_path(w);
	int info;

	info = app_file_info(file1);
	if (info & IS_FOLDER) {
		app_ask_ok(w->app, ERROR_STRING,
			"You must choose a valid file.");
		return;
	}
	if (! (info & IS_FILE)) {
		app_ask_ok(w->app, ERROR_STRING,
			"That is not a valid file.");
		return;
	}
	if (! (info & IS_READ)) {
		app_ask_ok(w->app, ERROR_STRING,
			"Sorry, the file isn't readable.");
		return;
	}

	split_file(w->app, file1);
	rescan_folder(m);
}

static void do_join_files(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	char * file1 = get_current_path(w);
	int info;

	info = app_file_info(file1);
	if (info & IS_FOLDER) {
		app_ask_ok(w->app, ERROR_STRING,
			"You must choose a valid file.");
		return;
	}
	if (! (info & IS_FILE)) {
		app_ask_ok(w->app, ERROR_STRING,
			"That is not a valid file.");
		return;
	}
	if (! (info & IS_READ)) {
		app_ask_ok(w->app, ERROR_STRING,
			"Sorry, the file isn't readable.");
		return;
	}

	join_files(w->app, file1);
	rescan_folder(m);
}

/*
 *  Text file conversion facilities:
 */
static void do_select_all(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	WindowData *d = data(w);

	app_check_menu_item(d->select_all);
	app_uncheck_menu_item(d->select_text);
	app_uncheck_menu_item(d->select_one);
}

static void do_select_text(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	WindowData *d = data(w);

	app_uncheck_menu_item(d->select_all);
	app_check_menu_item(d->select_text);
	app_uncheck_menu_item(d->select_one);
}

static void do_select_one(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	WindowData *d = data(w);

	app_uncheck_menu_item(d->select_all);
	app_uncheck_menu_item(d->select_text);
	app_check_menu_item(d->select_one);
}

static void convert_selected_files(Window *w, int conversion)
{
	WindowData *d = data(w);
	Control *file_list = d->file_list;
	FileListData *fd = get_file_list_data(file_list);
	char * file;
	char * path;
	char * filetype;
	char **conv = NULL;
	int i;

	if (app_menu_item_is_checked(d->select_all)) {
	    for (i=0; fd->filenames[i]; i++) {
		file = app_copy_string(fd->filenames[i]);
		append(conv, file);
	    }
	}
	if (app_menu_item_is_checked(d->select_text)) {
	    for (i=0; fd->filenames[i]; i++) {
		path = add_strings(NULL, fd->foldername);
		path = add_strings(path, fd->filenames[i]);
		filetype = get_file_type(path);
		app_del_string(path);
		if (string_starts_with(filetype, "text/")) {
			file = app_copy_string(fd->filenames[i]);
			append(conv, file);
		}
	    }
	}
	if (app_menu_item_is_checked(d->select_one)) {
		file = app_copy_string(app_get_control_text(d->filename));
		append(conv, file);
	}
	append(conv, NULL);
	for (i=0; conv[i]; i++) {
		convert_file(fd->foldername, conv[i], conversion);
		app_del_string(conv[i]);
	}
	discard(conv);
}

static void do_conversion(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);

	convert_selected_files(w, m->value);
	rescan_folder(m);
}

/*
 *  Generate a background colour:
 */
static Colour new_bgcolour(void)
{
	static int which_bg = -1;
	Colour bg;

	if (which_bg == -1)
		which_bg = rand() % NUM_BGS;

	bg = WINDOW_BG[which_bg++];
	if (which_bg >= NUM_BGS)
		which_bg = 0;
	return bg;
}

/*
 *  Edit settings:
 */
void do_edit_extensions(MenuItem *m)
{
	edit_extensions();
}

void do_edit_types(MenuItem *m)
{
	edit_types();
}

void do_change_bg(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);
	Colour col = new_bgcolour();

	app_set_window_background(w, col);
}

void do_load_settings(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);

	load_settings();
}

void do_save_settings(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);

	save_settings();
}

void do_about_box(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);

	handle_about_box(w->app);
}

void do_quit(MenuItem *m)
{
	Window *w = app_parent_window(m->parent->parent->ctrl);

	if (app_ask_ok_cancel(w->app, "Exit?",
	    "Close all Monty windows and exit?") == YES)
		app_del_all_windows(app_parent_window(m->parent->parent->ctrl)->app);
		//exit(0);
}

/*
 *  Utilities for creating a new window:
 */
static Rect new_winrect(void)
{
	static int offset = 0;
	Rect r;
	int h, hh;

	h = LABEL_HEIGHT;
	hh = CONTROL_HEIGHT;

	r = rect(50+offset,50+offset,630,440);

	offset += 40;
	if (offset > 200)
		offset = 0;

	return r;
}

/*
 *  Add menus to a newly created window:
 */
static void add_file_menu(WindowData *d)
{
	Menu *m;

	m = app_new_menu(d->mbar, "File");
	app_new_menu_item(m, "New Window",	'N', do_new);
	app_new_menu_item(m, "Open Folder",	'O', do_open_location);
	app_new_menu_item(m, "Close",		'W', do_close);
	app_new_menu_item(m, "Close All Windows", 0,  do_quit);
	app_new_menu_item(m, "-",		 0,  NULL);
	app_new_menu_item(m, "Exit",		'Q', do_close);
}

static void add_edit_menu(WindowData *d)
{
	Menu *m;

	m = app_new_menu(d->mbar, "Edit");
	app_new_menu_item(m, "Reload Folder",	 0,  rescan_folder);
	app_new_menu_item(m, "-",		 0,  NULL);
	app_new_menu_item(m, "Open as hex",	'H', do_hexdump);
	app_new_menu_item(m, "Open as text",	'T', do_opentext);
	app_new_menu_item(m, "Open as image",	'I', do_openimage);
	app_new_menu_item(m, "-",		 0,  NULL);
	app_new_menu_item(m, "New Folder...",	'F', do_make_folder);
	app_new_menu_item(m, "-",		 0,  NULL);
	app_new_menu_item(m, "Duplicate file...",'D',do_copy_file);
	app_new_menu_item(m, "Rename file...",	'R', do_rename_file);
	app_new_menu_item(m, "Delete file",	DEL, do_delete_file);
	app_new_menu_item(m, "-",		 0,  NULL);
	app_new_menu_item(m, "Split File",	 0,  do_split_file);
	app_new_menu_item(m, "Join Files",	 0,  do_join_files);
}

static void add_convert_menu(WindowData *d)
{
	Menu *m;
	MenuItem *mi;

	m = app_new_menu(d->mbar, "Convert");

	d->select_all = app_new_menu_item(m, "All Files in this folder",
					0, do_select_all);
	d->select_text = app_new_menu_item(m, "All Text Files in this folder",
					0, do_select_text);
	d->select_one = app_new_menu_item(m, "Selected File",
					0, do_select_one);
	app_check_menu_item(d->select_one);

	app_new_menu_item(m, "-", 0,  NULL);

	mi = app_new_menu_item(m, "to DOS text file",  0, do_conversion);
	mi->value = CONVERT_TO_DOS;

	mi = app_new_menu_item(m, "to Unix text file", 0, do_conversion);
	mi->value = CONVERT_TO_UNIX;

	mi = app_new_menu_item(m, "to Mac text file",  0, do_conversion);
	mi->value = CONVERT_TO_MAC;

	app_new_menu_item(m, "-", 0,  NULL);

	mi = app_new_menu_item(m, "to Lower case names",  0, do_conversion);
	mi->value = CONVERT_TO_LOWER;

	mi = app_new_menu_item(m, "to Upper case names",  0, do_conversion);
	mi->value = CONVERT_TO_UPPER;
}

static void add_tools_menu(WindowData *d)
{
	Menu *m, *sm;

	m = app_new_menu(d->mbar, "Tools");
	app_new_menu_item(m, "shell",    SHIFT+'S', do_exec_shell);
	app_new_menu_item(m, "python",		0, do_exec);
	app_new_menu_item(m, "javac",		0, do_exec_file);

	sm = app_new_sub_menu(m, "make");
	app_new_menu_item(sm, "make",		0, do_exec);
	app_new_menu_item(sm, "make all",	0, do_exec);
	app_new_menu_item(sm, "make dynamic",	0, do_exec);
	app_new_menu_item(sm, "make install",	0, do_exec);
	app_new_menu_item(sm, "make tarfile",	0, do_exec);
	app_new_menu_item(sm, "make view",	0, do_exec);
	app_new_menu_item(sm, "-",		0, NULL);
	app_new_menu_item(sm, "make clean",	0, do_exec);
}

static void add_options_menu(WindowData *d)
{
	Menu *m;

	m = app_new_menu(d->mbar, "Options");
	app_new_menu_item(m, "File name Extensions...",	0, do_edit_extensions);
	app_new_menu_item(m, "File type Handlers...",	0, do_edit_types);
	app_new_menu_item(m, "-",			0, NULL);
	app_new_menu_item(m, "Change Background",	0, do_change_bg);
	app_new_menu_item(m, "-",			0, NULL);
	app_new_menu_item(m, "Load Settings",		0, do_load_settings);
	app_new_menu_item(m, "Save All Settings",	0, do_save_settings);
}

static void add_shortcuts_menu(WindowData *d)
{
	Menu *m, *sm;
	int i;
	char *path, *temp;

	m = app_new_menu(d->mbar, "Shortcuts");

	sm = app_new_sub_menu(m, "Local");
	app_new_menu_item(sm, "a:/", 0, do_change_folder);
	app_new_menu_item(sm, "b:/", 0, do_change_folder);
	app_new_menu_item(sm, "c:/", 0, do_change_folder);
	if (valid_folder(path = "/mnt/floppy"))
		app_new_menu_item(sm, path, 0, do_change_folder);
	if (valid_folder(path = "/mnt/cdrom"))
		app_new_menu_item(sm, path, 0, do_change_folder);
	if (valid_folder(path = "/mnt/other"))
		app_new_menu_item(sm, path, 0, do_change_folder);
	if (valid_folder(path = "/mnt/win"))
		app_new_menu_item(sm, path, 0, do_change_folder);

	sm = app_new_sub_menu(m, "Network");
	if (valid_folder(path = "/n/black/"))
		app_new_menu_item(sm, path, 0, do_change_folder);
	if (valid_folder(path = "/n/white/"))
		app_new_menu_item(sm, path, 0, do_change_folder);
	if (valid_folder(path = "/n/grey/"))
		app_new_menu_item(sm, path, 0, do_change_folder);
	if (valid_folder(path = "/n/staff/"))
		app_new_menu_item(sm, path, 0, do_change_folder);
	if (valid_folder(path = "/n/pgrad/"))
		app_new_menu_item(sm, path, 0, do_change_folder);
	if (valid_folder(path = "/n/hons/"))
		app_new_menu_item(sm, path, 0, do_change_folder);

	app_new_menu_item(m, "-", 0, NULL);

	if (getenv("HOME") == NULL)
		return;

	app_new_menu_item(m, "$HOME", 0, do_goto_folder);
	if (valid_folder(temp = expand_path(path = "$HOME/bin/")))
		app_new_menu_item(m, path, 0, do_goto_folder);
	if (temp)
		app_del_string(temp);
	if (valid_folder(temp = expand_path(path = "$HOME/apps/")))
		app_new_menu_item(m, path, 0, do_goto_folder);
	if (temp)
		app_del_string(temp);
	if (valid_folder(temp = expand_path(path = "$HOME/apps/app/")))
		app_new_menu_item(m, path, 0, do_goto_folder);
	if (temp)
		app_del_string(temp);
	if (valid_folder(temp = expand_path(path = "$HOME/me/")))
		app_new_menu_item(m, path, 0, do_goto_folder);
	if (temp)
		app_del_string(temp);
	if (valid_folder(temp = expand_path(path = "$HOME/me/games/")))
		app_new_menu_item(m, path, 0, do_goto_folder);
	if (temp)
		app_del_string(temp);
	if (valid_folder(temp = expand_path(path = "$HOME/me/stories/")))
		app_new_menu_item(m, path, 0, do_goto_folder);
	if (temp)
		app_del_string(temp);
	if (valid_folder(temp = expand_path(path = "$HOME/me/stories/novels/")))
		app_new_menu_item(m, path, 0, do_goto_folder);
	if (temp)
		app_del_string(temp);
}

static void add_help_menu(WindowData *d)
{
	Menu *m;

	m = app_new_menu(d->mbar, "Help");
	app_new_menu_item(m, "About Monty...", CONTROL+SHIFT+'A', do_about_box);
}

static void check_menubar(MenuBar *m)
{
	chdir_here(app_parent_window(m->ctrl));
}

static void add_menus(WindowData *d)
{
	d->mbar = app_new_menu_bar(d->win);

	add_file_menu(d);
	add_edit_menu(d);
	add_convert_menu(d);
	add_tools_menu(d);
	add_shortcuts_menu(d);
	add_options_menu(d);
	add_help_menu(d);
}

/*
 *  Change colour based on mouse drags in main window.
 */
static Point prev_mouse_pos;
static Colour prev_win_bg;

static void mouse_down_monty_window(Window *w, int buttons, Point p)
{
	prev_mouse_pos = p;
	prev_win_bg = app_get_window_background(w);
}

static void mouse_drag_monty_window(Window *w, int buttons, Point p)
{
	int dx, dy;
	int min_dist = 10; /* pixels motion before something happens */
	Colour bg = prev_win_bg;
	Rect cube = rect(0,0,32,32);
	Rect wr = app_get_window_area(w);

	cube.y = wr.height - cube.height;

	dx = prev_mouse_pos.x - p.x;
	dy = prev_mouse_pos.y - p.y;
	if (dx * dx + dy * dy < min_dist * min_dist) {
		return;
	}
	if (app_point_in_rect(p, cube)) {
		int r, g, b;

		if (dx > min_dist)
			dx -= min_dist;
		else if (dx < -min_dist)
			dx += min_dist;
		if (dy > min_dist)
			dy -= min_dist;
		else if (dy < -min_dist)
			dy += min_dist;

		r = bg.red;
		g = bg.green + dx;
		b = bg.blue + dy;
		bg = app_new_rgb(r, g, b);
	}
	else {
		bg = app_get_window_background(w);
	}

	if (! app_colours_equal(bg, prev_win_bg)) {
		Graphics *g = app_get_window_graphics(w);

		prev_win_bg = bg;
		app_set_colour(g, bg);
		app_fill_rect(g, cube);

		app_del_graphics(g);
	}
}

static void mouse_up_monty_window(Window *w, int buttons, Point p)
{
	Colour bg = app_get_window_background(w);

	if (! app_colours_equal(bg, prev_win_bg))
		app_set_window_background(w, prev_win_bg);
}

/*
 *  Create a resizeable window:
 */
static void resize_monty_window(Window *w)
{
	int h, hh, y;
	Rect r;
	WindowData *d = data(w);

	r = app_get_window_area(w);

	/* account for menu bar */
	r.y += d->mbar->ctrl->area.height;
	r.height -= r.y;

	h = LABEL_HEIGHT;
	hh = CONTROL_HEIGHT;
	y = r.height - 55 - hh*3 - h;

	app_set_control_area(d->filename, rect(60,r.y+10,r.width-60-10,hh));
	app_set_control_area(d->dirname, rect(60,r.y+15+hh,r.width-60-10,hh));

	if (r.width <= 400) {
		/* half of space is for folder list */
		app_set_control_area(d->folder_label,
			rect(r.width/2+10,r.y+25+hh*2,100,h));
		app_set_control_area(d->folder_list,
			rect(r.width/2+10,r.y+30+hh*2+h,r.width/2-20,y));
		app_set_control_area(d->file_list,
			rect(10,r.y+30+hh*2+h,r.width/2-20,y));
	}
	else {
		/* folder list has a maximal width */
		app_set_control_area(d->folder_label,
			rect(r.width-190,r.y+25+hh*2,100,h));
		app_set_control_area(d->folder_list,
			rect(r.width-190,r.y+30+hh*2+h,180,y));
		app_set_control_area(d->file_list,
			rect(10,r.y+30+hh*2+h,r.width-220,y));
	}

	app_set_control_area(d->open,
		rect(r.width/2-80-15,r.y+r.height-10-hh,80,hh));
	app_set_control_area(d->run, 
		rect(r.width/2+15,r.y+r.height-10-hh,80,hh));
}

Window *init_monty_window(App *app)
{
	Window *win;
	WindowData *d;
	char * path;
	int h, hh, y;

	path = app_current_folder();

	h = LABEL_HEIGHT;
	hh = CONTROL_HEIGHT;

	win = app_new_window(app, new_winrect(), PROGRAM_NAME,
			STANDARD_WINDOW + CENTERED);
	add_data(win);
	d = data(win);
	app_set_window_background(win, new_bgcolour());
	app_on_window_key_down(win, hit_key);
	app_on_window_resize(win, resize_monty_window);
	app_on_window_mouse_down(win, mouse_down_monty_window);
	app_on_window_mouse_drag(win, mouse_drag_monty_window);
	app_on_window_mouse_up  (win, mouse_up_monty_window);
	app_set_window_icon(win, montyicon_image);

	add_menus(d);
	y = d->mbar->ctrl->area.height;

	d->name_label = app_new_label(win, rect(10,y+10,60,hh), "File:",
				VALIGN_CENTER);
	d->filename   = app_new_field(win, rect(60,y+10,320,hh), "");
	/* app_set_control_textfont(d->filename, TEXT_FONT); */

	d->path_label = app_new_label(win, rect(10,y+15+hh,60,hh), "Path:",
				VALIGN_CENTER);
	d->dirname    = app_new_field(win, rect(60,y+15+hh,320,hh), path);
	/* app_set_control_textfont(d->dirname, TEXT_FONT); */
	app_disable(d->dirname);

	d->open = app_new_button(win, rect(110,y+195+hh*2+h,80,hh),
				OPEN_STRING, do_open);
	d->run  = app_new_button(win, rect(210,y+195+hh*2+h,80,hh),
				RUN_STRING, do_run);

	d->file_label = app_new_label(win, rect(10,y+25+hh*2,100,h),
				"Files:", ALIGN_LEFT);
	d->file_list  = newfilelist(win, rect(10,y+30+hh*2+h,180,150),
				choose_file);
	/* app_set_control_textfont(d->file_list, TEXT_FONT); */
	setlistfolder(d->file_list, path, IS_FILE);
	app_set_focus(d->file_list);

	d->folder_label = app_new_label(win, rect(210,y+25+hh*2,100,h),
				"Folders:", ALIGN_LEFT);
	d->folder_list  = newfolderlist(win, rect(210,y+30+hh*2+h,180,150),
				choose_folder);
	/* app_set_control_textfont(d->folder_list, TEXT_FONT); */
	setlistfolder(d->folder_list, path, IS_FOLDER);

	/*
	app_set_control_background(d->open, BUTTON_BG);
	app_set_control_foreground(d->open, BUTTON_FG);
	app_set_control_background(d->run,  BUTTON_BG);
	app_set_control_foreground(d->run,  BUTTON_FG);
	*/

	return win;
}

/*
 *  The main function:
 */

int main(int argc, char *argv[])
{
	App *app;

	srand(time(NULL));
	app_debug_memory(1);
	app = app_new_app(argc, argv);
	if (argc < 3) {
		app_show_window(init_monty_window(app));
	}
	else if (! strcmp(argv[1], "-hex")) {
		hexdump(argc-2, &(argv[2]));
	}
	else {
		app_show_window(init_monty_window(app));
	}
	app_main_loop(app);
	app_del_app(app);
	return 0;
}
