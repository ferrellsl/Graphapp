/*
 *  Headless pixel test for the Cocoa backend.
 *
 *  Draws the same scene into a window's surface and into a bitmap
 *  (without showing any window), checks the two agree, and saves the
 *  bitmap as a PNG so it can be looked at.
 *
 *    APP_FONT_PATH=../fonts build-macos/test/pixeltest out.png
 */

#include "appint.h"

Bitmap *app_new_white_bitmap(Window *w, int width, int height);

#define W 320
#define H 200

static void draw_scene(App *app, Graphics *g, Window *win)
{
	Font *nat, *natb, *port;
	Image *img;
	int x, y;
	Point poly[4];

	/* background */
	app_set_rgb(g, WHITE);
	app_fill_rect(g, rect(0, 0, W, H));

	/* rectangles and lines */
	app_set_rgb(g, RED);
	app_fill_rect(g, rect(10, 10, 60, 40));
	app_set_rgb(g, BLUE);
	app_draw_rect(g, rect(5, 5, 70, 50));
	app_set_rgb(g, BLACK);
	app_draw_line(g, pt(10, 70), pt(120, 95));
	app_set_line_width(g, 3);
	app_draw_line(g, pt(10, 100), pt(120, 80));
	app_set_line_width(g, 1);

	/* polygon and ellipse */
	poly[0] = pt(150, 10); poly[1] = pt(200, 20);
	poly[2] = pt(190, 60); poly[3] = pt(140, 50);
	app_set_rgb(g, rgb(0, 160, 0));
	app_fill_polygon(g, poly, 4);
	app_set_rgb(g, rgb(200, 0, 200));
	app_fill_ellipse(g, rect(210, 10, 50, 40));

	/* native fonts */
	nat = app_new_font(app, "Helvetica", PLAIN | NATIVE_FONT, 16);
	natb = app_new_font(app, "Helvetica", BOLD | NATIVE_FONT, 20);
	app_set_rgb(g, BLACK);
	if (nat) {
		app_set_font(g, nat);
		app_draw_utf8(g, pt(10, 110), "Native: Hello gjpqy", 19);
	}
	if (natb) {
		app_set_font(g, natb);
		app_draw_utf8(g, pt(10, 130), "Bold 20", 7);
	}

	/* portable font */
	port = app_find_default_font(app);
	if (port) {
		app_set_font(g, port);
		app_draw_utf8(g, pt(10, 160), "Portable font ABC xyz", 21);
	}

	/* XOR: a rect over the red one, and one drawn twice (vanishes) */
	app_set_xor_mode(g, WHITE);
	app_set_rgb(g, GREEN);
	app_fill_rect(g, rect(40, 30, 50, 30));
	app_set_rgb(g, BLUE);
	app_fill_rect(g, rect(100, 30, 20, 20));
	app_fill_rect(g, rect(100, 30, 20, 20));
	app_set_paint_mode(g);

	/* clipping */
	app_set_clip_rect(g, rect(270, 10, 30, 30));
	app_set_rgb(g, rgb(255, 160, 0));
	app_fill_rect(g, rect(250, 0, 70, 70));
	app_set_clip_rect(g, rect(0, 0, W, H));

	/* image with transparent corners, drawn with a stencil */
	img = app_new_image(40, 40, 32);
	for (y = 0; y < 40; y++) {
		for (x = 0; x < 40; x++) {
			int dx = x - 20, dy = y - 20;
			if (dx*dx + dy*dy < 19*19)
				img->data32[y][x] = argb(0, x*6, 255 - y*6, 128);
			else
				img->data32[y][x] = argb(255, 0, 0, 0);
		}
	}
	app_set_rgb(g, rgb(230, 230, 60));
	app_fill_rect(g, rect(250, 110, 60, 60));	/* to show through */
	app_draw_image(g, rect(260, 120, 40, 40), img, app_get_image_area(img));
	app_del_image(img);

	/* scroll a region onto itself (overlapping copy) */
	app_copy_rect(g, pt(20, 172), g, rect(10, 160, 120, 20));
}

int main(int argc, char *argv[])
{
	App *app;
	Window *win;
	Bitmap *bmp;
	Graphics *wg, *bg;
	Surface *ws, *bs;
	size_t i, bad = 0;
	Image *out;

	app = app_new_app(argc, argv);
	if (! app || ! app->gui_available) {
		fprintf(stderr, "no GUI\n");
		return 1;
	}
	win = app_new_window(app, rect(100, 100, W, H), "pixeltest", STANDARD_WINDOW);
	bmp = app_new_white_bitmap(win, W, H);

	wg = app_get_window_graphics(win);
	bg = app_get_bitmap_graphics(bmp);
	draw_scene(app, wg, win);
	draw_scene(app, bg, win);

	ws = &win_extra(win)->surf;
	bs = &bitmap_extra(bmp)->surf;
	for (i = 0; i < (size_t) W * H; i++)
		if (ws->pixels[i] != bs->pixels[i])
			bad++;
	printf("window surface %dx%d, bitmap %dx%d, differing pixels: %zu\n",
		ws->width, ws->height, bs->width, bs->height, bad);

	if (argc > 1) {
		out = app_bitmap_to_image(bmp);
		printf("write %s: %d\n", argv[1], app_write_image(out, argv[1]));
	}

	app_del_graphics(wg);
	app_del_graphics(bg);
	return bad ? 2 : 0;
}
