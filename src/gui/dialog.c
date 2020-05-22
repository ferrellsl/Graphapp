/*
 *  Dialog box functions.
 *
 *  Platform: Neutral.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Added partial non-graphical support.
 *  Version: 3.03  2001/10/20  Slight change to text_height signature.
 *  Version: 3.25  2002/07/07  File dialogs now remember directory location.
 *  Version: 3.26  2002/07/31  File dialogs now remember position and size.
 *  Version: 3.30  2002/08/25  Dialogs now have pale grey backgrounds.
 *  Version: 3.35  2002/12/23  Now uses app_get_string for localisation.
 *  Version: 3.40  2003/03/07  More Windows-like look.
 *  Version: 3.41  2003/03/20  Modal windows are now also floating.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced some size_t conversion warnings.
 *  Version: 3.57  2005/08/16  Memory leaks plugged: now deletes graphics.
 *  Version: 3.60  2007/06/06  Now using auto-layout.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "app.h"
#include "appgui.h"

/*
 *  Read-only variables:
 *  Keys used to find the actual strings to display.
 */

static const char * OK_STRING                 = "OK";
static const char * YES_STRING                = "Yes";
static const char * NO_STRING                 = "No";
static const char * CANCEL_STRING             = "Cancel";
static const char * QUIT_STRING               = "Quit";
static const char * CREATE_STRING             = "Create";
static const char * OVERWRITE_STRING          = "Overwrite";
static const char * ERROR_STRING              = "Error";
static const char * FILE_STRING               = "File:";
static const char * FILES_STRING              = "Files:";
static const char * PATH_STRING               = "Path:";
static const char * FOLDERS_STRING            = "Folders:";
static const char * CREATE_FILE_STRING        = "Create File";
static const char * ERROR_CREATE_FILE_STRING  = "Error: Create File";
static const char * CONFIRM_CREATE_STRING     = "Create file?";
static const char * ALREADY_FOLDER_STRING     = "Already a folder!";
static const char * OVERWRITE_FILE_STRING     = "Overwrite File";
static const char * ERROR_SAVE_FILE_STRING    = "Error: Save File";
static const char * CONFIRM_OVERWRITE_STRING  = "Overwrite file?";

/*
 *  Private functions:
 */

  typedef struct SimpleDialog  SimpleDialog;
  struct SimpleDialog {
	const char *message;
	const char *result;
	Control *btn[3];
	Control *field;
  };

  typedef struct FileDialog  FileDialog;
  struct FileDialog
  {
	/* General fields used by dialogs: */
	const char *message;
	const char *result;
	Control *btn[3];
	Control *field;

	/* The following are used in the file dialogs: */
	Control *filename;	/* field */
	Control *dirname;	/* field */
	Control *name_label;	/* label */
	Control *path_label;	/* label */
	Control *file_label;	/* label */
	Control *folder_label;	/* label */
	Control *file_list;	/* list box */
	Control *folder_list;	/* list box */
	Rect    *area;          /* remember dialog position */

	WindowFunc confirm;

	char *   foldername;
	char **  filenames;
	char **  dirnames;
  };

static void app_dialog_draw_text(Window *w, Graphics *g)
{
	Rect r;
	SimpleDialog *sd;
	const char *str;
	int len;

	sd = app_get_window_data(w);
	str = sd->message;
	if (str == NULL)
		return;
	len = (int) strlen(str);

	r = app_get_window_area(w);
	if (r.width > 30)
		r.x += 10; r.width -= 20;
	if (r.height > 50)
		r.y += 10; r.height -= 50;

	app_draw_text(g, r, ALIGN_LEFT | VALIGN_TOP, str, len);
}

static void app_dialog_resize(Window *w)
{
	Control *c;
	Rect r, cr;
	SimpleDialog *sd;

	sd = app_get_window_data(w);
	r = app_get_window_area(w);

	c = sd->btn[0];
	if (c != NULL) {
		cr = c->area;
		cr.x = 10;
		cr.y = r.height - 5 - cr.height;
		app_move_control(c, cr);
	}

	c = sd->btn[1];
	if (c != NULL) {
		cr = c->area;
		cr.x = (r.width - cr.width) / 2;
		cr.y = r.height - 5 - cr.height;
		app_move_control(c, cr);
	}

	c = sd->btn[2];
	if (c != NULL) {
		cr = c->area;
		cr.x = r.width - 10 - cr.width;
		cr.y = r.height - 5 - cr.height;
		app_move_control(c, cr);
	}

	c = sd->field;
	if (c != NULL) {
		cr = c->area;
		cr.x = 10;
		cr.y = r.height - 5 - 30 - 20 - cr.height;
		cr.width = r.width - 20;
		app_set_control_area(c, cr);
	}
}

