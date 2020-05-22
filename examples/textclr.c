/*
 *  Text Colour Tester
 *  ------------------
 *  An example of drawing coloured text on different
 *  shades of background.
 */

#include <stdio.h>
#include <string.h>
#include <graphapp.h>

enum {
	NUM_FONTS = 2,
	NUM_BG    = 5,
	NUM_FG    = 4
};

Font *fnt[NUM_FONTS];

/*
 *  The user interface.
 */

void draw_it(Window *w, Graphics *g)
{
	Rect r;
	int half, i, j, k;
	char text[30];
	Colour bg[NUM_BG];
	char *bg_name[NUM_BG];
	Colour fg[NUM_FG];
	char *fg_name[NUM_FG];

	r = get_window_area(w);
	r.height = 16;
	half = r.width / 2;

	bg[0] = WHITE;		bg_name[0] = "white";
	bg[1] = LIGHT_GREY;	bg_name[1] = "light grey";
	bg[2] = GREY;		bg_name[2] = "grey";
	bg[3] = DARK_GREY;	bg_name[3] = "dark grey";
	bg[4] = BLACK;		bg_name[4] = "black";

	fg[0] = RED;		fg_name[0] = "red";
	fg[1] = GREY;		fg_name[1] = "grey";
	fg[2] = BLACK;		fg_name[2] = "black";
	fg[3] = WHITE;		fg_name[3] = "white";

	for (i=0; i < NUM_FONTS; i++) {  /* try each font */
	  for (j=0; j < NUM_BG; j++) {   /* try each bg colour */
		for (k=0; k < NUM_FG; k++) { /* try each fg colour */

		set_font(g, fnt[i]);
		set_colour(g, bg[j]);
		fill_rect(g, r);
		set_colour(g, fg[k]);
		draw_ellipse(g, rect(r.x,r.y+4,10,10));
		sprintf(text, "%s on %s", fg_name[k], bg_name[j]);
		draw_utf8(g, pt(r.x+12,r.y), text, strlen(text));

		r.x += half;
		set_xor_mode(g, bg[j]);
		draw_ellipse(g, rect(r.x,r.y+4,10,10));
		sprintf(text, "%s xor %s", fg_name[k], bg_name[j]);
		draw_utf8(g, pt(r.x+12,r.y), text, strlen(text));
		set_paint_mode(g);
		r.x = 0;

		r.y += r.height;
		}
	  }
	}
}

void key_it(Window *w, unsigned long key)
{
	redraw_window(w);
}

int main(int argc, char *argv[])
{
	App *app;
	Window *w;

	app = new_app(argc, argv);

	fnt[1] = new_font(app, "unifont", PLAIN, 16);
	fnt[0] = new_font(app, "Times", PLAIN, 16);

	w = new_window(app, rect(0,0,400,400),
			"Text Colour Test", STANDARD_WINDOW);
	on_window_redraw(w, draw_it);
	on_window_key_down(w, key_it);
	show_window(w);
	main_loop(app);

	return 0;
}

