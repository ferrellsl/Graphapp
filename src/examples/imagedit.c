/*
 *  Image Editor
 *  ------------
 *  This program allows the direct editing of
 *  images. Currently it only handles GIF files,
 *  because it is designed as a palette-based
 *  editor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <graphapp.h>

enum {
	SCALE = 10,
	XSIZE = 32,
	YSIZE = 32
};

typedef struct ImageEditor  ImageEditor;

struct ImageEditor {
	App *app;
	Window *win;		/* image editor main window */
	Colour bgcolour;	/* background colour for the window */

	MenuBar *mbar;		/* menubar */

	Menu *file_menu;
	MenuItem *new_item;
	MenuItem *open_item;
	MenuItem *save_item;
	MenuItem *line_1;
	MenuItem *quit_item;

	Menu *edit_menu;
	MenuItem *undo_item;
	MenuItem *line_2;
	MenuItem *sort_palette;

	Menu *tool_menu;
	MenuItem *pencil, *sampler;

	Control *pixedit;	/* the pixel editor */
	Control *vert, *horiz;	/* scroll bars */

	Control *paledit;	/* the palette editor */
	Control *display;	/* real-size display */

	Control *pixval;	/* colour editor */
	Control *transparent;		/* check box */
	Control *red, *green, *blue;	/* fields */
	Control *add, *change;		/* buttons */

	char *filename;		/* current file */

	Image *prev_img;	/* the image before the modification */
	Image *img;		/* the image being edited */

	int size;		/* size of one pixel when expanded */
	int x, y;		/* current x and y of top-left shown */
	int xsize, ysize;	/* page sizes */
	int changed;		/* has this image been changed? */

	int new_width;		/* pixel width of new images */
	int new_height;		/* pixel height of new images */
	int new_depth;		/* pixel depth of new images */

	Window *new_image_window; /* for setting a new image's size */
	Control *new_width_input; /* new image's width input field */
	Control *new_height_input; /* new image's height input field */
};

/*
 *  Utilities:
 */
char * int_to_string(int i)
{
	static char buffer[80];
	sprintf(buffer, "%d", i);
	return buffer;
}

void reduce_palette(Image *img)
{
	long	i, x, y;
	int 	old_value, new_value;
	int 	new_size;
	int *	translate;
	Colour *new_cmap;

	if (! img)
		return;
	if (img->depth != 8)
		return;

	translate = alloc(256 * sizeof(int));

	/* Remove redundant colours: */
	for (i=0; i < 256; i++)
		translate[i] = -1;		/* colour unused */
	for (y=0; y < img->height; y++)
	  for (x=0; x < img->width; x++)
		translate[img->data8[y][x]] = 1;	/* colour used */
	new_size = 0;
	for (i=0; i < 256; i++)
		if (translate[i] == 1)
			translate[i] = new_size++; /* map to this entry */

	/* Generate the new colour map: */
	new_cmap = alloc(new_size * sizeof(Colour));

	for (i=0; i < img->cmap_size; i++) {
		old_value = i;
		new_value = translate[i];
		if (new_value >= 0)
			new_cmap[new_value] = img->cmap[old_value];
	}

	/* Change the existing colour map: */
	img->cmap_size = new_size;
	for (i=0; i < new_size; i++)
		img->cmap[i] = new_cmap[i];
	free(new_cmap);

	/* Translate the pixels to the new colour map: */
	for (y=0; y < img->height; y++)
	  for (x=0; x < img->width; x++)
		img->data8[y][x] = translate[img->data8[y][x]];

	/* Clean up: */
	free(translate);
}


/*
 *  Functions:
 */

ImageEditor *find_image_editor(Control *c)
{
	Window *win = parent_window(c);
	ImageEditor *editor = get_window_data(win);
	return editor;
}

ImageEditor *find_image_editor_from_menu_item(MenuItem *mi)
{
	Window *win = parent_window(mi->parent->parent->ctrl);
	ImageEditor *editor = get_window_data(win);
	return editor;
}

void update_colour_editor(ImageEditor *editor, int entry)
{
	Colour colour;
	Colour *palette;

	if (entry >= editor->img->cmap_size)
		return;

	palette = editor->img->cmap;
	colour = palette[entry];

	set_control_text(editor->pixval, int_to_string(entry));
	if (colour.alpha > 0x7F)
		check(editor->transparent);
	else
		uncheck(editor->transparent);
	set_control_text(editor->red, int_to_string(colour.red));
	set_control_text(editor->green, int_to_string(colour.green));
	set_control_text(editor->blue, int_to_string(colour.blue));
	draw_control(editor->paledit);
}