static void app_dialog_key(Window *w, unsigned long key)
{
	SimpleDialog *sd;

	sd = app_get_window_data(w);
	if ((key == '\n') && (sd->btn[0])) {
		sd->result = app_get_control_text(sd->btn[0]);
		app_hide_window(w);
	}
	else if ((key == ESC) && (sd->btn[2])) {
		sd->result = app_get_control_text(sd->btn[2]);
		app_hide_window(w);
	}
	else if ((key == '\n') && (sd->btn[1]) && (! sd->btn[2])) {
		sd->result = app_get_control_text(sd->btn[1]);
		app_hide_window(w);
	}
}

static void app_dialog_dismiss(Control *c)
{
	Window *w;
	SimpleDialog *sd;

	w = app_parent_window(c);
	sd = app_get_window_data(w);
	sd->result = app_get_control_text(c);
	app_hide_window(w);
}

static int app_handle_message_box(Window *w, int cancel)
{
	SimpleDialog *sd;
	const char *str;
	App *app = w->app;
	int result = cancel;

	sd = app_get_window_data(w);

	app_show_window(w);
	while (w->state & VISIBLE)
		app_wait_event(app);

	str = sd->result;
	if (str != NULL)
	{
		if (!strcmp(str, app_get_string(app, OK_STRING)))
			result = YES;
		else if (!strcmp(str, app_get_string(app, YES_STRING)))
			result = YES;
		else if (!strcmp(str, app_get_string(app, NO_STRING)))
			result = NO;
		else if (!strcmp(str, app_get_string(app, CANCEL_STRING)))
			result = CANCEL;
		else if (!strcmp(str, app_get_string(app, CREATE_STRING)))
			result = YES;
		else if (!strcmp(str, app_get_string(app, OVERWRITE_STRING)))
			result = YES;
	}

	return result;
}

static int app_text_message_box(const char *title, const char *message,
	const char *name1, const char *name2, const char *name3, int cancel)
{
	int ch, result, found;

	printf("%s: %s", title, message);
	if (name1 == NULL) {
		/* shuffle down */
		name1 = name2;
		name2 = name3;
		name3 = NULL;
	}
	if (name1 == NULL) {
		/* shuffle down */
		name1 = name2;
		name2 = NULL;
	}
	else if (name2 == NULL) {
		/* shuffle down */
		name2 = name3;
		name3 = NULL;
	}
	if (name1 == NULL) {
		printf("\n");
		return cancel;
	}
	printf("(");
	if (name1)
		printf("%s", name1);
	if (name2)
		printf(",%s", name2);
	if (name3)
		printf(",%s", name3);
	printf(")\n");

	result = cancel;
	found = 0;
	while ((ch = getc(stdin)) != EOF) {
		if (ch == '\n')
			break;
		if (ch == ' ')
			continue;
		if (ch == '\t')
			continue;
		if (found)
			continue;
		if (name1 && (ch == name1[0]))
			result = YES;
		else if (name2 && (ch == name2[0]))
			result = NO;
		else if (name3 && (ch == name3[0]))
			result = CANCEL;
		found = 1;
	}
	return result;
}

