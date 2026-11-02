// The OpenGL view on macOS. The only file that sees AppKit and Core Text; what it needs from
// GraphApp's insides is in qwt3d_cocoa_bridge.c. It must stay clear of graphapp.h: GraphApp's
// Point and Rect are Apple's too.

#ifdef __APPLE__

#define GL_SILENCE_DEPRECATION 1

#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>
#import <OpenGL/gl.h>

#include <algorithm>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qwt3d_glwidget.h"
#include "qwt3d_cocoa_bridge.h"

@interface Qwt3DGLView : NSOpenGLView
{
@public
  Qwt3D::GLWidget* widget;
  int held; // the Qwt3D::MouseButton of the drag in progress, or none
}
@end

namespace
{
  struct Native
  {
    ::Window* parent;
    NSWindow* nswindow;
    Qwt3DGLView* view;
    bool frozen;
  };

  Native* nativeOf(void* p)
  {
    return (Native*)p;
  }

  int modifiersOf(NSEvent* e)
  {
    NSEventModifierFlags f = [e modifierFlags];
    int m = Qwt3D::NoModifier;
    if (f & NSEventModifierFlagShift)
      m |= Qwt3D::ShiftModifier;
    if (f & NSEventModifierFlagControl)
      m |= Qwt3D::ControlModifier;
    if (f & NSEventModifierFlagOption)
      m |= Qwt3D::AltModifier;
    return m;
  }

