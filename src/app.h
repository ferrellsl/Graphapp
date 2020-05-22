/*
 *  App programming interface (main header file).
 *
 *  Copyright (c) L.Patrick
 *
 *  This is the only file which need be included in a user's source
 *  code to use the App library. The library itself will also need
 *  to be linked to a finished program.
 */


/*
 *  Data object declarations:
 */

  typedef unsigned char      byte;
  typedef unsigned long      Char;

  typedef struct Point       Point;
  typedef struct Rect        Rect;
  typedef struct Colour      Color;
  typedef struct Colour      Colour;

  struct Point {
	int   x;
	int   y;
  };

  struct Rect {
	int   x;        /* left-most pixel inside rectangle */
	int   y;        /* top-most pixel inside rectangle */
	int   width;    /* in pixels */
	int   height;   /* in pixels */
  };

  struct Colour {
	byte  alpha;    /* transparency, 0=opaque, 255=transparent */
	byte  red;      /* intensity, 0=black, 255=bright red */
	byte  green;    /* intensity, 0=black, 255=bright green */
	byte  blue;     /* intensity, 0=black, 255=bright blue */
  };

/*
 *  Reference object declarations:
 */

  typedef struct App            App;
  typedef struct Window         Window;
  typedef struct Bitmap         Bitmap;
  typedef struct Graphics       Graphics;
  typedef struct FontWidth      FontWidth;
  typedef struct Subfont        Subfont;
  typedef struct Font           Font;
  typedef struct Folder         Folder;
  typedef struct Control        Control;
  typedef struct MenuBar        MenuBar;
  typedef struct Menu           Menu;
  typedef struct MenuItem       MenuItem;
  typedef struct Timer          Timer;
  typedef struct Cursor         Cursor;
  typedef struct Region         Region;
  typedef struct Palette        Palette;
  typedef struct Image          Image;
  typedef struct ImageList      ImageList;
  typedef struct ImageReader    ImageReader;
  typedef struct StringNode     StringNode;
  typedef struct StringTable    StringTable;

/*
 *  Private platform-specific declarations:
 */

#ifndef APP_PRIVATE_DECLARATIONS

  /*
   *  This ensures we have no incomplete types.
   *  Some compilers dislike incomplete types.
   */

  #define AppExtra       void
  #define WindowExtra    void
  #define BitmapExtra    void
  #define GraphicsExtra  void
  #define SubfontExtra   void
  #define FontExtra      void
  #define FolderExtra    void
  #define CursorExtra    void

#endif /* APP_PRIVATE_DECLARATIONS */

/*
 *  Event handlers:
 */

  typedef void (*WindowFunc)      (Window *w);
  typedef void (*WindowMouseFunc) (Window *w, int buttons, Point xy);
  typedef void (*WindowKeyFunc)   (Window *w, unsigned long key);
  typedef void (*WindowDrawFunc)  (Window *w, Graphics *g);

  typedef void (*ControlFunc)     (Control *c);
  typedef void (*MouseFunc)       (Control *c, int buttons, Point xy);
  typedef void (*KeyFunc)         (Control *c, unsigned long key);
  typedef void (*DrawFunc)        (Control *c, Graphics *g);

  typedef void (*MenuAction)      (MenuItem *mi);

  typedef void (*TimerAction)     (Timer *t);

  typedef char * (*TipFunc)       (Control *c, Graphics *wg, Rect *r); //!!

/*
 *  ImageReader call-backs:
 */

  typedef int (*ImageMessageFunc) (ImageReader *reader, char *message);
  typedef int (*ImageProgressFunc)(ImageReader *reader);


/*
 *  Drawing operation prototypes:
 */

  typedef int (*CopyRectFunc)(Graphics *g, Point dp, Graphics *src, Rect sr);
  typedef int (*FillRectFunc)(Graphics *g, Rect r);
  typedef int (*DrawUTF8Func)(Graphics *g, Point p, const char *utf8, int nbytes);
  typedef int (*DrawLineFunc)(Graphics *g, Point p1, Point p2);


