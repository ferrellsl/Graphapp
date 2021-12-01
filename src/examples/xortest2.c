/*
 *  XOR Test 2
 *  ----------
 *  This program is the same as the XOR Test, except
 *  the image is blitted through a bitmap to test
 *  bitmap XOR handling.
 */

#include <stdio.h>
#include <string.h>
#include <graphapp.h>

Bitmap *b;

Font *fnt[2];
int initialised = 0;
Image *img;
char *text = "Drag me around";
Point pos = {200,0};
#define BG  GREY
#define FG  RED

void draw_it(Bitmap *b)
{
	Graphics *g;

	g = get_bitmap_graphics(b);

	set_xor_mode(g, BG);
	set_colour(g, FG);

	if (img)
		draw_image(g, rect(pos.x,pos.y,32,32), img, rect(0,0,32,32));
	draw_rect(g, rect(pos.x-1,pos.y-1,34,34));
	draw_line(g, pt(pos.x+36,pos.y), pt(pos.x+46,pos.y+20));
	set_font(g, fnt[0]);
	draw_utf8(g, pt(pos.x,pos.y+32), text, strlen(text));
	set_font(g, fnt[1]);
	draw_utf8(g, pt(pos.x,pos.y+50), text, strlen(text));

	del_graphics(g);
}

void mouse_it(Window *w, int buttons, Point p)
{
	Graphics *g, *g2;

	draw_it(b);	/* erase */
	pos = p;	/* update position */
	draw_it(b);	/* draw in new position */

	g = get_window_graphics(w);
	g2 = get_bitmap_graphics(b);
	copy_rect(g, pt(0,0), g2, get_window_area(w));
	del_graphics(g2);
	del_graphics(g);
}

void draw_background(Graphics *g)
{
	Rect r;

	r = g->area;
	r.width /= 2;
	r.height /= 2;

	set_colour(g, WHITE);
	fill_rect(g, r);
	r.x += r.width;
	set_colour(g, GREY);
	fill_rect(g, r);
	r.x = 0;
	r.y = r.height;
	set_colour(g, RED);
	fill_rect(g, r);
	r.x += r.width;
	set_colour(g, BLACK);
	fill_rect(g, r);
}

void init_bitmap(Bitmap *b)
{
	Graphics *g;

	g = get_bitmap_graphics(b);
	draw_background(g);
	del_graphics(g);
}

void redraw_it(Window *w, Graphics *g)
{
	Graphics *g2;

	if (! initialised) {
		initialised = 1;
		draw_it(b);
	}

	g2 = get_bitmap_graphics(b);
	copy_rect(g, pt(0,0), g2, get_window_area(w));
	del_graphics(g2);
}

int main(int argc, char *argv[])
{
	App *app;
	Window *w;

	app = new_app(argc, argv);

	fnt[0] = new_font(app, "unifont", PLAIN, 16);
	fnt[1] = new_font(app, "Times", PLAIN, 16);
	img = read_image("../src/demo/imgtest2.png", 32);

	w = new_window(app, rect(0,0,400,400),
			"XOR Drawing Test", STANDARD_WINDOW);
	on_window_redraw(w, redraw_it);
	on_window_mouse_down(w, mouse_it);
	on_window_mouse_drag(w, mouse_it);
	b = new_bitmap(w, 400, 400);
	init_bitmap(b);
	show_window(w);
	main_loop(app);

	return 0;
}