static int app_message_box(App *app, const char *title, const char *message,
	const char *name1, const char *name2, const char *name3, int cancel)
{
	Rect r;
	Window *w;
	SimpleDialog *sd;
	Font *f;
	Graphics *g;
	int h;
	int result;

	if ((app == NULL) || (app->gui_available == 0)) {
		return app_text_message_box(title, message,
			name1, name2, name3, cancel);
	}

	r = rect(500,400,400,150);
	w = app_new_window(app, r, title,
		BASE | MODAL | FLOATING | TITLEBAR | CLOSEBOX | CENTERED);

	app_set_window_background(w, BACKGROUND);
	f = app_new_font(app, "menufont", PLAIN, 16);
	if (! f)
		f = app_find_default_font(app);
	g = app_get_window_graphics(w);
	app_set_font(g, f);
	h = app_text_height(f, r.width-20, message, (int) strlen(message));
	app_del_graphics(g);
	if (h > 400)
		h = 400;
	if (h > r.height - 50) {
		r.height = h + 50;
		app_size_window(w, r);
	}

	sd = app_zero_alloc(sizeof(SimpleDialog));

	sd->message = message;
	sd->result = NULL;

	if (name1)
		sd->btn[0] = app_new_button(w,
			rect(10,r.height-35,80,30),
			name1, app_dialog_dismiss);
	if (name2)
		sd->btn[1] = app_new_button(w,
			rect(r.width/2-40,r.height-35,80,30),
			name2, app_dialog_dismiss);
	if (name3)
		sd->btn[2] = app_new_button(w,
			rect(r.width-90,r.height-35,80,30),
			name3, app_dialog_dismiss);

	app_on_window_redraw(w, app_dialog_draw_text);
	app_on_window_resize(w, app_dialog_resize);
	app_on_window_key_down(w, app_dialog_key);
	app_set_window_data(w, sd);

	result = app_handle_message_box(w, cancel);

	app_free(sd);
	app_del_window(w);
	if (f != app_find_default_font(app))
		app_del_font(f);
	return result;
}


/*
 *  Public dialog functions:
 */

void app_error(App *app, const char *message)
{
	if (app == NULL) {
		fprintf(stderr, "%s: %s\n", ERROR_STRING, message);
		exit(2);
	}
	if (app->gui_available == 0) {
		fprintf(stderr, "%s: %s\n",
			app_get_string(app, ERROR_STRING), message);
		exit(2);
	}
	app_beep(app);
	app_message_box(app, app_get_string(app, ERROR_STRING), message,
		NULL,
		app_get_string(app, QUIT_STRING),
		NULL,
		YES);
	exit(2);
}

void app_ask_ok(App *app, const char *title, const char *message)
{
	app_message_box(app, title, message,
		NULL,
		app_get_string(app, OK_STRING),
		NULL,
		YES);
}

int app_ask_ok_cancel(App *app, const char *title, const char *message)
{
	return app_message_box(app, title, message,
		app_get_string(app, OK_STRING),
		NULL,
		app_get_string(app, CANCEL_STRING),
		CANCEL);
}

int app_ask_yes_no(App *app, const char *title, const char *message)
{
	return app_message_box(app, title, message,
		app_get_string(app, YES_STRING),
		NULL,
		app_get_string(app, NO_STRING),
		NO);
}

int app_ask_yes_no_cancel(App *app, const char *title, const char *message)
{
	return app_message_box(app, title, message,
		app_get_string(app, YES_STRING),
		app_get_string(app, NO_STRING),
		app_get_string(app, CANCEL_STRING),
		CANCEL);
}

char *app_ask_string(App *app, const char *title, const char *message, const char *answer)
{
	Rect r;
	Window *w;
	const char *str;
	SimpleDialog *sd;
	Font *f;
	Graphics *g;
	int h;
	char *result = NULL;

	r = rect(500,400,400,150);
	w = app_new_window(app, r, title,
		BASE | MODAL | FLOATING | TITLEBAR | CLOSEBOX | CENTERED);

	app_set_window_background(w, BACKGROUND);
	f = app_new_font(app, "menufont", PLAIN, 16);
	if (! f)
		f = app_find_default_font(app);
	g = app_get_window_graphics(w);
	app_set_font(g, f);
	h = app_text_height(f, r.width-20, message, (int) strlen(message));
	app_del_graphics(g);
	if (h > 400)
		h = 400;
	if (h > r.height - 50) {
		r.height = h + 50;
		app_size_window(w, r);
	}

	sd = app_zero_alloc(sizeof(SimpleDialog));

	sd->message = message;
	sd->result = NULL;

	sd->btn[0] = app_new_button(w, rect(10,r.height-35,80,30),
			app_get_string(app, OK_STRING),
			app_dialog_dismiss);
	sd->btn[2] = app_new_button(w, rect(r.width-90,r.height-35,80,30),
			app_get_string(app, CANCEL_STRING),
			app_dialog_dismiss);

	sd->field = app_new_field(w,
			rect(10,r.height-35-20-24,r.width-20,24), answer);
	app_set_focus(sd->field);

	app_on_window_redraw(w, app_dialog_draw_text);
	app_on_window_resize(w, app_dialog_resize);
	app_on_window_key_down(w, app_dialog_key);
	app_set_window_data(w, sd);
	app_show_window(w);
	while (w->state & VISIBLE)
		app_wait_event(app);

	str = sd->result;
	if (str != NULL) {
		if (strcmp(str, app_get_string(app, OK_STRING)) == 0) {
			result = app_get_control_text(sd->field);
			result = app_copy_string(result);
		}
	}
	app_free(sd);
	app_del_window(w);
	if (f != app_find_default_font(app))
		app_del_font(f);
	return result;
}

