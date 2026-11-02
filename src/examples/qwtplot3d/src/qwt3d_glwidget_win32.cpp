// The OpenGL child window. The only file that sees windows.h and WGL.

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
  #define NOMINMAX
#endif
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <GL/gl.h>
#include <string.h>

#include "qwt3d_glwidget.h"

using namespace Qwt3D;

namespace
{
  const char* const className = "Qwt3DGLWidget";

  int modifiers()
  {
    int m = NoModifier;
    if (GetKeyState(VK_SHIFT) < 0)
      m |= ShiftModifier;
    if (GetKeyState(VK_CONTROL) < 0)
      m |= ControlModifier;
    if (GetKeyState(VK_MENU) < 0)
      m |= AltModifier;
    return m;
  }

  // mouse messages carry Shift and Ctrl themselves; only Alt must be asked for
  int mouseModifiers(WPARAM keys)
  {
    int m = NoModifier;
    if (keys & MK_SHIFT)
      m |= ShiftModifier;
    if (keys & MK_CONTROL)
      m |= ControlModifier;
    if (GetKeyState(VK_MENU) < 0)
      m |= AltModifier;
    return m;
  }

  int mouseButtons(WPARAM wp)
  {
    int b = NoButton;
    if (wp & MK_LBUTTON)
      b |= LeftButton;
    if (wp & MK_RBUTTON)
      b |= RightButton;
    if (wp & MK_MBUTTON)
      b |= MidButton;
    return b;
  }

  int translateKey(WPARAM vk)
  {
    switch (vk)
    {
      case VK_LEFT:  return Key_Left;
      case VK_RIGHT: return Key_Right;
      case VK_UP:    return Key_Up;
      case VK_DOWN:  return Key_Down;
      default:       return Key_unknown;
    }
  }

  const char* const parentProcProp = "Qwt3DParentProc";
  const char* const parentWindowProp = "Qwt3DParentWindow";

  GLWidget* plotOf(HWND child)
  {
    char name[32];
    if (!GetClassNameA(child, name, sizeof(name)) || strcmp(name, className) != 0)
      return 0;
    return (GLWidget*)GetWindowLongPtr(child, GWLP_USERDATA);
  }

  // Leaves a picture of the plot on the parent window in place of the child window.
  // No redraw of the parent may result: a WM_PAINT would close GraphApp's menu.
  BOOL CALLBACK freezePlot(HWND child, LPARAM)
  {
    GLWidget* w = plotOf(child);
    if (!w || !w->isVisible())
      return TRUE;

    std::vector<unsigned char> rgba;
    int width, height;
    if (!w->grabFrameBuffer(rgba, width, height))
      return TRUE;
    for (size_t i = 0; i + 3 < rgba.size(); i += 4)
    {
      unsigned char red = rgba[i];
      rgba[i] = rgba[i+2];
      rgba[i+2] = red;
    }

    HWND parent = GetParent(child);
    RECT r;
    GetWindowRect(child, &r);
    MapWindowPoints(0, parent, (POINT*)&r, 2);
    SetWindowPos(child, 0, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOREDRAW | SWP_NOMOVE
                 | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height; // bottom row first
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    HDC dc = GetDC(parent);
    SetDIBitsToDevice(dc, r.left, r.top, width, height, 0, 0, 0, height,
                      &rgba[0], &bmi, DIB_RGB_COLORS);
    ReleaseDC(parent, dc);
    ValidateRect(parent, &r);
    return TRUE;
  }

  BOOL CALLBACK thawPlot(HWND child, LPARAM)
  {
    GLWidget* w = plotOf(child);
    if (w && w->isVisible())
    {
      SetWindowPos(child, 0, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE
                   | SWP_NOZORDER | SWP_NOACTIVATE);
      InvalidateRect(child, 0, FALSE);
    }
    return TRUE;
  }

  /*
    GraphApp draws its pull-down menus straight on the window with GDI, which cannot
    draw over an OpenGL child window. So the child steps aside while a menu is open:
    presses on the menu bar and Alt keys are what open one, and GraphApp stays in the
    handler until it closes.

    GraphApp's window procedure ignores a window whose GWLP_WNDPROC is not itself, so
    it has to be put back for the length of each call.
  */
  LRESULT CALLBACK parentProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
  {
    WNDPROC original = (WNDPROC)GetPropA(hwnd, parentProcProp);
    SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)original);
    if (msg == WM_NCDESTROY)
    {
      RemovePropA(hwnd, parentProcProp);
      RemovePropA(hwnd, parentWindowProp);
      return CallWindowProc(original, hwnd, msg, wp, lp);
    }

