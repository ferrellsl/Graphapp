/*
 *  App programming interface (internal header file).
 *
 *  Copyright (c) L.Patrick
 *
 *  This header file never needs to be included by a user's
 *  program. It is only used internally while compiling the
 *  the App library. A user program need only include app.h
 *  and link with the app.lib or libapp.a library.
 */

#define APP_PRIVATE_DECLARATIONS

#undef USE_ALARM	//!!

  typedef struct AppExtra       AppExtra;
  typedef struct WindowExtra    WindowExtra;
  typedef struct BitmapExtra    BitmapExtra;
  typedef struct GraphicsExtra  GraphicsExtra;
  typedef struct SubfontExtra   SubfontExtra;
  typedef struct FontExtra      FontExtra;
  typedef struct FolderExtra    FolderExtra;
  typedef struct CursorExtra    CursorExtra;

#include "apputils.h"

#define  Cursor   X_Cursor
#define  Font     X_Font
#define  Region   X_Region
#define  Window   X_Window
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#undef   Cursor
#undef   Font
#undef   Region
#undef   Window
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

/*
 *  Private platform-specific data structures,
 *  usually linked to the 'extra' fields in public data structures.
 */

typedef struct CLUT	/* Colour Look-Up Table */
{
	int             refcount;  /* this structure deleted when zero */
	int             private;   /* true = read/write, false = shared */
	int             size;      /* count of how many cells in cmap */
	int             full;      /* true if it seems to be full now */
	Display *       disp;      /* display of this cmap */
	Colormap        cmap;      /* for a private cmap */
	XColor *        table;     /* table of colours in this cmap */
	byte   *        in_use;    /* which cells cannot be changed? */
} CLUT;


struct AppExtra
{
	Display *	display;	/* connection to the X server */
	CLUT *		clut;		/* for shared system palette */
	Palette *	pal;		/* shared palette */
	unsigned long	last_event_time;
	unsigned long	last_event_number;
	XComposeStatus	xcompose;	/* remember compose key status */
	Atom		xsel;		/* to handle text selections */
	Atom		xctext;		/* to handle text selections */
	Atom		xwmhint;	/* to handle window decorations */
	Atom		xclip;		/* to own CLIPBOARD selection */
	int 	timer_id;	/* If USE_ALARM */
};

#define app_extra(app) ((app)->extra)


struct WindowExtra
{
	int		exposed;	/* 1 if need No/GraphicsExpose */
	int		is_paletted;	/* 0 if TrueColor/DirectColor */
	CLUT *		clut;		/* used if has private palette */
	XID		xid;		/* ID of this window */
	Atom		xdel;		/* deletion message ID */
	Bitmap *	icon_bitmap;	/* bitmap used for window icon */
};

#define win_extra(win) ((win)->extra)


struct BitmapExtra
{
	Pixmap		handle;
	Pixmap		clipmask;
	int		blitter;
};

#define bitmap_extra(bmap) ((bmap)->extra)


struct GraphicsExtra
{
	GC 		gc;
	long		bgpixval;
};

#define graphics_extra(g) ((g)->extra)


struct SubfontExtra
{
	Pixmap		clipmask;
};

#define subfont_extra(sub) ((sub)->extra)


struct FontExtra
{
	XFontStruct *	fnt;
};

#define font_extra(f) ((f)->extra)


struct CursorExtra
{
	int		shape;
	X_Cursor	cursor;
};

#define cursor_extra(c) ((c)->extra)


/*
 *  Private internal routines:
 */

/* Constants: */

enum {
	TIMER_INTERVAL = 55
};

/* Bitmaps and image to bitmap conversion: */

int 	app_bitmap_created(Display *disp, Pixmap bmap);
XID 	app_new_clipmask(Display *disp, int width, int height);
Bitmap *app_new_monochrome_bitmap(Window *win, int width, int height);
XID 	app_image_to_clipmask(App *app, Image *img);
Bitmap *app_image_to_monochrome_bitmap(Window *win, Image *img);

/* Clipboard events: */

void	app_send_clipboard(App *app, XSelectionRequestEvent *e);
char *	app_receive_clipboard(App *app, XSelectionEvent *e, long *nb);

/* Colours and X CLUT features: */

int 	app_is_true_colour_display(Display *disp);
CLUT *	app_new_clut(Display *disp, XID xwin, int size, Colour *colours);
void	app_del_clut(CLUT *clut);
long	app_window_find_colour(Graphics *g, Window *win, Colour c);
Palette *app_new_palette_from_clut(CLUT *clut);

/* KeySym to Unicode conversion: */

long	app_keysym_to_ucs(KeySym keysym);

/* Folders: */

char *  app_to_native_path(const char *path);
char *  app_to_portable_path(const char *path);
