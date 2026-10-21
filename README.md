# GraphApp
Lightweight cross-platform GUI Library in C for Windows, Linux and MacOS.  MacOS devs have the option of creating macOS/X-Windows binaries or true macOS/Cocoa native apps.  If you choose to create X-Windows binaries under macOS, your target systems must have XQuartz installed so it is recommended that you just use GraphApp's Cocoa back-end instead.
 
This is a fork of the GraphApp GUI library found at http://enchantia.com/software/graphapp/
It is a cross-platform GUI Library in C for Windows, Linux and MacOS but is easily adapted to C++.
See the "Graphapp and C PlusPlus.txt" file for info about using this library with C++.
This version of GraphApp has been updated to create 64-bit binaries on Windows, Linux and MacOS.
If you want 32-bit Windows binaries, then download the original archive from Enchantia and build your binaries from
those sources.  MacOS users will need to ensure that they've installed XQuartz or other X window manager
on the Mac in order to build and run the binaries properly (or use the native Cocoa backend described under "MacOS Native (Cocoa) Build" below, which
needs no X server).  Pre-compiled Windows binaries and a static library for
Windows users can be found in the "Windows Binaries" folder of this repository.  This fork has also
been successfully compiled using MSYS2/cygwin and Visual Studio versions 2010 thru 2019.  Successful builds have also been completed 
under MacOS using the XCode command line tools and on Ubuntu 24.04.  A progress-bar indicator, a busy spinner and mouse-wheel scrolling have also been
added to this fork; the original library has none of them except a basic wheel handler in its last release (3.71).  

The file dialogs that are part of GraphApp are very rudimentary and very ugly.  I recommend using TinyFileDialogs found
here:https://sourceforge.net/projects/tinyfiledialogs/files/ which also provide native file dialogs for the target system.

A complete reference manual can be found online at:  http://enchantia.com/software/graphapp/doc/manual/index.html
or in the https://github.com/ferrellsl/Graphapp/blob/master/graphapp_manual.pdf file in the root of this repository.

Tutorials can be found here:  http://enchantia.com/software/graphapp/doc/tutorial/index.html

<h2>Windows Build</h2>


To build Windows binaries from source using Visual Studio, open a Visual Studio x64 native tools command prompt.  Go to the GraphApp
directory and run the Build_MSVC.bat found there.  Successful builds have been tested on Visual Studio 2010 thru 2019.

<h2>Linux Build</h2>

Linux users simply need to open a terminal window and move to the GraphApp/src folder and type:  make

This will build the link library. To build some of the examples, just type make followed by the name of the example you want to build such as the alldemo example:  make alldemo
or the fastline example by typing: make fastline

Successful builds have been accomplished on Ubuntu 16 thru Ubuntu 24.04.  I have not attempted builds on any other Linux distros.

<h2>MacOS Build/X windows</h2>

MacOS users will need to install the XCode command line tools prior to compiling.  

Go to the Apple Developer More Downloads page. https://developer.apple.com/download/

Sign in with your Apple ID (a paid developer account is not required).

Search for "Command Line Tools" in the left-hand search bar.

Select the version that matches your macOS release and download the .dmg file.

Open the downloaded file and double-click the .pkg installer to run it


There is a macOS makefile located in the src folder.  Delete the Linux makefile and rename makefile.MacOS to makefile and then run make from the src folder. MacOS users will also need an XWindows manager such as XQuartz which can be found here:  https://www.xquartz.org/

<h2>MacOS Native (Cocoa) Build</h2>