  // GL works in backing pixels, mouse positions and GraphApp in points
  NSSize backingSize(Qwt3DGLView* view)
  {
    return [view convertRectToBacking:[view bounds]].size;
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

  static void press(GLWidget* w, int x, int y)
  {
    qwtc_clear_key_focus(nativeOf(w->hwnd_)->parent); // the arrow keys are the plot's now
    w->mousePressEvent(PixelPoint(x, y));
  }

  static void release(GLWidget* w, int x, int y)
  {
    w->mouseReleaseEvent(PixelPoint(x, y));
  }

  static void move(GLWidget* w, int x, int y, int buttons, int modifiers)
  {
    w->mouseMoveEvent(PixelPoint(x, y), MouseState(buttons, modifiers));
  }

  static void wheel(GLWidget* w, int delta, int modifiers)
  {
    w->wheelEvent(delta, modifiers);
  }

  static void expose(GLWidget* w)
  {
    if (!nativeOf(w->hwnd_)->frozen)
      w->updateGL();
  }

  static void backingChanged(GLWidget* w)
  {
    w->resized_ = true;
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

  // Leaves a picture of the plot in the window's own picture and hides the view, so that
  // GraphApp's menus, which it draws in that picture, show over the plot.
  static void freeze(void* user)
  {
    GLWidget* w = widget(user);
    Native* n = nativeOf(w->hwnd_);
    if (n->frozen || !w->isVisible())
      return;

    std::vector<unsigned char> rgba;
    int bw, bh;
    if (!w->grabFrameBuffer(rgba, bw, bh))
      return;

    // backing pixels, bottom row first, down to points, top row first
    int pw = w->width(), ph = w->height();
    if (pw < 1 || ph < 1)
      return;
    std::vector<unsigned char> top((size_t)pw * ph * 4);
    for (int row = 0; row != ph; ++row)
      for (int col = 0; col != pw; ++col)
      {
        int x0 = col * bw / pw, x1 = std::max(x0 + 1, (col + 1) * bw / pw);
        int y0 = row * bh / ph, y1 = std::max(y0 + 1, (row + 1) * bh / ph);
        unsigned sum[3] = {0, 0, 0};
        int count = 0;
        for (int sy = y0; sy != y1 && sy < bh; ++sy)
          for (int sx = x0; sx != x1 && sx < bw; ++sx, ++count)
          {
            unsigned char const* p = &rgba[((size_t)(bh - 1 - sy) * bw + sx) * 4];
            sum[0] += p[0];
            sum[1] += p[1];
            sum[2] += p[2];
          }
        unsigned char* d = &top[((size_t)row * pw + col) * 4];
        d[0] = (unsigned char)(sum[0] / std::max(count, 1));
        d[1] = (unsigned char)(sum[1] / std::max(count, 1));
        d[2] = (unsigned char)(sum[2] / std::max(count, 1));
        d[3] = 255;
      }

    NSRect frame = [n->view frame];
    n->frozen = true;
    [n->view setHidden:YES];
    qwtc_paste(n->parent, (int)frame.origin.x, (int)frame.origin.y, pw, ph, &top[0]);
    qwtc_flush(n->parent);
  }

  static void thaw(void* user)
  {
    GLWidget* w = widget(user);
    Native* n = nativeOf(w->hwnd_);
    if (!n->frozen)
      return;
    n->frozen = false;
    if (w->isVisible())
    {
      [n->view setHidden:NO];
      w->updateGL();
    }
  }
};

} // ns

@implementation Qwt3DGLView

- (BOOL)isFlipped
{
  return YES; // positions from the top left, like GraphApp's
}

- (BOOL)isOpaque
{
  return YES;
}

- (BOOL)acceptsFirstResponder
{
  return NO; // keys stay with GraphApp's window, for its menu shortcuts and key handlers
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event
{
  return YES;
}

- (void)drawRect:(NSRect)dirty
{
  if (widget)
    Qwt3D::GLWidgetEvents::expose(widget);
}

- (void)reshape
{
  [super reshape];
  [[self openGLContext] update];
  if (widget)
    Qwt3D::GLWidgetEvents::backingChanged(widget);
}

- (void)viewDidChangeBackingProperties
{
  [super viewDidChangeBackingProperties];
  if (widget)
    Qwt3D::GLWidgetEvents::backingChanged(widget);
}

- (void)begin:(int)button event:(NSEvent*)e
{
  if (!widget)
    return;
  NSPoint p = [self convertPoint:[e locationInWindow] fromView:nil];
  held = button;
  Qwt3D::GLWidgetEvents::press(widget, (int)p.x, (int)p.y);
}

- (void)drag:(NSEvent*)e
{
  if (!widget || !held)
    return;
  NSPoint p = [self convertPoint:[e locationInWindow] fromView:nil];
  Qwt3D::GLWidgetEvents::move(widget, (int)p.x, (int)p.y, held, modifiersOf(e));
}

- (void)end:(NSEvent*)e
{
  if (!widget)
    return;
  NSPoint p = [self convertPoint:[e locationInWindow] fromView:nil];
  held = Qwt3D::NoButton;
  Qwt3D::GLWidgetEvents::release(widget, (int)p.x, (int)p.y);
}

- (void)mouseDown:(NSEvent*)e { [self begin:Qwt3D::LeftButton event:e]; }
- (void)mouseDragged:(NSEvent*)e { [self drag:e]; }
- (void)mouseUp:(NSEvent*)e { [self end:e]; }

// On a Mac, Ctrl+click is the system's right click: the plot's Ctrl+left drag is a Ctrl+left
// drag to the user, so it is passed on as that, and a plain right button as the right one.
- (void)rightMouseDown:(NSEvent*)e
{
  BOOL ctrlClick = ([e modifierFlags] & NSEventModifierFlagControl) != 0;
  [self begin:(ctrlClick ? Qwt3D::LeftButton : Qwt3D::RightButton) event:e];
}
- (void)rightMouseDragged:(NSEvent*)e { [self drag:e]; }
- (void)rightMouseUp:(NSEvent*)e { [self end:e]; }

- (void)otherMouseDown:(NSEvent*)e { [self begin:Qwt3D::MidButton event:e]; }
- (void)otherMouseDragged:(NSEvent*)e { [self drag:e]; }
- (void)otherMouseUp:(NSEvent*)e { [self end:e]; }

- (void)scrollWheel:(NSEvent*)e
{
  if (!widget)
    return;
  double dy = [e scrollingDeltaY];
  // with Shift held, AppKit moves the amount to the horizontal axis (it is how a Mac scrolls
  // sideways): Shift+wheel is the plot's own gesture, for exaggerating Z. Without Shift a
  // sideways scroll is not the plot's.
  if (dy == 0 && ([e modifierFlags] & NSEventModifierFlagShift))
    dy = [e scrollingDeltaX];
  if (dy == 0)
    return;
  // a wheel turned away from you zooms in, whatever "natural scrolling" says
  if ([e isDirectionInvertedFromDevice])
    dy = -dy;
  // 120 to the notch. A wheel gives about 1 a notch; a trackpad gives points, taken as GraphApp's
  // own wheel takes them: 12 to the line and 3 lines to the notch
  double scale = [e hasPreciseScrollingDeltas] ? 120.0 / 36.0 : 120.0;
  int delta = (int)lround(dy * scale);
  if (delta == 0)
    delta = dy > 0 ? 1 : -1;
  Qwt3D::GLWidgetEvents::wheel(widget, delta, modifiersOf(e));
}

@end

// The color panel is an application-wide panel with no OK or Cancel: two buttons are added
// and it runs in a modal session of its own, ended by either one or by closing it.
@interface Qwt3DColorTarget : NSObject <NSWindowDelegate>
{
@public
  BOOL accepted;
  BOOL finished;
}
@end

@implementation Qwt3DColorTarget
- (void)finish:(BOOL)ok
{
  if (finished)
    return;
  finished = YES;
  accepted = ok;
  [NSApp stopModal];
}
- (void)pickOK:(id)sender { [self finish:YES]; }
- (void)pickCancel:(id)sender { [self finish:NO]; }
- (void)windowWillClose:(NSNotification*)note { [self finish:NO]; }
@end

// The font panel has no OK or Cancel either: closing it keeps the last font picked, and
// closing it without picking one changes nothing.
@interface Qwt3DFontTarget : NSObject <NSWindowDelegate>
{
@public
  NSFont* start;
  NSFont* chosen;
  BOOL finished;
}
@end

@implementation Qwt3DFontTarget
- (void)changeFont:(id)sender
{
  NSFont* f = [sender convertFont:start];
  if (f)
    chosen = f;
}
- (void)windowWillClose:(NSNotification*)note
{
  if (!finished)
  {
    finished = YES;
    [NSApp stopModal];
  }
}
@end

// Core Text has the glyphs GraphApp's native fonts lack (pi, for one) and falls back to
// another font for any the named one has not.
bool Qwt3D::GraphAppHost::renderUnicodeText(std::string const& text, std::string const& family,
                                            int pixelHeight, bool bold, bool italic,
                                            double r, double g, double b,
                                            std::vector<unsigned char>& rgba, int& width, int& height)
{
  @autoreleasepool
  {
    NSString* s = [NSString stringWithUTF8String:text.c_str()];
    NSString* name = [NSString stringWithUTF8String:family.c_str()];
    if (!s || [s length] == 0 || !name)
      return false;

    CTFontRef base = CTFontCreateWithName((__bridge CFStringRef)name, pixelHeight, NULL);
    if (!base)
      return false;
    CTFontSymbolicTraits wanted = (bold ? kCTFontBoldTrait : 0) | (italic ? kCTFontItalicTrait : 0);
    CTFontRef font = wanted ? CTFontCreateCopyWithSymbolicTraits(base, pixelHeight, NULL, wanted, wanted)
                            : NULL;
    if (font)
      CFRelease(base);
    else
      font = base;

    NSDictionary* attributes = @{
      (__bridge id)kCTFontAttributeName : (__bridge id)font,
      (__bridge id)kCTForegroundColorFromContextAttributeName : @YES
    };
    NSAttributedString* string = [[NSAttributedString alloc] initWithString:s attributes:attributes];
    CTLineRef line = CTLineCreateWithAttributedString((__bridge CFAttributedStringRef)string);
    CGFloat ascent = 0, descent = 0, leading = 0;
    double w = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
    width = (int)ceil(w);
    height = (int)ceil(ascent + descent);

    bool ok = width > 0 && height > 0;
    if (ok)
    {
      CGColorSpaceRef gray = CGColorSpaceCreateDeviceGray();
      CGContextRef ctx = CGBitmapContextCreate(NULL, width, height, 8, width, gray, kCGImageAlphaNone);
      CGColorSpaceRelease(gray);
      ok = ctx != NULL;
      if (ok)
      {
        CGContextSetGrayFillColor(ctx, 0, 1);
        CGContextFillRect(ctx, CGRectMake(0, 0, width, height));
        CGContextSetGrayFillColor(ctx, 1, 1);
        CGContextSetShouldAntialias(ctx, true);
        CGContextSetTextPosition(ctx, 0, descent);
        CTLineDraw(line, ctx);

        unsigned char const* data = (unsigned char const*)CGBitmapContextGetData(ctx);
        size_t stride = CGBitmapContextGetBytesPerRow(ctx);
        rgba.resize((size_t)width * height * 4);
        unsigned char red = (unsigned char)(r * 255 + 0.5);
        unsigned char green = (unsigned char)(g * 255 + 0.5);
        unsigned char blue = (unsigned char)(b * 255 + 0.5);
        for (int row = 0; row != height; ++row) // the bitmap's first row is its top
        {
          unsigned char const* in = data + (size_t)row * stride;
          unsigned char* out = &rgba[(size_t)(height - 1 - row) * width * 4];
          for (int col = 0; col != width; ++col, out += 4)
          {
            out[0] = red;
            out[1] = green;
            out[2] = blue;
            out[3] = in[col];
          }
        }
        CGContextRelease(ctx);
      }
    }
    CFRelease(line);
    CFRelease(font);
    return ok;
  }
}

Qwt3D::GLWidget::GLWidget(::Window* parent, int x, int y, int width, int height)
  : hwnd_(0), hdc_(0), hglrc_(0), width_(width), height_(height),
    visible_(false), initialized_(false), resized_(true)
{
  GraphAppHost::nativeWindow(parent); // registers the application

  Native* n = new Native;
  n->parent = parent;
  n->frozen = false;
  n->nswindow = (__bridge NSWindow*)qwtc_nswindow(parent);
  assert(n->nswindow);

  // no profile attribute: the library needs the legacy context, not a core one
  NSOpenGLPixelFormatAttribute attributes[] = {
    NSOpenGLPFADoubleBuffer, NSOpenGLPFADepthSize, 24, NSOpenGLPFAColorSize, 32, 0
  };
  NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
  assert(format);
  n->view = [[Qwt3DGLView alloc] initWithFrame:NSMakeRect(x, y, width, height) pixelFormat:format];
  assert(n->view);
  n->view->widget = this;
  n->view->held = NoButton;
  [n->view setWantsBestResolutionOpenGLSurface:YES];
  [n->view setHidden:YES];
  hwnd_ = n; // before the view goes in: AppKit calls back into it as it does
  // GraphApp's window shows its own picture in its content view: the plot goes over that
  [[n->nswindow contentView] addSubview:n->view];

  static QwtcCallbacks const calls = {GLWidgetEvents::arrow, GLWidgetEvents::freeze,
                                      GLWidgetEvents::thaw};
  bool registered = qwtc_register(parent, &calls, this) != 0;
  assert(registered);
  (void)registered;
  makeCurrent();
}

std::string Qwt3D::GLWidget::askOpenFile(std::string const& title, std::string const& filter)
{
  // a menu command runs while the plot is stepped aside for the menu: bring it back
  qwtc_thaw(nativeOf(hwnd_)->parent);
  return GraphAppHost::askFile(title, filter, "", false);
}

std::string Qwt3D::GLWidget::askSaveFile(std::string const& title, std::string const& filter,
                                         std::string const& name)
{
  qwtc_thaw(nativeOf(hwnd_)->parent);
  return GraphAppHost::askFile(title, filter, name, true);
}

bool Qwt3D::GLWidget::askColor(std::string const& title, unsigned char rgb[3])
{
  qwtc_thaw(nativeOf(hwnd_)->parent);

  @autoreleasepool
  {
    NSColorPanel* panel = [NSColorPanel sharedColorPanel];
    panel.showsAlpha = NO;
    panel.color = [NSColor colorWithSRGBRed:rgb[0] / 255.0 green:rgb[1] / 255.0
                                       blue:rgb[2] / 255.0 alpha:1.0];
    panel.title = title.empty() ? @"Colors" : [NSString stringWithUTF8String:title.c_str()];
    panel.restorable = NO; // a shared panel: macOS would reopen it at the next launch

    Qwt3DColorTarget* target = [[Qwt3DColorTarget alloc] init];
    NSView* accessory = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 220, 36)];
    NSButton* ok = [NSButton buttonWithTitle:@"OK" target:target action:@selector(pickOK:)];
    ok.frame = NSMakeRect(120, 4, 90, 28);
    ok.keyEquivalent = @"\r";
    NSButton* cancel = [NSButton buttonWithTitle:@"Cancel" target:target action:@selector(pickCancel:)];
    cancel.frame = NSMakeRect(10, 4, 100, 28);
    [accessory addSubview:ok];
    [accessory addSubview:cancel];
    panel.accessoryView = accessory;
    panel.delegate = target;

    [NSApp activateIgnoringOtherApps:YES];
    [panel makeKeyAndOrderFront:nil];
    [panel orderFrontRegardless];
    [NSApp runModalForWindow:panel];

    panel.delegate = nil;
    panel.accessoryView = nil; // leave the shared panel as it was found
    [panel close];
    [nativeOf(hwnd_)->nswindow makeKeyAndOrderFront:nil]; // keys go to the main window again

    if (!target->accepted)
      return false;
    NSColor* c = [panel.color colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];
    rgb[0] = (unsigned char)lround(c.redComponent * 255.0);
    rgb[1] = (unsigned char)lround(c.greenComponent * 255.0);
    rgb[2] = (unsigned char)lround(c.blueComponent * 255.0);
    return true;
  }
}

