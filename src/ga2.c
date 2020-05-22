/*
 *  GraphApp version 2 compatability library.
 *
 *  This is an experimental library of functions which
 *  implement the old GraphApp version 2 interface
 *  using the version 3 library.
 *
 *  Old version 2 programs should be able to work by
 *  linking with the version 3 library and this file.
 *  Type "make ga2" to create a combined library which
 *  emulates the old GraphApp 2 interface.
 *
 *  The Makefile for your program will need to link
 *  to the ga2 library. You may need to adjust it to
 *  say something like this (on Linux):
 *
 *  GRAPHAPP = /home/yourlogin/apps/app/graphapp/src
 *  LINK = -L$(GRAPHAPP) -Xlinker -rpath -Xlinker $(GRAPHAPP)
 *  LIBS = -lga2
 *
 *  You could also rename libga2.so to libgraphapp.so
 *  to replace your installation of GraphApp version 2,
 *  but I recommend against that in case you want to
 *  fall back to using the old version.
 *
 *  A copy of the old graphapp.h header file can be
 *  found in the ga2.h file. It has been renamed to
 *  avoid conflicting with the new version 3 graphapp.h
 *  header file. If you are using this file to
 *  support compilation of an old GraphApp version 2
 *  program, you probably already have a copy of the
 *  old header file somewhere. You might want to rename
 *  graphapp.h to ga2.h in your program's source code,
 *  to avoid confusion. This file does not use the
 *  ga2.h header file, it solely relies on app.h.
 *
 *  Some things won't be exactly the same, for example,
 *  version 3 uses UTF-8 encoded strings, so ISO Latin 1
 *  strings may appear strange. Hopefully this will be
 *  a rare occurrence. ASCII programs should work fine.
 *  Many version 2 programs do not pay attention to the
 *  size of fonts; often programmers design the interface
 *  until it "looks about right" and then stop there.
 *  The version 3 portable fonts are used by default, so
 *  font sizes might be a little different.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <app.h>

/*
 *  Definitions needed inside this file.
 */

typedef int cursor;
enum {
	MENU_BAR_HEIGHT = 24
};

typedef struct drawstruct
{
	Control *     dest;
	unsigned long hue;
	int           mode;
	Point         p;
	int           linewidth;
	Font *        fnt;
	cursor        crsr;
} drawstate;

typedef void (*voidfn)(void);
typedef void (*timerfn)(void *data);
typedef void (*actionfn)(Control *c);
typedef void (*drawfn)(Control *c, Rect r);
typedef void (*mousefn)(Control *c, int buttons, Point xy);
typedef void (*intfn)(Control *c, int argument);
typedef void (*keyfn)(Control *c, int key);
typedef void (*menufn)(MenuItem *m);
typedef void (*scrollfn)(Control *c, int position);

#define REDEFINE_FUNC_NAMES
#define addpt      GA_addpt
#define subpt      GA_subpt
#define equalpt    GA_equalpt
#define newmenu    GA_newmenu
#define newcontrol GA_newcontrol
#define newwindow  GA_newwindow
#define gettext    GA_gettext
#define settext    GA_settext


/*
 *  Some global variables are required for complete GraphApp 2 support,
 *  since version 3 is more generalised.
 */

static App *          app               = NULL;
static Graphics *     gc                = NULL;
static Control *      current_dest      = NULL;
static Window *       current_win       = NULL;
static MenuBar *      current_menu_bar  = NULL;
static Menu *         current_menu      = NULL;
static Point          current_point     = {0,0};
static unsigned long  current_rgb       = 0UL;
static unsigned int   current_msec      = 0UL;
static Timer *        current_timer     = NULL;

/*
 *  Internal functions.
 */

/*
 *  Associations are a way of storing extra pointers with
 *  controls, windows and other objects. There isn't space
 *  inside the objects themselves to store the pointer,
 *  so we store them in a linked list, which uses the
 *  object's pointer as a lookup key.
 */

typedef struct Association  Association;

struct Association {
  Association * next;
  Association * prev;
  void *        key;
  int           kind;
  void *        data;
};

enum {
	ASSOC_ALL = 0,
	ASSOC_HIT,
	ASSOC_REDRAW,
	ASSOC_RESIZE,
	ASSOC_DATA,
	ASSOC_CMAP,
	ASSOC_TIMER
};

static Association * assoc_list = NULL;

static void del_association(void *key, int kind)
{
	Association *a, *next;

	a = assoc_list;
	do {
		if (a == NULL)
			return;
		next = a->next;
		if ( (a->key == key)
		  && ((kind == ASSOC_ALL) || (a->kind == kind)) )
		{
			if (a == assoc_list)
				assoc_list = a->next;
			if (a->next == a->prev)
				assoc_list = next = NULL;
			a->next->prev = a->prev;
			a->prev->next = a->next;
			app_free(a);
		}
		a = next;
	} while (a != assoc_list);
}

static void add_association(void *key, int kind, void *data)
{
	Association *a;

	del_association(key, kind);

	a = app_alloc(sizeof(Association));
	a->key = key;
	a->kind = kind;
	a->data = data;

	if (assoc_list == NULL) {
		assoc_list = a;
		a->next = a->prev = a;
	}
	else {
		a->next = assoc_list;
		a->prev = a->next->prev;
		a->next->prev = a;
		a->prev->next = a;
	}
}

static void * get_association(void *key, int kind)
{
	Association *a;

	a = assoc_list;
	if (a == NULL)
		return NULL;
	do {
		if ((a->key == key) && (a->kind == kind))
			return a->data;
		a = a->next;
	} while (a != assoc_list);
	return NULL;
}

/*
 *  Some functions used to determine what kind of thing
 *  an object is. In GraphApp version 2, there was an
 *  integer as the first field in every object's data
 *  structure. This integer was guaranteed to be unique
 *  for each type of object. So switch statements could
 *  be used to determine quickly what kind of object
 *  a run-time-polymorphic function was examining.
 *  Version 3 favours compiler-based polymorphism instead,
 *  and has abandoned this run-time strategy. So we
 *  need these functions to look through the window list
 *  and check whether the object is a window, control,
 *  menu item, font, etc.
 *
 *  In some cases this is little better than a guess, but
 *  for windows, controls, menu bars, menus and menu items
 *  at least (the most common objects) it should work
 *  exactly, because we have a global App object we can
 *  examine, without interpreting the object pointer in
 *  any way. We look forward from the App's data structures
 *  for a match, rather than interpreting the object
 *  pointer and trying to look backwards. This avoids
 *  violating memory as much as possible.
 */
static int is_a_window(Window *w)
{
	int i;

	if (w == NULL)
		return 0;
	if (app == NULL)
		return 0;
	/* just look through the window list for a match */
	for (i=0; i < app->num_windows; i++)
		if (w == app->windows[i])
			return 1;
	return 0;
}

static int is_a_child_control(Control *c, Control *parent)
{
	int i;

	if (c == parent)
		return 1;
	/* look recursively through the child controls */
	for (i=0; i < parent->num_children; i++)
		if (is_a_child_control(c, parent->children[i]))
			return 1;
	return 0;
}

static int is_a_control(Control *c)
{
	int i, j;
	Window *w;

	if (c == NULL)
		return 0;
	if (app == NULL)
		return 0;
	for (i=0; i < app->num_windows; i++) {
		w = app->windows[i];
		/* look recursively through the window's controls */
		for (j=0; j < w->num_children; j++)
			if (is_a_child_control(c, w->children[j]))
				return 1;
	}
	return 0;
}

static int is_a_menu_bar(MenuBar *mb)
{
	int i;
	Window *w;

	if (mb == NULL)
		return 0;
	if (app == NULL)
		return 0;
	/* look through the window list for a matching menu bar */
	for (i=0; i < app->num_windows; i++) {
		w = app->windows[i];
		if (w->menubar == mb)
			return 1;
	}
	return 0;
}

static int is_a_child_menu(Menu *m, Menu *parent)
{
	int i;
	MenuItem *mi;

	if (m == parent)
		return 1;
	/* recursively look for submenus */
	for (i=0; i < parent->num_items; i++) {
		mi = parent->items[i];
		if (mi->submenu == NULL)
			continue;
		if (is_a_child_menu(m, mi->submenu))
			return 1;
	}
	return 0;
}

