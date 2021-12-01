/*
 *  UTF-8 Edit
 *  ----------
 *
 *  This is a simple UTF-8 text editor. It allows you to
 *  directly load, edit and save UTF-8 text files.
 *
 *  UTF-8 is a way of storing Unicode characters in a
 *  variable multi-byte manner. The number of bytes used
 *  for a particular character is variable, so that
 *  ASCII characters are stored in one byte, Greek letters
 *  in two bytes, Chinese in three bytes etc.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <graphapp.h>

char *prog_name = "UTF-8 Edit";
char *file_name = NULL;

App *app;
Window *win = NULL;
MenuBar *menu_bar;
Menu *file_menu;
Menu *edit_menu;
Menu *win_menu;
Control *btn[10];
Control *text_box;
Control *status_bar;

void update_window_title(void)
{
	int length;
	char *buffer;

	if (win == NULL)
		return;
	if (! (win->state & VISIBLE))
		return;

	/* update the window's title to include te file name */
	if (file_name) {
		length = strlen(prog_name) +2 + strlen(file_name) +1;
		buffer = alloc(length);
		strcpy(buffer, prog_name);
		strcat(buffer, ": ");
		strcat(buffer, file_name);
		set_window_title(win, buffer);
		free(buffer);
	}
	else {
		set_window_title(win, prog_name);
	}
}

void load_file(char *name)
{
	FILE *f;
	char *buffer;
	long size, num, bufsize;

	size = file_size(name);
	f = open_file(name, "r");
	if (f == NULL)
		return;

	/* remember file name */
	if (file_name)
		free(file_name);
	file_name = alloc(strlen(name)+1);
	strcpy(file_name, name);

	update_window_title();

	/* read first few kilobytes of data from the file */
	bufsize = 3000;
	buffer = alloc(bufsize);
	num = fread(buffer, 1, 3000-1, f);
	if (num >= 0) {
		bufsize = num;
		size -= num;
		buffer[bufsize] = '\0';
		set_control_text(text_box, buffer);
	} else {
		size = -1;
		ask_ok(app, "Error",
			"An error occurred reading that file.");
	}

	/* read rest of file now */
	while (size > 0) {
		buffer = realloc(buffer, bufsize+3000+1);
		num = fread(buffer+bufsize, 1, 3000-1, f);
		if (num >= 0) {
			bufsize += num;
			size -= num;
			buffer[bufsize] = '\0';
		} else {
			size = -1;
			ask_ok(app, "Error",
				"An error occurred reading that file.");
		}
	}

	if (size == 0)
		set_control_text(text_box, buffer);

	free(buffer);
	close_file(f);
}

void save_file(char *name, char *text)
{
	FILE *f;
	long length;

	/* try to open the file for writing*/
	f = open_file(name, "w");
	if (f == NULL)
		return;

	/* save the text */
	length = strlen(text);
	if (fwrite(text, 1, length, f) != length)
		ask_ok(app, "Error",
			"An error occurred writing that file.");

	/* remember new file name */
	if (file_name)
		if (file_name != name) {
			free(file_name);
			file_name = alloc(strlen(name)+1);
			strcpy(file_name, name);
		}

	update_window_title();
}

void open_file_as(void)
{
	char *name;

	name = ask_file_open(app, "Open File", "Open", file_name);
	if (name != NULL) {
		load_file(name);
		free(name);
	}
}

void save_file_as(void)
{
	char *name;

	name = ask_file_save(app, "Save File As", "Save", file_name);
	if (name != NULL) {
		save_file(name, get_control_text(text_box));
		free(name);
	}
}

void try_save_file(void)
{
	if (file_name == NULL)
		save_file_as();
	else
		save_file(file_name, get_control_text(text_box));
}

void do_open(MenuItem *mi)
{
	open_file_as();
}

void do_save_as(MenuItem *mi)
{
	save_file_as();
}

void do_quit(MenuItem *mi)
{
	int result = ask_yes_no_cancel(app, "Save Changes",
			"Save changes now?");

	if (result == CANCEL)
		return;
	if (result == YES)
		try_save_file();
	exit(0);
}

void window_resize(Window *win)
{
	Rect r = get_window_area(win);

	size_control(text_box, rect(5,30,r.width-10,r.height-35));
}

int main(int argc, char *argv[])
{
	Rect r;

	app = new_app(argc, argv);
	r = rect(50,10,8*80+40,480);
	win = new_window(app, r, "UTF-8 Edit", STANDARD_WINDOW);
	set_window_background(win, PALE_GREY);

	menu_bar = new_menu_bar(win);
	file_menu = new_menu(menu_bar, "File");
	new_menu_item(file_menu, "Open...", 'O', do_open);
	new_menu_item(file_menu, "Save As...", 'A', do_save_as);
	new_menu_item(file_menu, "-", 0, NULL);
	new_menu_item(file_menu, "Quit", 'Q', do_quit);

	text_box = new_text_box(win, rect(5,30,r.width-10,r.height-35), "");

	on_window_resize(win, window_resize);

	if (argc >= 2)
		load_file(argv[1]);

	show_window(win);
	set_focus(text_box);
	main_loop(app);
	del_app(app);
	return 0;
}