/*
 *  Reference object structures:
 */

  struct App {
	AppExtra *      extra;              /* platform-specific data */
	void *          data;               /* user-defined data */

	int             gui_available;      /* is the GUI available? */
	Rect            screen_area;        /* screen pixel dimensions */
	Rect            screen_mm;          /* screen size in millimetres */

	char *          program_name;       /* path to starting program */
	short           has_resources;      /* this app has resources? */
	short           use_X_copy_paste;   /* use X11 mouse copy/paste? */
	int             socket_fd;          /* X11 socket file descriptor */

	int             num_windows;        /* list of all windows */
	Window **       windows;
	int             visible_windows;    /* how many are visible */

	int             num_modals;         /* stack of modal windows */
	Window **       modals;

	int             num_fonts;          /* list of fonts */
	Font **         fonts;

	int             num_timers;         /* list of active timers */
	Timer **        timers;

	int             num_cursors;        /* list of cursors */
	Cursor **       cursors;

	int             compose_key;        /* for producing Unicode input */

	StringTable *   string_table;       /* for localisation */

	char *          open_folder;        /* file open dialog folder */
	Rect            open_dialog_area;   /* dialog screen location */
	char *          save_folder;        /* file save dialog folder */
	Rect            save_dialog_area;   /* dialog screen location */

	int             deleting;           /* doing delayed-deletion now? */
	int             num_deleted_windows;
	Window **       deleted_windows;    /* list of windows to delete */
	int             num_deleted_controls;
	Control **      deleted_controls;   /* list of controls to delete */

	void *			tip;				/* tip control */

	void *          ft_library;         /* for FreeType support */
  };

  struct Window  {
	WindowExtra *   extra;              /* platform-specific data */
	void *          data;               /* user-defined data */

	App *           app;                /* back-pointer */

	char *          text;               /* title bar name */
	long            flags;              /* decorations bit-field */
	Rect            area;               /* drawable area */

	Palette *       pal;                /* private palette */
	Colour          bg;                 /* background fill colour */

	int             state;              /* visible? enabled? */
	Region *        redraw_rgn;         /* accumulated area to draw */
	Region *        visible;            /* visible region */
	MenuBar *       menubar;            /* menu list, if any */
	int             num_children;       /* list of child controls */
	Control **      children;
	Control *       mouse_grab;         /* last mouse down was here */
	Control *       key_focus;          /* keyboard focus control */
	int             pass_event;         /* pass event up the chain? */

	Cursor *        cursor;             /* this window's current cursor */
	Cursor *        old_cursor;         /* original cursor used */

	WindowFunc *        close;          /* after user selects Close */
	WindowFunc *        move;           /* after window was moved */
	WindowFunc *        resize;         /* after window was resized */
	WindowDrawFunc *    redraw;         /* to redraw the window */

	WindowMouseFunc *   mouse_down;     /* mouse button click */
	WindowMouseFunc *   mouse_up;       /* mouse button released */
	WindowMouseFunc *   mouse_drag;     /* mouse button down, moved */
	WindowMouseFunc *   mouse_move;     /* mouse buttons up, moved */

	WindowKeyFunc *     key_down;       /* normal Unicode keystroke */
	WindowKeyFunc *     key_action;     /* arrow keys, etc */
  };

  struct Bitmap {
	BitmapExtra *   extra;              /* platform-specific data */
	Window *        win;                /* for colour format info */
	Rect            area;               /* drawable area */
  };

  struct Graphics {
	GraphicsExtra * extra;              /* platform-specific data */

	Colour          colour;             /* current drawing colour */
	long            pixval;             /* current colour index */
	Font *          font;               /* current text font */
	int             line_width;         /* current pixel line width */
	int             text_direction;     /* current text direction */
	int             xor_mode;           /* currently XOR drawing? */

	App *           app;                /* if required */
	Window *        win;                /* target window, or */
	Bitmap *        bmap;               /* target bitmap, or */
	Control *       ctrl;               /* target control, or */
	Image *         img;                /* target image */

	Rect            area;               /* target's drawable area */
	Region *        clip;               /* clip drawing to region */
	Point           offset;             /* (0,0) is actually here */

	CopyRectFunc    copy_rect;          /* pointer to drawing func */
	FillRectFunc    fill_rect;          /* pointer to drawing func */
	DrawUTF8Func    draw_utf8;          /* pointer to drawing func */
	DrawLineFunc    draw_line;          /* pointer to drawing func */
  };

  struct FontWidth {
	int             width;              /* -1=non-existent, or >= 0 */
	int             num_ranges;         /* num of (start,end) pairs */
	byte *          range_list;         /* (start,end) byte pairs */
  };

  struct Subfont {
	SubfontExtra *  extra;              /* platform-specific data */
	unsigned long   base;               /* Unicode num. of 1st char */
	int             num_widths;         /* list of widths */
	FontWidth **    widths;
	short *         width;              /* alternative widths array */
	Image *         img;                /* glyph array */
	int             anti_alias;         /* anti-alias greyscales? */
  };

  struct Font {
	FontExtra *     extra;              /* platform-specific data */
	int             refcount;           /* for caching on App */
	int             maximum_width;      /* max. width of any char */
	int             height;             /* pixel height of any char */
	char *          name;               /* family name of font */
	int             style;              /* style bit-field */
	App *           app;                /* back-pointer to App */
	int             num_subfonts;       /* cache of subfonts */
	Subfont **      subfonts;
    void *          ft_face;            /* for FreeType support */
  };

  struct Folder {
	int             id;                 /* directory identifier */
  };

  struct Control {
	Rect            area;               /* drawable area */
	Rect            natural;            /* natural area */
	int             padding;            /* inner gap */
	int             margin;             /* outer gap */
	Point           corner;             /* from parent RB-corner */
	Point           offset;             /* to window co-ordinates */
	Window *        win;                /* parent window */
	Control *       parent;             /* parent control (optional) */
	int             num_children;       /* list of child controls */
	Control **      children;
	Colour          bg;                 /* background fill colour */
	Colour          fg;                 /* foreground draw colour */
	long            state;              /* enabled? layout? border? */
	Region *        visible;            /* visible region on window */
	void *          data;               /* user-defined data */
	char *          text;               /* control-defined data */
	void *          extra;              /* control-defined data */
	long            value;              /* control-defined data */
	Image *         img;                /* control-defined data */
	Font *          font;               /* control-defined data */
	Cursor *        cursor;             /* control-defined data */
	ControlFunc *   resize;             /* call-back used to resize */
	DrawFunc *      redraw;             /* call-back used to redraw */
	MouseFunc *     mouse_down;         /* mouse event handler */
	MouseFunc *     mouse_up;           /* mouse event handler */
	MouseFunc *     mouse_drag;         /* mouse event handler */
	MouseFunc *     mouse_move;         /* mouse event handler */
	KeyFunc *       key_down;           /* Unicode key handler */
	KeyFunc *       key_action;         /* arrow key, etc, handler */
	ControlFunc *   action;             /* action to perform */
	ControlFunc *   update;             /* used when any data changes */
	ControlFunc *   refocus;            /* used when focus changes */
	ControlFunc *   del;                /* used during deletion */
  };

  struct MenuBar {
	Control *       ctrl;               /* associated control */
	Font *          font;               /* for displaying all text */
	int             align;              /* text alignment,direction */
	int             num_menus;          /* list of menus */
	Menu **         menus;
  };

  struct Menu {
	MenuBar *       parent;             /* enclosing menubar */
	char *          text;               /* name of the menu */
	int             num_items;          /* list of menu items */
	MenuItem **     items;
	int             lasthit;            /* which item was chosen */
	Font *          font;               /* for drawing menu's name */
	Colour          fg;                 /* for drawing menu's name */
  };

  struct MenuItem {
	Menu *          parent;             /* enclosing menu */
	char *          text;               /* menu item name */
	unsigned long   shortcut;           /* shortcut key */
	int             state;              /* enabled? checked? */
	Menu *          submenu;            /* pop-up menu */
	MenuAction      action;             /* user-defined action */
	void *          data;               /* user-defined data */
	int             value;              /* user-defined value */
	Font *          font;               /* for drawing item's text */
	Colour          fg;                 /* for drawing item's text */
  };

  struct Timer {
	App *           app;                /* associated App */
	int             milliseconds;       /* interval between actions */
	unsigned long   last_time;          /* last checked time */	//!!
	TimerAction     action;             /* user-defined action */
	void *          data;               /* user-defined data */
	int             value;              /* user-defined value */
  };

  struct Cursor {
	App *           app;                /* associated App */
	CursorExtra *   extra;              /* platform-specific data */
  };

  struct Region {
	Rect            extents;            /* maximum affected rect */
	int             size;               /* current allocated size */
	int             num_rects;          /* list of distinct rects */
	Rect *          rects;
  };

  struct Palette {
	int             size;               /* list of colours */
	Colour *        element;
  };

  struct Image {
	int             depth;              /* 8 (indexed), 32 (direct) */
	int             width;              /* in pixels */
	int             height;             /* in pixels */
	int             cmap_size;          /* indexed list of colours */
	Colour *        cmap;
	byte **         data8;              /* indexed 8-bit data, or */
	Colour **       data32;             /* direct 32-bit data */
  };

  struct ImageList {
	int             num_images;         /* list of images */
	Image **        images;
  };

  struct ImageReader {
	char *              filename;           /* user-given fields */
	FILE *              file;               /* file to read from */
	byte *              memsrc;             /* memory source to read from */
	int                 memsize;            /* byte size of memory source */
	Palette *           src_pal;            /* dither to this palette */
	int                 max_cmap_size;      /* only use this many colours */
	int                 required_depth;     /* 8 or 32 */

	ImageMessageFunc    message_func;       /* to report some problem */
	ImageProgressFunc   error_func;         /* if error occurs, tidy up */
	ImageProgressFunc   startup_func;       /* called before starting */
	ImageProgressFunc   after_dither_func;  /* called after dithering */
	ImageProgressFunc   progress_func;      /* called after each line */
	ImageProgressFunc   rendering_func;     /* called after each line */
	ImageProgressFunc   success_func;       /* called after success */
	void *              user_data;          /* user-defined data */

	int                 state;              /* DITHERING, RENDERING etc */
	int                 bytes_read;         /* header bytes read already */
	int                 stage;              /* 1 <= stage <= max_stages */
	int                 max_stages;         /* total stages to happen */
	int                 row;                /* 0 <= row < height, random */
	int                 rows_done;          /* 0,1,2,...,height, ordered */
	int                 row_height;         /* height of current stage */
	int                 width;              /* in pixels */
	int                 height;             /* in pixels */
	byte **             data8;              /* implies palette */
	Colour **           data32;             /* implies no palette */
	Palette *           pal;                /* if data8 used */
  };

  struct StringNode {
	StringNode *    next;               /* next in list */
	const char *    key;                /* string key */
	unsigned long   length;             /* key's length in bytes */
	unsigned long   hash;               /* full hash number */
	char *          value;              /* pointer to value data */
  };

  struct StringTable {
	unsigned long   size;               /* size of node array */
	unsigned long   num_nodes;          /* node array */
	StringNode **   nodes;
  };