static int is_a_menu(Menu *m)
{
	int i, j;
	Window *w;
	Menu *toplevel;

	if (m == NULL)
		return 0;
	if (app == NULL)
		return 0;
	/* look through the window list for menu bars */
	/* then look through each menu bar for menus */
	for (i=0; i < app->num_windows; i++) {
		w = app->windows[i];
		if (w->menubar == NULL)
			continue;
		for (j=0; j < w->menubar->num_menus; j++) {
			toplevel = w->menubar->menus[j];
			if (is_a_child_menu(m, toplevel))
				return 1;
		}
	}
	return 0;
}

static int is_a_child_menu_item(MenuItem *mi, MenuItem *parent)
{
	int i;
	Menu *m;

	if (mi == parent)
		return 1;
	if (parent->submenu == NULL)
		return 0;
	/* recursively look for menu items inside submenus */
	m = parent->submenu;
	for (i=0; i < m->num_items; i++) {
		if (is_a_child_menu_item(mi, m->items[i]))
			return 1;
	}
	return 0;
}

static int is_a_menu_item(MenuItem *mi)
{
	int i, j, k;
	Window *w;
	MenuBar *mb;
	Menu *m;

	if (mi == NULL)
		return 0;
	if (app == NULL)
		return 0;
	/* look through the window list for menu bars */
	/* then look through each menu bar for menus */
	/* then look through each menu for menu items */
	for (i=0; i < app->num_windows; i++) {
		w = app->windows[i];
		if (w->menubar == NULL)
			continue;
		mb = w->menubar;
		for (j=0; j < mb->num_menus; j++) {
			m = mb->menus[j];
			for (k=0; k < m->num_items; k++) {
				if (is_a_child_menu_item(mi, m->items[k]))
					return 1;
			}
		}
	}
	return 0;
}

static int is_a_bitmap(Bitmap *b)
{
	if (b == NULL)
		return 0;
	if (is_a_window((Window *)b))
		return 0;
	if (is_a_control((Control *)b))
		return 0;
	if (is_a_menu_bar((MenuBar *)b))
		return 0;
	if (is_a_menu((Menu *)b))
		return 0;
	if (is_a_menu_item((MenuItem *)b))
		return 0;
	if (is_a_window(b->win))
		return 1;
	/* but what if the bitmap's window has been deleted? */
	return 0;
}

static int is_a_font(Font *f)
{
	if (f == NULL)
		return 0;
	if (is_a_window((Window *)f))
		return 0;
	if (is_a_control((Control *)f))
		return 0;
	if (is_a_menu_bar((MenuBar *)f))
		return 0;
	if (is_a_menu((Menu *)f))
		return 0;
	if (is_a_menu_item((MenuItem *)f))
		return 0;
	if (is_a_bitmap((Bitmap *)f))
		return 0;
	if (f->app == app)
		return 1;
	return 0;
}

static int is_a_image(Image *img)
{
	if (img == NULL)
		return 0;
	if (is_a_window((Window *)img))
		return 0;
	if (is_a_control((Control *)img))
		return 0;
	if (is_a_menu_bar((MenuBar *)img))
		return 0;
	if (is_a_menu((Menu *)img))
		return 0;
	if (is_a_menu_item((MenuItem *)img))
		return 0;
	if ((img->depth != 8) && (img->depth != 32))
		return 0;
	if (is_a_bitmap((Bitmap *)img))
		return 0;
	return 1;
}

/*
 *  A function which ensures there is a window, there is
 *  a current drawing destination, and there is a valid
 *  Graphics context for drawing to it.
 *  This function is called often within this file,
 *  to ensure the GraphApp version 2 global drawing state
 *  is valid.
 */
static void enable_drawing(void)
{
	if (current_dest == NULL) {
		if (current_win == NULL)
			current_win = app_new_window(app,
				rect(0,0,640,480),
				"Graphics", STANDARD_WINDOW);
		app_show_window(current_win);
		if (gc)
			app_del_graphics(gc);
		gc = app_get_window_graphics(current_win);
		current_dest = (Control *) current_win;
		return;
	}
	if (gc == NULL) {
		if (is_a_window((Window *) current_dest))
			gc = app_get_window_graphics((Window *)current_dest);
		else if (is_a_control(current_dest))
			gc = app_get_control_graphics(current_dest);
		else if (is_a_bitmap((Bitmap *) current_dest))
			gc = app_get_bitmap_graphics((Bitmap *)current_dest);
	}
}


/*
 *  Library supplied variables.
 */

Font *	SystemFont;	/* system font */
Font *	FixedFont;	/* fixed-width font */
Font *	Times;  	/* times roman font (serif) */
Font *	Helvetica;	/* helvetica font (sans serif) */
Font *	Courier;	/* courier font (fixed width) */

cursor	ArrowCursor;	/* normal arrow cursor */
cursor	BlankCursor;	/* invisible cursor */
cursor	WatchCursor;	/* wait for the computer */
cursor	CaretCursor;	/* insert text */
cursor	TextCursor;	/* insert text */
cursor	HandCursor;	/* hand pointer */

/*
 *  GraphApp version 2 functions.
 */

int initapp(int argc, char *argv[])
{
	app_debug_memory(1);
	app = app_new_app(argc, argv);
	if (app == NULL)
		return 0;

	SystemFont = app_find_default_font(app);
	FixedFont = SystemFont;
	Times = app_new_font(app, "Times", PLAIN, 16);
	Helvetica = app_new_font(app, "Helvetica", PLAIN, 16);
	Courier = FixedFont;

	BlankCursor = 0;
	ArrowCursor = 1;
	WatchCursor = 2;
	CaretCursor = 3;
	TextCursor  = 4;
	HandCursor  = 5;

	return 1;
}

void exitapp(void)
{
	if (app)
		app_del_app(app);
	exit(0);
}

void drawall(void)
{
	app_draw_all(app);
}

int peekevent(void)
{
	return app_peek_event(app);
}

int doevent(void)
{
	return app_do_event(app);
}

void mainloop(void)
{
	if (app == NULL)
		initapp(0, NULL);
	app_main_loop(app);
}

int execapp(char *cmd)
{
	return app_exec(app, cmd);
}

void beep(void)
{
	app_beep(app);
}

/*
 *  Point and rectangle arithmetic.
 */

Point newpoint(int x, int y)
{
	Point p;

	p.x = x;
	p.y = y;
	return p;
}

Rect newrect(int x, int y, int width, int height)
{
	Rect r;

	r.x = x;
	r.y = y;
	r.width = width;
	r.height = height;
	return r;
}

Rect rpt(Point min, Point max)
{
	Rect r;

	r.x = min.x;
	r.y  = min.y;
	r.width = max.x - min.x;
	r.height = max.y - min.y;
	return r;
}

Point topleft(Rect r)
{
	Point p;

	p.x = r.x;
	p.y = r.y;

	return p;
}

Point topright(Rect r)
{
	Point p;

	p.x = r.x + r.width - 1;
	p.y = r.y;

	return p;
}

Point bottomright(Rect r)
{
	Point p;

	p.x = r.x + r.width - 1;
	p.y = r.y + r.height - 1;

	return p;
}

Point bottomleft(Rect r)
{
	Point p;

	p.x = r.x;
	p.y = r.y + r.height - 1;

	return p;
}

Point addpt(Point p1, Point p2)
{
	p1.x += p2.x;
	p1.y += p2.y;
	return p1;
}

Point subpt(Point p1, Point p2)
{
	p1.x -= p2.x;
	p1.y -= p2.y;
	return p1;
}

Point midpt(Point p1, Point p2)
{
	Point p;

	p.x = (p1.x + p2.x)/2;
	p.y = (p1.y + p2.y)/2;
	return p;
}

Point mulpt(Point p, int i)
{
	p.x *= i;
	p.y *= i;
	return p;
}

Point divpt(Point p, int i)
{
	p.x /= i;
	p.y /= i;
	return p;
}

