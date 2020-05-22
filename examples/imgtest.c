/*
 *  A program to test the display of semi-tranparent images.
 *
 *  This program loads a series of up to 10 images,
 *  some of which are transparent, some of which are not.
 *  The images must be called "imgtestN.png" where N is
 *  a number from 0 to 9. The program draws the images onto
 *  a window to demonstrate that copying semi-transparent
 *  images to a window, or over other bitmaps, works as
 *  expected. Resizing the window also moves where the
 *  images are drawn, so that the different transparent
 *  portions can more easily be seen.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <graphapp.h>

Image *  img[10] = { NULL };
Bitmap * src[10] = { NULL };
Bitmap * dst[10] = { NULL };

void window_redraw(Window *w, Graphics *g)
{
	int i;
	int hmiddle, hspace, vmiddle, vspace;
	Rect r;
	Graphics *sg;
	Graphics *dg;

	r = get_window_area(w);
	set_rgb(g, rgb(240,240,240));
	fill_rect(g, r);

	hmiddle = r.width / 2;
	hspace = hmiddle / 12;
	vmiddle = r.height / 2;
	vspace = vmiddle / 12;

	/* test overlapping semi-transparent images copied directly */
	for (i=0; i < 10; i++) {
		if (src[i] == NULL)
			continue;
		sg = get_bitmap_graphics(src[i]);
		r = get_bitmap_area(src[i]);
		copy_rect(g, pt((i+1)*hspace, (i+1)*vspace), sg, r);
		del_graphics(sg);
	}

	/* test overlapping semi-transparent images copied in memory */
	for (i=1; i < 10; i++) {
		if ((src[i] == NULL) || (dst[i-1] == NULL))
			continue;
		sg = get_bitmap_graphics(src[i]);
		dg = get_bitmap_graphics(dst[i-1]);
		r = get_bitmap_area(src[i]);
		copy_rect(dg, pt(0,0), sg, r);
		copy_rect(g, pt(i*hspace + hmiddle, i*vspace), dg, r);
		del_graphics(sg);
		del_graphics(dg);
	}
}

int main(int argc, char *argv[])
{
	App *app;
	Window *win;
	int i;
	char name[30];

	app = new_app(argc, argv);

	win = new_window(app, rect(50,100,350,240),
		"Test Image Display", STANDARD_WINDOW);
	on_window_redraw(win, window_redraw);

	for (i=0; i < 10; i++) {
		sprintf(name, "imgtest%d.png", i);
 		img[i] = read_image(name, 32);
		if (img[i] == NULL) {
			ask_ok(app, "Error", "ImgTest: try running this program in the src/demo sub-directory.");
			return 2;
		}
		if (img[i] != NULL) {
			src[i] = image_to_bitmap(win, img[i]);
			dst[i] = image_to_bitmap(win, img[i]);
		}
		else {
			/* hmm, maybe we're not in the correct folder */
			sprintf(name, "demo/imgtest%d.png", i);
 			img[i] = read_image(name, 32);
			if (img[i] != NULL) {
				src[i] = image_to_bitmap(win, img[i]);
				dst[i] = image_to_bitmap(win, img[i]);
			}
		}
	}

	show_window(win);
	main_loop(app);

	del_app(app);
	return 0;
}