/*
 *  ANSI character codes:
 */

enum ANSIKey {
	BELL  = 0x07,
	BKSP  = 0x08,
	VTAB  = 0x0B,
	FF    = 0x0C,
	ESC   = 0x1B
};

/*
 *  Edit-key codes:
 */

enum EditKey {
	INS   = 0x21AF,
	DEL   = 0x2326,
	HOME  = 0x21F1,
	END   = 0x21F2,
	PGUP  = 0x21DE,
	PGDN  = 0x21DF,
	ENTER = 0x21B5
};

/*
 *  Arrow-key codes:
 */

enum ArrowKey {
	LEFT       = 0x2190,
	UP         = 0x2191,
	RIGHT      = 0x2192,
	DOWN       = 0x2193
};

/*
 *  Function-key codes:
 */

enum FunctionKey {
	F1    = 0x276C,
	F2    = 0x276D,
	F3    = 0x276E,
	F4    = 0x276F,
	F5    = 0x2770,
	F6    = 0x2771,
	F7    = 0x2772,
	F8    = 0x2773,
	F9    = 0x2774,
	F10   = 0x2775
};

/*
 *  Modifier-key bit-fields:
 */

enum ModifierBit {
	CONTROL = 0x20000000L,
	SHIFT   = 0x10000000L
};

/*
 *  Mouse button states:
 */

enum MouseState {
	NO_BUTTON     = 0,
	LEFT_BUTTON   = 1,
	MIDDLE_BUTTON = 2,
	RIGHT_BUTTON  = 4
};


/*
 *  Application:
 */

App *	app_new_app(int argc, char *argv[]);
void	app_del_app(App *app);

int 	app_exec(App *app, const char *command);


/*
 *  Memory management:
 */

void *	app_alloc(long size);
void *	app_zero_alloc(long size);
void *	app_realloc(void *ptr, long newsize);
void	app_free(void *ptr);
long	app_memory_used(void);
void	app_debug_memory(int on);


/*
 *  Event handling:
 */

void	app_main_loop(App *app);
int 	app_wait_event(App *app);
int 	app_process_events(App *app);
int 	app_do_event(App *app);
int 	app_peek_event(App *app);

int 	app_get_mouse_event(App *app, int *buttons, Point *p);

void	app_pass_event(Control *c);

void	app_draw_all(App *app);


/*
 *  Clipboard:
 */

int 	app_set_clipboard_text(App *app, const char *text);
char *	app_get_clipboard_text(App *app);


/*
 *  Timing:
 */

int           app_delay(App *app, int milliseconds);
unsigned long app_current_time(App *app);

Timer *	app_new_timer(App *app, TimerAction action, int milliseconds);
void	app_del_timer(Timer *t);
void	app_reset_timer(Timer *t, int milliseconds);


/*
 *  Sound:
 */
void	app_beep(App *app);


/*
 *  Localisation:
 */
void    app_set_string(App *app, const char *key, char *value);
char *  app_get_string(App *app, const char *key);


/*
 *  File and folder access:
 */

enum FileInfo {
	IS_APP    = 1,
	IS_WRITE  = 2,
	IS_READ   = 4,
	IS_FOLDER = 8,
	IS_FILE   = 16
};

FILE *	app_open_file(const char *filepath, const char *mode);
int 	app_close_file(FILE *f);
int 	app_remove_file(const char *filepath);
int 	app_rename_file(const char *oldpath, const char *newpath);

Folder *app_open_folder(const char *folderpath);
char *	app_read_folder(Folder *f);
int 	app_close_folder(Folder *f);
int 	app_make_folder(const char *folderpath, int mode);
int 	app_remove_folder(const char *folderpath);
int 	app_rename_folder(const char *oldpath, const char *newpath);