/*
 *  File dialog functions:
 */

static void app_file_dialog_key(Window *w, unsigned long key)
{
	FileDialog *fd;
	App *app = w->app;

	fd = app_get_window_data(w);
	if ((key == '\n') && (fd->btn[0])) {
		fd->result = app_get_control_text(fd->btn[0]);
		if (fd->confirm)
			fd->confirm(w);
		if (strcmp(fd->result, app_get_string(app, CANCEL_STRING)) != 0)
			app_hide_window(w);
	}
	else if ((key == ESC) && (fd->btn[2])) {
		fd->result = app_get_control_text(fd->btn[2]);
		app_hide_window(w);
	}
}

static void app_file_dialog_yes(Control *c)
{
	Window *w;
	FileDialog *fd;
	App *app;

	w = app_parent_window(c);
	app = w->app;
	fd = app_get_window_data(w);
	if (fd->confirm)
		fd->confirm(w);
	if (strcmp(fd->result, app_get_string(app, CANCEL_STRING)) != 0)
		app_hide_window(w);
}

static void app_file_dialog_cancel(Control *c)
{
	Window *w;
	FileDialog *fd;

	w = app_parent_window(c);
	fd = app_get_window_data(w);
	fd->result = app_get_control_text(c);
	app_hide_window(w);
}

static int app_sort_strings(const void *a, const void *b)
{
	char *s = (*(char **)a);
	char *t = (*(char **)b);

	return strcmp(s,t);
}

static char **app_get_file_names(const char *foldername, int flags)
{
	int i, info;
	Folder *f;
	char *filename;
	char *fullname;
	char **names = NULL;

	f = app_open_folder(foldername);
	if (f == NULL)
		return NULL;
	i = 0;
	while ((filename = app_read_folder(f)) != NULL)
	{
		if (filename[0] == '.' && filename[1] == '\0')
			continue;
		fullname = app_form_file_path(foldername, filename);
		info = app_file_info(fullname);
		app_del_string(fullname);
		if (info & flags) {
			filename = app_copy_string(filename);
			names = app_realloc(names, (i+1) * sizeof(char *));
			names[i++] = filename;
		}
	}
	qsort(names, i, sizeof(char*), app_sort_strings);
	names = app_realloc(names, (i+1) * sizeof(char *));
	names[i++] = NULL;
	app_close_folder(f);

	return names;
}

static void app_set_file_list_folder(Control *box, const char *foldername)
{
	int i;
	char **names;
	FileDialog *fd;

	names = app_get_file_names(foldername, IS_FILE);
	app_change_list_box(box, names);
	fd = app_get_window_data(app_parent_window(box));

	app_del_string(fd->foldername);
	fd->foldername = app_copy_string(foldername);

	if (fd->filenames)
		for (i=0; fd->filenames[i]; i++)
			app_del_string(fd->filenames[i]);
	app_free(fd->filenames);
	fd->filenames = names;
}

static void app_set_folder_list_folder(Control *box, const char *foldername)
{
	int i;
	char **names;
	FileDialog *fd;

	names = app_get_file_names(foldername, IS_FOLDER);
	app_change_list_box(box, names);
	fd = app_get_window_data(app_parent_window(box));

	app_del_string(fd->foldername);
	fd->foldername = app_copy_string(foldername);

	if (fd->dirnames)
		for (i=0; fd->dirnames[i]; i++)
			app_del_string(fd->dirnames[i]);
	app_free(fd->dirnames);
	fd->dirnames = names;
}

