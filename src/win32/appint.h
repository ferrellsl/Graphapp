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

  typedef struct AppExtra       AppExtra;
  typedef struct WindowExtra    WindowExtra;
  typedef struct BitmapExtra    BitmapExtra;
  typedef struct GraphicsExtra  GraphicsExtra;
  typedef struct SubfontExtra   SubfontExtra;
  typedef struct FontExtra      FontExtra;
  typedef struct FolderExtra    FolderExtra;
  typedef struct CursorExtra    CursorExtra;

#include "apputils.h"

#include <setjmp.h>
#include <windows.h>

/*
 *  Fix a few compiler-specific definitions:
 */

#ifndef _export
	#define _export
#endif
#ifndef FAR
	#define FAR
#endif
#ifndef PASCAL
	#define PASCAL
#endif

/*
 *  Private platform-specific data structures,
 *  usually linked to the 'extra' fields in public data structures.
 */

struct AppExtra
{
	HINSTANCE	this_instance;
	Window *	timer_win;
	int		timer_id;
	int		window_number; /* unique increasing count */
};

#define app_extra(app) ((app)->extra)


struct WindowExtra
{
	HWND		hwnd;
	HPALETTE	winpal;
	HICON		hicon;
};

#define win_extra(win) ((win)->extra)


struct BitmapExtra
{
	HBITMAP		handle;
	HBITMAP		clipmask;
};

#define bitmap_extra(bmap) ((bmap)->extra)


struct GraphicsExtra
{
	int 		kind;
	HDC 		dc;
	HBRUSH		oldbr;
	HBRUSH		brush;
	HPEN		oldpen;
	HPEN		pen;
	HBITMAP		oldbm;
	HPALETTE	oldpal;
	Colour		bg;
};

#define graphics_extra(g) ((g)->extra)


struct SubfontExtra
{
	HBITMAP		clipmask;	/* Windows depth-1 stencil */
};

#define subfont_extra(sub) ((sub)->extra)


struct FontExtra
{
	HFONT		fnt;
};

#define font_extra(f) ((f)->extra)


struct FolderExtra
{
	char		first;
	char		dots;
	char		root;
	HANDLE		hnd;
	WIN32_FIND_DATA	data;
	char            temp[4];
};

#define folder_extra(f) ((FolderExtra*)((f)))


struct CursorExtra
{
	int		shape;
	HCURSOR		cursor;
};

#define cursor_extra(c) ((c)->extra)


/*
 *  Private internal definitions:
 */

/* Constants: */

enum {
	TIMER_INTERVAL = 55  /* the Win32 timer is limited to this resolution */
};

/* Events: */

LRESULT FAR PASCAL _export
app_winproc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* Graphics context manipulation: */

Graphics *app_get_window_redraw(HDC dc, Window *w);

/* Image/bitmap conversion: */

HBITMAP app_image_to_clipmask(App *app, Image *img);

/* Palettes: */

int app_realize_palette(Window *win);

/* Folders: */

char *  app_to_native_path(const char *path);
char *  app_to_portable_path(const char *path);
