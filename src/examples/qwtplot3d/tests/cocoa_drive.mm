// Test driver for the macOS plot view. Linked into a test build of the demo (see
// build_drive.sh) and not into the library or the shipped demo.
//
// It drives the demo from inside its own process, and never uses the system's keyboard,
// mouse or clipboard: mouse and wheel events are built as NSEvents and handed straight to
// the plot view; keys and menu clicks go through GraphApp's own in-process test hooks
// (gab_test_key, gab_test_mouse_front), which queue events for the demo's own window only.
//
//   QWT3D_DRIVE=basic|menu|dialogs|saveas|color|font|pi|resize|close   which script to run
//   QWT3D_LOG=file                                   the demo's --log file, to report what each step did
//   QWT3D_OUT=dir                                    where the report and pictures go (default /tmp/q3d)
//
// Each step reports the lines the demo logged while it ran.

#include <qwt3d_surfaceplot.h>
#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

extern "C" {
void gab_test_key(void* handle, int keycode, const char* chars, int mods);
void gab_test_mouse_front(int kind, int x, int y, int button, int mods);
}


@interface FontPicker : NSObject
@end
@implementation FontPicker
- (NSFont*)convertFont:(NSFont*)f
{
  return [NSFont fontWithName:@"Times New Roman" size:22];
}
@end

namespace
{
  const char* outdir = "/tmp/q3d";
  FILE* report = 0;
  std::string logPath;
  size_t logRead = 0;
  NSWindow* mainWindow = nil;
  NSView* plotView = nil;

  void onMain(void (^block)(void))
  {
    if ([NSThread isMainThread])
      block();
    else
      dispatch_sync(dispatch_get_main_queue(), block);
  }