void prepare_for_change(ImageEditor *editor)
{
	Image *img_copy;

	img_copy = copy_image(editor->img);
	if (editor->prev_img)
		del_image(editor->prev_img);
	editor->prev_img = img_copy;
}

void image_changed(ImageEditor *editor)
{
	if (editor->changed == 0) {
		enable_menu_item(editor->undo_item);
		editor->changed = 1;
	}
}

void draw_pixel_block(Graphics *g, Colour colour, Rect r)
{
	set_colour(g, rgb(colour.red, colour.green, colour.blue));
	fill_rect(g, r);
	if ((colour.red==255)&&(colour.green==255)&&(colour.blue==255)) {
		/* white = draw a grey border */
		set_colour(g, LIGHT_GREY);
		draw_rect(g, r);
	}
	if (colour.alpha > 0x7F) {
		/* transparent = draw a 'T' */

		if ((colour.red   <= 0x33) &&
		    (colour.green <= 0x33) &&
		    (colour.blue  <= 0x33))
			set_colour(g, WHITE); /* white on dark bg */
		else
			set_colour(g, BLACK); /* black on light bg */

		draw_line(g, pt(r.x+r.width/2-2,r.y+r.height/2-2),
			 pt(r.x+r.width/2+2,r.y+r.height/2-2));
		draw_line(g, pt(r.x+r.width/2,r.y+r.height/2-2),
			 pt(r.x+r.width/2,r.y+r.height/2+2));
	}
}

void redraw_pixedit(Control *pixedit, Graphics *g)
{
	ImageEditor *editor;
	Image *img;
	int x, y, width, height;
	int size;
	int maxy, maxx;
	Rect r, pix_rect;
	byte **pixels;
	Colour *palette;
	int palsize;
	byte pixval;
	Colour colour;

	editor = find_image_editor(pixedit);
	pix_rect = get_control_area(pixedit);
	img = editor->img;

	width = img->width;
	height = img->height;

	size = editor->size;

	pixels = img->data8;
	palette = img->cmap;
	palsize = img->cmap_size;

	maxy = editor->ysize;
	if (maxy > height)
		maxy = height;
	maxx = editor->xsize;
	if (maxx > width)
		maxx = width;

	for (y=0; y < maxy; y++) {
	  for (x=0; x < maxx; x++) {
	    pixval = pixels[y+editor->y][x+editor->x];
	    r = rect(x*size+1,y*size+1,size-1,size-1);
	    if (pixval >= palsize) {	/* not in palette! */
		  set_colour(g, BLACK);
		  draw_line(g, pt(r.x,r.y),
				pt(r.x+r.width,r.y+r.height));
		  draw_line(g, pt(r.x,r.y+r.height),
				pt(r.x+r.width,r.y));
	    }
	    else {	/* correct palette entry */
		colour = palette[pixval];
		draw_pixel_block(g, colour, r);
	    }
	  }
	}

	set_colour(g, BLACK);
	draw_rect(g, pix_rect);
}

void handle_pixedit_drag(Control *pixedit, int buttons, Point p)
{
	ImageEditor *editor;
	Image *img;
	int x, y, width, height;
	int size;
	int maxy, maxx;
	Rect r;
	byte **pixels;
	Colour *palette;
	int palsize;
	byte pixval;
	Colour colour;
	int index;
	Graphics *g;

	editor = find_image_editor(pixedit);
	img = editor->img;

	width = img->width;
	height = img->height;

	size = editor->size;

	pixels = img->data8;
	palette = img->cmap;
	palsize = img->cmap_size;

	maxy = editor->ysize;
	if (maxy > height)
		maxy = height;
	maxx = editor->xsize;
	if (maxx > width)
		maxx = width;

	x = (p.x - 1) / editor->size;
	y = (p.y - 1) / editor->size;
	if (x >= maxx) return;
	if (y >= maxy) return;
	if (x < 0) return;
	if (y < 0) return;

	pixval = pixels[y+editor->y][x+editor->x];
	r = rect(x*size+1,y*size+1,size-1,size-1);

	if (menu_item_is_checked(editor->sampler)) {
		update_colour_editor(editor, pixval);
	} else if (menu_item_is_checked(editor->pencil)) {
		if (buttons & LEFT_BUTTON) {
			/* left button draws with selected colour */
			index = atoi(get_control_text(editor->pixval));
		}
		else if (buttons & MIDDLE_BUTTON) {
			/* middle button is a colour sampler */
			update_colour_editor(editor, pixval);
			return;
		}
		else if (buttons & RIGHT_BUTTON) {
			/* right button draws last colour in palette */
			index = palsize - 1;
		}
		else
			index = 0; /* should never happen actually */
		if (pixval == index)
			return;
		pixval = index;
		pixels[y+editor->y][x+editor->x] = pixval;

		/* redraw this pixel */
		colour = palette[pixval];
		g = get_control_graphics(pixedit);
		draw_pixel_block(g, colour, r);
		del_graphics(g);
		draw_control(editor->display);
		image_changed(editor);
	}
}

