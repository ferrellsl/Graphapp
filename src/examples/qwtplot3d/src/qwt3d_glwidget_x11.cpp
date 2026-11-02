// The OpenGL child window on X11. The only file that sees Xlib and GLX; what it needs from
// GraphApp's insides is in qwt3d_x11_bridge.c.

#ifndef _WIN32

// Xlib's names for these are also GraphApp's and the library's
#define Window X_Window
#define Font X_Font
#define Cursor X_Cursor
#define Region X_Region
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#undef Window
#undef Font
#undef Cursor
#undef Region

namespace
{
  const long xNone = None;
  const int glxEnd = None;
  const int xSuccess = Success;
}
#undef None
#undef Bool
#undef Status
#undef True
#undef False
#undef Success
#undef Always
#undef CursorShape
#undef KeyPress
#undef KeyRelease

#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qwt3d_glwidget.h"
#include "qwt3d_x11_bridge.h"

using namespace Qwt3D;

namespace
{
  Display* display = 0; // the same connection as GraphApp's

  struct Native
  {
    ::Window* parent;
    ::Window* child;
    unsigned long xparent;
    X_Window xchild;
    GLXContext context;
    bool frozen;
  };

  Native* nativeOf(void* p)
  {
    return (Native*)p;
  }

  int modifiersOf(unsigned mask)
  {
    int m = NoModifier;
    if (mask & ShiftMask)
      m |= ShiftModifier;
    if (mask & ControlMask)
      m |= ControlModifier;
    if (mask & Mod1Mask)
      m |= AltModifier;
    return m;
  }

  int buttonsOf(unsigned mask)
  {
    int b = NoButton;
    if (mask & Button1Mask)
      b |= LeftButton;
    if (mask & Button3Mask)
      b |= RightButton;
    if (mask & Button2Mask)
      b |= MidButton;
    return b;
  }
}

namespace Qwt3D
{

struct GLWidgetEvents
{
  static GLWidget* widget(void* user)
  {
    return (GLWidget*)user;
  }

  static void press(void* user, int x, int y)
  {
    widget(user)->mousePressEvent(PixelPoint(x, y));
  }

  static void release(void* user, int x, int y)
  {
    widget(user)->mouseReleaseEvent(PixelPoint(x, y));
  }

  // GraphApp turns Shift + left into the right button, so X is asked what is really held
  static void move(void* user, int x, int y, int graphappButtons)
  {
    GLWidget* w = widget(user);
    Native* n = nativeOf(w->hwnd_);

    // one redraw for all that is waiting: a large plot must not fall behind the mouse
    XEvent next;
    while (XCheckTypedWindowEvent(display, n->xchild, MotionNotify, &next))
    {
      x = next.xmotion.x;
      y = next.xmotion.y;
    }

    X_Window root, inner;
    int rx, ry, cx, cy;
    unsigned mask = 0;
    XQueryPointer(display, n->xchild, &root, &inner, &rx, &ry, &cx, &cy, &mask);
    int buttons = buttonsOf(mask);
    int modifiers = modifiersOf(mask);
    if (!buttons && graphappButtons)
    {
      // the button came up after this event was sent: go by what GraphApp saw
      buttons = LeftButton;
      if (graphappButtons & 2)
        modifiers |= ControlModifier;
      if (graphappButtons & 4)
        modifiers |= ShiftModifier;
    }
    w->mouseMoveEvent(PixelPoint(x, y), MouseState(buttons, modifiers));
  }

  static void wheel(void* user, int delta, int shift, int control)
  {
    int m = NoModifier;
    if (shift)
      m |= ShiftModifier;
    if (control)
      m |= ControlModifier;
    widget(user)->wheelEvent(delta, m);
  }

  static void expose(void* user)
  {
    GLWidget* w = widget(user);
    if (!nativeOf(w->hwnd_)->frozen)
      w->updateGL();
  }

  static void arrow(void* user, int which, int shift, int control)
  {
    static const int keys[] = {Key_unknown, Key_Left, Key_Right, Key_Up, Key_Down};
    int m = NoModifier;
    if (shift)
      m |= ShiftModifier;
    if (control)
      m |= ControlModifier;
    widget(user)->keyPressEvent(KeyboardState(keys[which], m));
  }