bool Qwt3D::GLWidget::askFont(Font& font)
{
  qwtc_thaw(nativeOf(hwnd_)->parent);

  @autoreleasepool
  {
    NSFontManager* manager = [NSFontManager sharedFontManager];
    NSFontTraitMask traits = 0;
    if (font.weight() >= Font::Bold)
      traits |= NSBoldFontMask;
    if (font.italic())
      traits |= NSItalicFontMask;
    NSFont* start = [manager fontWithFamily:[NSString stringWithUTF8String:font.family().c_str()]
                                     traits:traits weight:5 size:font.pointSize()];
    if (!start)
      start = [NSFont systemFontOfSize:font.pointSize()];

    Qwt3DFontTarget* target = [[Qwt3DFontTarget alloc] init];
    target->start = start;
    [manager setTarget:target];
    [manager setSelectedFont:start isMultiple:NO];
    NSFontPanel* panel = [manager fontPanel:YES];
    panel.delegate = target;
    panel.restorable = NO;

    [NSApp activateIgnoringOtherApps:YES];
    [panel makeKeyAndOrderFront:nil];
    [panel orderFrontRegardless];
    [NSApp runModalForWindow:panel];

    panel.delegate = nil;
    [manager setTarget:nil];
    [panel close];
    [nativeOf(hwnd_)->nswindow makeKeyAndOrderFront:nil]; // keys go to the main window again

    if (!target->chosen)
      return false;
    NSFontTraitMask got = [manager traitsOfFont:target->chosen];
    font = Font([[target->chosen familyName] UTF8String], (int)lround([target->chosen pointSize]),
                (got & NSBoldFontMask) ? Font::Bold : Font::Normal, (got & NSItalicFontMask) != 0);
    return true;
  }
}