static void app_choose_file(Control *box)
{
	Window *win = app_parent_window(box);
	FileDialog *fd;
	char *filename;
	int which;

	which = app_get_control_value(box);
	if (which < 0)
		return;

	fd = app_get_window_data(win);
	filename = fd->filenames[which];
	app_set_control_text(fd->filename, filename);
}

static void app_choose_folder(Control *box)
{
	Window *win = app_parent_window(box);
	FileDialog *fd;
	char *foldername;
	char *path;
	char *dirname;
	int which;

	which = app_get_control_value(box);
	if (which < 0)
		return;

	fd = app_get_window_data(win);

	dirname = app_get_control_text(fd->dirname);
	path = fd->dirnames[which];
	foldername = app_form_file_path(dirname, path);
	path = app_form_file_path(foldername, "");

	app_set_control_text(fd->dirname, path);
	app_set_folder_list_folder(fd->folder_list, path);
	app_set_list_box_item(fd->folder_list, -1);
	app_set_file_list_folder(fd->file_list, path);

	app_del_string(path);
	app_del_string(foldername);
}

static void app_rescan_folder(Window *w)
{
	FileDialog *fd;
	char *dirname;

	fd = app_get_window_data(w);
	dirname = app_get_control_text(fd->dirname);
	app_set_folder_list_folder(fd->folder_list, dirname);
	app_set_file_list_folder(fd->file_list, dirname);
}

static void app_move_file_dialog(Window *w)
{
	FileDialog *fd = app_get_window_data(w);

	if (fd->area)
		*fd->area = w->area;
}

#if 0	//!!
static void app_resize_file_dialog(Window *w)
{
	int h, hh, y;
	Rect r;
	FileDialog *fd = app_get_window_data(w);

	r = app_get_window_area(w);

	h = 16;
	hh = h * 2;
	y = r.height - 75 - hh*3 - h;

	app_set_control_area(fd->filename, rect(80,10,r.width-90,hh));
	app_set_control_area(fd->dirname, rect(80,15+hh,r.width-90,hh));

	if (r.width <= 400) {
		/* half of space is for folder list */
		app_set_control_area(fd->folder_label,
				rect(r.width/2+10,25+hh*2,100,h));
		app_set_control_area(fd->folder_list,
				rect(r.width/2+10,30+hh*2+h,r.width/2-20,y));
		app_set_control_area(fd->file_list,
				rect(10,30+hh*2+h,r.width/2-20,y));
	}
	else {
		/* folder list has a maximal width */
		app_set_control_area(fd->folder_label,
				rect(r.width-190,25+hh*2,100,h));
		app_set_control_area(fd->folder_list,
				rect(r.width-190,30+hh*2+h,180,y));
		app_set_control_area(fd->file_list,
				rect(10,30+hh*2+h,r.width-220,y));
	}

	app_set_control_area(fd->btn[0],
			rect(r.width/2-80-10,r.height-20-hh,80,hh));
	app_set_control_area(fd->btn[2],
			rect(r.width/2+10,r.height-20-hh,80,hh));

	if (fd->area)
		*fd->area = w->area;
}
#endif