    bool menu = msg == WM_SYSCHAR || (msg == WM_LBUTTONDOWN
      && GraphAppHost::inMenuBar((Window*)GetPropA(hwnd, parentWindowProp), GET_X_LPARAM(lp), GET_Y_LPARAM(lp)));
    HWND focus = GetFocus();
    if (menu)
      EnumChildWindows(hwnd, freezePlot, 0);
    LRESULT res = CallWindowProc(original, hwnd, msg, wp, lp);
    if (IsWindow(hwnd))
    {
      if (menu)
      {
        EnumChildWindows(hwnd, thawPlot, 0);
        if (IsWindow(focus))
          SetFocus(focus);
      }
      SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)parentProc);
    }
    return res;
  }
}

namespace Qwt3D
{

struct GLWidgetEvents
{
  static LRESULT CALLBACK proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
  {
    GLWidget* w = (GLWidget*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!w)
      return DefWindowProc(hwnd, msg, wp, lp);

    switch (msg)
    {
      case WM_ERASEBKGND:
        return 1;

      case WM_PAINT:
      {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        w->updateGL();
        return 0;
      }

      case WM_LBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_RBUTTONDOWN:
        SetFocus(hwnd);
        SetCapture(hwnd);
        w->mousePressEvent(PixelPoint(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)));
        return 0;

      case WM_LBUTTONUP:
      case WM_MBUTTONUP:
      case WM_RBUTTONUP:
        if (!(wp & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON)))
          ReleaseCapture();
        w->mouseReleaseEvent(PixelPoint(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)));
        return 0;

      case WM_MOUSEMOVE:
        w->mouseMoveEvent(PixelPoint(GET_X_LPARAM(lp), GET_Y_LPARAM(lp)),
                          MouseState(mouseButtons(wp), mouseModifiers(wp)));
        return 0;

      case WM_MOUSEWHEEL:
      {
        // one redraw for all that is waiting: a large plot must not fall behind the wheel
        int delta = GET_WHEEL_DELTA_WPARAM(wp);
        MSG next;
        while (PeekMessage(&next, hwnd, WM_MOUSEWHEEL, WM_MOUSEWHEEL, PM_REMOVE))
          delta += GET_WHEEL_DELTA_WPARAM(next.wParam);
        w->wheelEvent(delta, mouseModifiers(GET_KEYSTATE_WPARAM(wp)));
        return 0;
      }

      // Keys the plot has no use for go on to the parent: GraphApp's menu shortcuts are there.
      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
        if (int key = translateKey(wp))
        {
          w->keyPressEvent(KeyboardState(key, modifiers()));
          return 0;
        }
        if (msg == WM_KEYDOWN)
          return SendMessage(GetParent(hwnd), msg, wp, lp);
        break;

      case WM_CHAR:
      case WM_SYSCHAR:
        return SendMessage(GetParent(hwnd), msg, wp, lp);
    }
    return DefWindowProc(hwnd, msg, wp, lp);
  }
};

} // ns

