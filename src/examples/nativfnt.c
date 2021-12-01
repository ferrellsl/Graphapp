/*
 *  Native Font Demo
 *  ----------------
 *  This is the "alldemo" program, using native fonts for
 *  the various widgets.
 */

#include <stdio.h>
#include <stdlib.h>
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

Image *icon[NUM_ICONS] = {
  &ic_new_imagedata,   &ic_open_imagedata,   &ic_save_imagedata,
  &ic_print_imagedata, NULL,
  &ic_undo_imagedata,  &ic_redo_imagedata,   NULL,
  &ic_bold_imagedata,  &ic_italic_imagedata, &ic_under_imagedata, NULL,
  &ic_cut_imagedata,   &ic_copy_imagedata,   &ic_paste_imagedata,
  &ic_glue_imagedata,  NULL,
  &ic_left_imagedata,  &ic_center_imagedata, &ic_right_imagedata,
  &ic_flush_imagedata, NULL,
  &ic_graph_imagedata, &ic_image_imagedata,  &ic_import_imagedata, NULL,
  &ic_bullet_imagedata, &ic_num_imagedata,   &ic_view_imagedata,   NULL,
  &ic_spell_imagedata,
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
	set_colour(g, PALE_GREY);
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
	w = new_window(app, rect(50,50,514,313),
				"Feature Demo", STANDARD_WINDOW);
	on_window_redraw(w, draw_toolbar);
	on_window_resize(w, resize_window);

	/* Create some menus. */
	mb = new_menu_bar(w);

	m[0] = new_menu(mb, "File");
	mi[0] = new_menu_item(m[0], "Quit", 'Q', quit_program);

	m[1] = new_menu(mb, "Edit");
	mi[1] = new_menu_item(m[1], "Undo",  'Z', NULL);
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
	mi[19] = new_menu_item(m[7], "About...",   0,  NULL);
	mi[20] = new_menu_item(m[7], "-",          0,  NULL);
	mi[21] = new_menu_item(m[7], "Contents",   0,  NULL);
	mi[22] = new_menu_item(m[7], "Search",    'S', NULL);
	mi[23] = new_menu_item(m[7], "Web Site", 'W', NULL);

	/* Set up a rectangle for placing imagebuttons. */
	r = rect(4,24+2,19+4,17+4);

	/* Create some image buttons. */
	for (i=0; i < NUM_ICONS; i++) {
		if (icon[i] == NULL) {
			ib[i] = NULL;
			r.x += 4;
		}
		else {
			ib[i] = new_image_button(w, r, icon[i], NULL);
			r.x += r.width;
		}
	}

	/* Set up a rectangle to use in placing objects. */
	r = rect(15,24+35,80,25);

	/* Create a push button and add it to the window. */
	r.x = 200;
	b = new_button(w, r, "Button", NULL);
	set_control_font(b, new_font(app, "times", PLAIN, 16));
	r.y += r.height + 10;
	r.x = 15;

	/* Create some radiobuttons. */
	r.width = 140;
	r1 = new_radio_button(w, r, "Radio button 1", NULL);
	set_control_font(r1, new_font(app, "times", PLAIN, 16));
	r.y += r.height + 5;
	r2 = new_radio_button(w, r, "Radio button 2", NULL);
	set_control_font(r2, new_font(app, "times", PLAIN, 16));
	r.y += r.height + 10;

	/* Create some checkboxes. */
	r.x += r.width + 45;
	r.y = 24+35 + r.height + 10;
	c1 = new_check_box(w, r,"Checkbox 1", NULL);
	set_control_font(c1, new_font(app, "times", PLAIN, 16));
	r.y += r.height + 5;
	c2 = new_check_box(w, r, "Checkbox 2", NULL);
	set_control_font(c2, new_font(app, "times", PLAIN, 16));
	r.y += r.height + 10;

	/* Create a label. */
	r.x = 15;
	r.width = 160;
	l = new_label(w, r, "Label", ALIGN_CENTER + VALIGN_CENTER);
	set_control_font(l, new_font(app, "times", PLAIN, 16));

	/* Create a text field. */
	r.x += r.width + 25;
	f = new_field(w, r, "Field");
	set_control_font(f, new_font(app, "times", PLAIN, 16));
	r.y += r.height + 10;

	/* Create the textbox and add it to the window. */
	r.x = 15;
	r.width = 160;
	r.height = 4 * font_height(find_default_font(app)) + 2;
	t = new_text_box(w, r, "Textbox: This tests multi-line text."
	" This is a fairly long line of text used to check word wrapping."
	"\nNext line.\nAnother line. \nYet another line.\nLast.");
	set_control_font(t, new_font(app, "times", PLAIN, 16));

	/* Create a listbox. */
	r.x += r.width + 25;
	lb = new_list_box(w, r, listbox_elements, NULL);
	set_control_font(lb, new_font(app, "times", PLAIN, 16));
	r.y += r.height + 10;

	/* Create some drop-down lists. */
	r.x = 15;
	r.height = 25;
	dl = new_drop_list(w, r, droplist_elements, NULL);
	set_control_font(dl, new_font(app, "times", PLAIN, 16));

	r.x += r.width + 25;
	df = new_drop_field(w, r, dropfield_elements);
	set_control_font(df, new_font(app, "times", PLAIN, 16));

	/* Create a drawing area. */
	r = get_window_area(w);
	d = new_control(w, rect(r.width-16-10-75, 24+25+10,75,75));
	on_control_redraw(d, draw_border);
	on_control_mouse_down(d, mouse_down);
	on_control_mouse_drag(d, mouse_drag);

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