static Window *app_init_file_dialog(App *app,
	const char *title, const char *btnname,
	const char *path, Rect *rp, WindowFunc confirm)
{
	Window *win;
	FileDialog *fd;
	char *filename, *folder;
	int h, hh;
	long flags;
	Rect r;

	if (path == NULL)
		path = "";

	filename = app_get_file_name(path);
	folder = app_copy_string(path);
	folder[filename - path] = '\0';
	if (folder[0] == '\0') {	/* no folder specified */
		app_del_string(folder);
		folder = app_current_folder(); /* use current folder */
	}

	h = 16;
	hh = h * 2;

	flags = BASE | TITLEBAR | CLOSEBOX | RESIZE | MODAL | FLOATING;
	r = *rp;
	if ((r.x <= 0) || (r.y <= 0))
		flags |= CENTERED;
	if (r.width < 50)
		r.width = 400;
	if (r.height < 50)
		r.height = 205+hh*3+h;

	win = app_new_window(app, r, title, flags);
	fd = app_zero_alloc(sizeof(FileDialog));
	app_set_window_data(win, fd);

	app_set_window_background(win, BACKGROUND);
	app_on_window_key_down(win, app_file_dialog_key);

	app_on_window_move(win, app_move_file_dialog);
#if 0	//!!
	app_on_window_resize(win, app_resize_file_dialog);
#endif
	fd->area = rp;

	fd->name_label = app_new_label(win, rect(10,10,60,hh),
				app_get_string(app, FILE_STRING),
				VALIGN_CENTER);
	fd->filename   = app_new_field(win, rect(80,10,310,hh), filename);
	app_set_control_layout(fd->filename, EDGE_LEFT | EDGE_RIGHT);
	app_set_focus(fd->filename);
	fd->filename->state |= TABSTOP;

	fd->path_label = app_new_label(win, rect(10,15+hh,60,hh),
				app_get_string(app, PATH_STRING),
				VALIGN_CENTER);
	fd->dirname    = app_new_field(win, rect(80,15+hh,310,hh), folder);
	app_disable(fd->dirname);
	app_set_control_layout(fd->dirname, EDGE_LEFT | EDGE_RIGHT);
	fd->dirname->state |= TABSTOP;

	fd->file_label = app_new_label(win, rect(10,25+hh*2,150,h),
				app_get_string(app, FILES_STRING),
				VALIGN_CENTER);
	fd->file_list  = app_new_list_box(win, rect(10,30+hh*2+h,180,150),
				NULL, app_choose_file);
	app_set_file_list_folder(fd->file_list, folder);
	app_set_control_layout(fd->file_list, EDGE_ALL);
	fd->file_list->state |= TABSTOP;

	fd->folder_label = app_new_label(win, rect(210,25+hh*2,150,h),
				app_get_string(app, FOLDERS_STRING),
				ALIGN_LEFT);
	app_set_control_layout(fd->folder_label, EDGE_RIGHT);
	fd->folder_list = app_new_list_box(win, rect(210,30+hh*2+h,180,150),
				NULL, app_choose_folder);
	app_set_folder_list_folder(fd->folder_list, folder);
	app_set_control_layout(fd->folder_list, EDGE_ALL & ~ EDGE_LEFT);
	fd->folder_list->state |= TABSTOP;

	fd->btn[0] = app_new_button(win, rect(210,195+hh*2+h,80,hh),
			btnname,
			app_file_dialog_yes);
	app_set_control_layout(fd->btn[0], EDGE_RIGHT | EDGE_BOTTOM);
	fd->btn[0]->state |= TABSTOP;
	fd->btn[2] = app_new_button(win, rect(310,195+hh*2+h,80,hh),
			app_get_string(app, CANCEL_STRING),
			app_file_dialog_cancel);
	app_set_control_layout(fd->btn[2], EDGE_RIGHT | EDGE_BOTTOM);
	fd->btn[2]->state |= TABSTOP;
	fd->confirm = confirm;

	app_del_string(folder);

	return win;
}

static void app_can_open(Window *w)
{
	int info;
	char *filename;
	App *app = w->app;
	FileDialog *fd = app_get_window_data(w);

	filename = app_form_file_path(app_get_control_text(fd->dirname),
			app_get_control_text(fd->filename));
	info = app_file_info(filename);
	if (info & IS_FOLDER) {
		app_message_box(app,
			app_get_string(app, ERROR_CREATE_FILE_STRING),
			app_get_string(app, ALREADY_FOLDER_STRING),
			NULL,
			app_get_string(app, CANCEL_STRING),
			NULL,
			CANCEL);
		fd->result = app_get_string(app, CANCEL_STRING);
	}
	else if (! (info & IS_FILE)) {
		if (app_message_box(app,
			app_get_string(app, CREATE_FILE_STRING),
			app_get_string(app, CONFIRM_CREATE_STRING),
			app_get_string(app, CREATE_STRING),
			NULL,
			app_get_string(app, CANCEL_STRING),
			CANCEL) != CANCEL)
				fd->result = app_get_string(app, YES_STRING);
			else
				fd->result = app_get_string(app, CANCEL_STRING);
	} else {
		fd->result = app_get_string(app, YES_STRING);
	}
	app_del_string(filename);
}