GraphApp also has a native Cocoa backend for macOS (Apple Silicon, arm64) in src/cocoa.  It draws real macOS windows, needs no X server and
no XQuartz, and needs only the XCode command line tools.  From the src folder:

    make -f Makefile.cocoa            (builds build-macos/libapp.a)
    make -f Makefile.cocoa demo       (builds the example programs into build-macos/examples)
    make -f Makefile.cocoa test       (runs the backend's pixel test and an end-to-end window/event test)

Link your own programs with libapp.a and: -framework Cocoa -framework CoreText -framework CoreGraphics

Everything is built in the build-macos folder, so the source folders stay clean.  Unlike the Linux makefile this one does not need to be
renamed or edited.  If GraphApp can find its portable bitmap fonts (set APP_FONT_PATH to the fonts folder, or ship them as
Contents/Resources/fonts in an .app bundle) they become the default font, otherwise the default is a native font (Arial).  See src/cocoa/README.md for
how the backend works and what is not done yet (notably Retina resolution, native file dialogs, and app bundling).

<h2>Busy Spinner</h2>

![alt text](https://github.com/ferrellsl/Graphapp/blob/master/busyspinner.gif?raw=true)

A "Busy..." window with a smoothly spinning ring, so users can see that a long task is still working and the program
hasn't frozen.  It is a single header, src/examples/spinner.h, used the same way as the progress bar:

    Window *spin = new_spinner_window(app, "Please wait", NULL);   /* NULL gives the message "Busy..." */
    while (working) {
        /* ...a piece of the work... */
        spinner_step(spin);                    /* call it as often as you like */
    }
    del_spinner_window(spin);

spinner_step() only draws when the next frame is due (60 frames a second) and lets GraphApp handle pending events, so the
window keeps repainting and can be moved.  While a program waits in its main loop instead, the spinner's own timer keeps
it turning.  set_spinner_message() changes the message ("Almost done...").  The ring is drawn antialiased into an image
and copied to the window in one step, so it does not flicker.  It uses only the GraphApp API and builds on Windows,
Linux and macOS.  See src/examples/spinner_demo.c; on Linux build it with:  make spinner_demo
(macOS/Cocoa: make -f Makefile.cocoa examples/spinner_demo).  A Windows build is in the "Windows Binaries" folder.

<h2>Mouse Wheel</h2>

List boxes, text boxes and scroll bars scroll with the mouse wheel on Windows, Linux (X11) and macOS (Cocoa, including
trackpads): 3 lines a notch, or as many as Windows is set to, and sideways with a tilting wheel or Shift + wheel.  The wheel
events that are already waiting are combined into one scroll, so a fast spin, or a remote display such as WSLg, doesn't lag
behind, and the small steps of touchpads and smooth-scrolling mice add up instead of being lost.

Programs can handle the wheel themselves, on a window or on a control, for example to zoom a drawing:

    void wheel(Control *c, int buttons, Point p, int dx, int dy)    /* lines turned; dy > 0 = toward you */
    {
        ...
    }
    on_control_mouse_wheel(canvas, wheel);        /* or on_window_mouse_wheel(win, ...) */

nudge_scroll_bar(bar, amount) moves a scroll bar by an amount and tells its owner.  The handlers of GraphApp 3.70/3.71,
on_window_mouse_wheel_up/down and on_control_mouse_wheel_up/down (one call a notch), work too.  See
src/examples/wheel_demo.c; on Linux build it with:  make wheel_demo   (macOS/Cocoa: make -f Makefile.cocoa examples/wheel_demo).
A Windows build is in the "Windows Binaries" folder.

Custom Soft Fonts and UTF8 Fonts

GraphApp will use your system fonts by default but if you want UTF8 and custom soft fonts you'll want to copy the fonts
folders found in the archive under GraphApp/fonts to a folder called "graphappfonts" in the root of the drive where you expect to
run your binaries.  This location can be changed or customized by editing line number 39 of fontutil.c found in the src/utility folder
and rebuilding the library.  Some of the examples and demos such as "blend" and "alldemo" will fail to run unless you've copied the aforementioned fonts to the
location mentioned earlier.

![alt text](https://github.com/ferrellsl/Graphapp/blob/master/graphapp-demo.png?raw=true)
![alt text](https://github.com/ferrellsl/Graphapp/blob/master/progressbar.png?raw=true)
![alt text](https://github.com/ferrellsl/Graphapp/blob/master/textfields.png?raw=true)
![alt text](https://github.com/ferrellsl/Graphapp/blob/master/rainbow.png?raw=true)
![alt text](https://github.com/ferrellsl/Graphapp/blob/master/tabpane1.png?raw=true)
![alt text](https://github.com/ferrellsl/Graphapp/blob/master/tabpane2.png?raw=true)
![alt text](https://github.com/ferrellsl/Graphapp/blob/master/utf8edit.png?raw=true)
![alt text](https://github.com/ferrellsl/Graphapp/blob/master/unicode.png?raw=true)
