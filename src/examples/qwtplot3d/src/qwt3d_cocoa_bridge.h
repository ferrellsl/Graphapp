#ifndef qwt3d_cocoa_bridge_h__2026_11_01_begin_guarded_code
#define qwt3d_cocoa_bridge_h__2026_11_01_begin_guarded_code

// Between the Cocoa OpenGL view and GraphApp's private parts (qwt3d_cocoa_bridge.c).
// Plain C and no Apple or GraphApp types, so either side can include it.

#ifdef __cplusplus
extern "C" {
#endif

struct Window; // GraphApp's

typedef struct QwtcCallbacks
{
  void (*arrow)(void* user, int which, int shift, int control); // left, right, up, down: 1 to 4
  void (*freeze)(void* user);
  void (*thaw)(void* user);
} QwtcCallbacks;

// The parent's NSWindow, as a plain pointer (not retained)
void* qwtc_nswindow(struct Window* parent);

// Starts serving a plot on this window: its arrow keys, and stepping aside for menus
int qwtc_register(struct Window* parent, QwtcCallbacks const* calls, void* user);
void qwtc_unregister(void* user);
void qwtc_hook_menu_bar(struct Window* parent);

void qwtc_freeze(struct Window* parent);
void qwtc_thaw(struct Window* parent);
void qwtc_clear_key_focus(struct Window* parent);

// Writes RGBA rows, top row first, into the window's picture (points, not backing pixels)
// at x, y, and shows it
void qwtc_paste(struct Window* parent, int x, int y, int width, int height,
                unsigned char const* rgba);
void qwtc_flush(struct Window* parent);

#ifdef __cplusplus
}
#endif

#endif
