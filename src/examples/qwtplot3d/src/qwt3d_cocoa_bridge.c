/*
  What the Cocoa OpenGL view needs from inside GraphApp. The only file that sees GraphApp's
  private headers, and it knows nothing of the library.

  The plot is an NSOpenGLView of its own, so it takes its own mouse and wheel events and
  GraphApp never sees them. It does not take key events: they go to GraphApp's window as
  before, where the menu shortcuts and key handlers are, and the arrow keys are passed on
  from the window's key action.

  GraphApp draws its pull-down menus into the window's own picture, under any view placed
  over it. So the plot steps aside for as long as a menu is open: the menu bar's press
  handler stays in GraphApp until the menu closes. Before it hides, the plot's last frame is
  written into that picture, so the menu opens over the plot and not over a blank.
*/

#include "appint.h"
#include "qwt3d_cocoa_bridge.h"

#define MAX_PLOTS 8

typedef struct Plot
{
  Window* parent;
  void* user;
  QwtcCallbacks calls;
} Plot;

static Plot plots[MAX_PLOTS];
static int numplots = 0;

void* qwtc_nswindow(Window* parent)
{
  return win_extra(parent)->handle;
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

void qwtc_freeze(Window* parent)
{
  int i;
  for (i = 0; i < numplots; i++)
    if (plots[i].parent == parent)
      plots[i].calls.freeze(plots[i].user);
}

void qwtc_thaw(Window* parent)
{
  int i;
  for (i = 0; i < numplots; i++)
    if (plots[i].parent == parent)
      plots[i].calls.thaw(plots[i].user);
}

static MouseFunc menuPress = NULL;

static void menuBarPress(Control* c, int buttons, Point xy)
{
  Window* parent = app_parent_window(c);
  qwtc_freeze(parent);
  menuPress(c, buttons, xy);
  qwtc_thaw(parent);
}

/* (done at show time too: the menu bar may come after the plot) */
void qwtc_hook_menu_bar(Window* parent)
{
  Control* bar = parent->menubar ? parent->menubar->ctrl : NULL;
  if (bar && bar->mouse_down && bar->mouse_down[0] != menuBarPress)
  {
    menuPress = bar->mouse_down[0];
    bar->mouse_down[0] = menuBarPress;
  }
}

int qwtc_register(Window* parent, QwtcCallbacks const* calls, void* user)
{
  int i, already = 0;
  Plot* p;

  if (numplots == MAX_PLOTS)
    return 0;
  for (i = 0; i < numplots; i++)
    if (plots[i].parent == parent)
      already = 1;
  p = &plots[numplots++];
  p->parent = parent;
  p->user = user;
  p->calls = *calls;
  if (!already)
    app_on_window_key_action(parent, parentKeyAction);
  qwtc_hook_menu_bar(parent);
  return 1;
}

void qwtc_unregister(void* user)
{
  int i;
  for (i = 0; i < numplots; i++)
    if (plots[i].user == user)
    {
      plots[i] = plots[--numplots];
      break;
    }
}

void qwtc_clear_key_focus(Window* parent)
{
  parent->key_focus = NULL;
}

void qwtc_paste(Window* parent, int x, int y, int width, int height, unsigned char const* rgba)
{
  Surface* s = &win_extra(parent)->surf;
  int row, col;

  for (row = 0; row < height; row++)
  {
    int sy = y + row;
    if (sy < 0 || sy >= s->height)
      continue;
    for (col = 0; col < width; col++)
    {
      int sx = x + col;
      unsigned char const* p = rgba + ((size_t)row * width + col) * 4;
      if (sx < 0 || sx >= s->width)
        continue;
      s->pixels[(size_t)sy * s->width + sx] =
        ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
    }
  }
  app_surface_mark_dirty(s, rect(x, y, width, height));
}

void qwtc_flush(Window* parent)
{
  app_flush_window(parent);
}