  // Leaves a picture of the plot on the parent window in place of the child window.
  // An expose of the parent must not get through: it would close GraphApp's menu.
  static void freeze(void* user)
  {
    GLWidget* w = widget(user);
    Native* n = nativeOf(w->hwnd_);
    if (n->frozen || !w->isVisible())
      return;

    std::vector<unsigned char> rgba;
    int width, height;
    if (!w->grabFrameBuffer(rgba, width, height))
      return;

    X_Window root, inner;
    int x, y;
    unsigned cw, ch, border, depth;
    XGetGeometry(display, n->xchild, &root, &x, &y, &cw, &ch, &border, &depth);

    n->frozen = true;
    XUnmapWindow(display, n->xchild);
    XSync(display, 0);
    XEvent drop;
    while (XCheckTypedWindowEvent(display, (X_Window)n->xparent, Expose, &drop))
      ;

    int screen = DefaultScreen(display);
    Visual* visual = DefaultVisual(display, screen);
    int bits = DefaultDepth(display, screen);
    XImage* image = XCreateImage(display, visual, bits, ZPixmap, 0, 0, width, height, 32, 0);
    if (!image)
      return;
    image->data = (char*)malloc(image->bytes_per_line * height);
    for (int row = 0; row != height; ++row)
    {
      unsigned char const* p = &rgba[(height - 1 - row) * width * 4];
      for (int col = 0; col != width; ++col, p += 4)
        XPutPixel(image, col, row,
                  ((p[0] << 16) & visual->red_mask) | ((p[1] << 8) & visual->green_mask)
                  | (p[2] & visual->blue_mask));
    }
    GC gc = XCreateGC(display, (X_Window)n->xparent, 0, 0);
    XPutImage(display, (X_Window)n->xparent, gc, image, 0, 0, x, y, width, height);
    XFreeGC(display, gc);
    XDestroyImage(image);
    XSync(display, 0);
  }

  static void thaw(void* user)
  {
    GLWidget* w = widget(user);
    Native* n = nativeOf(w->hwnd_);
    if (!n->frozen)
      return;
    n->frozen = false;
    if (w->isVisible())
      XMapWindow(display, n->xchild);
    XSync(display, 0);
  }
};

} // ns

namespace
{
  // X's own fonts: the iso10646 ones hold the characters GraphApp's native fonts lack
  XFontStruct* unicodeFont(int pixelHeight, bool bold)
  {
    static const char* const formats[] = {
      "-*-*-%s-r-normal-*-%d-*-*-*-*-*-iso10646-1",
      "-misc-fixed-%s-r-normal--%d-*-*-*-*-*-iso10646-1"
    };
    static const int sizes[] = {0, 1, -1, 2, -2, 3, -3, 4, -4, 5, -5, 6, -6};
    for (size_t s = 0; s != sizeof(sizes) / sizeof(sizes[0]); ++s)
      for (size_t f = 0; f != sizeof(formats) / sizeof(formats[0]); ++f)
      {
        char name[128];
        snprintf(name, sizeof(name), formats[f], bold ? "bold" : "medium", pixelHeight + sizes[s]);
        if (XFontStruct* font = XLoadQueryFont(display, name))
          return font;
      }
    return 0;
  }

  std::vector<XChar2b> utf8ToChar2b(std::string const& text)
  {
    std::vector<XChar2b> out;
    for (size_t i = 0; i < text.size();)
    {
      unsigned c = (unsigned char)text[i];
      unsigned code = c;
      int extra = 0;
      if (c >= 0xF0) { code = c & 0x07; extra = 3; }
      else if (c >= 0xE0) { code = c & 0x0F; extra = 2; }
      else if (c >= 0xC0) { code = c & 0x1F; extra = 1; }
      ++i;
      for (; extra && i < text.size(); --extra, ++i)
        code = (code << 6) | ((unsigned char)text[i] & 0x3F);
      XChar2b ch;
      ch.byte1 = (unsigned char)(code >> 8);
      ch.byte2 = (unsigned char)code;
      out.push_back(ch);
    }
    return out;
  }
}