void handle_pixedit_click(Control *pixedit, int buttons, Point p)
{
	ImageEditor *editor;

	editor = find_image_editor(pixedit);
	prepare_for_change(editor);
	handle_pixedit_drag(pixedit, buttons, p);
}

void redraw_display(Control *display, Graphics *g)
{
	ImageEditor *editor;
	Rect sr, dr;

	editor = find_image_editor(display);

	sr = rect(0, 0, editor->img->width, editor->img->height);
	dr = rect(1, 1, editor->img->width, editor->img->height);
	draw_image(g, dr, editor->img, sr);

	set_colour(g, BLACK);
	draw_rect(g, get_control_area(display));
}

void redraw_paledit(Control *paledit, Graphics *g)
{
	ImageEditor *editor;
	int i, rowsize, palsize, selected, wrong = 0;
	int height;
	Colour *palette;
	Colour colour;
	Rect r;

	editor = find_image_editor(paledit);
	r = get_control_area(paledit);
	palette = editor->img->cmap;
	palsize = editor->img->cmap_size;

	selected = atoi(get_control_text(editor->pixval));
	if (selected < 0)
		selected = 0, wrong = 1;
	if (selected >= palsize)
		selected = palsize - 1, wrong = 1;
	if (wrong)
		update_colour_editor(editor, selected);

	rowsize = (r.width - 2) / SCALE;
	height = (r.height - 2) / SCALE;

	for (i=0; i < palsize; i++) {
		colour = palette[i];
		r = rect(2+SCALE*(i%rowsize),2+SCALE*(i/rowsize),8,8);
		draw_pixel_block(g, colour, r);
		if (i == selected)
			set_colour(g, BLACK);
		else
			set_colour(g, WHITE);
		draw_rect(g, inset_rect(r,-1));
	}
	set_colour(g, WHITE);
	for (; i < rowsize * height; i++) {
		r = rect(2+SCALE*(i%rowsize),2+SCALE*(i/rowsize),8,8);
		fill_rect(g, inset_rect(r,-1));
	}
	set_colour(g, BLACK);
	draw_rect(g, get_control_area(paledit));
}

void handle_paledit_click(Control *paledit, int buttons, Point p)
{
	ImageEditor *editor;
	int entry, rowsize, palsize;
	Colour *palette;
	Rect r;

	r = inset_rect(get_control_area(paledit),1);
	if (! point_in_rect(p,r))
		return;
	p = pt(p.x-r.x,p.y-r.y);
	rowsize = r.width / SCALE;
	if (p.x > rowsize*SCALE) return;

	editor = find_image_editor(paledit);
	palette = editor->img->cmap;
	palsize = editor->img->cmap_size;

	entry = (p.x / SCALE) + (p.y / SCALE) * rowsize;
	if (entry >= palsize)
		return;

	update_colour_editor(editor, entry);
}

int find_colour_component(Control *which)
{
	int wrong = 0;
	int value;

	value = atoi(get_control_text(which));
	if (value < 0)   value = 0, wrong = 1;
	if (value > 255) value = 255, wrong = 1;
	if (wrong) set_control_text(which, int_to_string(value));
	return value;
}