Rect rmove(Rect r, Point p)
{
	r.x = p.x;
	r.y  = p.y;
	return r;
}

Rect raddpt(Rect r, Point p)
{
	r.x += p.x;
	r.y += p.y;
	return r;
}

Rect rsubpt(Rect r, Point p)
{
	r.x -= p.x;
	r.y  -= p.y;
	return r;
}

Rect rmul(Rect r, int i)
{
	if (i != 1) {
		r.x *= i;
		r.y  *= i;
		r.width *= i;
		r.height *= i;
	}
	return r;
}

Rect rdiv(Rect r, int i)
{
	if (i != 1) {
		r.x /= i;
		r.y  /= i;
		r.width /= i;
		r.height /= i;
	}
	return r;
}

Rect growr(Rect r, int w, int h)
{
	r.x -= w;
	r.y  -= h;
	r.width += 2*w;
	r.height += 2*h;
	return r;
}

Rect insetr(Rect r, int i)
{
	r.x += i;
	r.y  += i;
	r.width -= 2*i;
	r.height -= 2*i;
	return r;
}

Rect rcenter(Rect r1, Rect r2) /* center r1 on r2 */
{
	Rect r;

	r.x = r2.x + (r2.width-r1.width)/2;
	r.y  = r2.y + (r2.height-r1.height)/2;
	r.width = r1.width;
	r.height = r1.height;

	return r;
}

int ptinr(Point p, Rect r)
{
	if ((p.x>=r.x) && (p.x<r.x+r.width) &&
			(p.y>=r.y) && (p.y<r.y+r.height))
		return 1;
	else
		return 0;
}

int rinr(Rect r1, Rect r2)
{
	if ((r1.x>=r2.x) && (r1.y>=r2.y) &&
		(r1.x+r1.width<=r2.x+r2.width) &&
		(r1.y+r1.height<=r2.y+r2.height))
		return 1;
	else
		return 0;
}

int rxr(Rect r1, Rect r2)
{
	if ((r1.x<r2.x+r2.width) &&
		(r2.x<r1.x+r1.width) &&
		(r1.y<r2.y+r2.height) &&
		(r2.y<r1.y+r1.height))
		return 1;
	else
		return 0;
}

int equalpt(Point p1, Point p2)
{
	if ((p1.x==p2.x) && (p1.y==p2.y))
		return 1;
	else
		return 0;
}

int equalr(Rect r1, Rect r2)
{
	if ((r1.x==r2.x) && (r1.width==r2.width) &&
		(r1.y==r2.y) && (r1.height==r2.height))
		return 1;
	else
		return 0;
}

Rect clipr(Rect r1, Rect r2)
{
	if (rxr(r1,r2) == 0)
		return rect(0,0,0,0); /* they don't overlap */

	if (r1.x < r2.x) {
		r1.width -= (r2.x - r1.x);
		r1.x = r2.x;
	}
	if (r1.y < r2.y) {
		r1.height -= (r2.y - r1.y);
		r1.y = r2.y;
	}
	if (r1.x + r1.width > r2.x + r2.width)
		r1.width = r2.x + r2.width - r1.x;
	if (r1.y + r1.height > r2.y + r2.height)
		r1.height = r2.y + r2.height - r1.y;
	return r1; /* they do overlap */
}

Rect rcanon(Rect r)
{
	if (r.width < 0) {
		r.x += r.width;
		r.width = -r.width;
	}
	if (r.height < 0) {
		r.y += r.height;
		r.height = -r.height;
	}
	return r;
}

/*
 *  Colour functions and constants.
 */

#define getalpha(col) (((col)>>24)&0x00FFUL)
#define getred(col)   (((col)>>16)&0x00FFUL)
#define getgreen(col) (((col)>>8)&0x00FFUL)
#define getblue(col)  ((col)&0x00FFUL)

static unsigned long colour_to_rgb(Colour col)
{
	return (  (((unsigned long)col.alpha)<<24)
		| (((unsigned long)col.red  )<<16)
		| (((unsigned long)col.green)<<8 )
		|  ((unsigned long)col.blue )     );
}

void setrgb(unsigned long c)
{
	app_set_rgb(gc, argb(getalpha(c), getred(c),
				getgreen(c), getblue(c)));
}

/*
 *  Context functions for bitmaps, windows, controls.
 */

void addto(Window *w)
{
	current_win = w;
}

void drawto(Control *c)
{
	Window *w = (Window *) c;
	Bitmap *b = (Bitmap *) c;

	if (is_a_window(w)) {
		current_win = w;
		current_dest = c;
		if (gc)
			app_del_graphics(gc);
		gc = app_get_window_graphics(w);
		app_set_font(gc, app_find_default_font(app));
	}
	else if (is_a_control(c)) {
		current_dest = c;
		if (gc)
			app_del_graphics(gc);
		gc = app_get_control_graphics(c);
		app_set_font(gc, app_find_default_font(app));
	}
	else if (is_a_bitmap(b)) {
		current_dest = c;
		if (gc)
			app_del_graphics(gc);
		gc = app_get_bitmap_graphics(b);
		app_set_font(gc, app_find_default_font(app));
	}
}

void setlinewidth(int width)
{
	app_set_line_width(gc, width);
}

/*
 *  Transfer modes for drawing operations, S=source, D=destination.
 *  The modes are arranged so that, for example, (~D)|S == notDorS.
 */

void setdrawmode(int mode) /* incomplete */
{
	Colour c;

	switch (mode) {
	 case 0x00: /* Zeros */
		app_set_rgb(gc, BLACK);
		app_set_paint_mode(gc);
		break;
	 case 0x01: /* DnorS */
		break;
	 case 0x02: /* DandnotS */
		break;
	 case 0x03: /* notS */
		c = gc->colour;
		app_set_rgb(gc, rgb(255-c.red, 255-c.green, 255-c.blue));
		app_set_paint_mode(gc);
		break;
	 case 0x04: /* notDandS */
		break;
	 case 0x05: /* notD */
		app_set_rgb(gc, WHITE);
		app_set_xor_mode(gc, WHITE);
		break;
	 case 0x06: /* DxorS */
		app_set_xor_mode(gc, WHITE);
		break;
	 case 0x07: /* DnandS */
		break;
	 case 0x08: /* DandS */
		break;
	 case 0x09: /* DxnorS */
		app_set_xor_mode(gc, BLACK);
		break;
	 case 0x0A: /* D */
		app_set_rgb(gc, CLEAR);
		break;
	 case 0x0B: /* DornotS */
		break;
	 case 0x0C: /* S */
		app_set_paint_mode(gc);
		break;
	 case 0x0D: /* notDorS */
		break;
	 case 0x0E: /* DorS */
		break;
	 case 0x0F: /* Ones */
		app_set_rgb(gc, WHITE);
		app_set_paint_mode(gc);
		break;
	 default: break;
	}
}

/*
 *  Drawing functions.
 */

void bitblt(Control *dst, Control *src, Point dp, Rect sr,
		int mode) /* mode is currently ignored (use S) */
{
	Graphics *g1, *g2;

	g1 = app_get_control_graphics(dst);
	g2 = app_get_control_graphics(src);

	app_copy_rect(g1, dp, g2, sr);

	app_del_graphics(g2);
	app_del_graphics(g1);
}

void scrollrect(Point dp, Rect sr)
{
	app_copy_rect(gc, dp, gc, sr);
}

void copyrect(Control *src, Point dp, Rect sr)
{
	Graphics *g2;

	g2 = app_get_control_graphics(src);

	app_copy_rect(gc, dp, g2, sr);

	app_del_graphics(g2);
}

void texturerect(Control *src, Rect r)
{
	long x, y, sw, sh, sdx, sdy;
	long right, bottom;
	Rect sr;

	enable_drawing();

	sr = app_get_control_area(src);
	sw = r.width;
	sh = r.height;
	right = r.x+r.width;
	bottom = r.y+r.height;

	for (y=r.y; y<bottom; y+=sh)
		for (x=r.x; x<right; x+=sw) {

			/* reduce size of source rectangle for clipping */
			if (x+sw > right)	sdx = right - x;
			else			sdx = sw;
			if (y+sh > bottom)	sdy = bottom - y;
			else			sdy = sh;

			r = rect(sr.x, sr.y, sdx, sdy);
			copyrect(src, pt(x,y), r);
		}
}