void Qwt3D::GLWidget::tellUser(std::string const& title, std::string const& message)
{
  qwtc_thaw(nativeOf(hwnd_)->parent);
  GraphAppHost::tell(title, message);
}

Qwt3D::GLWidget::~GLWidget()
{
  Native* n = nativeOf(hwnd_);
  qwtc_unregister(this);
  n->view->widget = 0;
  [NSOpenGLContext clearCurrentContext];
  [n->view removeFromSuperview];
  delete n;
}

void Qwt3D::GLWidget::show()
{
  visible_ = true;
  Native* n = nativeOf(hwnd_);
  qwtc_hook_menu_bar(n->parent);
  if (!n->frozen)
    [n->view setHidden:NO];
  updateGL();
}

void Qwt3D::GLWidget::hide()
{
  visible_ = false;
  [nativeOf(hwnd_)->view setHidden:YES];
}

void Qwt3D::GLWidget::setGeometry(int x, int y, int width, int height)
{
  width_ = width;
  height_ = height;
  resized_ = true;
  Native* n = nativeOf(hwnd_);
  [n->view setFrame:NSMakeRect(x, y, width, height)];
  [[n->view openGLContext] update];
  updateGL();
}

void Qwt3D::GLWidget::setFocus()
{
  qwtc_clear_key_focus(nativeOf(hwnd_)->parent);
}

void Qwt3D::GLWidget::makeCurrent()
{
  [[nativeOf(hwnd_)->view openGLContext] makeCurrentContext];
}

void Qwt3D::GLWidget::prepare()
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
    NSSize b = backingSize(nativeOf(hwnd_)->view);
    resizeGL((int)b.width, (int)b.height);
  }
}

void Qwt3D::GLWidget::updateGL()
{
  if (!visible_)
    return;
  prepare();
  paintGL();
  [[nativeOf(hwnd_)->view openGLContext] flushBuffer];
}

bool Qwt3D::GLWidget::grabFrameBuffer(std::vector<unsigned char>& rgba, int& width, int& height)
{
  prepare();
  paintGL();
  while (glGetError() != GL_NO_ERROR)
    ; // only the read below decides
  NSSize b = backingSize(nativeOf(hwnd_)->view); // a picture has every pixel the screen has
  width = (int)b.width;
  height = (int)b.height;
  rgba.resize((size_t)width * height * 4);
  if (rgba.empty())
    return false;
  glReadBuffer(GL_BACK);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &rgba[0]);
  return glGetError() == GL_NO_ERROR;
}

#endif // __APPLE__
