/*
 *  Feature Demo
 *  ------------
 *  A demonstration program which shows many of the available
 *  graphical user interaction objects in GraphApp.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graphapp.h>
#include "icons.h"

/** Variables for this application: **/

Window *w;
MenuBar *mb;
Menu *m[8];
MenuItem *mi[24];
Control *l;
Control *f;
Control *b;
Control *r1, *r2;
Control *c1, *c2;
Control *t;
Control *lb, *dl, *df;
Control *hs, *vs;
Control *d;

#define NUM_ICONS 31

struct  {Image *image; int toggle;}
icon[NUM_ICONS] = {
	{ &ic_new_imagedata,    0 },
	{ &ic_open_imagedata,   0 },
	{ &ic_save_imagedata,   0 },
	{ &ic_print_imagedata,  0 },
	{ NULL, 0 },
	{ &ic_undo_imagedata,   0 },
	{ &ic_redo_imagedata,   0 },
	{ NULL, 0 },
	{ &ic_bold_imagedata,   1 },
	{ &ic_italic_imagedata, 1 },
	{ &ic_under_imagedata,  1 },
	{ NULL, 0 },
	{ &ic_cut_imagedata,    0 },
	{ &ic_copy_imagedata,   0 },
	{ &ic_paste_imagedata,  0 },
	{ &ic_glue_imagedata,   0 },
	{ NULL, 0 },
	{ &ic_left_imagedata,   1 },
	{ &ic_center_imagedata, 1 },
	{ &ic_right_imagedata,  1 },
	{ &ic_flush_imagedata,  1 },
	{ NULL, 0 },
	{ &ic_graph_imagedata,  0 },
	{ &ic_image_imagedata,  0 },
	{ &ic_import_imagedata, 0 },
	{ NULL, 0 },
	{ &ic_bullet_imagedata, 0 },
	{ &ic_num_imagedata,    0 },
	{ &ic_view_imagedata,   0 },
	{ NULL, 0 },
	{ &ic_spell_imagedata,  0 }
};

Control * ib[NUM_ICONS];

char * listbox_elements[] = {
	"ListBox element 1",
	"element 2",
	"element 3",
	"element 4",
	"element 5",
	NULL
};
char * droplist_elements[] = {
	"DropList element 1",
	"element 2",
	"element 3",
	"element 4",
	"element 5",
	NULL
};
char * dropfield_elements[] = {
	"DropField type here",
	"try typing here",
	"element 3",
	"element 4",
	"element 5",
	NULL
};

/** Callback functions: **/

void quit_program(MenuItem *mi)
{
	exit(0);
}

void draw_toolbar(Window *w, Graphics *g)
{
	Rect r = get_window_area(w);

	r.y = 24;
	r.height = 26;
	set_colour(g, LIGHT_GREY);
	fill_rect(g, r);
	set_colour(g, BLACK);
	draw_rect(g, rect(0,r.y+r.height-1,r.width,1));
}

void resize_window(Window *w)
{
	Rect wr = get_window_area(w);
	Rect sr;
	int size;

	sr = get_control_area(vs);
	size = sr.width;
	set_control_area(vs,
		rect(wr.width-size, 26, size, wr.height -26 -size));

	set_control_area(hs,
		rect(0, wr.height-size, wr.width-size, size));
}

void toggle(Control *b)
{
	Control *c = get_control_data(b);
	char *name = get_control_text(b);
	char buffer[80];

	if (strncmp(name, "Disable", 7) == 0) {
		if (strcmp(name+8, "Undo") == 0)
			disable_menu_item(mi[1]);
		sprintf(buffer, "Enable %s", name+8);
		set_control_text(b, buffer);
		disable(c);
	}
	else if (strncmp(name, "Enable", 6) == 0) {
		if (strcmp(name+7, "Undo") == 0)
			enable_menu_item(mi[1]);
		sprintf(buffer, "Disable %s", name+7);
		set_control_text(b, buffer);
		enable(c);
	}
}

void change_text(Control *b) /* cycle text within the control */
{
	Control *c = get_control_data(b);
	char *name = get_control_text(c);
	char buffer[80];

	sprintf(buffer, "%s%c", name+1, name[0]);
	set_control_text(c, buffer);
}

void set_drop_list_item(Control *b)
{
	Control *dl = get_control_data(b);

	set_control_value(dl, 0);
	/* redraw_control(dl); */
}

/** Drawing area routines: **/

Point last_mouse;

void draw_border(Control *d, Graphics *g)
{
	Rect r = get_control_area(d);

	set_colour(g, BLACK);
	draw_rect(g, r);
	draw_utf8(g, pt(5,5), "Drawing", 7);
}