void invertrect(Rect r)
{
	enable_drawing();

	app_set_rgb(gc, WHITE);
	app_set_xor_mode(gc, WHITE);
	app_fill_rect(gc, r);
	app_set_paint_mode(gc);
}

unsigned long getpixel(Point p)
{
	return 0x00FFFFFF; /* cheat for now, return white */
}

void setpixel(Point p, unsigned long c)
{
	enable_drawing();

	setrgb(c);
	app_draw_point(gc, p);
}

/*
 *  Drawing using the current colour.
 */

void moveto(Point p)
{
	current_point = p;
}

void lineto(Point p)
{
	app_draw_line(gc, current_point, p);
	current_point = p;
}

void drawpoint(Point p)
{
	app_draw_point(gc, p);
}

void drawline(Point p1, Point p2)
{
	app_draw_line(gc, p1, p2);
}

void drawrect(Rect r)
{
	app_draw_rect(gc, r);
}

void fillrect(Rect r)
{
	app_fill_rect(gc, r);
}

void drawarc(Rect r, int start_angle, int end_angle)
{
	app_draw_arc(gc, r, start_angle, end_angle);
}

void fillarc(Rect r, int start_angle, int end_angle)
{
	app_fill_arc(gc, r, start_angle, end_angle);
}

void drawellipse(Rect r)
{
	app_draw_ellipse(gc, r);
}

void fillellipse(Rect r)
{
	app_fill_ellipse(gc, r);
}

void drawroundrect(Rect r)
{
	app_draw_round_rect(gc, r);
}

void fillroundrect(Rect r)
{
	app_fill_round_rect(gc, r);
}

void drawpolygon(Point *p, int n)
{
	app_draw_polygon(gc, p, n);
}

void fillpolygon(Point *p, int n)
{
	app_fill_polygon(gc, p, n);
}

/*
 *  Drawing text, selecting fonts.
 */

Font * newfont(char *name, int style, int size)
{
	return app_new_font(app, name, style, size);
}

void setfont(Font *f)
{
	app_set_font(gc, f);
}

int fontwidth(Font *f)
{
	return app_font_width(f, "W", 1); /* guess */
}

int fontheight(Font *f)
{
	return app_font_height(f);
}

int fontascent(Font *f)
{
	return app_font_height(f) * 3/4; /* guess */
}

int fontdescent(Font *f)
{
	return app_font_height(f) * 1/4; /* guess */
}

int strwidth(Font *f, char *s)
{
	return app_font_width(f, s, strlen(s));
}

Point strsize(Font *f, char *s)
{
	Point p;

	p.x = app_font_width(f, s, strlen(s));
	p.y = app_font_height(f);

	return p;
}

Rect strrect(Font *f, char *s)
{
	Rect r;

	r.x = 0;
	r.y = 0;
	r.width = app_font_width(f, s, strlen(s));
	r.height = app_font_height(f);

	return r;
}

int drawstr(Point p, char *s)
{
	app_draw_utf8(gc, p, s, strlen(s));
	return strwidth(gc->font, s);
}

int textheight(int width, char *s)
{
	return app_text_width(gc->font, width, s, strlen(s));
}

/*
 *  Text alignments.
 */

#define AlignTop        0x0000
#define AlignBottom     0x0100
#define VJustify        0x0200
#define VCenter         0x0400
#define VCentre         0x0400
#define AlignLeft       0x0000
#define AlignRight      0x1000
#define Justify	        0x2000
#define Center	        0x4000
#define Centre          0x4000
#define AlignCenter     0x4000
#define AlignCentre     0x4000
#define Underline       0x0800

static int alignment_to_align(int alignment)
{
	int align = 0;

	if (alignment & AlignBottom)
		align |= VALIGN_BOTTOM;
	if (alignment & VJustify)
		align |= VALIGN_JUSTIFY;
	if (alignment & VCenter)
		align |= VALIGN_CENTER;
	if (alignment & AlignRight)
		align |= ALIGN_RIGHT;
	if (alignment & Justify)
		align |= ALIGN_JUSTIFY;
	if (alignment & AlignCenter)
		align |= ALIGN_CENTER;

	return align;
}

char *drawtext(Rect r, int alignment, char *s)
{
	int align = alignment_to_align(alignment);

	return app_draw_text(gc, r, align, s, strlen(s));
}

int gprintf(char *fmt, ...)
{
	va_list ap;
	char s[256];
	int len, result;

	va_start(ap, fmt);
	result = vsprintf(s, fmt, ap);
	len = strlen(s);
	app_draw_utf8(gc, current_point, s, len);
	if ((len > 0) && (s[len-1] == '\n'))
		current_point.y += fontheight(gc->font);
	else
		current_point.x += fontwidth(gc->font) * len;
	va_end(ap);
	return result;
}

/*
 *  Find the current state of drawing.
 */

Control *currentdrawing(void)
{
	return current_dest;
}

unsigned long currentrgb(void)
{
	return current_rgb;
}

int currentmode(void)
{
	return 0x0C; /* S - a guess */
}

Point currentpoint(void)
{
	return current_point;
}

int currentlinewidth(void)
{
	return 1; /* guess */
}

Font *currentfont(void)
{
	if (gc)
		return gc->font;
	else if (app)
		return app_find_default_font(app);
	else
		return NULL;
}

cursor currentcursor(void)
{
	return ArrowCursor;
}

/*
 *  Find current keyboard state.
 */

#define AltKey  	0x0001
#define CmdKey  	0x0001
#define CtrlKey		0x0002
#define OptionKey	0x0002
#define ShiftKey	0x0004

int getkeystate(void)
{
	return 0;
}

/*
 *  Bitmaps.
 */

Bitmap * newbitmap(int width, int height, int depth)
{
	Bitmap *b;

	enable_drawing();

	b = app_new_bitmap(current_win, width, height);
	drawto((Control *)b);
	return b;
}

Bitmap * loadbitmap(char *name)
{
	Image *img;
	Bitmap *bmap;

	enable_drawing();

	img = app_read_image(name, 8);
	if (img == NULL)
		app_read_image(name, 32);
	if (img == NULL)
		return NULL;

	bmap = app_image_to_bitmap(current_win, img);
	app_del_image(img);
	return bmap;
}

Bitmap * imagetobitmap(Image *img)
{
	enable_drawing();

	return app_image_to_bitmap(current_win, img);
}

Bitmap * createbitmap(int width, int height, int depth, byte *data)
{
	return NULL; /* cheat because this function is deprecated */
}

void setbitmapdata(Bitmap *b, byte data[])
{
	return; /* cheat because this function is deprecated */
}

void getbitmapdata(Bitmap *b, byte data[])
{
	return; /* cheat because this function is deprecated */
}

/*
 *  Images.
 */

Image * newimage(int width, int height, int depth)
{
	return app_new_image(width, height, depth);
}

Image * copyimage(Image *img)
{
	return app_copy_image(img);
}

void delimage(Image *img)
{
	app_del_image(img);
}

void setpixels(Image *img, byte pixels[])
{
	int i, w, h;
	unsigned long c;

	i = 0;
	if (img->depth == 8)
		for (h=0; h < img->height; h++)
			for (w=0; w < img->width; i++)
				img->data8[h][w] = pixels[i++];
	else
		for (h=0; h < img->height; h++)
			for (w=0; w < img->width; i++) {
				c = 0;
				c |= pixels[i++];
				c <<= 8;
				c |= pixels[i++];
				c <<= 8;
				c |= pixels[i++];
				c <<= 8;
				c |= pixels[i++];
				img->data8[h][w] = c;
			}
}

