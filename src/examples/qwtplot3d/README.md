# QwtPlot3D for GraphApp

An example of moving a Qt application to GraphApp: QwtPlot3D 0.2.6, a 3D surface plotting library,
with Qt removed, and its demo rebuilt on GraphApp. This is an altered version of Michael Bieber's
library, not the original. See `COPYING` (zlib license) and `3rdparty/gl2ps/COPYING.*`. It runs on
Windows, on Linux with X11 and on macOS.

![QwtPlot3D in GraphApp showing wiazrd1580.mes, a 1581x1581 mesh of a volcano in false color with shading](qwtplot3d-volcano-false-color.png)

The Qt build this came from was a 60 MB folder of programs, Qt DLLs and plugins. This demo is one
small executable that links only `OPENGL32`, `GLU32` and Windows system DLLs, and it fits on a single
1.44 MB floppy with room to spare. The Qt folder would take about 40. The Linux build is a single
926 KB file that needs only the system's OpenGL, GLU and X11 libraries, so it fits on a floppy too. The
macOS build is one 1 MB executable per architecture (2 MB with both in one file) that links only Apple's
system frameworks.

## What the port took

About 9,000 lines of library code, most of them raw OpenGL that never touched Qt. The work was in a
few places:

- `QGLWidget` became a small child window with an OpenGL context.
- `QString` became `std::string`, and `QFont` a small value type.
- Label text, drawn by `QPainter` before, now comes from a GraphApp bitmap.
- Signals became `std::function` members.

Moving it to X then took one file for the OpenGL window, `src/qwt3d_glwidget_x11.cpp`, and a small
bridge to GraphApp's insides, and macOS took the same two, `src/qwt3d_glwidget_cocoa.mm` and
`src/qwt3d_cocoa_bridge.c`. The library and the 1,500-line demo compile unchanged, and so does
GraphApp: because the port keeps everything that depends on the system behind `GLWidget`, the plot
and its window don't know which one they're on.

## Build

**Windows:** run `build.bat` from an x64 Native Tools Command Prompt. It uses this repository's
GraphApp headers and `Windows Binaries\graphapp.lib`, and produces `lib\qwtplot3d.lib` and
`demo\surface_demo.exe`.

**Linux:** run `make` in the `src` folder for `libapp.a`, then `sh build.sh` here. It needs the
`libgl-dev`, `libglu1-mesa-dev` and `libx11-dev` packages and produces `lib/libqwtplot3d.a` and
`demo/surface_demo`. Set `GRAPHAPP_LIB` to link another `libapp.a`.

**macOS:** run `make -f Makefile.cocoa` in the `src` folder for `build-macos/libapp.a`, then
`sh build-macos.sh` here. It needs the Xcode command line tools and produces
`lib/libqwtplot3d_macos.a` and `demo/surface_demo_macos`. For a universal binary, build a universal
`libapp.a` and run `GRAPHAPP_LIB=... ARCHS="-arch arm64 -arch x86_64" BUILD=build-macos/universal sh build-macos.sh`.
The library calls Apple's deprecated but still shipped OpenGL 2.1 legacy context, so the script defines
`GL_SILENCE_DEPRECATION`, adds `-Wno-c++11-narrowing` (newer clang rejects some initializers in the
library, which is left as it was), and puts `src/macos_gl` on the include path, where `GL/gl.h` and
`GL/glu.h` forward to Apple's `OpenGL/` headers.

GraphApp itself needs no changes.

## Use

```cpp
#include <qwt3d_surfaceplot.h>   // before graphapp.h: its macros would break these headers
#include <qwt3d_function.h>

extern "C" {
#include "graphapp.h"
}

Window* win = new_window(app, rect(60, 60, 900, 680), "Plot", STANDARD_WINDOW);
Qwt3D::SurfacePlot* plot = new Qwt3D::SurfacePlot(win, 0, 0, 900, 680);
// load data as with the Qt version, then set axis labels: new data resets them
show_window(win);
plot->show();
main_loop(app);
```

Link `qwtplot3d.lib`, `graphapp.lib`, `opengl32.lib` and `glu32.lib`, or on Linux `libqwtplot3d.a`,
`libapp.a` and `-lGL -lGLU -lX11 -lm`, or on macOS `libqwtplot3d_macos.a`, `libapp.a` and
`-framework Cocoa -framework OpenGL -framework CoreText -framework CoreGraphics -framework QuartzCore`. Resize the plot with `plot->setGeometry()` from your
`on_window_resize` handler.

Set an axis label on all four parallel axes (`X1`..`X4` and so on): the box shows whichever one
faces the viewer.

## Prebuilt binary

`bin\surface_demo.exe` is a 64-bit Windows build. It needs no other files: the C runtime is linked in
and the icons are embedded. It is linked as a GUI program, so it opens no console window.

`bin/surface_demo` is a 64-bit Linux build, made on Ubuntu 24.04. It needs the system's OpenGL, GLU
and X11 libraries. For the file dialogs, GraphApp uses zenity, kdialog, qarma or matedialog if one is
installed, and its own dialog if not.