int 	app_file_info(const char *filepath);
long	app_file_size(const char *filepath);
long	app_file_time(const char *filepath);
char *	app_current_folder(void);
int 	app_set_current_folder(const char *folderpath);
char *	app_form_file_path(const char *folderpath, const char *filename);
char *	app_get_file_name(const char *filepath);


/*
 *  Control and Window state flags:
 */

/* State flags: */
#define VISIBLE        0x00000001L
#define ENABLED        0x00000002L
#define CHECKED        0x00000004L
#define HIGHLIGHTED    0x00000008L
#define ARMED          0x00000010L

/* Layout flags: */
#define EDGE_LEFT      0x00000100L
#define EDGE_TOP       0x00000200L
#define EDGE_RIGHT     0x00000400L
#define EDGE_BOTTOM    0x00000800L
#define EDGE_ALL       (EDGE_LEFT | EDGE_TOP | EDGE_RIGHT | EDGE_BOTTOM)
#define DOCK           0x00001000L
#define FILL           (EDGE_ALL | DOCK)

#define FLOW           0x00002000L
#define FLOW_VERT      0x00004000L
#define LAYOUT_MASK    0x0000FF00L
#define AUTOSIZE       0x00010000L
#define TABSTOP        0x00020000L
#define MANAGER        0x00040000L

/* Border flags: */
#define BORDER_LEFT    0x00100000L
#define BORDER_TOP     0x00200000L
#define BORDER_RIGHT   0x00400000L
#define BORDER_BOTTOM  0x00800000L
#define BORDER_ALL     (BORDER_LEFT |BORDER_TOP |BORDER_RIGHT |BORDER_BOTTOM)

/* Tips flags: */	//!!
#define TIP_TEXT       0x01000000L
#define TIP_HANDLER    0x02000000L
#define TIP_MASK       (TIP_TEXT | TIP_HANDLER)


/*
 *  Window creation flags:
 */

#define SIMPLE_WINDOW   0x00000000L

#define MENUBAR         0x00000010L
#define TITLEBAR        0x00000020L
#define CLOSEBOX        0x00000040L
#define RESIZE          0x00000080L
#define MAXIMIZE        0x00000100L
#define MINIMIZE        0x00000200L
#define HSCROLLBAR      0x00000400L
#define VSCROLLBAR      0x00000800L

#define MODAL           0x00001000L
#define FLOATING        0x00002000L
#define CENTERED        0x00004000L
#define CENTRED         0x00004000L
#define POPUP           0x00008000L
#define BASE            0x00010000L
#define TEMP_CURSOR     0x80000000L

#define STANDARD_WINDOW (TITLEBAR|CLOSEBOX|RESIZE|MAXIMIZE|MINIMIZE)


/*
 *  Windows:
 */

Window *app_new_window(App *app, Rect area, const char *name, long flags);
void	app_del_window(Window *w);

void	app_show_window(Window *w);
void	app_hide_window(Window *w);

Window *app_get_window_under_cursor(App *app);

void	app_move_window(Window *w, Rect r);
void	app_size_window(Window *w, Rect r);
void	app_redraw_rect(Window *w, Rect r);
void	app_redraw_window(Window *w);
void	app_draw_window(Window *w);
Rect	app_get_window_area(const Window *w);
Rect	app_get_window_rect(const Window *w);

void	app_set_window_title(Window *w, const char *title);
char *	app_get_window_title(Window *w);
void	app_set_window_icon(Window *w, Image *icon);

void	app_on_window_close (Window *w, WindowFunc close);
void	app_on_window_move(Window *w, WindowFunc move);
void	app_on_window_resize(Window *w, WindowFunc resize);
void	app_on_window_redraw(Window *w, WindowDrawFunc redraw);

void	app_on_window_mouse_down(Window *w, WindowMouseFunc mouse_down);
void	app_on_window_mouse_up  (Window *w, WindowMouseFunc mouse_up);
void	app_on_window_mouse_drag(Window *w, WindowMouseFunc mouse_drag);
void	app_on_window_mouse_move(Window *w, WindowMouseFunc mouse_move);
void	app_on_window_key_down  (Window *w, WindowKeyFunc key_down);
void	app_on_window_key_action(Window *w, WindowKeyFunc key_action);

void	app_set_window_background(Window *w, Colour col);
Colour	app_get_window_background(Window *w);

void	app_set_window_data(Window *w, void *data);
void *	app_get_window_data(Window *w);

void	app_hide_all_windows(App *app);
void	app_del_all_windows(App *app);


/*
 *  Controls:
 */

Control *app_new_control(Window *parent, Rect r);
Control *app_add_control(Control *parent, Rect r);
void	app_del_control(Control *c);

int 	app_attach_to_window(Window *parent, Control *c);
int 	app_attach_to_control(Control *parent, Control *c);
int	app_bring_control_to_front(Control *c);
int	app_send_control_to_back(Control *c);
int 	app_remove_control(Control *c);

void	app_place_window_controls(Window *w, int recalculate);
Control *app_locate_control(Window *w, Point p);

#define app_parent_window(c)	((c)->win)
void	app_draw_control(Control *c);
void	app_redraw_control(Control *c);
void	app_redraw_control_rect(Control *c, Rect r);

void	app_move_control(Control *c, Rect r);
void	app_size_control(Control *c, Rect r);
void	app_set_control_area(Control *c, Rect r);
Rect	app_get_control_area(Control *c);

int 	app_is_visible(Control *c);
void	app_show_control(Control *c);
void	app_hide_control(Control *c);

void	app_on_control_resize    (Control *c, ControlFunc resize);
void	app_on_control_redraw    (Control *c, DrawFunc redraw);
void	app_on_control_mouse_down(Control *c, MouseFunc mouse_down);
void	app_on_control_mouse_up  (Control *c, MouseFunc mouse_up);
void	app_on_control_mouse_drag(Control *c, MouseFunc mouse_drag);
void	app_on_control_mouse_move(Control *c, MouseFunc mouse_move);
void	app_on_control_key_down  (Control *c, KeyFunc key_down);
void	app_on_control_key_action(Control *c, KeyFunc key_action);
void	app_on_control_action    (Control *c, ControlFunc action);
void	app_on_control_update    (Control *c, ControlFunc update);
void	app_on_control_refocus   (Control *c, ControlFunc refocus);
void	app_on_control_deletion  (Control *c, ControlFunc del);

void	app_set_control_background(Control *c, Colour col);
Colour	app_get_control_background(Control *c);