byte * getpixels(Image *img) /* convert to a linear memory model */
{
	int i, w, h;
	byte *data;

	data = get_association(img, ASSOC_DATA);
	if (data)
		app_free(data);
	data = app_alloc(img->width * img->height * (img->depth/8));
	if (data == NULL)
		return NULL;
	add_association(img, ASSOC_DATA, data);

	if (img->depth == 8) {
		i = 0;
		for (h=0; h < img->height; h++)
			for (w=0; w < img->width; w++)
				data[i++] = img->data8[h][w];
	}
	else if (img->depth == 32) {
		i = 0;
		for (h=0; h < img->height; h++)
			for (w=0; w < img->width; w++) {
				data[i++] = img->data32[h][w].alpha;
				data[i++] = img->data32[h][w].red;
				data[i++] = img->data32[h][w].green;
				data[i++] = img->data32[h][w].blue;
			}
	}
	return data;
}

void setpalette(Image *img, int length, unsigned long cmap[])
{
	Colour pal[256]; /* assume length <= 256 */
	int i;

	for (i=0; i < length; i++) {
		pal[i].alpha = getalpha(cmap[i]);
		pal[i].red   = getred(cmap[i]);
		pal[i].green = getgreen(cmap[i]);
		pal[i].blue  = getblue(cmap[i]);
	}

	app_set_image_cmap(img, length, pal);
}

unsigned long *	getpalette(Image *img)
{
	int i;
	unsigned long *data;
	Colour col;

	data = get_association(img, ASSOC_CMAP);
	if (data)
		app_free(data);
	data = app_alloc(img->cmap_size * sizeof(unsigned long));
	if (data == NULL)
		return NULL;
	add_association(img, ASSOC_CMAP, data);

	for (i=0; i < img->cmap_size; i++) {
		col = img->cmap[i];
		data[i] = colour_to_rgb(col);
	}
	return data;
}

int getpalettesize(Image *img)
{
	return img->cmap_size;
}

Image * scaleimage(Image * src, Rect dr, Rect sr)
{
	return app_scale_image(src, dr, sr);
}

Image * convert32to8(Image *img)
{
	return app_image_convert_32_to_8(img);
}

Image * convert8to32(Image *img)
{
	return app_image_convert_8_to_32(img);
}

void sortpalette(Image *img)
{
	app_image_sort_palette(img);
}

Image * loadimage(char *filename)
{
	Image *img;

	img = app_read_image(filename, 8);
	if (img == NULL)
		img = app_read_image(filename, 32);
	return img;
}

void saveimage(Image *img, char *filename)
{
	app_write_image(img, filename);
}

void drawimage(Image *img, Rect dr, Rect sr)
{
	enable_drawing();

	app_draw_image(gc, dr, img, sr);
}

void drawmonochrome(Image *img, Rect dr, Rect sr)
{
	enable_drawing();

	app_draw_image_monochrome(gc, dr, img, sr);
}

void drawgreyscale(Image *img, Rect dr, Rect sr)
{
	enable_drawing();

	app_draw_image_greyscale(gc, dr, img, sr);
}

void drawdarker(Image *img, Rect dr, Rect sr)
{
	enable_drawing();

	app_draw_image_darker(gc, dr, img, sr);
}

void drawbrighter(Image *img, Rect dr, Rect sr)
{
	enable_drawing();

	app_draw_image_brighter(gc, dr, img, sr);
}


/*
 *  Windows.
 */

Window *newwindow(char *name, Rect r, long flags)
{
	Window *w;

	if (app == NULL)
		initapp(0, NULL);

	r.height += MENU_BAR_HEIGHT;
	w = app_new_window(app, r, name?name:"", flags);
	drawto((Control *)w);
	return w;
}

/*
 *  Functions which work for bitmaps, windows and controls.
 */

int objdepth(Window *w)
{
	Image *img = (Image *) w;

	if (is_a_window(w))
		return 8; /* guess */
	else if ((img->depth == 32) && (img->cmap_size == 0))
		return 32;
	else if ((img->depth == 8) && (img->cmap_size > 0)
		 && (img->cmap_size <= 256))
		return 8;
	else
		return 8; /* guess */
}

Rect objrect(void *obj)
{
	Rect r;

	if (is_a_window(obj)) {
		r = app_get_window_area(obj);
		r.y += MENU_BAR_HEIGHT; /* account for menu bar */
		r.height -= MENU_BAR_HEIGHT;
	}
	else if (is_a_control(obj))
		r = app_get_control_area(obj);
	else if (is_a_bitmap(obj))
		r = app_get_bitmap_area(obj);
	else if (is_a_font(obj))
		r = rect(0,0,fontheight(obj), fontwidth(obj));
	else if (is_a_image(obj))
		r = app_get_image_area(obj);
	else
		r = rect(0,0,0,0);
	return r;
}

int objwidth(void *obj)
{
	return objrect(obj).width;
}

int objheight(void *obj)
{
	return objrect(obj).height;
}

void delobj(void *obj)
{
	del_association(obj, ASSOC_ALL);
	if (is_a_window(obj))
		return app_del_window(obj);
	else if (is_a_control(obj))
		return app_del_control(obj);
	else if (is_a_menu_bar(obj))
		return app_del_menu_bar(obj);
	else if (is_a_menu(obj))
		return app_del_menu(obj);
	else if (is_a_menu_item(obj))
		return app_del_menu_item(obj);
	else if (is_a_bitmap(obj))
		return app_del_bitmap(obj);
	else if (is_a_image(obj))
		return app_del_image(obj);
}

/*
 *  Setting window and control callback functions.
 */

void setaction(Control *c, actionfn fn)
{
	if (is_a_control(c))
		app_on_control_action(c, fn);
}

static void private_hit_action(Control *c)
{
	intfn hit = get_association(c, ASSOC_HIT);

	if (hit != NULL)
		hit(c, app_get_control_value(c));
}

void sethit(Control *c, intfn fn)
{
	add_association(c, ASSOC_HIT, fn);
	if (is_a_control(c))
		app_on_control_action(c, private_hit_action);
}

void setdel(Control *c, actionfn fn)
{
	if (is_a_control(c))
		app_on_control_deletion(c, fn);
}

void setclose(Window *w, WindowFunc fn)
{
	if (is_a_window(w))
		app_on_window_close(w, fn);
}

static void private_redraw_window(Window *w, Graphics *g)
{
	Graphics *old;
	drawfn fn = get_association(w, ASSOC_REDRAW);

	old = gc;
	gc = g;
	if (fn != NULL)
		fn((Control *)w, app_get_window_area(w));
	gc = old;
}

static void private_redraw_control(Control *c, Graphics *g)
{
	Graphics *old;
	drawfn fn = get_association(c, ASSOC_REDRAW);

	old = gc;
	gc = g;
	if (fn != NULL)
		fn(c, app_get_control_area(c));
	gc = old;
}

void setredraw(Control *c, drawfn fn)
{
	Window *w = (Window *) c;

	add_association(c, ASSOC_REDRAW, fn);
	if (is_a_window(w))
		app_on_window_redraw(w, private_redraw_window);
	else if (is_a_control(c))
		app_on_control_redraw(c, private_redraw_control);
}

static void private_resize_window(Window *w)
{
	Graphics *old;
	drawfn fn = get_association(w, ASSOC_RESIZE);

	old = gc;
	gc = app_get_window_graphics(w);
	if (fn != NULL)
		fn((Control *)w, app_get_window_area(w));
	app_del_graphics(gc);
	gc = old;
}

static void private_resize_control(Control *c)
{
	Graphics *old;
	drawfn fn = get_association(c, ASSOC_RESIZE);

	old = gc;
	gc = app_get_control_graphics(c);
	if (fn != NULL)
		fn(c, app_get_control_area(c));
	app_del_graphics(gc);
	gc = old;
}

void setresize(Control *c, drawfn fn)
{
	Window *w = (Window *) c;

	add_association(c, ASSOC_RESIZE, fn);
	if (is_a_window(w))
		app_on_window_resize(w, private_resize_window);
	else if (is_a_control(c))
		app_on_control_resize(c, private_resize_control);
}

