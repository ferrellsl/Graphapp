/*
 *  Spectrum
 *  --------
 *  A program for displaying many colours on a window.
 *  Try re-arranging the red, green and blue components of the spectrum
 *  to see different spectrums. Or try reducing the number of colours to
 *  output, and see what happens.
 */

#include <stdio.h>
#include <graphapp.h>

enum {
	MAX_INTENSITY = 255,
	NCOLOURS      = 180 	/* try 15, 31, 90, 180 or 256 */
};

Colour colour[NCOLOURS];	/* an array of colours */

/* fill the array of colours with NCOLOURS rgb-values */
void create_spectrum(Colour colour[], int ncolours)
{
	int i;
	float c, x, y;
	long r, g, b;
  
	c = 1.0 / ( (1-0.3455)*(1-0.3455) * (1-0.90453)*(1-0.90453) );

	for(i = 0; i < ncolours ; i++)
	{
		/* ramp for first colour, y=x */
		x = (float)(i)/(float)(ncolours - 1);
		r = (long) (x * MAX_INTENSITY);

		/* single hump for next colour, y=x(4x-3)^2 */
		y = x*(4*x - 3)*(4*x - 3);
		if (y > 1.0) y = 1.0;
		if (y < 0.0) y = 0.0;
		g = (long) (y * MAX_INTENSITY);
     
		/* double hump for next colour, y=cx(x-a)^2(x-b)^2 */
		y = c*x*(x - 0.3455)*(x - 0.3455)*(x - 0.90453)*(x - 0.90453);
		if (y > 1.0) y = 1.0;
		if (y < 0.0) y = 0.0;
		b = (long) (y * MAX_INTENSITY);

		/* store resultant colour */
		/* change the order of r,g,b in this line to see more spectrums */
		colour[i] = rgb(r,g,b);
	}
}

void fill_spectrum_area(Graphics *g, Rect r, Colour colour[], int ncolours)
{
	int i, n;

	/* draw a black border around the display area */
	set_colour(g, BLACK);
	draw_rect(g, rect(r.x-1,r.y-1,r.width+2,r.height+2));

	/* step across the display rectangle, filling vertical strips */
	for (i = 0; i < r.width; i++)
	{
		n = (int) (i * (float) ncolours / r.width);
		set_colour(g, colour[n]);
		fill_rect(g, rect(i+r.x,r.y,1,r.height));
	}
}

void draw_spectrum(Window *w, Graphics *g)
{
	Rect r = get_window_area(w);
	Rect r2 = inset_rect(r,10);

	/* there is a minimum size for the display rectangle */
	if (r2.width < 10)
		r2.width = 10;
	if (r2.height < 10)
		r2.height = 10;
	r2 = center_rect(r2, r);

	/* fill the display area with a rainbow */
	fill_spectrum_area(g, r2, colour, NCOLOURS);
}

int main(int argc, char *argv[])
{
	App *app;
	Window *w;

	app = new_app(argc, argv);
	create_spectrum(colour, NCOLOURS);
	w = new_window(app, rect(50,50,532,250), "Spectrum",
		STANDARD_WINDOW);
	on_window_redraw(w, draw_spectrum);
	show_window(w);
	main_loop(app);
	del_app(app);
	return 0;
}