void	app_set_control_foreground(Control *c, Colour col);
Colour	app_get_control_foreground(Control *c);

void	app_set_control_text(Control *c, const char *text);
char *	app_get_control_text(Control *c);

void	app_set_control_data(Control *c, void *data);
void *	app_get_control_data(Control *c);

void	app_set_control_value(Control *c, long value);
long	app_get_control_value(Control *c);

void	app_set_control_image(Control *c, Image *img);
Image *	app_get_control_image(Control *c);

void	app_set_control_font(Control *c, Font *f);
Font *	app_get_control_font(Control *c);

void	app_set_control_cursor(Control *c, Cursor *cursor);
Cursor *app_get_control_cursor(Control *c);

int 	app_set_control_tip(Control *c, long mode, void *p);

void	app_set_control_border(Control *c, long flags);
void	app_set_control_layout(Control *c, long flags);
void	app_set_control_gap(Control *c, int padding, int margin);
void	app_set_control_autosize(Control *c, int autosizable);

int 	app_is_enabled(Control *c);
void	app_enable(Control *c);
void	app_disable(Control *c);

int 	app_is_checked(Control *c);
void	app_check(Control *c);
void	app_uncheck(Control *c);

int 	app_is_highlighted(Control *c);
void	app_highlight(Control *c);
void	app_unhighlight(Control *c);

int 	app_is_armed(Control *c);
void	app_arm(Control *c);
void	app_disarm(Control *c);

int 	app_has_focus(Control *c);
void	app_set_focus(Control *c);

void	app_flash_control(Control *c);
void	app_activate_control(Control *c);


/* Tips: */	//!!

int 	app_set_control_tip(Control *c, long mode, void *p);
void	app_handle_tip(Control *c);


/*
 *  Managers:	//!!
 */
void	app_manager_hbox(Control *c);
void	app_manager_vbox(Control *c);


/*
 *  Labels:
 */
Control *app_new_label(Window *w, Rect r, const char *text, int align);
Control *app_add_label(Control *c, Rect r, const char *text, int align);

Control *app_new_image_label(Window *w, Rect r, Image *img, int align);
Control *app_add_image_label(Control *c, Rect r, Image *img, int align);


/*
 *  Push-buttons:
 */
Control *app_new_button(Window *w, Rect r, const char *text, ControlFunc fn);
Control *app_add_button(Control *c, Rect r, const char *text, ControlFunc fn);

Control *app_new_image_button(Window *w, Rect r, Image *img, ControlFunc fn);
Control *app_add_image_button(Control *c, Rect r, Image *img, ControlFunc fn);


/*
 *  Check-boxes:
 */
Control *app_new_check_box(Window *w, Rect r, const char *text, ControlFunc fn);
Control *app_add_check_box(Control *c, Rect r, const char *text, ControlFunc fn);

Control *app_new_image_check_box(Window *w, Rect r, Image *img, ControlFunc fn);
Control *app_add_image_check_box(Control *c, Rect r, Image *img, ControlFunc fn);


/*
 *  Radio-buttons and radio-groups:
 */
Control *app_new_radio_group(Window *w);
Control *app_add_radio_group(Control *c);

Control *app_new_radio_button(Window *w, Rect r, const char *text, ControlFunc fn);
Control *app_add_radio_button(Control *c, Rect r, const char *text, ControlFunc fn);


/*
 *  Drop-down menu lists:
 */
Control *app_new_drop_list(Window *w, Rect r, char **lines, ControlFunc fn);
Control *app_add_drop_list(Control *c, Rect r, char **lines, ControlFunc fn);

Control *app_new_drop_field(Window *w, Rect r, char **lines);
Control *app_add_drop_field(Control *c, Rect r, char **lines);

int app_pop_up_list(Window *w, Font *f, char **lines, int buttons, Point p);


/*
 *  Pull-down Menus:
 */
MenuBar * app_new_menu_bar(Window *win);
void      app_del_menu_bar(MenuBar *mb);

Menu *    app_new_menu(MenuBar *mb, const char *name);
Menu *    app_new_sub_menu(Menu *m, const char *name);
void      app_del_menu(Menu *m);

MenuItem *app_new_menu_item(Menu *m, const char *name, unsigned long key, MenuAction fn);
void      app_del_menu_item(MenuItem *mi);

void    app_check_menu_item(MenuItem *mi);
void    app_uncheck_menu_item(MenuItem *mi);
int     app_menu_item_is_checked(MenuItem *mi);

void	app_enable_menu_item(MenuItem *mi);
void	app_disable_menu_item(MenuItem *mi);
int 	app_menu_item_is_enabled(MenuItem *mi);

void	app_set_menu_item_value(MenuItem *mi, int value);
int 	app_get_menu_item_value(MenuItem *mi);

void	app_set_menu_item_foreground(MenuItem *mi, Colour col);
Colour	app_get_menu_item_foreground(MenuItem *mi);

void	app_set_menu_item_font(MenuItem *mi, Font *font);
Font *	app_get_menu_item_font(MenuItem *mi);

void	app_set_menu_foreground(Menu *m, Colour col);
Colour	app_get_menu_foreground(Menu *m);

void	app_set_menu_font(Menu *m, Font *font);
Font *	app_get_menu_font(Menu *m);

void	app_set_menu_bar_font(MenuBar *mb, Font *font);
Font *	app_get_menu_bar_font(MenuBar *mb);


/*
 *  Scroll-bars:
 */
Control *app_new_scroll_bar(Window *w, Rect r, int max, int pagesize,
		ControlFunc fn);
Control *app_add_scroll_bar(Control *c, Rect r, int max, int pagesize,
		ControlFunc fn);

void	app_change_scroll_bar(Control *c, int pos, int max, int pagesize);


/*
 *  Separators:
 */
Control *app_new_separator(Window *win, Rect r, int align);
Control *app_add_separator(Control *parent, Rect r, int align);


/*
 *  Splitters:
 */
Control *app_new_splitter(Window *win, Rect r);
Control *app_add_splitter(Control *parent, Rect r);


/*
 *  List-boxes:
 */
Control *app_new_list_box(Window *w, Rect r, char **list, ControlFunc fn);
Control *app_add_list_box(Control *c, Rect r, char **list, ControlFunc fn);

void	app_change_list_box(Control *c, char **list);
void	app_reset_list_box(Control *c);
void	app_set_list_box_item(Control *c, int index);
int 	app_get_list_box_item(Control *c);