void setkeydown(Control *c, keyfn fn)
{
	/* assuming sizeof(int) == sizeof(unsigned long) ... */

	Window *w = (Window *) c;

	if (is_a_window(w))
		app_on_window_key_down(w, (WindowKeyFunc) fn);
	else if (is_a_control(c))
		app_on_control_key_down(c, (KeyFunc) fn);
}

void setkeyaction(Control *c, keyfn fn)
{
	/* assuming sizeof(int) == sizeof(unsigned long) ... */

	Window *w = (Window *) c;

	if (is_a_window(w))
		app_on_window_key_action(w, (WindowKeyFunc) fn);
	else if (is_a_control(c))
		app_on_control_key_action(c, (KeyFunc) fn);
}

void setmousedown(Control *c, mousefn fn)
{
	Window *w = (Window *) c;

	if (is_a_window(w))
		app_on_window_mouse_down(w, (WindowMouseFunc) fn);
	else if (is_a_control(c))
		app_on_control_mouse_down(c, fn);
}

void setmousedrag(Control *c, mousefn fn)
{
	Window *w = (Window *) c;

	if (is_a_window(w))
		app_on_window_mouse_drag(w, (WindowMouseFunc) fn);
	else if (is_a_control(c))
		app_on_control_mouse_drag(c, fn);
}

void setmouseup(Control *c, mousefn fn)
{
	Window *w = (Window *) c;

	if (is_a_window(w))
		app_on_window_mouse_up(w, (WindowMouseFunc) fn);
	else if (is_a_control(c))
		app_on_control_mouse_up(c, fn);
}

void setmousemove(Control *c, mousefn fn)
{
	Window *w = (Window *) c;

	if (is_a_window(w))
		app_on_window_mouse_move(w, (WindowMouseFunc) fn);
	else if (is_a_control(c))
		app_on_control_mouse_move(c, fn);
}

void setmouserepeat(Control *c, mousefn fn)
{
	/* difficult to do */
}

/*
 *  Using windows and controls.
 */

void clear(Control *c)
{
	Graphics *g;
	Window *w = (Window *) c;
	Bitmap *b = (Bitmap *) c;

	if (is_a_window(w)) {
		g = app_get_window_graphics(w);
		app_set_rgb(g, w->bg);
		app_fill_rect(g, app_get_window_area(w));
		app_del_graphics(g);
	}
	else if (is_a_control(c)) {
		g = app_get_control_graphics(c);
		app_set_rgb(g, c->bg);
		app_fill_rect(g, app_get_control_area(c));
		app_del_graphics(g);
	}
	else if (is_a_bitmap(b)) {
		g = app_get_bitmap_graphics(b);
		app_set_rgb(g, WHITE);
		app_fill_rect(g, app_get_bitmap_area(b));
		app_del_graphics(g);
	}
}

void draw(Control *c)
{
	Window *w = (Window *) c;

	if (is_a_window(w))
		app_draw_window(w);
	else if (is_a_control(c))
		app_draw_control(c);
}

void redraw(Control *c)
{
	Window *w = (Window *) c;

	if (is_a_window(w))
		app_redraw_window(w);
	else if (is_a_control(c))
		app_redraw_control(c);
}

static Rect adjust_rect(Rect r)
{
	r.y += MENU_BAR_HEIGHT; /* account for menu bar */
	return r;
}

void resize(Control *c, Rect r)
{
	Window *w = (Window *) c;

	if (is_a_window(w))
		app_size_window(w, r);
	else if (is_a_control(c))
		app_set_control_area(c, adjust_rect(r));
}

void show(Control *c)
{
	Window *w = (Window *) c;

	if (is_a_window(w))
		app_show_window(w);
	else if (is_a_control(c))
		app_show_control(c);
}

void hide(Control *c)
{
	Window *w = (Window *) c;

	if (is_a_window(w))
		app_hide_window(w);
	else if (is_a_control(c))
		app_hide_control(c);
}

int isvisible(Control *c)
{
	Window *w = (Window *) c;

	if (is_a_window(w))
		return w->state & VISIBLE;
	else if (is_a_control(c))
		return app_is_visible(c);
	return 0;
}

void enable(Control *c)
{
	Window *w = (Window *) c;
	MenuItem *mi = (MenuItem *) c;

	if (is_a_window(w))
		w->state |= ENABLED;
	else if (is_a_control(c))
		app_enable(c);
	else if (is_a_menu_item(mi))
		app_enable_menu_item(mi);
}

void disable(Control *c)
{
	Window *w = (Window *) c;
	MenuItem *mi = (MenuItem *) c;

	if (is_a_window(w))
		w->state &= ~ENABLED;
	else if (is_a_control(c))
		app_disable(c);
	else if (is_a_menu_item(mi))
		app_disable_menu_item(mi);
}

int isenabled(Control *c)
{
	Window *w = (Window *) c;
	MenuItem *mi = (MenuItem *) c;

	if (is_a_window(w))
		return (w->state & ENABLED) ? 1 : 0;
	else if (is_a_control(c))
		return app_is_enabled(c);
	else if (is_a_menu_item(mi))
		return app_menu_item_is_enabled(mi);
	return 1;
}

void check(Control *c)
{
	MenuItem *mi = (MenuItem *) c;

	if (is_a_control(c))
		app_check(c);
	else if (is_a_menu_item(mi))
		app_check_menu_item(mi);
}

void uncheck(Control *c)
{
	MenuItem *mi = (MenuItem *) c;

	if (is_a_control(c))
		app_uncheck(c);
	else if (is_a_menu_item(mi))
		app_uncheck_menu_item(mi);
}

int ischecked(Control *c)
{
	MenuItem *mi = (MenuItem *) c;

	if (is_a_control(c))
		return app_is_checked(c);
	else if (is_a_menu_item(mi))
		return app_menu_item_is_checked(mi);
	return 0;
}

void highlight(Control *c)
{
	if (is_a_control(c))
		app_highlight(c);
}

void unhighlight(Control *c)
{
	if (is_a_control(c))
		app_unhighlight(c);
}

int ishighlighted(Control *c)
{
	if (is_a_control(c))
		return app_is_highlighted(c);
	return 0;
}

void flashcontrol(Control *c)
{
	if (is_a_control(c))
		return app_flash_control(c);
}

void activatecontrol(Control *c)
{
	if (is_a_control(c))
		return app_activate_control(c);
}

/*
 *  Changing the state of a control.
 */

void settext(Control *c, char *text)
{
	Window *w = (Window *) c;
	Menu *m = (Menu *) c;
	MenuItem *mi = (MenuItem *) c;

	if (is_a_window(w))
		app_set_window_title(w, text);
	else if (is_a_control(c))
		app_set_control_text(c, text);
	else if (is_a_menu(m)) {
		char *old = m->text;
		m->text = app_copy_string(text?text:"");
		app_del_string(old);
		app_redraw_control(m->parent->ctrl);
	}
	else if (is_a_menu_item(mi)) {
		char *old = mi->text;
		mi->text = app_copy_string(text?text:"");
		app_del_string(old);
	}
}

char * gettext(Control *c)
{
	Window *w = (Window *) c;
	Menu *m = (Menu *) c;
	MenuItem *mi = (MenuItem *) c;

	if (is_a_window(w))
		return app_get_window_title(w);
	else if (is_a_control(c))
		return app_get_control_text(c);
	else if (is_a_menu(m))
		return m->text;
	else if (is_a_menu_item(mi))
		return mi->text;
	return "";
}

void settextfont(Control *c, Font *f)
{
	Window *w = (Window *) c;
	MenuBar *mb = (MenuBar *) c;
	Menu *m = (Menu *) c;
	MenuItem *mi = (MenuItem *) c;

	if (is_a_window(w))
		return;
	else if (is_a_control(c))
		app_set_control_font(c, f);
	else if (is_a_menu_bar(mb))
		app_set_menu_bar_font(mb, f);
	else if (is_a_menu(m))
		app_set_menu_font(m, f);
	else if (is_a_menu_item(mi))
		app_set_menu_item_font(mi, f);
}