void add_colour(Control *btn)
{
	ImageEditor *editor;
	int i, t, r, g, b;
	Image *img;
	Colour *palette;
	Colour palette2[256];
	int palsize;
	Colour colour;

	editor = find_image_editor(btn);
	img = editor->img;
	palette = img->cmap;
	palsize = img->cmap_size;
	if (palsize == 2<<img->depth) {
		ask_ok(editor->app, "Error", "Sorry, the palette is full.");
		return;
	}
	for (i=0; i < palsize; i++)
		palette2[i] = palette[i];

	if (is_checked(editor->transparent)) t = 0xFF;
	else t = 0x00;

	r = find_colour_component(editor->red);
	g = find_colour_component(editor->green);
	b = find_colour_component(editor->blue);

	colour = argb(t,r,g,b);
	palette2[palsize] = colour;

	prepare_for_change(editor);
	set_image_cmap(img, palsize+1, palette2);
	draw_control(editor->paledit);
	draw_control(editor->display);
	image_changed(editor);
}

void change_colour(Control *btn)
{
	ImageEditor *editor;
	int i, t, r, g, b;
	Image *img;
	Colour *palette;
	Colour palette2[256];
	int palsize;
	Colour colour;

	editor = find_image_editor(btn);
	img = editor->img;
	palette = img->cmap;
	palsize = img->cmap_size;

	for (i=0; i < palsize; i++)
		palette2[i] = palette[i];

	if (is_checked(editor->transparent)) t = 0xFF;
	else t = 0x00;

	r = find_colour_component(editor->red);
	g = find_colour_component(editor->green);
	b = find_colour_component(editor->blue);

	i = atoi(get_control_text(editor->pixval));
	if (i < 0) return;
	if (i >= palsize) return;

	colour = argb(t,r,g,b);
	palette2[i] = colour;

	prepare_for_change(editor);
	set_image_cmap(img, palsize, palette2);
	draw_control(editor->paledit);
	draw_control(editor->pixedit);
	draw_control(editor->display);
	image_changed(editor);
}

void update_scrollbars(ImageEditor *editor)
{
	int max;
	Rect r = get_control_area(editor->pixedit);

	editor->ysize = (r.height - 2) / editor->size;
	if (editor->ysize > editor->img->height)
		editor->ysize = editor->img->height;
	max = editor->img->height - editor->ysize;
	change_scroll_bar(editor->vert, editor->y, max, editor->ysize);

	editor->xsize = (r.width - 2) / editor->size;
	if (editor->xsize > editor->img->width)
		editor->xsize = editor->img->width;
	max = editor->img->width - editor->xsize;
	change_scroll_bar(editor->horiz, editor->x, max, editor->xsize);
}

void update_editor(ImageEditor *editor)
{
	update_scrollbars(editor);
	redraw_control(editor->pixedit);
	redraw_control(editor->display);
	redraw_control(editor->paledit);
}

void reset_editor(ImageEditor *editor)
{
	editor->x = editor->y = 0;
	update_colour_editor(editor, 0);
	update_editor(editor);
	redraw_window(editor->win);
}

void move_x(Control *s)
{
	ImageEditor *editor;
	int value;

	editor = find_image_editor(s);
	value = get_control_value(s);
	if (editor->x != value) {
		editor->x = value;
		redraw_control(editor->pixedit);
	}
}

void move_y(Control *s)
{
	ImageEditor *editor;
	int value;

	editor = find_image_editor(s);
	value = get_control_value(s);
	if (editor->y != value) {
		editor->y = value;
		redraw_control(editor->pixedit);
	}
}

int save_image(ImageEditor *editor, char *filename)
{
	if (! filename) /* cancel */
		return 0;
	if (! write_image(editor->img, filename)) {
		ask_ok(editor->app, "Error", "That file could not be saved. The file format might not yet be supported.");
		return 0;
	}

	/* change the editor filename */
	del_string(editor->filename);
	editor->filename = copy_string(filename);
	set_window_title(editor->win, filename);

	/* change the settings */
	editor->changed = 0;
	disable_menu_item(editor->undo_item);
	prepare_for_change(editor);

	return 1;
}

int open_image(ImageEditor *editor, char *filename)
{
	Image *img;

	img = read_image(filename, 8);
	if (! img) {
		ask_ok(editor->app, "Error", "Unable to load image.");
		return 0;
	}

	/* delete the old image from memory */
	if (editor->img)
		del_image(editor->img);

	/* change the editor filename */
	del_string(editor->filename);
	editor->filename = copy_string(filename);
	set_window_title(editor->win, filename);

	/* change the settings */
	editor->img = img;
	editor->changed = 0;
	disable_menu_item(editor->undo_item);
	prepare_for_change(editor);

	/* resize the display panel */
	size_control(editor->display, rect(0,0,img->width+2,img->height+2));

	/* redraw the various panels */
	reset_editor(editor);

	return 1;
}