bool Qwt3D::GraphAppHost::renderUnicodeText(std::string const& text, std::string const&,
                                            int pixelHeight, bool bold, bool,
                                            double r, double g, double b,
                                            std::vector<unsigned char>& rgba, int& width, int& height)
{
  if (!display)
    return false;
  XFontStruct* font = unicodeFont(pixelHeight, bold);
  if (!font)
    return false;

  std::vector<XChar2b> chars = utf8ToChar2b(text);
  width = XTextWidth16(font, &chars[0], (int)chars.size());
  height = font->ascent + font->descent;
  bool ok = width > 0 && height > 0;
  if (ok)
  {
    int screen = DefaultScreen(display);
    Pixmap pix = XCreatePixmap(display, RootWindow(display, screen), width, height,
                               DefaultDepth(display, screen));
    GC gc = XCreateGC(display, pix, 0, 0);
    XSetFont(display, gc, font->fid);
    XSetForeground(display, gc, BlackPixel(display, screen));
    XFillRectangle(display, pix, gc, 0, 0, width, height);
    XSetForeground(display, gc, WhitePixel(display, screen));
    XDrawString16(display, pix, gc, 0, font->ascent, &chars[0], (int)chars.size());
    XImage* image = XGetImage(display, pix, 0, 0, width, height, AllPlanes, ZPixmap);
    ok = image != 0;
    if (ok)
    {
      rgba.resize(width * height * 4);
      unsigned long white = WhitePixel(display, screen);
      for (int y = 0; y != height; ++y)
      {
        unsigned char* p = &rgba[(height - 1 - y) * width * 4];
        for (int x = 0; x != width; ++x, p += 4)
        {
          p[0] = (unsigned char)(r * 255 + 0.5);
          p[1] = (unsigned char)(g * 255 + 0.5);
          p[2] = (unsigned char)(b * 255 + 0.5);
          p[3] = XGetPixel(image, x, y) == white ? 255 : 0;
        }
      }
      XDestroyImage(image);
    }
    XFreeGC(display, gc);
    XFreePixmap(display, pix);
  }
  XFreeFont(display, font);
  return ok;
}

GLWidget::GLWidget(::Window* parent, int x, int y, int width, int height)
  : hwnd_(0), hdc_(0), hglrc_(0), width_(width), height_(height),
    visible_(false), initialized_(false), resized_(true)
{
  GraphAppHost::nativeWindow(parent); // registers the application
  display = (Display*)qwtx_display(parent);
  int screen = DefaultScreen(display);

  int attributes[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
                      GLX_BLUE_SIZE, 8, GLX_DEPTH_SIZE, 24, glxEnd};
  XVisualInfo* visual = glXChooseVisual(display, screen, attributes);
  assert(visual);

  XSetWindowAttributes wa;
  memset(&wa, 0, sizeof(wa));
  wa.colormap = XCreateColormap(display, RootWindow(display, screen), visual->visual, AllocNone);
  wa.background_pixmap = xNone;
  wa.border_pixel = 0;

  Native* n = new Native;
  n->parent = parent;
  n->xparent = qwtx_xid(parent);
  n->frozen = false;
  n->xchild = XCreateWindow(display, (X_Window)n->xparent, x, y, width, height, 0, visual->depth,
                            InputOutput, visual->visual,
                            CWColormap | CWBackPixmap | CWBorderPixel, &wa);
  n->context = glXCreateContext(display, visual, 0, 1);
  assert(n->context);
  XFree(visual);
  hwnd_ = n;

  static QwtxCallbacks const calls = {
    GLWidgetEvents::press, GLWidgetEvents::release, GLWidgetEvents::move, GLWidgetEvents::wheel,
    GLWidgetEvents::expose, GLWidgetEvents::arrow, GLWidgetEvents::freeze, GLWidgetEvents::thaw
  };
  n->child = qwtx_new_child(parent, n->xchild, &calls, this);
  assert(n->child);
  makeCurrent();
}

std::string GLWidget::askOpenFile(std::string const& title, std::string const& filter)
{
  // a menu command runs while the plot is stepped aside for the menu: bring it back
  qwtx_thaw(nativeOf(hwnd_)->parent);
  return GraphAppHost::askFile(title, filter, "", false);
}