Font *gettextfont(Control *c)
{
	Window *w = (Window *) c;
	MenuBar *mb = (MenuBar *) c;
	Menu *m = (Menu *) c;
	MenuItem *mi = (MenuItem *) c;

	if (is_a_window(w))
		return app_find_default_font(app);
	else if (is_a_control(c))
		return app_get_control_font(c);
	else if (is_a_menu_bar(mb))
		return app_get_menu_bar_font(mb);
	else if (is_a_menu(m))
		return app_get_menu_font(m);
	else if (is_a_menu_item(mi))
		return app_get_menu_item_font(mi);
	return app_find_default_font(app);
}

void setforeground(Control *c, unsigned long fg)
{
	Window *w = (Window *) c;
	MenuBar *mb = (MenuBar *) c;
	Menu *m = (Menu *) c;
	MenuItem *mi = (MenuItem *) c;

	Colour col;

	col.alpha = getalpha(fg);
	col.red   = getred(fg);
	col.green = getgreen(fg);
	col.blue  = getblue(fg);

	if (is_a_window(w))
		return;
	else if (is_a_control(c))
		app_set_control_foreground(c, col);
	else if (is_a_menu_bar(mb))
		app_set_control_foreground(mb->ctrl, col);
	else if (is_a_menu(m))
		app_set_menu_foreground(m, col);
	else if (is_a_menu_item(mi))
		app_set_menu_item_foreground(mi, col);
}

unsigned long getforeground(Control *c)
{
	Colour col = BLACK;
	Window *w = (Window *) c;
	MenuBar *mb = (MenuBar *) c;
	Menu *m = (Menu *) c;
	MenuItem *mi = (MenuItem *) c;

	if (is_a_window(w))
		;
	else if (is_a_control(c))
		col = app_get_control_foreground(c);
	else if (is_a_menu_bar(mb))
		col = app_get_control_foreground(mb->ctrl);
	else if (is_a_menu(m))
		col = app_get_menu_foreground(m);
	else if (is_a_menu_item(mi))
		col = app_get_menu_item_foreground(mi);

	return colour_to_rgb(col);
}

void setbackground(Control *c, unsigned long bg)
{
	Window *w = (Window *) c;
	MenuBar *mb = (MenuBar *) c;

	Colour col;

	col.alpha = getalpha(bg);
	col.red   = getred(bg);
	col.green = getgreen(bg);
	col.blue  = getblue(bg);

	if (is_a_window(w))
		return;
	else if (is_a_control(c))
		app_set_control_background(c, col);
	else if (is_a_menu_bar(mb))
		app_set_control_background(mb->ctrl, col);
}

unsigned long getbackground(Control *c)
{
	Colour col = WHITE;
	Window *w = (Window *) c;
	MenuBar *mb = (MenuBar *) c;

	if (is_a_window(w))
		;
	else if (is_a_control(c))
		col = app_get_control_background(c);
	else if (is_a_menu_bar(mb))
		col = app_get_control_background(mb->ctrl);

	return colour_to_rgb(col);
}

void setvalue(Control *c, int value)
{
	Window *w = (Window *) c;
	MenuBar *mb = (MenuBar *) c;
	MenuItem *mi = (MenuItem *) c;

	if (is_a_window(w))
		return;
	else if (is_a_control(c))
		app_set_control_value(c, value);
	else if (is_a_menu_bar(mb))
		app_set_control_value(mb->ctrl, value);
	else if (is_a_menu_item(mi))
		app_set_menu_item_value(mi, value);
}

int getvalue(Control *c)
{
	Window *w = (Window *) c;
	MenuBar *mb = (MenuBar *) c;
	MenuItem *mi = (MenuItem *) c;

	if (is_a_window(w))
		return 0;
	else if (is_a_control(c))
		return app_get_control_value(c);
	else if (is_a_menu_bar(mb))
		return app_get_control_value(mb->ctrl);
	else if (is_a_menu_item(mi))
		return app_get_menu_item_value(mi);
	return 0;
}

void setdata(Control *c, void *data)
{
	Window *w = (Window *) c;
	MenuBar *mb = (MenuBar *) c;
	Menu *m = (Menu *) c;
	MenuItem *mi = (MenuItem *) c;

	if (is_a_window(w))
		app_set_window_data(w, data);
	else if (is_a_control(c))
		app_set_control_data(c, data);
	else if (is_a_menu_bar(mb))
		app_set_control_data(mb->ctrl, data);
	else if (is_a_menu(m))
		add_association(m, ASSOC_DATA, data);
	else if (is_a_menu_item(mi))
		mi->data = data;
	else
		add_association(c, ASSOC_DATA, data);
}

void * getdata(Control *c)
{
	Window *w = (Window *) c;
	MenuBar *mb = (MenuBar *) c;
	Menu *m = (Menu *) c;
	MenuItem *mi = (MenuItem *) c;

	if (is_a_window(w))
		return app_get_window_data(w);
	else if (is_a_control(c))
		return app_get_control_data(c);
	else if (is_a_menu_bar(mb))
		return app_get_control_data(mb->ctrl);
	else if (is_a_menu(m))
		return get_association(m, ASSOC_DATA);
	else if (is_a_menu_item(mi))
		return mi->data;
	else
		return get_association(c, ASSOC_DATA);
}

Window *parentwindow(Control *c)
{
	Window *w = (Window *) c;
	MenuBar *mb = (MenuBar *) c;
	Menu *m = (Menu *) c;
	MenuItem *mi = (MenuItem *) c;

	if (is_a_window(w))
		return w;
	else if (is_a_control(c))
		return app_parent_window(c);
	else if (is_a_menu_bar(mb))
		return app_parent_window(mb->ctrl);
	else if (is_a_menu(m))
		return app_parent_window(m->parent->ctrl);
	else if (is_a_menu_item(mi))
		return app_parent_window(mi->parent->parent->ctrl);
	return NULL;
}


/*
 *  Create buttons, scrollbars, controls etc on the current window.
 */

Control * newcontrol(char *text, Rect r)
{
	Control *c;

	enable_drawing();

	c = app_new_control(current_win, adjust_rect(r));
	app_set_control_text(c, text?text:"");
	drawto(c);
	return c;
}

Control * newdrawing(Rect r, drawfn fn)
{
	Control *c;

	enable_drawing();

	c = app_new_control(current_win, adjust_rect(r));
	setredraw(c, fn);
	drawto(c);
	return c;
}

static void private_draw_picture(Control *c, Graphics *g)
{
	Rect r;
	Image *img;

	img = app_get_control_image(c);
	r = app_get_control_area(c);
	app_set_rgb(g, app_get_control_background(c));
	app_fill_rect(g, r);
	if (img)
		app_draw_image(g, r, img, app_get_image_area(img));
}

Control * newpicture(Image *img, Rect r)
{
	Control *c;

	enable_drawing();

	c = app_new_control(current_win, adjust_rect(r));
	app_on_control_redraw(c, private_draw_picture);
	app_set_control_image(c, img);
	return c;
}

Control * newbutton(char *text, Rect r, actionfn fn)
{
	Control *c;

	enable_drawing();

	c = app_new_button(current_win, adjust_rect(r), text?text:"", fn);
	return c;
}

Control * newimagebutton(Image *img, Rect r, actionfn fn)
{
	Control *c;

	enable_drawing();

	c = app_new_image_button(current_win, adjust_rect(r), img, fn);
	return c;
}

void setimage(Control *c, Image *img)
{
	if (is_a_control(c))
		app_set_control_image(c, img);
}

Control * newcheckbox(char *text, Rect r, actionfn fn)
{
	Control *c;

	enable_drawing();

	c = app_new_check_box(current_win, adjust_rect(r), text?text:"", fn);
	return c;
}

Control * newimagecheckbox(Image *img, Rect r, actionfn fn)
{
	Control *c;

	enable_drawing();

	c = app_new_image_button(current_win, adjust_rect(r), img, fn);
	return c;
}

Control * newradiobutton(char *text, Rect r, actionfn fn)
{
	Control *c;

	enable_drawing();

	c = app_new_radio_button(current_win, adjust_rect(r), text?text:"", fn);
	return c;
}

Control * newradiogroup(void)
{
	enable_drawing();

	return app_new_radio_group(current_win);
}

