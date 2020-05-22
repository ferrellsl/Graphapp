/*
 *  App programming interface (utility header file).
 *
 *  Copyright (c) L.Patrick
 *
 *  This header file never needs to be included by a user's
 *  program. It is only used internally while compiling the
 *  the App library. A user program need only include app.h
 *  and link with the app.lib or libapp.a library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>

#include "app.h"

/*
 * APP_PRIVATE could be declared static if you don't want internal function
 * definitions leaking into a library or DLL.
 */
#ifndef APP_PRIVATE
#define APP_PRIVATE 
#endif

/*
 *  Utility module definitions:
 */

/* Initialisation hooks: */

void    app_app_initialise(App *app);
void    app_app_deinitialise(App *app);

/* Controls: */

void	app_do_draw_controls(Graphics *g, int num, Control **list, int clear);

/* Drawing ops (these are called via pointers within Graphics objects): */

int 	app_bitmap_fill_rect(Graphics *g, Rect r);
int 	app_bitmap_copy_rect(Graphics *dst, Point dp, Graphics *src, Rect sr);
int 	app_bitmap_draw_utf8(Graphics *g, Point p, const char *s, int nbytes);
int 	app_bitmap_draw_line(Graphics *g, Point p1, Point p2);

int 	app_image_fill_rect(Graphics *g, Rect r);
int 	app_image_copy_rect(Graphics *dst, Point dp, Graphics *src, Rect sr);
int 	app_image_draw_utf8(Graphics *g, Point p, const char *s, int nbytes);

int 	app_window_fill_rect(Graphics *g, Rect r);
int 	app_window_copy_rect(Graphics *dst, Point dp, Graphics *src, Rect sr);
int 	app_window_draw_utf8(Graphics *g, Point p, const char *s, int nbytes);
int 	app_window_draw_line(Graphics *g, Point p1, Point p2);

/* Native fonts: */

int     app_load_native_font(Font *f, const char *name, int size, int height, int style);
void    app_release_native_font(Font *f);
void    app_release_native_subfont(Font *f, Subfont *sub);
int     app_native_font_height(Font *f);
int     app_native_font_width(Font *f);
int     app_native_font_string_width(Font *f, const char *s, int nbytes);

/* Composing Unicode characters: */

long	app_compose_unicode(int accent, int letter);

/* Menu events: */

int     app_activate_menu_bar_short_cut(MenuBar *mb, unsigned long ch);

/* Window utility functions: */

long    app_actual_window_flags(long flags);
int     app_remember_window(App *app, Window *win);
int     app_forget_window(App *app, Window *win);
int     app_remember_modal(App *app, Window *win);
int     app_forget_modal(App *app, Window *win);

/* Delayed-deletion functions: */

int     app_remember_deleted_window(App *app, Window *win);
int     app_remember_deleted_control(App *app, Control *c);
void    app_do_delayed_deletion(App *app);

/* Dispatching events: */

int     app_do_key_down(Window *win, unsigned long ch);
int     app_send_key_value(Window *win, unsigned long ch, int pass_to);
int     app_do_alt_key_down(Window *win, unsigned long ch, int alt);

/* Arrays: */

void ** app_add_array_element(void **array, void *insertion);
void ** app_del_array_element(void **array, void *deletion);
