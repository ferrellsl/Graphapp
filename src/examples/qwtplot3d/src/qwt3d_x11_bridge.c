/*
  What the X11 OpenGL child window needs from inside GraphApp. The only file that sees
  GraphApp's private headers, and it knows nothing of the library.

  GraphApp reads the X events itself and drops those of windows it does not know, so the
  child window is made a GraphApp window: an ordinary one is created and its X window
  swapped for ours. It is never shown with app_show_window, so the main loop does not
  wait for it. It takes no key events: X passes those up to the main window, where the
  menu shortcuts and key handlers are.
*/

#include "appint.h"
#include "qwt3d_x11_bridge.h"

#define MAX_PLOTS 8

typedef struct Plot
{
  Window* parent;
  Window* child;
  void* user;
  QwtxCallbacks calls;
} Plot;

static Plot plots[MAX_PLOTS];
static int numplots = 0;

static Plot* plotOfChild(Window* child)
{
  int i;
  for (i = 0; i < numplots; i++)
    if (plots[i].child == child)
      return &plots[i];
  return NULL;
}

static void childPress(Window* w, int buttons, Point xy)
{
  Plot* p = plotOfChild(w);
  if (p)
  {
    p->parent->key_focus = NULL; /* arrow keys are the plot's now */
    p->calls.press(p->user, xy.x, xy.y);
  }
}

static void childRelease(Window* w, int buttons, Point xy)
{
  Plot* p = plotOfChild(w);
  if (p)
    p->calls.release(p->user, xy.x, xy.y);
}

static void childMove(Window* w, int buttons, Point xy)
{
  Plot* p = plotOfChild(w);
  if (p)
    p->calls.move(p->user, xy.x, xy.y, buttons);
}

static void childWheel(Window* w, int buttons, Point xy, int dx, int dy)
{
  Plot* p = plotOfChild(w);
  if (p && dy)
    /* three lines to the notch; the plot counts 120 per notch, up positive */
    p->calls.wheel(p->user, -dy * 40, (buttons & SHIFT) != 0, (buttons & CONTROL) != 0);
}

static void childRedraw(Window* w, Graphics* g)
{
  Plot* p = plotOfChild(w);
  if (p)
    p->calls.expose(p->user);
}

static void parentKeyAction(Window* w, unsigned long key)
{
  int i, arrow;
  switch (key & ~(unsigned long)(SHIFT | CONTROL))
  {
    case LEFT:  arrow = 1; break;
    case RIGHT: arrow = 2; break;
    case UP:    arrow = 3; break;
    case DOWN:  arrow = 4; break;
    default:    return;
  }
  for (i = 0; i < numplots; i++)
    if (plots[i].parent == w)
      plots[i].calls.arrow(plots[i].user, arrow, (key & SHIFT) != 0, (key & CONTROL) != 0);
}

/*
  GraphApp draws its pull-down menus straight on the window, which cannot draw over an
  OpenGL child window. So the child steps aside for as long as a menu is open: the menu
  bar's press handler stays in GraphApp until the menu closes.
*/
static MouseFunc menuPress = NULL;

void qwtx_freeze(Window* parent)
{
  int i;
  for (i = 0; i < numplots; i++)
    if (plots[i].parent == parent)
      plots[i].calls.freeze(plots[i].user);
}

void qwtx_thaw(Window* parent)
{
  int i;
  for (i = 0; i < numplots; i++)
    if (plots[i].parent == parent)
      plots[i].calls.thaw(plots[i].user);
}

static void menuBarPress(Control* c, int buttons, Point xy)
{
  Window* parent = app_parent_window(c);
  qwtx_freeze(parent);
  menuPress(c, buttons, xy);
  qwtx_thaw(parent);
}

/* (done at show time too: the menu bar may come after the plot) */
void qwtx_hook_menu_bar(Window* parent)
{
  Control* bar = parent->menubar ? parent->menubar->ctrl : NULL;
  if (bar && bar->mouse_down && bar->mouse_down[0] != menuBarPress)
  {
    menuPress = bar->mouse_down[0];
    bar->mouse_down[0] = menuBarPress;
  }
}

void* qwtx_display(Window* w)
{
  return app_extra(w->app)->display;
}

unsigned long qwtx_xid(Window* w)
{
  return win_extra(w)->xid;
}

Window* qwtx_new_child(Window* parent, unsigned long xid, QwtxCallbacks const* calls, void* user)
{
  Display* disp = app_extra(parent->app)->display;
  Window* child;
  Plot* p;

  if (numplots == MAX_PLOTS)
    return NULL;

  child = app_new_window(parent->app, rect(0, 0, 16, 16), "", 0);
  if (!child)
    return NULL;
  XDestroyWindow(disp, win_extra(child)->xid);
  win_extra(child)->xid = xid;
  XSelectInput(disp, xid, ButtonPressMask | ButtonReleaseMask | PointerMotionMask
               | ButtonMotionMask | ExposureMask | StructureNotifyMask);

  p = &plots[numplots++];
  p->parent = parent;
  p->child = child;
  p->user = user;
  p->calls = *calls;

  app_on_window_mouse_down(child, childPress);
  app_on_window_mouse_up(child, childRelease);
  app_on_window_mouse_move(child, childMove);
  app_on_window_mouse_drag(child, childMove);
  app_on_window_mouse_wheel(child, childWheel);
  app_on_window_redraw(child, childRedraw);
  app_on_window_key_action(parent, parentKeyAction);
  qwtx_hook_menu_bar(parent);
  return child;
}

void qwtx_del_child(Window* child)
{
  int i;
  for (i = 0; i < numplots; i++)
    if (plots[i].child == child)
    {
      plots[i] = plots[--numplots];
      break;
    }
  app_del_window(child);
}

void qwtx_clear_key_focus(Window* parent)
{
  parent->key_focus = NULL;
}