Control * newscrollbar(Rect r, int max, int pagesize, scrollfn fn)
{
	Control *c;

	enable_drawing();

	c = app_new_scroll_bar(current_win, adjust_rect(r), max, pagesize,
			private_hit_action);
	add_association(c, ASSOC_HIT, fn);
	return c;
}

void changescrollbar(Control *c, int where, int max, int size)
{
	app_change_scroll_bar(c, where, max, size);
}

Control * newlabel(char *text, Rect r, int alignment)
{
	int align = alignment_to_align(alignment);

	enable_drawing();

	return app_new_label(current_win, adjust_rect(r), text?text:"", align);
}

Control * newfield(char *text, Rect r)
{
	enable_drawing();

	return app_new_field(current_win, adjust_rect(r), text?text:"");
}

Control * newpassword(char *text, Rect r)
{
	enable_drawing();

	return app_new_password_field(current_win, adjust_rect(r), text?text:"");
}

Control * newtextbox(char *text, Rect r)
{
	enable_drawing();

	return app_new_text_box(current_win, adjust_rect(r), text?text:"");
}

Control * newtextarea(char *text, Rect r)
{
	return newtextbox(text, r); /* cheat */
}

Control * newlistbox(char *list[], Rect r, scrollfn fn)
{
	Control *c;
	enable_drawing();

	c = app_new_list_box(current_win, adjust_rect(r), list, private_hit_action);
	add_association(c, ASSOC_HIT, fn);
	return c;
}

Control * newdroplist(char *list[], Rect r, scrollfn fn)
{
	Control *c;
	enable_drawing();

	c = app_new_drop_list(current_win, adjust_rect(r), list, private_hit_action);
	add_association(c, ASSOC_HIT, fn);
	return c;
}

Control * newdropfield(char *list[], Rect r, scrollfn fn)
{
	Control *c;
	enable_drawing();

	c = app_new_drop_field(current_win, adjust_rect(r), list);
	add_association(c, ASSOC_HIT, fn);
	app_on_control_action(c, private_hit_action);
	return c;
}

Control * newmultilist(char *list[], Rect r, scrollfn fn)
{
	return newlistbox(list, r, fn); /* cheat */
}

int isselected(Control *c, int index)
{
	if (is_a_control(c))
		return (c->value == index) ? 1 : 0;
	return 0;
}

void setlistitem(Control *c, int index)
{
	if (is_a_control(c)) /* cheat */
		app_set_list_box_item(c, index);
}

int getlistitem(Control *c)
{
	if (is_a_control(c)) /* cheat */
		return app_get_list_box_item(c);
	return 0;
}

void changelistbox(Control *c, char *new_list[])
{
	if (is_a_control(c)) /* cheat */
		app_change_list_box(c, new_list);
}

MenuBar * newmenubar(actionfn adjust_menus)
{
	enable_drawing();

	current_menu_bar = app_new_menu_bar(current_win);
	return current_menu_bar;
}

Menu * newsubmenu(Menu *parent, char *name)
{
	enable_drawing();

	if (current_menu_bar == NULL)
		newmenubar(NULL);

	current_menu = app_new_sub_menu(parent, name?name:"");
	return current_menu;
}

Menu * newmenu(char *name)
{
	enable_drawing();

	if (current_menu_bar == NULL)
		newmenubar(NULL);

	current_menu = app_new_menu(current_menu_bar, name?name:"");
	return current_menu;
}

MenuItem * newmenuitem(char *name, int key, menufn fn)
{
	enable_drawing();

	if (current_menu == NULL)
		newmenu("File");

	return app_new_menu_item(current_menu, name?name:"", key, fn);
}

/*
 *  Text editing functions.
 */

void cuttext(Control *c)
{
	app_cut_text(c);
}

void copytext(Control *c)
{
	app_copy_text(c);
}

void cleartext(Control *c)
{
	app_clear_text(c);
}

void pastetext(Control *c)
{
	app_paste_text(c);
}

void inserttext(Control *c, char *text)
{
	app_insert_text(c, text);
}

void selecttext(Control *c, long start, long end)
{
	app_select_text(c, start, end);
}

void textselection(Control *c, long *start, long *end)
{
	if (start)
		*start = 0; /* cheat */
	if (end)
		*end = 0; /* cheat */
}


/*
 *  Dialogs.
 */

void apperror(char *errstr)
{
	app_error(app, errstr);
}

void askok(char *info)
{
	app_ask_ok(app, "Information", info);
}

int askokcancel(char *question)
{
	int result = app_ask_ok_cancel(app, "Question", question);

	if (result == -1)
		return 0;
	else if (result == 0)
		return -1;
	else
		return 1;
}

int askyesno(char *question)
{
	int result = app_ask_yes_no(app, "Question", question);

	if (result == -1)
		return 0;
	else if (result == 0)
		return -1;
	else
		return 1;
}

int askyesnocancel(char *question)
{
	int result = app_ask_yes_no_cancel(app, "Question", question);

	if (result == -1)
		return 0;
	else if (result == 0)
		return -1;
	else
		return 1;
}

char * askstring(char *question, char *answer)
{
	return app_ask_string(app, "Question", question, answer);
}

char * askpassword(char *question, char *answer)
{
	return app_ask_string(app, "Password", question, answer);
}

char * askfilename(char *title, char *name)
{
	return app_ask_file_open(app, "Open File", "Open", name);
}

char * askfilesave(char *title, char *name)
{
	return app_ask_file_open(app, "Save File", "Save", name);
}

/*
 *  Time functions.
 */

int settimer(unsigned millisec)
{
	current_msec = millisec;
	if (millisec == 0) {
		if (current_timer) {
			app_del_timer(current_timer);
			current_timer = NULL;
		}
	}
	return 1;
}

static void private_timeout(Timer *t)
{
	timerfn fn = get_association(t, ASSOC_TIMER);

	if (fn != NULL)
		fn(t->data);
}

void settimerfn(timerfn timeout, void *data)
{
	if (current_timer)
		app_del_timer(current_timer);
	current_timer = app_new_timer(app, private_timeout, current_msec);
	add_association(current_timer, ASSOC_TIMER, timeout);
	current_timer->data = data;
}

int setmousetimer(unsigned millisec)
{
	return 0; /* cheat */
}

void delay(unsigned millisec)
{
	app_delay(app, millisec);
}

long currenttime(void)
{
	return app_current_time(app);
}

/*
 *  Cursors.
 */

cursor newcursor(Point hotspot, Image *img)
{
	return ArrowCursor; /* cheat */
}

cursor createcursor(Point offset, byte *white_mask, byte *black_shape)
{
	return ArrowCursor; /* cheat */
}

cursor loadcursor(char *name)
{
	return ArrowCursor; /* cheat */
}

void setcursor(cursor c)
{
	return; /* cheat */
}

/*
 *  Change the drawing state.
 */

drawstate copydrawstate(void)
{
	drawstate d;
	Colour col;

	col = gc->colour;

	d.dest = current_dest;
	d.hue = colour_to_rgb(col);
	d.mode = 0x0C; /* S */
	d.p = current_point;
	d.linewidth = 1;
	d.fnt = gc->font;
	d.crsr = ArrowCursor;
	return d;
}

void setdrawstate(drawstate d)
{
	drawto(d.dest);
	setrgb(d.hue);
	setdrawmode(d.mode);
	moveto(d.p);
	setlinewidth(d.linewidth);
	setfont(d.fnt);
	setcursor(d.crsr);
}

void restoredrawstate(drawstate d)
{
	setdrawstate(d);
}

void resetdrawstate(void)
{
	setrgb(0UL);
	setdrawmode(0x0C); /* S */
	moveto(pt(0,0));
	setlinewidth(1);
	setfont(app_find_default_font(app));
	setcursor(ArrowCursor);
}

/*
 *  Miscellany:
 */

char * new_string(char *s)
{
	return app_copy_string(s);
}

void del_string(char *s)
{
	app_del_string(s);
}

char * int_to_string(int i)
{
	char buf[80];

	sprintf(buf, "%d", i);
	return app_copy_string(buf);
}