int make_new_image(ImageEditor *editor)
{
	int x, y, width, height;
	Image *img;
	byte **pixels;
	Colour palette[5];
	int palsize = 5;

	palette[0] = BLACK;
	palette[1] = WHITE;
	palette[2] = RED;
	palette[3] = GREEN;
	palette[4] = BLUE;

	width = editor->new_width;
	height = editor->new_height;

	img = new_image(width, height, editor->new_depth);
	if (! img) {
		ask_ok(editor->app, "Error",
			"Insufficient memory to create a new image.");
		return 0;
	}
	pixels = img->data8;

	for (y=0; y < height; y++)
	  for (x=0; x < width; x++)
		pixels[y][x] = 1;
	set_image_cmap(img, palsize, palette);

	/* delete the old image from memory */
	if (editor->img)
		del_image(editor->img);

	/* change the editor filename */
	del_string(editor->filename);
	editor->filename = copy_string("untitled");
	set_window_title(editor->win, editor->filename);

	/* change the settings */
	editor->img = img;
	editor->changed = 0;
	disable_menu_item(editor->undo_item);
	prepare_for_change(editor);

	/* resize the display panel */
	size_control(editor->display, rect(0,0,width+2,height+2));

	/* redraw the various panels */
	reset_editor(editor);

	return 1;
}

int ask_save_changed_image(ImageEditor *editor)
{
	int result;
	char *savename;

	/* save existing modified image */
	if (editor->changed) {
		result = ask_yes_no_cancel(editor->app,
			"Save Changes?", "Image has changed. Save changes?");
		if (result == CANCEL) /* cancel */
			return CANCEL;
		if (result == YES) { /* save changes */
			savename = ask_file_save(editor->app,
				"Save Image", "Save",
				editor->filename);
			if (! save_image(editor, savename))
				return CANCEL;
			del_string(savename);
		}
	}
	return YES;
}

void set_image_size(Control *b)
{
	ImageEditor *editor = find_image_editor(b);
	editor->new_width = abs(atoi(get_control_text(editor->new_width_input)));
	editor->new_height = abs(atoi(get_control_text(editor->new_height_input)));
	make_new_image(editor);
	hide_window(parent_window(b));
}

void ask_new_image_size(ImageEditor *editor)
{
	Window *w;
	Font *f;
	int h;
	char buf[30];

	if (! editor->new_image_window) {
		f = find_default_font(editor->app);
		h = font_height(f) + 5;

		editor->new_image_window = w = new_window(editor->app,
			rect(50,50,200,h+h+h+70),
			"New Image Size", TITLEBAR + MODAL);
		set_window_data(w, editor);
		new_label(w, rect(10,10,120,h+10),
				"Pixel width:", ALIGN_LEFT);
		new_label(w, rect(10,h+30,120,h+10),
				"Pixel height:", ALIGN_LEFT);
		editor->new_width_input = new_field(w,
				rect(135,10,50,h+10), "");
		editor->new_height_input = new_field(w,
				rect(135,h+30,50,h+10), "");
		new_button(w, rect(60,h+h+50,80,h+10),
				"Okay", set_image_size);
	}
	sprintf(buf, "%d", editor->new_width);
	set_control_text(editor->new_width_input, buf);
	sprintf(buf, "%d", editor->new_height);
	set_control_text(editor->new_height_input, buf);
	show_window(editor->new_image_window);
}

void ask_new_image(MenuItem *mi)
{
	ImageEditor *editor;

	editor = find_image_editor_from_menu_item(mi);

	if (ask_save_changed_image(editor) == CANCEL)
		return;
	ask_new_image_size(editor);
}

void ask_open_image(MenuItem *mi)
{
	char *filename;
	ImageEditor *editor;

	editor = find_image_editor_from_menu_item(mi);

	if (ask_save_changed_image(editor) == CANCEL)
		return;
	filename = ask_file_open(editor->app, "Open Image",
			"Open", "");
	if (filename) {
		open_image(editor, filename);
		del_string(filename);
	}
}