static void app_can_save(Window *w)
{
	int info;
	char *filename;
	App *app = w->app;
	FileDialog *fd = app_get_window_data(w);

	filename = app_form_file_path(app_get_control_text(fd->dirname),
			app_get_control_text(fd->filename));
	info = app_file_info(filename);
	if (info & IS_FOLDER) {
		app_message_box(app,
			app_get_string(app, ERROR_SAVE_FILE_STRING),
			app_get_string(app, ALREADY_FOLDER_STRING),
			NULL,
			app_get_string(app, CANCEL_STRING),
			NULL,
			CANCEL);
		fd->result = app_get_string(app, CANCEL_STRING);
	}
	else if (info & IS_FILE) {
		if (app_message_box(app,
			app_get_string(app, OVERWRITE_FILE_STRING),
			app_get_string(app, CONFIRM_OVERWRITE_STRING),
			app_get_string(app, OVERWRITE_STRING),
			NULL,
			app_get_string(app, CANCEL_STRING),
			CANCEL) != CANCEL)
				fd->result = app_get_string(app, YES_STRING);
			else
				fd->result = app_get_string(app, CANCEL_STRING);
	} else {
		fd->result = app_get_string(app, YES_STRING);
	}
	app_del_string(filename);
}

static char *app_handle_file_dialog(Window *w)
{
	FileDialog *fd;
	const char *str;
	App *app = w->app;

	fd = app_get_window_data(w);

	app_show_window(w);
	while (w->state & VISIBLE)
		app_wait_event(app);

	str = fd->result;
	if ((str != NULL) && (!strcmp(str, app_get_string(app, YES_STRING))))
		str = app_form_file_path(
			app_get_control_text(fd->dirname),
			app_get_control_text(fd->filename));
	else
		str = NULL;

	return (char *) str; /* cast away const */
}

char *app_ask_file_open(App *app, const char *title, const char *message,
	const char *name)
{
	Window *win;
	FileDialog *fd;
	char *folder;
	char *current;
	char *path;

	/* use remembered open_folder, if any */
	current = app_current_folder();
	folder = app->open_folder;
	if (folder == NULL)
		folder = current;
	if (name && (name[0] != '\0'))
		path = app_form_file_path(folder, app_get_file_name(name));
	else
		path = app_copy_string(folder);
	win = app_init_file_dialog(app, title, message, path,
			& app->open_dialog_area, app_can_open);
	app_del_string(path);

	/* handle the dialog */
	fd = app_get_window_data(win);
	app_rescan_folder(win);
	path = app_handle_file_dialog(win);
	app_free(fd);
	app_del_window(win);

	if (path == NULL) {
		app_del_string(current);
		return path;
	}

	/* remember the new open_folder */
	if (app->open_folder)
		app_del_string(app->open_folder);
	folder = app_copy_string(path);
	folder[app_get_file_name(path) - path] = '\0';
	if (strcmp(folder, current))	/* not the current folder */
		app->open_folder = folder;
	else {
		app_del_string(folder);
		app->open_folder = NULL;
	}
	app_del_string(current);

	return path;
}

char *app_ask_file_save(App *app, const char *title, const char *message,
	const char *name)
{
	Window *win;
	FileDialog *fd;
	char *folder;
	char *current;
	char *path;

	/* use remembered save_folder, if any */
	current = app_current_folder();
	folder = app->save_folder;
	if (folder == NULL)
		folder = current;
	if (name && (name[0] != '\0'))
		path = app_form_file_path(folder, app_get_file_name(name));
	else
		path = app_copy_string(folder);
	win = app_init_file_dialog(app, title, message, path,
			& app->save_dialog_area, app_can_save);
	app_del_string(path);

	/* handle the dialog */
	fd = app_get_window_data(win);
	app_rescan_folder(win);
	path = app_handle_file_dialog(win);
	app_free(fd);
	app_del_window(win);

	if (path == NULL) {
		app_del_string(current);
		return path;
	}

	/* remember the new save_folder */
	if (app->save_folder)
		app_del_string(app->save_folder);
	folder = app_copy_string(path);
	folder[app_get_file_name(path) - path] = '\0';
	if (strcmp(folder, current))	/* not the current folder */
		app->save_folder = folder;
	else {
		app_del_string(folder);
		app->save_folder = NULL;
	}
	app_del_string(current);

	return path;
}