void mouse_down(Control *d, int buttons, Point p)
{
	last_mouse = p;
}

void mouse_drag(Control *d, int buttons, Point p)
{
	Graphics *g = get_control_graphics(d);

	set_colour(g, RED);
	draw_line(g, last_mouse, p);
	last_mouse = p;

	del_graphics(g);
}


/** The main function is used to initialise the application: **/
int main(int argc, char *argv[])
{
	int i;
	Rect r;
	App *app;

	/* Start the app. */
	app = new_app(argc, argv);

	/* Create a window. */
	w = new_window(app, rect(50,50,600,430),
				"Feature Demo", STANDARD_WINDOW);
	set_window_background(w, rgb(200,200,250));
	on_window_redraw(w, draw_toolbar);
	on_window_resize(w, resize_window);

	/* Create some menus. */
	mb = new_menu_bar(w);

	m[0] = new_menu(mb, "File");
	mi[0] = new_menu_item(m[0], "Quit", 'Q', quit_program);

	m[1] = new_menu(mb, "Edit");
	mi[1] = new_menu_item(m[1], "Undo",  'Z', NULL);
	disable_menu_item(mi[1]);
	mi[2] = new_menu_item(m[1], "-",      0,  NULL);
	mi[3] = new_menu_item(m[1], "Cut",   'X', NULL);
	mi[4] = new_menu_item(m[1], "Copy",  'C', NULL);
	mi[5] = new_menu_item(m[1], "Paste", 'V', NULL);

	m[2] = new_menu(mb, "Font");
	m[3] = new_sub_menu(m[2], "Family");
	mi[6] = new_menu_item(m[3], "Times",     0, NULL);
	set_menu_item_font(mi[6], new_font(app, "times", PLAIN, 16));
	mi[7] = new_menu_item(m[3], "Helvetica", 0, NULL);
	set_menu_item_font(mi[7], new_font(app, "helvetica", PLAIN, 16));
	mi[8] = new_menu_item(m[3], "Courier", 0, NULL);
	set_menu_item_font(mi[8], new_font(app, "courier", PLAIN, 16));
	mi[9] = new_menu_item(m[3], "Unifont", 0, NULL);
	set_menu_item_font(mi[9], new_font(app, "unifont", PLAIN, 16));

	m[4] = new_sub_menu(m[2], "Size");
	mi[10] = new_menu_item(m[4], "8",  0, NULL);
	set_menu_item_font(mi[10], new_font(app, "plain", PLAIN, 8));
	mi[11] = new_menu_item(m[4], "10", 0, NULL);
	set_menu_item_font(mi[11], new_font(app, "plain", PLAIN, 10));
	mi[12] = new_menu_item(m[4], "16", 0, NULL);
	set_menu_item_font(mi[12], new_font(app, "plain", PLAIN, 16));

	m[5] = new_sub_menu(m[2], "Style");
	mi[13] = new_menu_item(m[5], "Plain",  0, NULL);
	set_menu_item_font(mi[13], new_font(app, "unifont", PLAIN, 16));
	mi[14] = new_menu_item(m[5], "Bold",   0, NULL);
	set_menu_item_font(mi[14], new_font(app, "unifont", BOLD, 16));
	mi[15] = new_menu_item(m[5], "Italic", 0, NULL);
	set_menu_item_font(mi[15], new_font(app, "unifont", ITALIC, 16));

	m[6] = new_sub_menu(m[2], "Colour");
	mi[16] = new_menu_item(m[6], "Red",  0, NULL);
	set_menu_item_foreground(mi[16], RED);
	mi[17] = new_menu_item(m[6], "Green",   0, NULL);
	set_menu_item_foreground(mi[17], DARK_GREEN);
	mi[18] = new_menu_item(m[6], "Blue", 0, NULL);
	set_menu_item_foreground(mi[18], BLUE);

	m[7] = new_menu(mb, "Help");
	mi[19] = new_menu_item(m[7], "About...", CONTROL+SHIFT+'A', NULL);
	mi[20] = new_menu_item(m[7], "-",          0,  NULL);
	mi[21] = new_menu_item(m[7], "Contents", CONTROL+SHIFT+'c', NULL);
	mi[22] = new_menu_item(m[7], "Search",   SHIFT+'s', NULL);
	mi[23] = new_menu_item(m[7], "Web Site", 'W', NULL);

	/* Set up a rectangle for placing imagebuttons. */
	r = rect(4,24+2,19+4,17+4);

	/* Create some image buttons and image check boxes. */
	for (i=0; i < NUM_ICONS; i++) {
		if (icon[i].image == NULL) { /* gap to separate */
			ib[i] = NULL;
			r.x += 4;
		}
		else if (icon[i].toggle == 0) {
			ib[i] = new_image_button(w, r, icon[i].image, NULL);
			r.x += r.width;
		}
		else {
			ib[i] = new_image_check_box(w, r, icon[i].image, NULL);
			r.x += r.width;
		}
	}

	/* Set up a rectangle to use in placing objects. */
	r = rect(15,24+35,80,25);

	/* Create a push button and add it to the window. */
	r.x = 200;
	b = new_button(w, r, "Button", NULL);
	r.y += r.height + 10;
	r.x = 15;

	/* Create some radiobuttons. */
	r.width = 140;
	r1 = new_radio_button(w, r, "Radio button 1", NULL);
	r.y += r.height + 5;
	r2 = new_radio_button(w, r, "Radio button 2", NULL);
	r.y += r.height + 10;

	/* Create some checkboxes. */
	r.x += r.width + 45;
	r.y = 24+35 + r.height + 10;
	c1 = new_check_box(w, r,"Checkbox 1", NULL);
	r.y += r.height + 5;
	c2 = new_check_box(w, r, "Checkbox 2", NULL);
	r.y += r.height + 10;

	/* Create a label. */
	r.x = 15;
	r.width = 160;
	l = new_label(w, r, "Label", ALIGN_CENTER + VALIGN_CENTER);

	/* Create a text field. */
	r.x += r.width + 25;
	f = new_field(w, r, "Field");
	r.y += r.height + 10;

	/* Create the textbox and add it to the window. */
	r.x = 15;
	r.width = 160;
	r.height = 4 * font_height(find_default_font(app)) + 2;
	t = new_text_box(w, r, "Textbox: This tests multi-line text."
	" This is a fairly long line of text used to check word wrapping."
	"\nNext line.\nAnother line. \nYet another line.\nLast.");

	/* Create a listbox. */
	r.x += r.width + 25;
	lb = new_list_box(w, r, listbox_elements, NULL);
	r.y += r.height + 10;

	/* Create some drop-down lists. */
	r.x = 15;
	r.height = 25;
	dl = new_drop_list(w, r, droplist_elements, NULL);

	r.x += r.width + 25;
	df = new_drop_field(w, r, dropfield_elements);

	/* Create a drawing area. */
	r = get_window_area(w);
	d = new_control(w, rect(r.width-16-10-75, 24+25+10,75,75));
	on_control_redraw(d, draw_border);
	on_control_mouse_down(d, mouse_down);
	on_control_mouse_drag(d, mouse_drag);

	/* Create some enabling/disabling buttons. */
	r = get_window_area(w);
	r = rect(r.width-16-10-160, 24+25+10+75+5,160,25);
	set_control_data(new_button(w, r, "Disable Button", toggle), b);
	r.y += r.height + 5;
	set_control_data(new_button(w, r, "Disable Radio Button", toggle), r1);
	r.y += r.height + 5;
	set_control_data(new_button(w, r, "Disable Check Box", toggle), c1);
	r.y += r.height + 5;
	set_control_data(new_button(w, r, "Disable Label", toggle), l);
	r.y += r.height + 5;
	set_control_data(new_button(w, r, "Disable Field", toggle), f);
	r.y += r.height + 5;
	set_control_data(new_button(w, r, "Disable Text Box", toggle), t);
	r.y += r.height + 5;
	set_control_data(new_button(w, r, "Disable List Box", toggle), lb);
	r.y += r.height + 5;
	set_control_data(new_button(w, r, "Disable Drop List", toggle), dl);
	r.y += r.height + 5;
	set_control_data(new_button(w, r, "Disable Drop Field", toggle), df);
	r.y += r.height + 5;
	set_control_data(new_button(w, r, "Disable Drawing Area", toggle), d);
	r.x -= r.width + 5;
	set_control_data(new_button(w, r, "Disable Undo", toggle), ib[5]);
	r.x -= r.width + 5;
	set_control_data(new_button(w, r, "Change Label", change_text), l);
	r.y -= r.height + 5;
	set_control_data(new_button(w, r, "Set Drop List Item", set_drop_list_item), dl);

	/* Create some scrollbars. */
	r = get_window_area(w);
	r.x = r.width - 15;
	r.y -= 1;
	r.y += 26;
	r.width = 16;
	r.height -= r.y + 14;
	vs = new_scroll_bar(w, r, 100, 24, NULL);

	r = get_window_area(w);
	r.y = r.height - 15;
	r.x -= 1;
	r.height = 16;
	r.width -= r.x + 14;
	hs = new_scroll_bar(w, r, 100, 24, NULL);

	/* Show the window. */
	show_window(w);

	/* Allow GraphApp to handle all events. */
	main_loop(app);

	/* Tidy up. */
	del_app(app);
	return 0;
}
