/*
 *  Polygons
 *  --------
 *
 *  A program to test the filling of polygons.
 *  This program draws several kinds of polygons across
 *  the window, using the timer functions to slow drawing
 *  so that each drawing operation can be seen briefly.
 *  Polygons are drawn such that they fit neatly together,
 *  to allow tiling to work as expected.
 */

#include <stdio.h>
#include <graphapp.h>

#define DRAW_POLYGON(PointList,Color,X,Y)				\
	g->offset = pt(X,Y);						\
	set_rgb(g, Color);						\
	fill_polygon(g, PointList, sizeof(PointList)/sizeof(Point));\
	draw_all(g->app);

void draw_polygons(Window *w, Graphics *g)
{
	int i, j;
	Point Polygon1[] =
		{{0,0},{100,150},{320,0},{0,200},{220,50},{320,200}};
	Point Polygon2[] =
		{{0,0},{320,0},{320,200},{0,200},{0,0},{50,50},
		 {270,50},{270,150},{50,150},{50,50}};
	Point Polygon3[] =
		{{0,0},{10,0},{105,185},{260,30},{15,150},{5,150},{5,140},
		 {260,5},{300,5},{300,15},{110,200},{100,200},{0,10}};
	Point Polygon4[] =
		{{0,0},{30,-20},{30,0},{0,20},{-30,0},{-30,-20}};
	Point Triangle1[] = {{30,0},{15,20},{0,0}};
	Point Triangle2[] = {{30,20},{15,0},{0,20}};
	Point Triangle3[] = {{0,20},{20,10},{0,0}};
	Point Triangle4[] = {{20,20},{20,0},{0,10}};

	/* Draw three complex polygons */
	DRAW_POLYGON(Polygon1, RED,   0, 0);
	delay(g->app, 2000);
	DRAW_POLYGON(Polygon2, GREEN, 0, 0);
	delay(g->app, 2000);
	DRAW_POLYGON(Polygon3, BLUE,  0, 0);
	delay(g->app, 2000);

	/* Draw some adjacent nonconvex polygons */
	for (i=0; i<5; i++) {
		for (j=0; j<8; j++) {
			DRAW_POLYGON(Polygon4, rgb(0,0,50+5*(i*8+j)),
				40+(i*60), 30+(j*20));
			delay(g->app, 50);
		}
	}

	/* Draw adjacent triangles across the screen */
	for (j=0; j<=80; j+=20) {
		for (i=0; i<290; i += 30) {
			DRAW_POLYGON(Triangle1, ORANGE, i, j);
			delay(g->app, 50);
			DRAW_POLYGON(Triangle2, YELLOW, i+15, j);
			delay(g->app, 50);
		}
	}
	for (j=100; j<=170; j+=20) {
		/* Draw a row of right-pointing triangles */
		for (i=0; i<290; i += 20) {
			DRAW_POLYGON(Triangle3, RED, i, j);
			delay(g->app, 50);
		}
		/* Draw a row of left-pointing triangles halfway
		 * between one row of right-pointing triangles and
		 * the next, to fit between */
		for (i=0; i<290; i += 20) {
			DRAW_POLYGON(Triangle4, MAGENTA, i, j+10);
			delay(g->app, 50);
		}
	}
}

int main(int argc, char *argv[])
{
	App *app;
	Window *w;

	app = new_app(argc, argv);
	w = new_window(app, rect(10,10,448,400), "Polygons Test",
			STANDARD_WINDOW);
	on_window_redraw(w, draw_polygons);
	show_window(w);
	main_loop(app);
	del_app(app);
	return 0;
}