/*
 *  Tab-pane buttons:
 */
Control *app_new_tab_button(Window *w, Rect r, const char *text, ControlFunc fn);
Control *app_add_tab_button(Control *c, Rect r, const char *text, ControlFunc fn);


/*
 *  Notebook-pane buttons:
 */
Control *app_new_note_button(Window *w, Rect r, const char *text, ControlFunc fn);
Control *app_add_note_button(Control *c, Rect r, const char *text, ControlFunc fn);


/*
 *  Text fields and text editing areas:
 */
Control *app_new_field(Window *w, Rect r, const char *text);
Control *app_add_field(Control *c, Rect r, const char *text);
void	 app_set_field_allowed_width(Control *c, int width);
void	 app_set_field_allowed_chars(Control *c, const char *allowed);
void	 app_set_field_disallowed_chars(Control *c, const char *disallowed);

Control *app_new_password_field(Window *w, Rect r, const char *text);
Control *app_add_password_field(Control *c, Rect r, const char *text);

Control *app_new_text_box(Window *w, Rect r, const char *text);
Control *app_add_text_box(Control *c, Rect r, const char *text);

void	 app_cut_text(Control *c);
void	 app_copy_text(Control *c);
void	 app_clear_text(Control *c);
void	 app_paste_text(Control *c);
void	 app_insert_text(Control *c, const char *text);
void	 app_select_text(Control *c, long start, long end);
void	 app_text_selection(Control *c, long *start, long *end);


/*
 *  Bitmaps:
 */

Bitmap *app_new_bitmap(Window *w, int width, int height);
void	app_del_bitmap(Bitmap *b);
Rect	app_get_bitmap_area(Bitmap *b);


/*
 *  Dialogs:
 */

enum DialogAnswer {
	CANCEL = -1,
	NO     =  0,
	YES    =  1
};

void	app_error(App *app, const char *message);
void	app_ask_ok(App *app, const char *title, const char *message);
int 	app_ask_ok_cancel(App *app, const char *title, const char *message);
int 	app_ask_yes_no(App *app, const char *title, const char *message);
int 	app_ask_yes_no_cancel(App *app, const char *title, const char *message);
char *	app_ask_string(App *app, const char *title, const char *question, const char *answer);
char *	app_ask_file_open(App *app, const char *title, const char *msg, const char *name);
char *	app_ask_file_save(App *app, const char *title, const char *msg, const char *name);

/*
 *  Graphics:
 */

Graphics *app_get_window_graphics(Window *w);
Graphics *app_get_control_graphics(Control *c);
Graphics *app_get_bitmap_graphics(Bitmap *b);
Graphics *app_get_image_graphics(Image *img);

void	app_del_graphics(Graphics *g);

#define app_set_color(g,col)  app_set_rgb((g),(col))
#define app_set_colour(g,col) app_set_rgb((g),(col))

void	app_set_rgb(Graphics *g, Colour col);
void	app_set_rgbindex(Graphics *g, int index);

void	app_set_line_width(Graphics *g, int width);
void	app_set_text_direction(Graphics *g, int direction);

void	app_set_font(Graphics *g, Font *f);
void    app_set_default_font(Graphics *g);

void	app_set_clip_rect(Graphics *g, Rect r);
void	app_set_clip_region(Graphics *g, Region *rgn);

void	app_set_xor_mode(Graphics *g, Colour bgcol);
void	app_set_paint_mode(Graphics *g);


/*
 *  Drawing primitives (implemented as macros for speed):
 */

#define app_copy_rect(g,dp,src,sr) ((g)->copy_rect((g),(dp),(src),(sr)))
#define app_fill_rect(g,r)         ((g)->fill_rect((g),(r)))
#define app_draw_utf8(g,p,utf8,nb) ((g)->draw_utf8((g),(p),(utf8),(nb)))
#define app_draw_line(g,p1,p2)     ((g)->draw_line((g),(p1),(p2)))


/*
 *  Drawing operations:
 */

int 	app_copy_bits(Graphics *g, Rect r, Palette *pal, byte **rows);
int 	app_copy_rgbs(Graphics *g, Rect r, Colour **rows);

int 	app_draw_point(Graphics *g, Point p);
int 	app_draw_rect(Graphics *g, Rect r);
int 	app_draw_shadow_rect(Graphics *g, Rect r, Colour c1, Colour c2);
int 	app_draw_round_rect(Graphics *g, Rect r);
int 	app_fill_round_rect(Graphics *g, Rect r);
int 	app_draw_ellipse(Graphics *g, Rect r);
int 	app_fill_ellipse(Graphics *g, Rect r);
int 	app_draw_arc(Graphics *g, Rect r, int start_angle, int end_angle);
int 	app_fill_arc(Graphics *g, Rect r, int start_angle, int end_angle);
int 	app_draw_polyline(Graphics *g, Point *p, int n);
int 	app_draw_polygon(Graphics *g, Point *p, int n);
int 	app_fill_polygon(Graphics *g, Point *p, int n);
int 	app_texture_rect(Graphics *g, Rect dr, Graphics *src, Rect sr);
int 	app_fill_region(Graphics *g, Region *reg);

int 	app_portable_draw_line(Graphics *g, Point p1, Point p2);

/*
 *  Drawing text with alignments and word-wrapping:
 */

enum TextFormat {
	ALIGN_LEFT     = 1,
	ALIGN_RIGHT    = 2,
	ALIGN_JUSTIFY  = 3,
	ALIGN_CENTER   = 4,
	ALIGN_CENTRE   = 4,
	VALIGN_TOP     = 8,
	VALIGN_BOTTOM  = 16,
	VALIGN_JUSTIFY = 24,
	VALIGN_CENTER  = 32,
	VALIGN_CENTRE  = 32,
	LR_TB          = 64,
	RL_TB          = 128,
	TB_LR          = 256,
	TB_RL          = 512
};

int     app_text_line_length(Font *f, int pixel_width, const char *utf8, int nb);
int     app_text_width(Font *f, int pixel_width, const char *utf8, int nbytes);
int     app_text_height(Font *f, int pixel_width, const char *utf8, int nbytes);
char *app_draw_text(Graphics *g, Rect r, int align, const char *utf8, int nbytes);


/*
 *  Fonts:
 */