bool Qwt3D::GraphAppHost::renderUnicodeText(std::string const& text, std::string const& family,
                                            int pixelHeight, bool bold, bool italic,
                                            double r, double g, double b,
                                            std::vector<unsigned char>& rgba, int& width, int& height)
{
  std::wstring wide(MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, 0, 0), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wide[0], (int)wide.size());
  wide.resize(wide.size() - 1);

  HDC dc = CreateCompatibleDC(0);
  HFONT font = CreateFontA(pixelHeight, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, italic, 0, 0,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           ANTIALIASED_QUALITY, DEFAULT_PITCH, family.c_str());
  HGDIOBJ oldfont = SelectObject(dc, font);
  SIZE size;
  GetTextExtentPoint32W(dc, wide.c_str(), (int)wide.size(), &size);
  width = size.cx;
  height = size.cy;

  BITMAPINFO bmi;
  ZeroMemory(&bmi, sizeof(bmi));
  bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = height; // bottom row first
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  void* bits = 0;
  HBITMAP dib = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, 0, 0);
  bool ok = dib != 0;
  if (ok)
  {
    HGDIOBJ oldbmp = SelectObject(dc, dib);
    ZeroMemory(bits, width * height * 4);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    TextOutW(dc, 0, 0, wide.c_str(), (int)wide.size());
    GdiFlush();

    rgba.resize(width * height * 4);
    unsigned char const* px = (unsigned char const*)bits;
    for (size_t i = 0; i < rgba.size(); i += 4)
    {
      rgba[i] = (unsigned char)(r * 255 + 0.5);
      rgba[i+1] = (unsigned char)(g * 255 + 0.5);
      rgba[i+2] = (unsigned char)(b * 255 + 0.5);
      rgba[i+3] = (unsigned char)((px[i] + px[i+1] + px[i+2]) / 3);
    }
    SelectObject(dc, oldbmp);
    DeleteObject(dib);
  }
  SelectObject(dc, oldfont);
  DeleteObject(font);
  DeleteDC(dc);
  return ok;
}

GLWidget::GLWidget(Window* parent, int x, int y, int width, int height)
  : hwnd_(0), hdc_(0), hglrc_(0), width_(width), height_(height),
    visible_(false), initialized_(false), resized_(true)
{
  HINSTANCE inst = GetModuleHandle(0);
  HWND parenthwnd = (HWND)GraphAppHost::nativeWindow(parent);

  static bool registered = false;
  if (!registered)
  {
    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = GLWidgetEvents::proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.lpszClassName = className;
    RegisterClassA(&wc);
    registered = true;
  }

  SetWindowLongPtr(parenthwnd, GWL_STYLE,
                   GetWindowLongPtr(parenthwnd, GWL_STYLE) | WS_CLIPCHILDREN);
  if (!GetPropA(parenthwnd, parentProcProp))
  {
    SetPropA(parenthwnd, parentWindowProp, (HANDLE)parent);
    SetPropA(parenthwnd, parentProcProp, (HANDLE)GetWindowLongPtr(parenthwnd, GWLP_WNDPROC));
    SetWindowLongPtr(parenthwnd, GWLP_WNDPROC, (LONG_PTR)parentProc);
  }

  HWND hwnd = CreateWindowExA(0, className, "", WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                              x, y, width, height, parenthwnd, 0, inst, 0);
  HDC dc = GetDC(hwnd);

  PIXELFORMATDESCRIPTOR pfd;
  ZeroMemory(&pfd, sizeof(pfd));
  pfd.nSize = sizeof(pfd);
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;
  pfd.cAlphaBits = 8;
  pfd.cDepthBits = 24;
  SetPixelFormat(dc, ChoosePixelFormat(dc, &pfd), &pfd);

  hwnd_ = hwnd;
  hdc_ = dc;
  hglrc_ = wglCreateContext(dc);
  assert(hglrc_);
  makeCurrent();
  SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
}