`bin/surface_demo_macos` is a universal (arm64 and x86_64) macOS build for macOS 11 or later. It needs no
other files and links only Apple's system frameworks. It is not signed or notarized, so Gatekeeper may
ask you to approve it the first time. The Intel half has only been launched, under Rosetta.

## LIDAR grids

`bin\surface_demo.exe grid.mes`, or File > Open. Color > Data color takes `.map` files of `r g b`
lines from 0 to 255, and `colormaps\` has 141 of them. No grids are included. **Shift+wheel exaggerates Z** and the plain wheel zooms,
as in mesh2: each notch adds 0.0513 to the Z scale. Grids open at 1 point in 3, like mesh2; the
Resolution menu goes down to every point. The title bar shows what mesh2's status bar did.

The window follows mesh2. The toolbar has open, FEM open, save with its format list, the function
and surface lists, animation, coordinate style, the six side grids, six plot styles, three floor
styles and normals, with mesh2's icons (embedded in `demo/icons.h`). The menus have the color and
font choices. On the right are Ortho, Legend, Autoscale, Mouse and Shading, with sliders for the
quality and length of the normals; below are Std, the lighting check box, the Lighting dialog, and
sliders for Polygon Offset and Resolution. In the dialog, turn the sphere with the mouse to move the
light; its sliders set Distance, Emission, Ambient and Diffuse, Specular and Shininess.
Lighting turns relief into shading, which often shows more than color alone.

Unlike mesh2, a grid opens in the Filled style, because the mesh lines of a dense grid come out
solid black, and with box axes, where mesh2 starts with none. File > Save As is an addition.

Tested on 1409x1409 and 1581x1581 LIDAR grids (2.5 million points): about 1.3 s to open, and at
every resolution redraws keep up with the 240 Hz display. A failed read leaves the plot as it was.

## What changed from the Qt version

| Qt | Here |
|---|---|
| `QGLWidget` | `Qwt3D::GLWidget`, a child window with an OpenGL context |
| `SurfacePlot(QWidget*)` | `SurfacePlot(Window*, x, y, width, height)` |
| `QString`, `QStringList` | `std::string` (UTF-8), `std::vector<std::string>` |
| `QFont` | `Qwt3D::Font` |
| Signals | `std::function` members: `onRotationChanged`, `onShiftChanged`, `onViewportShiftChanged`, `onScaleChanged`, `onZoomChanged`, `onProjectionChanged`, `onResolutionChanged` |
| `Qt::LeftButton`, `Qt::ShiftModifier` | `Qwt3D::LeftButton`, `Qwt3D::ShiftModifier` |
| Image formats from Qt | PNG and GIF, from GraphApp |
| `renderPixmap` | Removed; `grabFrameBuffer` returns RGBA |
| `PixmapWriter::setQuality` | Kept, has no effect |

Everything else, including mouse and keyboard defaults, is as it was. Labels are now antialiased,
where the original had a 1-bit mask.

EPS, PS and PDF output is the same, but faster on large plots:

- Hidden surfaces are left out of the file (gl2ps's occlusion culling) only for plots of up to
  50,000 primitives: the time it takes grows much faster than the plot (38 s of a 39 s export for
  283,000 triangles), and few parts of a surface are hidden. That PDF now takes 1.3 s.
  `VectorWriter::setOcclusionCullLimit` changes the limit.
- gl2ps's buffer doubles until the plot fits, instead of growing by 2 M floats and drawing the plot
  again each time (13 passes for a 751 x 752 grid).
- `VectorWriter::setBusyCallback` sets a function called now and then while the file is written.
  The demo uses it for GraphApp's busy spinner (`src/examples/spinner.h`), shown if the file
  takes more than half a second.

For these, `3rdparty/gl2ps/gl2ps.c` has two additions, `gl2psSetBusyCallback` and
`gl2psSetOcclusionCullLimit`; the rest of gl2ps 1.3.9 is unchanged.

## How it sits on GraphApp

- `src/qwt3d_glwidget_win32.cpp` is the only file that sees `windows.h` and WGL.
- `src/qwt3d_graphapp_host.cpp` is the only file that sees `graphapp.h`.

The plot is a child window because GraphApp fills its window with GDI before every redraw and does
not report Alt with the mouse or Alt+arrow keys. Three GraphApp behaviors shape that file:

- **Menus:** GraphApp draws them on the window with GDI, which cannot draw over an OpenGL child. When
  the parent gets a left press or Alt key, the plot's current frame is copied to the parent and the
  child hides until GraphApp returns. No parent redraw may happen in between, because a `WM_PAINT`
  ends GraphApp's menu loop.
- **Subclassing:** `app_winproc` ignores a window unless `GWLP_WNDPROC` is itself, so the parent's
  original procedure is put back around every call.
- **Text beyond Latin-1:** GraphApp's native fonts keep only the low byte of a character (pi comes
  out as A with a grave accent), so such strings go through GDI directly. Everything else uses
  GraphApp fonts.

The parent's HWND is read as the first field of GraphApp's private `WindowExtra`.

### On Cocoa

- `src/qwt3d_glwidget_cocoa.mm` is the only file that sees AppKit and Apple's OpenGL.
- `src/qwt3d_cocoa_bridge.c` is the only file that sees GraphApp's private headers.

GraphApp's window is an `NSWindow` whose content view blits a pixel surface, and its menus are drawn
into that surface, so the plot is an `NSOpenGLView` placed over it. The parent's `NSWindow` is the first
field of GraphApp's private `WindowExtra`, as the HWND is on Windows and the X window is on X11. Details
that differ:

- **Menus:** the same answer as on Windows and X11. The menu bar's press handler is wrapped, the current
  frame is copied into the window's surface and the view hidden, then shown again when GraphApp returns.
- **Keys:** the view never takes the keyboard, so keys reach the main window, where the menu shortcuts
  and key handlers are. The arrow keys arrive through the main window's key action.
- **Retina:** widths and positions are in points, as everywhere in GraphApp. The OpenGL surface is in
  pixels, so the viewport and `grabFrameBuffer` use the backing size, and a saved picture of a plot 900
  points wide is 1800 pixels wide. Label text is drawn at the display's backing factor, so labels are
  the same size relative to the plot at 1x and 2x.
- **Mouse:** Ctrl+click arrives from macOS as a right click with Control, and is treated as the left
  button with Ctrl, so Ctrl+drag moves the plot. Option is Alt. Shift+wheel arrives as a horizontal
  scroll, and is read as such. The wheel follows the system's natural scrolling setting, so turning it
  away from you zooms in, as on Windows. A trackpad's 36 points count as one notch.
- **Color and font:** `NSColorPanel` with OK and Cancel buttons added, and `NSFontPanel`. After either
  closes, the plot's window is made the key window again.
- **Text beyond Latin-1:** drawn with Core Text, so pi, Omega, alpha and Sigma come out as themselves, in titles and axis labels.

### On X11

- `src/qwt3d_glwidget_x11.cpp` is the only file that sees Xlib and GLX.
- `src/qwt3d_x11_bridge.c` is the only file that sees GraphApp's private headers.

GraphApp reads the X events itself and drops those of windows it doesn't know. So an ordinary
GraphApp window is created and its X window swapped for a child with an OpenGL visual, on GraphApp's
own connection, and its event loop then serves the plot like any other window. The child is never
shown with `show_window`, so the main loop doesn't wait for it. Four things differ from Windows:

- **Keys:** the child doesn't ask for key events, so X passes them up to the main window, where the
  menu shortcuts and key handlers are. The arrow keys arrive through the main window's key action.
- **Menus:** the same answer as on Windows. The menu bar's press handler is wrapped, the frame is
  copied to the parent and the child unmapped. The parent's expose events are dropped from the queue,
  because one would end GraphApp's menu loop.
- **Mouse state:** GraphApp reports Shift+left as the right button, so X is asked what is really held.
- **Text beyond Latin-1:** drawn with X's own iso10646 fonts, so no extra library is needed.

To port this to another system, start from those two files: everything else stays as it is.

## Color maps

The `.map` files in `colormaps\` came with the Qt version's binaries. Their license is unknown, and
they are not covered by the licenses above.

## Limits

- Windows, Linux with X11 and macOS.
- File dialogs and messages come from GraphApp. Its file dialogs take no filter, so all files are listed and the title says what to pick. It has no color or font dialog: Windows uses its own, macOS uses its color and font panels, and on Linux you type the color (`255 128 0`) or font (`Helvetica 12 bold`).
- On Linux the labels use GraphApp's bitmap X fonts, so they look plainer than on Windows, and text beyond Latin-1 comes out as wrong letters if the X server has no iso10646 fonts. WSLg has none, and that path has not yet been seen working.
- JPEG is not offered: `app_save_jpeg` in GraphApp is a stub that always fails.
- Needs a legacy OpenGL compatibility context. Tested on one GPU only. On Linux, tested only under WSLg, which runs X through Xwayland.
- macOS uses Apple's OpenGL, which is deprecated but still shipped, in a 2.1 legacy context. If Apple removes it, a Metal version would be a rewrite of the library's fixed-function drawing, not of the window. Tested on one Apple Silicon Mac under one macOS. The Intel half of the universal binary has only been launched, under Rosetta, and two things were not checked by script, only by what the code does: a click in the font panel's list, and the OK of the system's save panel, which runs out of process.
- The demo is not DPI aware, so at 150% scaling Windows enlarges it and text looks soft.
- Differences from mesh2: no color map preview, the save list has PNG, GIF, EPS, PS and PDF only,
  EPS and PDF text is part of the file where mesh2 wrote a separate TeX file, and the title bar
  stands in for its status bar.