enum FontStyle {
	PLAIN         = 0,
	BOLD          = 1,
	ITALIC        = 2,
	ANTI_ALIAS    = 8,
	PORTABLE_FONT = 16,
	NATIVE_FONT   = 32
};

Font *  app_new_font(App *app, const char *name, int style, int pixel_height);
void    app_del_font(Font *f);

int     app_font_height(Font *f);
int     app_font_width(Font *f, const char *utf8, int nbytes);

Font *  app_find_default_font(App *app);
void    app_change_default_font(const char *fontname);

Subfont *app_font_char_info(Font *f, unsigned long ch, int *width);


/*
 *  Cursors:
 */

enum StandardCursors {
	BLANK_CURSOR,
	ARROW_CURSOR,
	WAIT_CURSOR,
	CARET_CURSOR,
	CROSS_CURSOR,
	HAND_CURSOR,
	GRAB_CURSOR,
	POINTING_CURSOR,
	PENCIL_CURSOR,
	LASSO_CURSOR,
	DROPPER_CURSOR,
	MAGNIFY_CURSOR,
	MAGPLUS_CURSOR,
	MAGMINUS_CURSOR,
	SIZELR_CURSOR,
	SIZETB_CURSOR,
	TEXT_CURSOR      = CARET_CURSOR
};

void    app_find_best_cursor_size(App *app, int *width, int *height, int *depth);
Cursor *app_new_cursor(App *app, Image *img, Point hotspot);
Cursor *app_get_standard_cursor(App *app, int shape);
void    app_del_cursor(Cursor *c);
void    app_set_window_cursor(Window *win, Cursor *c);
void    app_set_window_temp_cursor(Window *win, Cursor *c);
Point   app_get_cursor_position(App *app);
void    app_set_cursor_position(App *app, Point p);

/*
 *  Images:
 */

Image *	app_new_image(int width, int height, int depth);
void	app_del_image(Image *img);

Rect	app_get_image_area(const Image *img);
Rect	app_get_image_rect(const Image *img);

Image *	app_copy_image(const Image *img);
void	app_set_image_cmap(Image *img, int cmap_size, Colour *cmap);

int 	app_image_has_transparent_pixels(const Image *img);

Image *	app_image_convert_32_to_8(const Image *img);
Image *	app_image_convert_8_to_32(const Image *img);
void	app_image_sort_palette(Image *img);
Image *	app_scale_image(const Image *src, Rect dr, Rect sr);

int 	app_draw_image(Graphics *g, Rect dr, Image *img, Rect sr);
int 	app_draw_image_monochrome(Graphics *g, Rect dr, Image *src, Rect sr);
int 	app_draw_image_greyscale(Graphics *g, Rect dr, Image *src, Rect sr);
int 	app_draw_image_darker(Graphics *g, Rect dr, Image *src, Rect sr);
int 	app_draw_image_brighter(Graphics *g, Rect dr, Image *src, Rect sr);

int 	app_image_find_colour(const Image *img, Colour col);

Bitmap *app_image_to_bitmap(Window *win, Image *img);
Image *	app_bitmap_to_image(Bitmap *bmp);


/*
 *  Image Lists:
 */

ImageList * app_new_image_list(void);
void	app_del_image_list(ImageList *imglist);
void	app_append_to_image_list(ImageList *imglist, Image *img);


/*
 *  Image Readers:
 */

enum ImageReaderState {
	STOPPED         = 0,    /* at start or end of processing */
	STARTING,               /* creating data structures */
	DITHERING,              /* now dithering to a palette */
	RENDERING,              /* processing lines of pixels */
	IMAGE_ERROR     = -1    /* an error has happened */
};

enum ImageFormat {
	PNG_FORMAT      = 1,    /* Portable Network Graphics */
	JPEG_FORMAT     = 2,    /* Joint Photographic Experts Group */
	GIF_FORMAT      = 3,    /* Graphic Interchange Format */
	GA_H_FORMAT     = 4,    /* GraphApp header image file format */
	UNKNOWN_FORMAT  = -1
};

ImageReader * app_new_image_reader(void);
void          app_del_image_reader(ImageReader *reader);

Image * app_read_image(const char *filename, int required_depth);
Image * app_read_image_file(FILE *file, int required_depth);
Image * app_read_image_memory(const byte *memsrc, int memsize, int required_depth);
Image * app_read_image_progressively(ImageReader *reader);
int     app_find_image_format(FILE *file);
int     app_find_image_format_in_memory(byte *memsrc, int memsize);


int     app_write_image(Image *img, const char *filename);
int     app_write_image_at(Image *img, const char *filename, int dpi, int interlace);


/*
 *  Palettes:
 */

Palette * app_new_palette(int size, Colour *elem);
void      app_del_palette(Palette *pal);

void      app_set_window_palette(Window *w, Palette *pal);
Palette * app_get_window_palette(Window *w);

byte *    app_palette_translation(Palette *target, byte *dest, int size, Colour *elem);


/*
 *  Points:
 */

#define pt(x,y)       app_new_point((x),(y))

Point app_new_point(int x, int y);
int   app_points_equal(Point p1, Point p2);
int   app_point_in_rect(Point p, Rect r);


/*
 *  Rectangles:
 */

#define rect(x,y,w,h) app_new_rect((x),(y),(w),(h))

Rect app_new_rect(int x, int y, int width, int height);
Rect app_center_rect(Rect r1, Rect r2);
Rect app_inset_rect(Rect r, int width);
int  app_rect_in_rect(Rect r1, Rect r2);
int  app_rect_intersects_rect(Rect r1, Rect r2);
int  app_rects_equal(Rect r1, Rect r2);
Rect app_clip_rect(Rect r1, Rect r2);
Rect app_rect_abs(Rect r);

int  app_clip_line_to_rect(Rect r, Point *p1, Point *p2);

/*
 *  Regions:
 */

Region * app_new_region(void);
void     app_del_region(Region *rgn);
Region * app_new_rect_region(Rect r);
Region * app_copy_region(Region *rgn);
void     app_move_region(Region *rgn, int dx, int dy);
int      app_union_region(Region *rgn1, Region *rgn2, Region *dest);
int      app_union_region_with_rect(Region *rgn, Rect r, Region *dest);
int      app_intersect_region(Region *rgn1, Region *rgn2, Region *dest);
int      app_intersect_region_with_rect(Region *rgn, Rect r, Region *dest);
int      app_subtract_region(Region *rgn1, Region *rgn2, Region *dest);
int      app_xor_region(Region *rgn1, Region *rgn2, Region *dest);
int      app_region_is_empty(Region *rgn);
int      app_regions_equal(Region *rgn1, Region *rgn2);
int      app_point_in_region(Point p, Region *rgn);
int      app_rect_intersects_region(Rect r, Region *rgn);
int      app_rect_in_region(Rect r, Region *rgn);