void do_save_image(MenuItem *mi)
{
	char *filename;
	ImageEditor *editor;

	editor = find_image_editor_from_menu_item(mi);

	if (editor->filename)
		filename = copy_string(editor->filename);
	else
		filename = ask_file_save(editor->app, "Save Image",
				"Save", editor->filename);
	if (filename) {
		save_image(editor, filename);
		del_string(filename);
	}
}

void ask_save_image(MenuItem *mi)
{
	char *filename;
	ImageEditor *editor;

	editor = find_image_editor_from_menu_item(mi);

	filename = ask_file_save(editor->app, "Save Image",
				"Save", editor->filename);
	if (filename) {
		save_image(editor, filename);
		del_string(filename);
	}
}

void do_quit_program_window(Window *w)
{
	ImageEditor *editor;

	editor = get_window_data(w);
	if (ask_save_changed_image(editor) == CANCEL)
		return;
	exit(0);
}

void do_quit_program(MenuItem *mi)
{
	ImageEditor *editor;

	editor = find_image_editor_from_menu_item(mi);
	if (ask_save_changed_image(editor) == CANCEL)
		return;
	exit(0);
}

void use_pencil(MenuItem *mi)
{
	ImageEditor *editor;

	editor = find_image_editor_from_menu_item(mi);
	check_menu_item(editor->pencil);
	uncheck_menu_item(editor->sampler);
}

void use_sampler(MenuItem *mi)
{
	ImageEditor *editor;

	editor = find_image_editor_from_menu_item(mi);
	uncheck_menu_item(editor->pencil);
	check_menu_item(editor->sampler);
}

void do_undo(MenuItem *mi)
{
	ImageEditor *editor;
	Image *new_img;

	editor = find_image_editor_from_menu_item(mi);
	if (editor->changed) {
		new_img = copy_image(editor->prev_img);
		del_image(editor->img);
		editor->img = new_img;
		update_editor(editor);
	}
}

void do_reduce_palette(MenuItem *mi)
{
	ImageEditor *editor;

	editor = find_image_editor_from_menu_item(mi);
	prepare_for_change(editor);
	reduce_palette(editor->img);
	reset_editor(editor);
	image_changed(editor);
}

void do_sort_palette(MenuItem *mi)
{
	ImageEditor *editor;

	editor = find_image_editor_from_menu_item(mi);
	prepare_for_change(editor);
	image_sort_palette(editor->img);
	reset_editor(editor);
	image_changed(editor);
}

