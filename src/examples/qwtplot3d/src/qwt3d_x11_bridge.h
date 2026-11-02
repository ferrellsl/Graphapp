#ifndef qwt3d_x11_bridge_h__2026_11_01_begin_guarded_code
#define qwt3d_x11_bridge_h__2026_11_01_begin_guarded_code

// Between the X11 OpenGL child window and GraphApp's private parts (qwt3d_x11_bridge.c).

#ifdef __cplusplus
extern "C" {
#endif

struct Window; // GraphApp's

typedef struct QwtxCallbacks
{
  void (*press)(void* user, int x, int y);
  void (*release)(void* user, int x, int y);
  void (*move)(void* user, int x, int y, int buttons); // GraphApp's: 1 left, 2 middle or Ctrl+left, 4 right or Shift+left
  void (*wheel)(void* user, int delta, int shift, int control);
  void (*expose)(void* user);
  void (*arrow)(void* user, int which, int shift, int control); // left, right, up, down: 1 to 4
  void (*freeze)(void* user);
  void (*thaw)(void* user);
} QwtxCallbacks;

void* qwtx_display(struct Window* w);
unsigned long qwtx_xid(struct Window* w);
struct Window* qwtx_new_child(struct Window* parent, unsigned long xid,
                              QwtxCallbacks const* calls, void* user);
void qwtx_del_child(struct Window* child);
void qwtx_hook_menu_bar(struct Window* parent);
void qwtx_freeze(struct Window* parent);
void qwtx_thaw(struct Window* parent);
void qwtx_clear_key_focus(struct Window* parent);

#ifdef __cplusplus
}
#endif

#endif