  void say(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
  void say(const char* fmt, ...)
  {
    va_list a;
    va_start(a, fmt);
    vfprintf(report, fmt, a);
    va_end(a);
    fputc('\n', report);
    fflush(report);
  }

  // what the demo has logged since the last call
  std::string newLog()
  {
    std::string all;
    if (FILE* f = fopen(logPath.c_str(), "r"))
    {
      char buf[4096];
      size_t n;
      while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        all.append(buf, n);
      fclose(f);
    }
    std::string fresh = all.size() > logRead ? all.substr(logRead) : "";
    logRead = all.size();
    return fresh;
  }

  void step(const char* name)
  {
    usleep(500000);
    std::string fresh = newLog();
    // first and last line of what changed, and how many there were
    size_t lines = 0;
    for (size_t i = 0; i != fresh.size(); ++i)
      lines += fresh[i] == '\n';
    std::string first, last;
    if (!fresh.empty())
    {
      size_t e = fresh.find('\n');
      first = fresh.substr(0, e);
      size_t b = fresh.rfind('\n', fresh.size() - 2);
      last = fresh.substr(b == std::string::npos ? 0 : b + 1);
      if (!last.empty() && last[last.size() - 1] == '\n')
        last.erase(last.size() - 1);
    }
    say("%-34s %3zu log lines   first: %-36s last: %s", name, lines, first.c_str(), last.c_str());
  }

  NSEvent* mouseEvent(NSEventType type, NSPoint inView, NSEventModifierFlags flags)
  {
    __block NSPoint p;
    onMain(^{ p = [plotView convertPoint:inView toView:nil]; });
    return [NSEvent mouseEventWithType:type location:p modifierFlags:flags
                             timestamp:[[NSProcessInfo processInfo] systemUptime]
                          windowNumber:[mainWindow windowNumber] context:nil
                           eventNumber:0 clickCount:1 pressure:1.0];
  }

  // a drag across the plot, sent to the plot view only
  void drag(NSPoint from, NSPoint to, NSEventModifierFlags flags, bool rightButton)
  {
    NSEventType down = rightButton ? NSEventTypeRightMouseDown : NSEventTypeLeftMouseDown;
    NSEventType dragged = rightButton ? NSEventTypeRightMouseDragged : NSEventTypeLeftMouseDragged;
    NSEventType up = rightButton ? NSEventTypeRightMouseUp : NSEventTypeLeftMouseUp;
    NSEvent* e = mouseEvent(down, from, flags);
    onMain(^{ rightButton ? [plotView rightMouseDown:e] : [plotView mouseDown:e]; });
    for (int i = 1; i <= 8; ++i)
    {
      NSPoint p = NSMakePoint(from.x + (to.x - from.x) * i / 8, from.y + (to.y - from.y) * i / 8);
      NSEvent* d = mouseEvent(dragged, p, flags);
      onMain(^{ rightButton ? [plotView rightMouseDragged:d] : [plotView mouseDragged:d]; });
      usleep(20000);
    }
    NSEvent* u = mouseEvent(up, to, flags);
    onMain(^{ rightButton ? [plotView rightMouseUp:u] : [plotView mouseUp:u]; });
  }

  void wheel(int amount, NSEventModifierFlags flags, bool precise)
  {
    CGEventRef c = CGEventCreateScrollWheelEvent(NULL, precise ? kCGScrollEventUnitPixel : kCGScrollEventUnitLine,
                                                 1, amount);
    CGEventSetFlags(c, (CGEventFlags)flags);
    NSEvent* e = [NSEvent eventWithCGEvent:c];
    CFRelease(c);
    onMain(^{ [plotView scrollWheel:e]; });
  }

  void key(int keycode, const char* chars, int mods)
  {
    onMain(^{ gab_test_key((__bridge void*)mainWindow, keycode, chars, mods); });
  }

  // for keys that open a modal panel: the call would not return until it closed
  void keyAsync(int keycode, const char* chars, int mods)
  {
    std::string c = chars;
    dispatch_async(dispatch_get_main_queue(), ^{ gab_test_key((__bridge void*)mainWindow, keycode, c.c_str(), mods); });
  }

  void shot(const char* name)
  {
    char cmd[400];
    snprintf(cmd, sizeof(cmd), "screencapture -x -o -l %d %s/%s.png", (int)[mainWindow windowNumber], outdir, name);
    system(cmd);
  }

  bool findPlot()
  {
    __block bool found = false;
    onMain(^{
      for (NSWindow* w in [NSApp windows])
        for (NSView* v in [[w contentView] subviews])
          if ([v isKindOfClass:NSClassFromString(@"Qwt3DGLView")] && ![v isHidden] && [w isVisible])
          {
            mainWindow = w;
            plotView = v;
            found = true;
          }
    });
    return found;
  }

  void basic()
  {
    NSRect b = [plotView bounds];
    NSPoint c = NSMakePoint(b.size.width / 2, b.size.height / 2);
    NSPoint to = NSMakePoint(c.x + 70, c.y + 40);
    step("start");
    drag(c, to, 0, false);                                step("drag (rotate)");
    drag(c, to, NSEventModifierFlagShift, false);         step("Shift+drag (rotate about y)");
    drag(c, to, NSEventModifierFlagOption, false);        step("Option+drag (scale; Alt)");
    // on a Mac, Ctrl+click arrives as a right click with Control held
    drag(c, to, NSEventModifierFlagControl, true);        step("Ctrl+drag (move; right-click event)");
    drag(c, to, 0, true);                                 step("right button drag (not assigned)");
    wheel(1, 0, false);                                   step("wheel +1 notch (zoom in)");
    wheel(-1, 0, false);                                  step("wheel -1 notch (zoom out)");
    wheel(3, 0, false);                                   step("wheel +3 (zoom in)");
    wheel(1, NSEventModifierFlagShift, false);            step("Shift+wheel +1 (exaggerate Z)");
    wheel(-1, NSEventModifierFlagShift, false);           step("Shift+wheel -1");
    wheel(36, 0, true);                                   step("trackpad +36 points (about 1 notch)");
    // arrows reach the plot through GraphApp's window, as key presses do in use
    key(123, "", 0);                                      step("arrow Left");
    key(124, "", 0);                                      step("arrow Right");
    key(126, "", 0);                                      step("arrow Up");
    key(125, "", 0);                                      step("arrow Down");
    key(123, "", 1);                                      step("Shift+arrow Left");
    key(126, "", 2);                                      step("Ctrl+arrow Up");
    key(1, "s", 0);                                       step("key s: save a PNG (testKey)");
    shot("basic_end");
  }

  void menu()
  {
    shot("menu_0_before");
    // the File title is at about (15, 11) of GraphApp's client area
    gab_test_mouse_front(0, 15, 11, 1, 0);        // press: the menu opens and holds GraphApp here
    usleep(700000);
    shot("menu_1_open");
    say("menu opened: see menu_1_open.png");
    gab_test_mouse_front(1, 15, 11, 1, 0);        // release on the title: the menu stays open
    usleep(500000);
    shot("menu_2_open_after_release");
    // move to the first item and press and release there to choose it
    gab_test_mouse_front(2, 40, 34, 0, 0);
    usleep(300000);
    shot("menu_3_hover_item");
    gab_test_mouse_front(0, 40, 34, 1, 0);
    usleep(200000);
    gab_test_mouse_front(1, 40, 34, 1, 0);
    usleep(1500000);
    shot("menu_4_after_choose");
    say("menu closed: see menu_4_after_choose.png");
  }

  void resize()
  {
    shot("resize_0_before");
    onMain(^{ [mainWindow setContentSize:NSMakeSize(1300, 880)]; });
    usleep(1200000);
    shot("resize_1_larger");
    NSRect b = [plotView frame];
    say("plot view after enlarging: %g x %g at (%g, %g)", b.size.width, b.size.height, b.origin.x, b.origin.y);
    onMain(^{ [mainWindow setContentSize:NSMakeSize(900, 640)]; });
    usleep(1200000);
    shot("resize_2_smaller");
    b = [plotView frame];
    say("plot view after shrinking: %g x %g at (%g, %g)", b.size.width, b.size.height, b.origin.x, b.origin.y);
    key(1, "s", 0);
    step("save at this size");
  }


  void shotWindow(NSWindow* w, const char* name)
  {
    char cmd[400];
    snprintf(cmd, sizeof(cmd), "screencapture -x -o -l %d %s/%s.png", (int)[w windowNumber], outdir, name);
    system(cmd);
  }

  // click a menu title and then one of its items, in GraphApp's own coordinates
  void chooseMenuItem(int titleX, int itemY)
  {
    gab_test_mouse_front(0, titleX, 11, 1, 0);
    usleep(600000);
    gab_test_mouse_front(1, titleX, 11, 1, 0);
    usleep(300000);
    gab_test_mouse_front(2, 50, itemY, 0, 0);
    usleep(200000);
    gab_test_mouse_front(0, 50, itemY, 1, 0);
    usleep(150000);
    gab_test_mouse_front(1, 50, itemY, 1, 0);
  }

  // the modal window now up, described, and a picture of it
  NSWindow* modalNow(const char* label)
  {
    __block NSWindow* m = nil;
    __block std::string d;
    onMain(^{
      m = [NSApp modalWindow];
      if (m)
      {
        d = std::string([NSStringFromClass([m class]) UTF8String]) + " \"" + [[m title] UTF8String] + "\"";
        if ([m isKindOfClass:[NSSavePanel class]])
          d += std::string(" message \"") + [[(NSSavePanel*)m message] UTF8String] + "\"";
      }
    });
    say("%s: %s", label, m ? d.c_str() : "NO MODAL WINDOW");
    return m;
  }

  void dialogs()
  {
    // File > Open File opens the system's open panel; Cancel returns nothing
    chooseMenuItem(15, 31);
    usleep(1500000);
    NSWindow* panel = modalNow("File > Open File");
    if (panel)
    {
      shotWindow(panel, "dialog_open_panel");
      onMain(^{ [(NSSavePanel*)panel cancel:nil]; });
    }
    step("Open File panel: Cancel");
    shot("dialog_after_open_cancel");

    // File > Save As: pick a name in the panel and confirm; the demo writes the picture
    unlink("/tmp/q3d/menu_saved.png");
    chooseMenuItem(15, 65);
    usleep(1500000);
    panel = modalNow("File > Save As");
    if (panel)
    {
      shotWindow(panel, "dialog_save_panel");
      onMain(^{
        NSSavePanel* sp = (NSSavePanel*)panel;
        [sp setDirectoryURL:[NSURL fileURLWithPath:@"/tmp/q3d"]];
        [sp setNameFieldStringValue:@"menu_saved.png"];
        NSButtonCell* def = [sp defaultButtonCell];
        say("  default button: %s", def ? [[def title] UTF8String] : "NONE");
        if (def)
          [def performClick:nil];
        else
          [NSApp stopModalWithCode:NSModalResponseOK];
      });
    }
    usleep(2000000);
    struct stat st;
    say("Save As wrote /tmp/q3d/menu_saved.png: %s",
        stat("/tmp/q3d/menu_saved.png", &st) == 0 ? "yes" : "NO");
    if (stat("/tmp/q3d/menu_saved.png", &st) == 0)
      say("  size %lld bytes", (long long)st.st_size);
    shot("dialog_after_save");
  }

  NSButton* colorPanelButton(const char* title)
  {
    __block NSButton* found = nil;
    onMain(^{
      for (NSView* v in [[[NSColorPanel sharedColorPanel] accessoryView] subviews])
        if ([v isKindOfClass:[NSButton class]] && [[(NSButton*)v title] isEqualToString:[NSString stringWithUTF8String:title]])
          found = (NSButton*)v;
    });
    return found;
  }

  void color()
  {
    shot("color_0_before");
    keyAsync(40, "k", 0); // testKey: Color > Background
    usleep(1500000);
    NSWindow* panel = modalNow("color panel (OK)");
    onMain(^{ say("  visible %d, accessory OK button %s", [panel isVisible], colorPanelButton("OK") ? "present" : "MISSING"); });
    shotWindow(panel, "color_1_panel");
    onMain(^{ [[NSColorPanel sharedColorPanel] setColor:[NSColor colorWithSRGBRed:0.85 green:0.15 blue:0.15 alpha:1]]; });
    NSButton* ok = colorPanelButton("OK");
    onMain(^{ [ok performClick:nil]; });
    usleep(1000000);
    shot("color_2_after_ok");
    say("after OK: modal window %s", modalNow("modal") ? "still up" : "gone (panel closed)");

    usleep(1500000);
    onMain(^{ say("before 2nd k: key window %s, color panel visible %d, app active %d",
                  [[[NSApp keyWindow] title] UTF8String] ?: "(none)", [[NSColorPanel sharedColorPanel] isVisible], [NSApp isActive]); });
    keyAsync(40, "k", 0);
    usleep(2500000);
    onMain(^{ say("after 2nd k: key window %s, color panel visible %d",
                  [[[NSApp keyWindow] title] UTF8String] ?: "(none)", [[NSColorPanel sharedColorPanel] isVisible]); });
    panel = modalNow("color panel (Cancel)");
    onMain(^{ [[NSColorPanel sharedColorPanel] setColor:[NSColor colorWithSRGBRed:0.1 green:0.1 blue:0.9 alpha:1]]; });
    NSButton* cancel = colorPanelButton("Cancel");
    onMain(^{ [cancel performClick:nil]; });
    usleep(1000000);
    shot("color_3_after_cancel");
    say("(color_3 should match color_2: Cancel must change nothing)");
  }

  void fontTest();


  void fontTest()
  {
    shot("font_0_before");
    keyAsync(40, "K", 1); // testKey: Fonts > Numbers
    usleep(1500000);
    NSWindow* panel = modalNow("font panel");
    if (panel)
      shotWindow(panel, "font_1_panel");
    // the real panel is shown; picking in it is by hand, so a pick is stood in for here:
    // the click in the panel's list is stood in for
    onMain(^{
      NSFontManager* fm = [NSFontManager sharedFontManager];
      id target = [fm target];
      say("  font manager target %s", target ? "set" : "MISSING");
      // a stand-in for the click in the panel's list, which a script can't make: the target is
      // sent changeFont: with a sender that converts to Times 22, as the font manager does
      [target changeFont:[[FontPicker alloc] init]];
      [[fm fontPanel:NO] close];
    });
    usleep(1500000);
    shot("font_2_after_pick");
    say("(font_2 should show the numbers in Times, size 22)");
  }


  // the SurfacePlot behind the plot view, through the view's public "widget" ivar
  Qwt3D::SurfacePlot* surfacePlot()
  {
    Ivar iv = class_getInstanceVariable([plotView class], "widget");
    if (!iv)
      return 0;
    void* w = *(void**)((char*)(__bridge void*)plotView + ivar_getOffset(iv));
    return dynamic_cast<Qwt3D::SurfacePlot*>((Qwt3D::GLWidget*)w);
  }

  void pi()
  {
    shot("pi_0_before");
    onMain(^{
      Qwt3D::SurfacePlot* p = surfacePlot();
      if (!p)
      {
        say("no SurfacePlot behind the view");
        return;
      }
      p->setTitle("Rosenbrock \xCF\x80 and \xCE\xA9 (pi, Omega)");
      const Qwt3D::AXIS xs[] = {Qwt3D::X1, Qwt3D::X2, Qwt3D::X3, Qwt3D::X4};
      const Qwt3D::AXIS ys[] = {Qwt3D::Y1, Qwt3D::Y2, Qwt3D::Y3, Qwt3D::Y4};
      const Qwt3D::AXIS zs[] = {Qwt3D::Z1, Qwt3D::Z2, Qwt3D::Z3, Qwt3D::Z4};
      for (int i = 0; i != 4; ++i)
      {
        p->coordinates()->axes[xs[i]].setLabelString("x / \xCF\x80");
        p->coordinates()->axes[ys[i]].setLabelString("y \xCE\xB1\xCE\xB2");
        p->coordinates()->axes[zs[i]].setLabelString("z \xCE\xA3");
      }
      p->updateGL();
    });
    usleep(800000);
    shot("pi_1_after");
    say("title and axis labels set with pi, Omega, alpha, beta, Sigma: see pi_1_after.png");
  }

  void saveAsPanel();


  void postKey(NSWindow* w, unsigned short code, NSString* chars, BOOL down)
  {
    NSEvent* e = [NSEvent keyEventWithType:down ? NSEventTypeKeyDown : NSEventTypeKeyUp
                                  location:NSZeroPoint modifierFlags:0 timestamp:0
                              windowNumber:[w windowNumber] context:nil characters:chars
                charactersIgnoringModifiers:chars isARepeat:NO keyCode:code];
    [NSApp postEvent:e atStart:NO];
  }

  void saveAsPanel()
  {
    unlink("/tmp/q3d/panel_saved.png");
    chooseMenuItem(15, 65);
    usleep(1500000);
    NSWindow* panel = modalNow("File > Save As");
    if (!panel)
      return;
    onMain(^{
      NSSavePanel* sp = (NSSavePanel*)panel;
      [sp setDirectoryURL:[NSURL fileURLWithPath:@"/tmp/q3d"]];
      [sp setNameFieldStringValue:@"panel_saved.png"];
      unsigned int n = 0;
      Method* ms = class_copyMethodList([panel class], &n);
      std::string l;
      for (unsigned i = 0; i != n; ++i)
      {
        std::string name = sel_getName(method_getName(ms[i]));
        if (name.find("ok") != std::string::npos || name.find("OK") != std::string::npos)
          l += name + " ";
      }
      free(ms);
      say("  panel class %s; ok-like methods: %s", class_getName([panel class]), l.c_str());
    });
    usleep(500000);
    // a key event sent to this process alone (not the system's input), which AppKit routes to
    // the panel as the key window, remote view and all
    CGEventRef down = CGEventCreateKeyboardEvent(NULL, 36, true);
    CGEventRef up = CGEventCreateKeyboardEvent(NULL, 36, false);
    CGEventPostToPid(getpid(), down);
    usleep(100000);
    CGEventPostToPid(getpid(), up);
    CFRelease(down);
    CFRelease(up);
    usleep(2500000);
    struct stat st;
    say("Save As through the panel wrote /tmp/q3d/panel_saved.png: %s",
        stat("/tmp/q3d/panel_saved.png", &st) == 0 ? "yes" : "NO");
    onMain(^{ if ([NSApp modalWindow]) [(NSSavePanel*)[NSApp modalWindow] cancel:nil]; }); // never leave it up
    usleep(500000);
  }

  void closeWindow()
  {
    say("closing the window");
    onMain(^{ [mainWindow performClose:nil]; });
  }

  void* run(void*)
  {
    const char* which = getenv("QWT3D_DRIVE");
    if (const char* o = getenv("QWT3D_OUT"))
      outdir = o;
    if (const char* l = getenv("QWT3D_LOG"))
      logPath = l;
    char path[400];
    snprintf(path, sizeof(path), "%s/drive_%s.txt", outdir, which ? which : "none");
    report = fopen(path, "w");
    for (int i = 0; i != 200 && !findPlot(); ++i)
      usleep(100000);
    if (!plotView)
    {
      say("no plot view found");
      return 0;
    }
    usleep(1500000);
    say("plot view frame %g x %g, backing scale %g", [plotView frame].size.width,
        [plotView frame].size.height, [[plotView window] backingScaleFactor]);
    if (!which || !strcmp(which, "basic"))
      basic();
    else if (!strcmp(which, "menu"))
      menu();
    else if (!strcmp(which, "resize"))
      resize();
    else if (!strcmp(which, "pi"))
      pi();
    else if (!strcmp(which, "saveas"))
      saveAsPanel();
    else if (!strcmp(which, "dialogs"))
      dialogs();
    else if (!strcmp(which, "color"))
      color();
    else if (!strcmp(which, "font"))
      fontTest();
    else if (!strcmp(which, "close"))
      closeWindow();
    say("done");
    return 0;
  }

  __attribute__((constructor)) void start()
  {
    pthread_t t;
    pthread_create(&t, NULL, run, NULL);
    pthread_detach(t);
  }
}