std::string GLWidget::askOpenFile(std::string const& title, std::string const& filter)
{
  // a menu command runs while the plot is stepped aside for the menu: bring it back
  EnumChildWindows(GetParent((HWND)hwnd_), thawPlot, 0);
  return GraphAppHost::askFile(title, filter, "", false);
}

std::string GLWidget::askSaveFile(std::string const& title, std::string const& filter,
                                  std::string const& name)
{
  EnumChildWindows(GetParent((HWND)hwnd_), thawPlot, 0);
  return GraphAppHost::askFile(title, filter, name, true);
}

// GraphApp has no color dialog
bool GLWidget::askColor(std::string const&, unsigned char rgb[3])
{
  EnumChildWindows(GetParent((HWND)hwnd_), thawPlot, 0);
  static COLORREF custom[16];
  CHOOSECOLORA cc;
  ZeroMemory(&cc, sizeof(cc));
  cc.lStructSize = sizeof(cc);
  cc.hwndOwner = GetParent((HWND)hwnd_);
  cc.rgbResult = RGB(255, 255, 255);
  cc.lpCustColors = custom;
  cc.Flags = CC_FULLOPEN | CC_RGBINIT;
  if (!ChooseColorA(&cc))
    return false;
  rgb[0] = GetRValue(cc.rgbResult);
  rgb[1] = GetGValue(cc.rgbResult);
  rgb[2] = GetBValue(cc.rgbResult);
  return true;
}

// nor a font dialog
bool GLWidget::askFont(Font& font)
{
  EnumChildWindows(GetParent((HWND)hwnd_), thawPlot, 0);

  LOGFONTA lf;
  ZeroMemory(&lf, sizeof(lf));
  lf.lfHeight = -(font.pointSize() * 96 + 36) / 72;
  lf.lfWeight = (font.weight() >= Font::Bold) ? FW_BOLD : FW_NORMAL;
  lf.lfItalic = font.italic();
  strncpy(lf.lfFaceName, font.family() == "Courier" ? "Courier New" : font.family().c_str(), LF_FACESIZE - 1);

  CHOOSEFONTA cf;
  ZeroMemory(&cf, sizeof(cf));
  cf.lStructSize = sizeof(cf);
  cf.hwndOwner = GetParent((HWND)hwnd_);
  cf.lpLogFont = &lf;
  cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
  if (!ChooseFontA(&cf))
    return false;
  font = Font(lf.lfFaceName, cf.iPointSize / 10,
              lf.lfWeight >= FW_BOLD ? Font::Bold : Font::Normal, lf.lfItalic != 0);
  return true;
}

void GLWidget::tellUser(std::string const& title, std::string const& message)
{
  EnumChildWindows(GetParent((HWND)hwnd_), thawPlot, 0);
  GraphAppHost::tell(title, message);
}

GLWidget::~GLWidget()
{
  SetWindowLongPtr((HWND)hwnd_, GWLP_USERDATA, 0);
  wglMakeCurrent(0, 0);
  wglDeleteContext((HGLRC)hglrc_);
  ReleaseDC((HWND)hwnd_, (HDC)hdc_);
  DestroyWindow((HWND)hwnd_);
}

void GLWidget::show()
{
  visible_ = true;
  ShowWindow((HWND)hwnd_, SW_SHOW);
  updateGL();
}

void GLWidget::hide()
{
  visible_ = false;
  ShowWindow((HWND)hwnd_, SW_HIDE);
}

void GLWidget::setGeometry(int x, int y, int width, int height)
{
  width_ = width;
  height_ = height;
  resized_ = true;
  // SWP_NOCOPYBITS: the old pixels would be shown for a moment at the new size
  SetWindowPos((HWND)hwnd_, 0, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
  updateGL();
}

void GLWidget::setFocus()
{
  SetFocus((HWND)hwnd_);
}

void GLWidget::makeCurrent()
{
  wglMakeCurrent((HDC)hdc_, (HGLRC)hglrc_);
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
  SwapBuffers((HDC)hdc_);
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
