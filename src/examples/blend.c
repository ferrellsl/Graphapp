/*
 *  A program to test alpha-blending and drawing to images.
 *
 *  Images can be drawn into in the same way as bitmaps or windows.
 *  We should, in theory, be able to load an image, draw some text
 *  into it, fill a rectangle with some colour, do some alpha blending,
 *  then copy it to a window.
 *
 *  Let's see if all that works!
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <graphapp.h>

Image * texture = NULL;
Image * lightmap = NULL;
Image * alphamap = NULL;
Image * addition = NULL;
Image * result = NULL;

void window_redraw(Window *w, Graphics *dst)
{
	Rect r;
	Graphics *src;

	r = get_window_area(w);
	set_rgb(dst, rgb(240,240,240));
	fill_rect(dst, r);
	set_rgb(dst, rgb(0,0,0));
	draw_utf8(dst, pt(144,66), "+", 1);
	draw_utf8(dst, pt(292,66), "x", 1);
	draw_utf8(dst, pt(440,66), "=", 1);

	if (texture) {
		src = get_image_graphics(texture);
		r = get_image_area(texture);
		copy_rect(dst, pt(10,10), src, r);
		del_graphics(src);
	}
	if (addition) {
		src = get_image_graphics(addition);
		r = get_image_area(addition);
		copy_rect(dst, pt(30+128,10), src, r);
		del_graphics(src);
	}
	if (lightmap) {
		src = get_image_graphics(lightmap);
		r = get_image_area(lightmap);
		copy_rect(dst, pt(50+128*2,10), src, r);
		del_graphics(src);
	}
	if (result) {
		src = get_image_graphics(result);
		r = get_image_area(result);
		copy_rect(dst, pt(70+128*3,10), src, r);
		del_graphics(src);
	}
}

void build_alpha_channel(Image *img)
{
	/* synthesise the alpha channel by copying the red channel */
	int x, y;

	for (y=0; y < img->height; y++)
		for (x=0; x < img->width; x++)
			img->data32[y][x].alpha = img->data32[y][x].red;
}

void make_transparent(Image *img)
{
	/* make the entire image transparent */
	int x, y;

	for (y=0; y < img->height; y++)
		for (x=0; x < img->width; x++)
			img->data32[y][x].alpha = 255;
}

void test_drawing_ops(App *app)
{
	Graphics *src, *dst;

	result = copy_image(texture);
	alphamap = copy_image(lightmap);

	build_alpha_channel(alphamap);

	/* create the image to add */

	addition = new_image(texture->width, texture->height, 32);
	make_transparent(addition);

	src = get_image_graphics(addition);

	set_rgb(src, rgb(0,255,0)); /* some green text */
	draw_utf8(src, pt(50,10), "Some text here", 14);

	set_rgb(src, rgb(255,0,0)); /* a red rectangle */
	fill_rect(src, rect(10,100,50,10));

	/* copy the added image over the result */

	dst = get_image_graphics(result);
	copy_rect(dst, pt(0,0), src, get_image_area(addition));
	del_graphics(src);

	/* alpha blend the result with the lightmap */

	src = get_image_graphics(alphamap);
	copy_rect(dst, pt(0,0), src, get_image_area(alphamap));
	del_graphics(src);
	del_graphics(dst);
}

int main(int argc, char *argv[])
{
	App *app;
	Window *win;

	app = new_app(argc, argv);

	texture  = read_image("texture.jpg", 32);
	if (texture == NULL)
		texture  = read_image("demo/texture.jpg", 32);
	if (texture == NULL)
		texture  = read_image("../src/demo/texture.jpg", 32);
	if (texture == NULL) {
		ask_ok(app, "Error", "Blend: Try running this program in the src/demo sub-directory.");
		return 2;
	}
	lightmap = read_image("lightmap.jpg", 32);
	if (lightmap == NULL)
		lightmap = read_image("demo/lightmap.jpg", 32);
	if (lightmap == NULL)
		lightmap = read_image("../src/demo/lightmap.jpg", 32);

	test_drawing_ops(app);

	win = new_window(app, rect(50,100,595,150),
		"Test Alpha-Blending and Image Drawing", STANDARD_WINDOW);
	on_window_redraw(win, window_redraw);

	show_window(win);
	main_loop(app);

	del_app(app);
	return 0;
}