/*
 *  Colours:
 */

#define rgb(r,g,b)             app_new_rgb((r),(g),(b))
#define argb(a,r,g,b)          app_new_argb((a),(r),(g),(b))
#define app_new_color(r,g,b)   app_new_rgb((r),(g),(b))
#define app_new_colour(r,g,b)  app_new_rgb((r),(g),(b))

Colour	app_new_rgb(int r, int g, int b);
Colour	app_new_argb(int a, int r, int g, int b);

int 	app_rgbs_equal(Colour a, Colour b);
#define app_colors_equal(a,b)  app_rgbs_equal((a),(b))
#define app_colours_equal(a,b) app_rgbs_equal((a),(b))


/*
 *  Some standard colours:
 */

#define CLEAR           argb(0xFF,0xFF,0xFF,0xFF)

#define BLACK           rgb(0x00,0x00,0x00)
#define WHITE           rgb(0xFF,0xFF,0xFF)
#define BLUE            rgb(0x00,0x00,0xFF)
#define YELLOW          rgb(0xFF,0xFF,0x00)
#define GREEN           rgb(0x00,0xFF,0x00)
#define MAGENTA         rgb(0xFF,0x00,0xFF)
#define RED             rgb(0xFF,0x00,0x00)
#define CYAN            rgb(0x00,0xFF,0xFF)

#define GREY            rgb(0x80,0x80,0x80)
#define GRAY            rgb(0x80,0x80,0x80)
#define LIGHT_GREY      rgb(0xC0,0xC0,0xC0)
#define LIGHT_GRAY      rgb(0xC0,0xC0,0xC0)
#define PALE_GREY       rgb(0xE0,0xE0,0xE0)
#define PALE_GRAY       rgb(0xE0,0xE0,0xE0)
#define DARK_GREY       rgb(0x40,0x40,0x40)
#define DARK_GRAY       rgb(0x40,0x40,0x40)

#define DARK_BLUE       rgb(0x00,0x00,0x80)
#define DARK_GREEN      rgb(0x00,0x80,0x00)
#define DARK_RED        rgb(0x80,0x00,0x00)
#define LIGHT_BLUE      rgb(0x80,0xC0,0xFF)
#define LIGHT_GREEN     rgb(0x80,0xFF,0x80)
#define LIGHT_RED       rgb(0xFF,0xC0,0xFF)
#define PINK            rgb(0xFF,0xAF,0xAF)
#define BROWN           rgb(0x60,0x30,0x00)
#define ORANGE          rgb(0xFF,0x80,0x00)
#define PURPLE          rgb(0xC0,0x00,0xFF)
#define LIME            rgb(0x80,0xFF,0x00)


/*
 *  Strings:
 */

char *	app_copy_string(const char *str);
void	app_del_string(char *str);


/*
 *  String hash tables:
 */
StringTable * app_new_string_table(void);
void          app_del_string_table(StringTable *table);
void          app_traverse_string_table(StringTable *table,
			void (*func)(StringNode *, void *), void *data);
StringNode *  app_locate_node(StringTable *table,
			const char *key, unsigned long length);
StringNode *  app_insert_node(StringTable *table,
			const char *key, unsigned long length);
StringNode *  app_remove_node(StringTable *table,
			const char *key, unsigned long length);


/*
 *  Resources:
 */

FILE *	app_open_resource(const char *file_name, const char *resource, long *length);
int 	app_file_has_resources(const char *file_name);


/*
 *  UTF-8 and ISO-Latin-1:
 */

#define IS_UTF8_EXTRA_BYTE(b) (((b)&0xC0)==0x80)

/*
 * Both of these routines converts the text between *sourceStart
 * and sourceEnd, putting the result into the buffer between *targetStart
 * and targetEnd. Note: the end pointers are *after* the last item:
 * i.e. *(sourceEnd - 1) is the last item.
 *
 * The return result indicates whether the conversion was successful,
 * and if not, whether the problem was in the source or target buffers.
 *
 * After the conversion, *sourceStart and *targetStart are both
 * updated to point to the end of last text successfully converted in
 * the respective buffers.
 */

typedef enum UTF8ConversionResult {
	ConversionSuccess = 0, 	/* conversion successful */
	SourceExhausted = 1,	/* partial char in source, but hit end */
	SourceCorrupt = 2,	/* corrupted character in source */
	TargetExhausted = 4	/* no room in target for conversion */
} ConversionResult;		/* bit-field holding conversion result */

int app_unicode_to_utf8(const Char ** source_start, const Char * source_end,
			      char ** target_start, const char * target_end);

int app_utf8_to_unicode(const char ** source_start, const char * source_end,
			      Char ** target_start, const Char * target_end);

int 	app_unicode_char_to_utf8(Char ch, char *utf8);
Char	app_utf8_char_to_unicode(const char *utf8);
int 	app_utf8_length(const char *utf8);

/*
 *  Transfer Unicode value to/from UTF-8 or ISO-Latin-1 files.
 *  The file must be open in binary mode.
 */

int	app_read_utf8_char (FILE *f, Char *dst);

char *	app_read_latin1_file (FILE *f, long *nbytes, long *nchars);
char *	app_read_utf8_file (FILE *f, long *nbytes, long *nchars);

char *	app_read_latin1_line (FILE *f, long *nbytes, long *nchars);
char *	app_read_utf8_line (FILE *f, long *nbytes, long *nchars);

int	app_write_latin1 (FILE *f, const char *s, long nbytes);
int	app_write_utf8 (FILE *f, const char *s, long nbytes);

char *	app_utf8_to_latin1 (const char *s, int *bytes);
char *	app_correct_utf8 (const char *s, int *bytes);

int 	app_utf8_is_ascii(const char *utf8, long nbytes);
int 	app_utf8_is_latin1(const char *utf8, long nbytes);

/*
 *  UTF-8 regular expressions:
 */

int 	app_regex_match(const char *regexp, const char *text);
