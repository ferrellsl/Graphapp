/*
 *  Bridge between the GraphApp C code and the Cocoa (Objective-C) code.
 *
 *  Platform: macOS.
 *
 *  GraphApp's Point and Rect types collide with Apple's MacTypes.h, so
 *  no single translation unit can see both app.h and Cocoa.  The backend
 *  is therefore split in two:
 *
 *    C files  (init.c win.c event.c ...)  include appint.h, never Cocoa.
 *    ObjC files (cocoa_*.m)               include this header and Cocoa,
 *                                         never app.h.
 *
 *  This header uses only plain C types, so both sides can include it.
 *  Opaque handles are void * (the ObjC side owns the objects).
 *  Coordinates are GraphApp's: origin top-left, y down, in points.
 */

#ifndef GA_COCOA_BRIDGE_H
#define GA_COCOA_BRIDGE_H

#include <stdint.h>

/* Window style bits for gab_window_create. */
enum {
	GAB_TITLED     = 1,
	GAB_CLOSABLE   = 2,
	GAB_MINIATURE  = 4,
	GAB_RESIZABLE  = 8,
	GAB_POPUP      = 16,
	GAB_FLOATING   = 32,
	GAB_MODAL      = 64
};

/* Modifier bits reported with mouse and key events. */
enum {
	GAB_MOD_SHIFT = 1,
	GAB_MOD_CTRL  = 2,
	GAB_MOD_ALT   = 4,
	GAB_MOD_CMD   = 8
};

/* Mouse event kinds. */
enum {
	GAB_MOUSE_DOWN = 0,
	GAB_MOUSE_UP   = 1,
	GAB_MOUSE_MOVE = 2,
	GAB_MOUSE_DRAG = 3
};

/* Results of gab_next_event. */
enum {
	GAB_EV_NONE   = 0,	/* timed out, nothing happened */
	GAB_EV_OTHER  = 1,	/* an event was dispatched normally */
	GAB_EV_MOUSE  = 2,	/* mouse event handed back, not dispatched */
	GAB_EV_KEY    = 3	/* key event; left queued for later dispatch */
};

/* Mouse event handed back by gab_next_event in "pull" mode. */
typedef struct GABMouse {
	void *win;		/* the Window * given to gab_window_create */
	int kind;		/* GAB_MOUSE_* */
	int pressed;		/* bit0 left, bit1 right, bit2 other, now down */
	int mods;		/* GAB_MOD_* */
	int x, y;		/* client coordinates */
} GABMouse;

/*
 *  Callbacks into the C side.  Register once with gab_set_callbacks.
 *  'win' is the user pointer given to gab_window_create.
 */
typedef struct GABCallbacks {
	void (*mouse)(void *win, int kind, int pressed, int mods, int x, int y);
	/* keycode is the macOS virtual key code, ch the first Unicode scalar
	   of the typed text (0 if none), mods are GAB_MOD_* bits. */
	void (*key)(void *win, int keycode, unsigned long ch, int mods);
	void (*resized)(void *win, int width, int height);
	void (*moved)(void *win);
	void (*close)(void *win);
	void (*quit)(void);	/* Cmd-Q or Dock "Quit" */
	void (*activated)(void *win);
} GABCallbacks;

void gab_set_callbacks(const GABCallbacks *cb);

/* Application. */
int  gab_init(void);
void gab_screen_info(int *width, int *height, int *mm_width, int *mm_height);
void gab_beep(void);

/*
 *  Wait up to timeout_ms (negative = forever, 0 = poll) for one event.
 *  If mouse is non-NULL, mouse events for App windows are returned in
 *  *mouse rather than dispatched (used by menu and list tracking).
 *  Returns a GAB_EV_* value.
 */
int  gab_next_event(int timeout_ms, GABMouse *mouse);
int  gab_events_pending(void);

/* Windows. (x, y, w, h) is the client area in top-left screen coords. */
void *gab_window_create(void *user, int x, int y, int w, int h,
			int style, int centered, const char *title);
void gab_window_destroy(void *handle);
void gab_window_show(void *handle);
void gab_window_hide(void *handle);
void gab_window_set_title(void *handle, const char *title);
void gab_window_move(void *handle, int x, int y);
void gab_window_resize(void *handle, int w, int h);
void gab_window_get_client(void *handle, int *x, int *y, int *w, int *h);
int  gab_window_is_minimised(void *handle);
void gab_window_set_icon(void *handle, const uint32_t *argb, int w, int h);
void gab_window_set_cursor(void *handle, void *cursor);

/*
 *  The pixels a window displays.  Memory is owned by the C side (0x00RRGGBB,
 *  row 0 at the top).  The window redraws from it on request.
 */
void gab_window_set_surface(void *handle, const uint32_t *pixels, int w, int h);
void gab_window_dirty(void *handle, int x, int y, int w, int h);
void gab_flush_windows(void);

/* Fonts (CoreText). Metrics are in pixels. */
void *gab_font_create(const char *name, int bold, int italic, int pixel_height,
			int *ascent, int *descent, int *max_width);
void gab_font_release(void *font);
int  gab_font_string_width(void *font, const char *utf8, int nbytes);
/*
 *  Render text to an 8-bit coverage bitmap whose height is the font's
 *  ascent+descent and whose row 0 is the top.  *coverage is malloc'd.
 */
int  gab_font_render(void *font, const char *utf8, int nbytes,
			uint8_t **coverage, int *w, int *h);

/* Clipboard (UTF-8). gab_clipboard_get returns a malloc'd string or NULL. */
char *gab_clipboard_get(void);
int   gab_clipboard_set(const char *utf8);

/* Cursors. */
void *gab_cursor_standard(int shape);	/* GraphApp StandardCursors value */
void *gab_cursor_from_argb(const uint32_t *argb, int w, int h, int hx, int hy);
void  gab_cursor_release(void *cursor);
void  gab_cursor_get_position(int *x, int *y);
void  gab_cursor_set_position(int x, int y);

/* Window under the mouse pointer (returns the user pointer, or NULL). */
void *gab_window_under_cursor(void);

/*
 *  Test hooks (used by cocoa/test, harmless otherwise).
 *
 *  gab_test_snapshot renders the window's view, through the normal
 *  drawRect path, to a PNG file; it needs no screen-recording permission.
 *  gab_test_mouse and gab_test_key post synthetic events to a window.
 */
int  gab_test_snapshot(void *handle, const char *png_path);
void gab_test_mouse(void *handle, int kind, int x, int y, int button, int mods);
void gab_test_key(void *handle, int keycode, const char *chars, int mods);
/* Same, aimed at the frontmost App window, for programs that hide their window handles. */
void gab_test_mouse_front(int kind, int x, int y, int button, int mods);

#endif
