/*
 *  View UTF-8 files in a window using a listbox.
 *
 *  Usage: listtest filename
 *  where filename refers to a UTF-8 encoded file
 *  (ordinary ASCII text files will also work)
 *
 *  This program creates a window which displays the
 *  text of the file graphically within a listbox.
 *  The code is similar to the viewutf8 example, except
 *  for the use of the listbox, and having a better
 *  method for loading lines of text. There are no
 *  global variables in this program, only global constants.
 */

#include <stdio.h>
#include <string.h>
#include <graphapp.h>

/*
 *  Global constants:
 */

char *usage[] = {
	"usage: listtest filename",
	NULL
};

char *file_not_found[] = {
	"error: listtest: file not found",
	"usage: listtest filename",
	NULL
};

/*
 *  Functions:
 */

void window_redraw(Window *w, Graphics *g)
{
	set_colour(g, LIGHT_BLUE);
	fill_rect(g, get_window_area(w));
}

void window_resize(Window *w)
{
	Rect r;
	Control *listbox;

	r = get_window_area(w);
	listbox = get_window_data(w);
	set_control_area(listbox, r);
}

void select_item(Control *c)
{
	/* do nothing for now */
}

char ** load_lines(char *name)
{
	long nbytes, nchars;
	char *line;
	int num_lines = 0;
	char **lines = NULL;
	FILE *f;

	/* load the lines of the file */
	f = open_file(name, "rb");
	if (f == NULL)
		return file_not_found;
	while (1) {
		line = read_utf8_line(f, &nbytes, &nchars);
		if (nbytes > 0) {
			if (line[nbytes-1] == '\n')
				line[--nbytes] = '\0';
			if (line[nbytes-1] == '\r')
				line[--nbytes] = '\0';
			lines = realloc(lines,
					(num_lines+1) * sizeof(char *));
			lines[num_lines] = line;
			num_lines++;
		}
		else {
			break;
		}
	}
	close_file(f);

	return lines;
}

int main(int argc, char *argv[])
{
	App *app;
	Rect r;
	Window *win;
	Control *listbox;
	char **lines;

	app = new_app(argc, argv);

	if (argc < 2) {
		lines = usage;
	}
	else {
		lines = load_lines(argv[1]);
	}

	r = rect(50,100,512,324);
	win = new_window(app, r, "List-Box Test", STANDARD_WINDOW);
	on_window_resize(win, window_resize);
	on_window_redraw(win, window_redraw);

	listbox = new_list_box(win, rect(0,0,r.width,r.height),
			lines, select_item);

	set_window_data(win, listbox); /* remember pointer */
	set_focus(listbox);
	show_window(win);
	main_loop(app);
	del_app(app);
	return 0;
}

