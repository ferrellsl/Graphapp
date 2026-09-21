/*
 *  App programming interface (internal header file).
 *
 *  Platform: macOS (Cocoa).
 *
 *  This header never needs to be included by a user's program.
 *  It is only used internally while compiling the App library.
 *
 *  It must not include any Cocoa header (see bridge.h for why).
 *  Native objects are held as opaque void * handles.
 */

#define APP_PRIVATE_DECLARATIONS

  typedef struct AppExtra       AppExtra;
  typedef struct WindowExtra    WindowExtra;
  typedef struct BitmapExtra    BitmapExtra;
  typedef struct GraphicsExtra  GraphicsExtra;
  typedef struct SubfontExtra   SubfontExtra;
  typedef struct FontExtra      FontExtra;
  typedef struct FolderExtra    FolderExtra;
  typedef struct CursorExtra    CursorExtra;

#include "apputils.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bridge.h"

/*
 *  Pixel storage.
 *
 *  Every drawable (window, bitmap) is a Surface: 32-bit 0x00RRGGBB pixels,
 *  row 0 at the top.  All drawing is done by the portable pixel engine in
 *  drawops.c.  Window surfaces are shown on screen by the Cocoa view.
 */

typedef struct Surface
{
	int		width;
	int		height;
	uint32_t *	pixels;
	int		has_dirty;	/* drawn to since last flush? */
	Rect		dirty;		/* bounding box of that drawing */
} Surface;

/*
 *  A stencil: nonzero byte = opaque pixel, zero = transparent.
 *  (The opposite sense to the Windows depth-1 clip mask.)
 */

typedef struct Mask
{
	int		width;
	int		height;
	unsigned char *	bits;
} Mask;

/*
 *  Private platform-specific data structures,
 *  linked to the 'extra' fields in public data structures.
 */

struct AppExtra
{
	int		started;
};

#define app_extra(app) ((app)->extra)


struct WindowExtra
{
	void *		handle;		/* NSWindow, via bridge */
	Surface		surf;
};

#define win_extra(win) ((win)->extra)


struct BitmapExtra
{
	Surface		surf;
	Mask *		clipmask;	/* NULL if fully opaque */
};

#define bitmap_extra(bmap) ((bmap)->extra)


enum GraphicsKinds {
	GK_NONE   = 0,		/* images: no surface */
	GK_WINDOW = 1,		/* flushed to screen when deleted */
	GK_BITMAP = 2
};

struct GraphicsExtra
{
	int		kind;
	Surface *	surf;
	uint32_t	pixval;		/* current colour, packed */
	Colour		bg;		/* background, for XOR drawing */
};

#define graphics_extra(g) ((g)->extra)


struct SubfontExtra
{
	Mask *		clipmask;	/* glyph stencil, or NULL */
};

#define subfont_extra(sub) ((sub)->extra)


struct FontExtra
{
	void *		handle;		/* CTFont, via bridge (native fonts) */
	int		ascent;
	int		descent;
};

#define font_extra(f) ((f)->extra)


struct FolderExtra
{
	int		unused;
};

#define folder_extra(f) ((FolderExtra*)((f)))


struct CursorExtra
{
	int		shape;
	void *		handle;		/* NSCursor, via bridge */
};

#define cursor_extra(c) ((c)->extra)


/*
 *  Private internal definitions:
 */

/* Surfaces and masks: */

int	app_surface_create(Surface *s, int width, int height);
void	app_surface_destroy(Surface *s);
void	app_surface_mark_dirty(Surface *s, Rect r);
void	app_mask_free(Mask *m);

/* Graphics contexts: */

Graphics *app_get_window_redraw(Window *w);

/* Image/bitmap conversion: */

Mask *	app_image_to_clipmask(App *app, Image *img);

/* Window management: */

void	app_flush_window(Window *w);
void	app_flush_all_windows(App *app);
void	app_install_bridge_callbacks(App *app);
void	app_do_redraw_window(Window *win, Rect r);
void	app_fix_window_area(Window *win);

/* Events: */

int	app_modal_in_front(Window *win);
void	app_do_close_window(Window *win);
void	app_do_move_window(Window *win);
void	app_do_resize_window(Window *win);

/* Folders: */

char *  app_to_native_path(const char *path);
char *  app_to_portable_path(const char *path);
