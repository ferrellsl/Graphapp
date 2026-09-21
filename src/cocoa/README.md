# GraphApp Cocoa backend (macOS)

A native macOS backend for GraphApp, alongside `win32/` and `x11/`. The portable
layers (`gui/`, `utility/`, `imgfmt/`, `lib*/`) are unchanged.

    make -f Makefile.cocoa            # build-macos/libapp.a   (arm64)
    make -f Makefile.cocoa demo       # the example programs
    make -f Makefile.cocoa test       # pixel test + end-to-end UI test

Link a program with `libapp.a -framework Cocoa -framework CoreText -framework CoreGraphics`.
Everything is built out of tree in `build-macos/`.

## Design

**Two halves.** GraphApp's `Point`/`Rect` collide with Apple's `MacTypes.h`, so no
translation unit can see both `app.h` and Cocoa. The backend is split:

| C side (`appint.h`, no Cocoa)          | Objective-C side (`bridge.h`, no GraphApp) |
|----------------------------------------|--------------------------------------------|
| `drawops.c` pixel engine               | `cocoa_window.m` NSWindow + NSView         |
| `graphics.c`, `bmap.c`, `bmapimg.c`    | `cocoa_app.m` NSApplication, event pump    |
| `win.c`, `event.c`, `init.c`           | `cocoa_font.m` CoreText                    |
| `font.c`, `cursor.c`, `clipbrd.c`, `timer.c`, `folder.c` | `cocoa_misc.m` pasteboard, cursors |

`bridge.h` is plain C (ints, `void *` handles, callbacks) and is the only interface.

**Everything is drawn into pixel buffers.** A window or bitmap is a `Surface` of
`0x00RRGGBB` pixels. `drawops.c` implements `fill_rect`, `copy_rect` and `draw_utf8`
directly on those pixels (lines use the portable Bresenham), with region clipping,
stencil masks and Windows-compatible XOR mode, so `xortest`-style rubber-banding works.
The NSView just blits the surface (`drawRect:`); drawing marks a dirty rect that is
flushed when a window `Graphics` is deleted and around event waits.

**Redraws are synchronous**, like `UpdateWindow`: `app_redraw_rect` runs the window's
redraw handlers immediately on the surface. AppKit expose events need no handling
because the surface persists.

**Events** are ported from the Windows `WndProc`. Cmd is treated as Ctrl (Cmd-C/V/X/A and
menu shortcuts work); Ctrl-click and right-click give button 4, Shift-click button 2, as on
Windows. Menu and drop-list tracking use `app_get_mouse_event`, which pulls mouse events
un-dispatched via `gab_next_event(..., &mouse)`. Timers are the portable kind, driven by
the event loop's wait timeout.

**Fonts.** Native fonts are CoreText fonts scaled so ascent+descent = the requested pixel
height, rendered to a coverage bitmap and blended. The default portable bitmap font
(`unifont`) needs its `fonts/` directory: set `APP_FONT_PATH`, or ship it as
`Contents/Resources/fonts` (or `fonts/` beside the executable), which is found automatically.

## Testing

`cocoa/test/pixeltest.c` draws one scene into a window surface and a bitmap without showing
anything, requires them identical, and writes a PNG. `cocoa/test/uitest.c` opens a real
window and drives it through the real event loop with synthetic events: button clicks, typing,
backspace, check box, Cmd-D menu shortcut, pull-down menu tracking, window resize, with
in-process view snapshots (`gab_test_snapshot`, no screen-recording permission needed).

## Status

Working: windows (titled, borderless, popup, modal, floating, centred), all portable
controls, menus with shortcuts and mouse tracking, keyboard (Unicode, dead keys, function
and edit keys), mouse (3 buttons + modifiers), cursors, clipboard text, timers, native and
portable fonts, images with transparency, XOR drawing, window resize, C++ consumers.

Known gaps / decisions still open:

- **Retina.** The surface is 1 pixel per point and shown pixel-doubled, unsmoothed, on Retina
  displays: crisp but chunky. A 2x surface needs a scale factor through the drawing code.
- **File dialogs** are GraphApp's portable ones, not `NSOpenPanel`/`NSSavePanel`.
- No mouse wheel event (GraphApp has none). No Alt-key menu mnemonics (Option types characters).
  Input-method marked text is not displayed.
- `WAIT_CURSOR` is the arrow (macOS has no public wait cursor). Palettes are stubs.
- Untested: minimise/restore, multiple displays, dismissing popups by clicking elsewhere.
- Not yet: `.app` bundle, `Info.plist`, icon, signing, notarisation, universal binary.