ImageEditor *create_image_editor(App *app)
{
	ImageEditor *editor;
	Rect r;
	Window *w;
	MenuBar *mb;
	Menu *m;
	Font *f;
	int x, y, h, minw, maxw;

	editor = alloc(sizeof(ImageEditor));
	if (! editor)
		return editor;

	editor->app = app;
	editor->filename = copy_string("Untitled");
	editor->x = editor->y = 0;
	editor->size = SCALE;
	editor->changed = 0;
	editor->new_width  = 32;
	editor->new_height = 32;
	editor->new_depth  = 8;

	editor->bgcolour = LIGHT_GREY;

	r = rect(50,50,576,450);
	editor->win = w = new_window(app, r, "Image Editor", STANDARD_WINDOW);
	set_window_data(w, editor);
	on_window_close(w, do_quit_program_window);

	editor->mbar = mb = new_menu_bar(w);
	y = mb->ctrl->area.height;

	editor->file_menu = m = new_menu(mb, "File");
	editor->new_item = new_menu_item(m, "New...", 'N', ask_new_image);
	editor->open_item = new_menu_item(m, "Open...", 'O', ask_open_image);
	editor->save_item = new_menu_item(m, "Save", 'S', do_save_image);
	editor->save_item = new_menu_item(m, "Save As...", 'A', ask_save_image);
	editor->line_1 = new_menu_item(m, "-", 0, NULL);
	editor->quit_item = new_menu_item(m, "Exit", 'Q', do_quit_program);

	editor->file_menu = m = new_menu(mb, "Edit");
	editor->undo_item = new_menu_item(m, "Undo", 'Z', do_undo);
	disable_menu_item(editor->undo_item);
	editor->line_2 = new_menu_item(m, "-", 0, NULL);
	editor->sort_palette = new_menu_item(m, "Reduce Palette", 0, do_reduce_palette);
	editor->sort_palette = new_menu_item(m, "Sort Palette", 0, do_sort_palette);

	editor->tool_menu = m = new_menu(mb, "Tools");
	editor->pencil = new_menu_item(m, "Pencil", 'P', use_pencil);
	check_menu_item(editor->pencil);
	editor->sampler = new_menu_item(m, "Colour Sampler", 'L', use_sampler);

	r = rect(10,y+10,2+XSIZE*SCALE,2+YSIZE*SCALE);
	editor->pixedit = new_control(w, r);
	on_control_redraw(editor->pixedit, redraw_pixedit);
	on_control_mouse_down(editor->pixedit, handle_pixedit_click);
	on_control_mouse_drag(editor->pixedit, handle_pixedit_drag);

	editor->ysize = YSIZE;
	editor->vert = new_scroll_bar(w,
			rect(r.x+r.width+2,r.y,16,r.height),
			0, YSIZE, move_y);
	editor->xsize = XSIZE;
	editor->horiz = new_scroll_bar(w,
			rect(r.x,r.y+r.height+2,r.width,16),
			0, XSIZE, move_x);

	x = r.x + r.width + 30;
	f = find_default_font(app);
	h = font_height(f);
	maxw = font_width(f, "<<Green>>", 9);
	minw = font_width(f, "456789", 6) + 5;
	if (maxw < minw + 5)
		maxw = minw + 5;

	r = rect(x,r.y,150,h);
	new_label(w, r, "Colour:", ALIGN_LEFT);

	r = rect(x,r.y+r.height+5,minw,h+10);
	editor->pixval = new_field(w, r, "0");
	disable(editor->pixval);
	r.x = r.x + maxw + 5;
	r.height = r.height + 5;

	r.width = font_width(f, "Transparent ", 12) + 30;
	editor->transparent = new_check_box(w, r, "Transparent", NULL);

	r = rect(x, r.y+r.height+5, maxw, h);

	new_label(w, r, "Red", ALIGN_LEFT);
	editor->red = new_field(w, rect(r.x,r.y+h+5,minw,h+10), "0");
	app_set_field_allowed_chars(editor->red, "0123456789");
	app_set_field_allowed_width(editor->red, 3);
	r.x = r.x + r.width + 5;

	new_label(w, r, "Green", ALIGN_LEFT);
	editor->green = new_field(w, rect(r.x,r.y+h+5,minw,h+10), "0");
	app_set_field_allowed_chars(editor->green, "0123456789");
	app_set_field_allowed_width(editor->green, 3);
 	r.x = r.x + r.width + 5;

	new_label(w, r, "Blue", ALIGN_LEFT);
	editor->blue = new_field(w, rect(r.x,r.y+h+5,minw,h+10), "0");
	app_set_field_allowed_chars(editor->blue, "0123456789");
	app_set_field_allowed_width(editor->blue, 3);

	r = rect(x,r.y + r.height + h + 20, 76, h+5);
	editor->add = new_button(w, r, "Add", add_colour);
	r.x = r.x + r.width + 10;
	editor->change = new_button(w, r, "Change", change_colour);

	r = rect(x,r.y + r.height + 5, 150, h);
	new_label(w, r, "Palette:", ALIGN_LEFT);

	r = rect(x,r.y+r.height+5,162,162);
	editor->paledit = new_control(w, r);
	on_control_redraw(editor->paledit, redraw_paledit);
	on_control_mouse_up(editor->paledit, handle_paledit_click);

	r = rect(x,r.y+r.height+10,66,66);
	editor->display = new_control(w, r);
	on_control_redraw(editor->display, redraw_display);

	set_window_background(editor->win, editor->bgcolour);

	editor->prev_img = NULL;
	editor->img = NULL;
	editor->new_image_window = NULL;

	if (! make_new_image(editor))
		return NULL;
	update_scrollbars(editor);
	prepare_for_change(editor);

	return editor;
}

void show_image_editor(ImageEditor *editor)
{
	show_window(editor->win);
}

int main(int argc, char *argv[])
{
	ImageEditor *editor;
	App *app;

	app = new_app(argc, argv);

	editor = create_image_editor(app);
	if (argv && argv[1])
		open_image(editor, argv[1]);
	show_image_editor(editor);

	main_loop(app);
	return 0;
}
