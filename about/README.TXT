
Important Note

   App is distributed in two ways:
      - as a complete package which includes all source code and fonts, or
      - as a pre-compiled library package.

   Each "pre-compiled library" package is designed for a specific
   version of a specific compiler. There is no guarantee that the
   library files provided in such packages will work if either the
   version number or compiler are different!

   The source code package is more difficult to use because it
   requires compilation, but at least you can be sure there are
   no viruses.

Unpacking App

   On Linux or Unix, unpack the compressed archive file into
   a new directory, called "app". Inside you'll find subdirectories
   such as fonts/ (which contains the font images) and src/
   (which contains the source code files).

   On Windows, it might be best to create an "apps" directory
   on your C: drive, and unpack the zip file into a directory
   called "app" within the "apps" directory. In other words,
   the tree of folders should look like this:

	C:/
	   Apps/		# create this on C:
		App/		# unpack things here
		    fonts/
		    src/
			win32/
			etc


Where should I put the fonts?

   The fonts/ subdirectory will have to be located in the same
   place as the fontutil.c file expects it to be. On Windows, this
   is hard-coded as "/apps/app/fonts/" so the above arrangement
   will work fine for Windows.

   On Linux platforms, you may have to change the path given in
   src/utility/fontutil.c to be whatever public directory you
   want the fonts to be served from. Personally, I use:

	$HOME/apps/app/fonts

   which is in the font search path, so this works without changes.
   But you might want to make these fonts available to many users
   in some more public location, such as:

	/usr/share/app/fonts/

   or

	/usr/local/lib/app/fonts/

   You decide, and change fontutil.c to point to the correct place.
   In future I may make this code search in a variety of likely
   locations, but for now it is hard-coded for efficiency.


Compiling App

   To compile app, do the following:

   cd app/src     # this directory
   ./configure    # create Makefile and apptypes.h
   make           # create objects files and app library


But I'm using Windows!

   Have a look in the src/builds folder. It contains some
   instructions on how to create the library for a variety
   of Windows compilers.

   If that doesn't work, there might be a specialised
   Makefile for your compiler in the src/rules/ folder.

   If that still doesn't work, read on.


But I cannot use configure on my machine!

   If you can't use configure, don't worry. It's only useful on
   Linux/Unix platforms. You can do without it.

   There might be a makefile for your compiler and platform
   in the src/rules/ folder. Look there and read the instructions.
   If not, the process of compilation is as follows.

   First, you should compile and run the apptypes.c program.
   This will create a file called apptypes.h which is specific
   to your operating system and compiler. It finds appropriate
   integer types which your C compiler can use when compiling
   other parts of App.

   Then you should compile all *.c files in all subdirectories.
   All of these (except the demo/ programs) are linked together
   to produce the App library.

   If compiling for Windows, don't compile the src/x11/ files.
   If compiling for Linux, don't compile the src/win32/ files.


More detail on compilation

   The configure script finds a compiler, linker, X libraries,
   and builds a Makefile from the template in rules/x11rules.txt
   This is only useful on a Linux/Unix platform, since it uses
   the Bourne shell scripting language and the make utility.

   Alternatively, you might find a pre-built Makefile for your
   operating system in the rules/ directory. If not, you can
   make a Makefile by modifying a copy of an existing one.

   The Makefile basically says this:

	compile all files in the following directories:
		utility/
		gui/
		imgfmt/
		x11/		(or win32/ if using Windows)
		libpng/
		libz/
		libjpeg/
		libgif/

	then link all the objects produced into the App library.

   To compile these files, all the header files from all those
   directories may need to be found. For example, many files need
   to be able to find the app.h header which is in the main src/
   directory. Some files will need to find the platform-specific
   appint.h header file, which is in x11/ or win32/. The libpng/
   files need to find header files from the libz/ directory,
   as well as the apptypes.h header file in the src/ directory.
   And so on.

   If your compiler has the ability to add multiple directories to
   the location to search for header files, add all the directories
   mentioned above. (On Linux, the gcc compiler does this with
   the -I flag, for example gcc -I. -Ilibz adds "." and "libz"
   to the compiler's search path for header files. Your compiler
   may have a different approach.)

   If your compiler can't add search paths, you can instead just
   copy the required header files to wherever your compiler complains
   they are missing from.

   Another solution is to create a "build" directory, for example,
   if you are using Windows, you could type the following in a DOS
   window:

        cd app/src
	mkdir build
	copy *.h build
	copy utility/*.* build
	copy gui/*.* build
	copy win32/*.* build
	copy imgfmt/*.* build
	copy libpng/*.* build
	copy libz/*.* build
	copy libjpeg/*.* build
	copy libgif/*.* build

   Then in your compiler, create a library project in that build/
   directory and add all the *.c files to it, and compile. The
   compiler should then be able to find all the appropriate header
   files with no problems.


Compiling programs to use App

   The demo/ subdirectory contains a few demonstration programs.
   The simplest is tester.c, while a more complex demonstration
   is the imagine.c program, which allows you to view image files
   in PNG or JPEG format.

   To compile one of these programs, you'll need to:
	- first build the App library
	- compile the demo program, e.g. imagine.c
	- then link the library with the compiled program to
	  produce an application (executable)
	- try running it, by double-clicking, or dragging an
	  image file onto the compiled application

   If using Linux, you can just type "make demo" in the src/
   directory and it will create the imagine program for you.


Linkage issues on Linux

   For programs to use App, you'll need to link them with the
   X-Windows library libX11 as well as libapp.a and use the
   -lc -lm options too. -lm is used to link in the mathematical
   functions such as pow() which are called from the libpng/ files,
   and some of the drawing functions.

   Your compiler might automatically link in mathematical functions,
   so the -lm option may or may not be needed.

   Look in the file examples/Makefile to see how I compile some
   example programs. Copy this technique for your own
   programs. You may need to modify that Makefile so that it
   uses the correct location to load the libapp.a library.


Linkage issue on Windows

   Some Windows compilers won't allow programs to have a main function,
   instead relying on a function called WinMain for startup.
   This can cause all kinds of problems:

   Some will allow main to exist, but try to generate WinMain itself
   (possible causing an error in the App win32/init.c code, since
   there is already a WinMain function there which calls main).

   Some will look for main, and if it is found will treat the program
   as if it was a console application, so that running the program
   will cause a console window to appear. WinMain will be ignored.
   (This is a problem since the program won't know its HINSTANCE.)

   Some will look for WinMain, and if it is found, main can exist
   and will be an ordinary function (which is what we want).

   If your compiler has problems with both a main and a WinMain
   function existing in the same program, you can redefine main
   as a different function (I suggest app_main for consistency
   with the rest of the library). The best way to do this is in
   your makefile. Add -Dmain=app_main to the makefile's CFLAGS
   (or other compiler options). This way, everywhere main is
   defined or called it will actually be app_main, and everything
   should work.

   You could also manually change main to app_main in win32/init.c
   but then you'd have to do it all through the example programs too.
   The -Dmain=app_main is a more elegant solution because the
   source code files can remain the same. In essence, -Dmain=app_main
   is the same as putting #define main app_main at the top of app.h
   (which is another solution if your compiler doesn't allow -D flags).

   Read the documentation of your compiler to ensure the -D flag
   works as described above, before using it. You'll need to do
   this in both the App library's makefile and the makefile used
   to generate the examples and your own programs.