std::string GLWidget::askSaveFile(std::string const& title, std::string const& filter,
                                  std::string const& name)
{
  qwtx_thaw(nativeOf(hwnd_)->parent);
  return GraphAppHost::askFile(title, filter, name, true);
}

// X has no color or font dialog, and GraphApp neither: a line of text
bool GLWidget::askColor(std::string const& title, unsigned char rgb[3])
{
  qwtx_thaw(nativeOf(hwnd_)->parent);
  std::string answer = GraphAppHost::askString(title, "Red, green and blue, 0 to 255", "255 255 255");
  int r, g, b;
  if (sscanf(answer.c_str(), "%d %d %d", &r, &g, &b) != 3
      || r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255)
    return false;
  rgb[0] = (unsigned char)r;
  rgb[1] = (unsigned char)g;
  rgb[2] = (unsigned char)b;
  return true;
}

bool GLWidget::askFont(Font& font)
{
  qwtx_thaw(nativeOf(hwnd_)->parent);

  char start[128];
  snprintf(start, sizeof(start), "%s %d%s%s", font.family().c_str(), font.pointSize(),
           font.weight() >= Font::Bold ? " bold" : "", font.italic() ? " italic" : "");
  std::string answer = GraphAppHost::askString("Font", "Family, size, and bold or italic if wanted", start);

  std::string family;
  int size = font.pointSize();
  bool bold = false, italic = false;
  char const* p = answer.c_str();
  while (*p)
  {
    while (*p == ' ')
      ++p;
    char const* end = p;
    while (*end && *end != ' ')
      ++end;
    std::string word(p, end);
    p = end;
    if (word == "bold")
      bold = true;
    else if (word == "italic")
      italic = true;
    else if (!word.empty() && atoi(word.c_str()) > 0)
      size = atoi(word.c_str());
    else if (!word.empty())
      family += (family.empty() ? "" : " ") + word;
  }
  if (family.empty())
    return false;
  font = Font(family, size, bold ? Font::Bold : Font::Normal, italic);
  return true;
}

void GLWidget::tellUser(std::string const& title, std::string const& message)
{
  qwtx_thaw(nativeOf(hwnd_)->parent);
  GraphAppHost::tell(title, message);
}

GLWidget::~GLWidget()
{
  Native* n = nativeOf(hwnd_);
  glXMakeCurrent(display, 0, 0);
  glXDestroyContext(display, n->context);
  qwtx_del_child(n->child);
  delete n;
}

void GLWidget::show()
{
  visible_ = true;
  Native* n = nativeOf(hwnd_);
  qwtx_hook_menu_bar(n->parent);
  XMapWindow(display, n->xchild);
  XSync(display, 0);
  updateGL();
}

void GLWidget::hide()
{
  visible_ = false;
  XUnmapWindow(display, nativeOf(hwnd_)->xchild);
}

void GLWidget::setGeometry(int x, int y, int width, int height)
{
  width_ = width;
  height_ = height;
  resized_ = true;
  XMoveResizeWindow(display, nativeOf(hwnd_)->xchild, x, y, width, height);
  XSync(display, 0);
  updateGL();
}

void GLWidget::setFocus()
{
  qwtx_clear_key_focus(nativeOf(hwnd_)->parent);
}

void GLWidget::makeCurrent()
{
  Native* n = nativeOf(hwnd_);
  glXMakeCurrent(display, n->xchild, n->context);
}

void GLWidget::prepare()
{
  makeCurrent();
  if (!initialized_)
  {
    initialized_ = true;
    initializeGL();
  }
  if (resized_)
  {
    resized_ = false;
    resizeGL(width_, height_);
  }
}

void GLWidget::updateGL()
{
  if (!visible_)
    return;
  prepare();
  paintGL();
  glXSwapBuffers(display, nativeOf(hwnd_)->xchild);
}

bool GLWidget::grabFrameBuffer(std::vector<unsigned char>& rgba, int& width, int& height)
{
  prepare();
  paintGL();
  while (glGetError() != GL_NO_ERROR)
    ; // only the read below decides
  width = width_;
  height = height_;
  rgba.resize(width * height * 4);
  if (rgba.empty())
    return false;
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &rgba[0]);
  return glGetError() == GL_NO_ERROR;
}

#endif // _WIN32
