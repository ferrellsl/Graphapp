# GraphApp
 Cross-platform GUI Library in C for Windows, Linux and MacOS.  
 
This is a fork of the GraphApp GUI library found at http://enchantia.com/software/graphapp/
It is a cross-platform GUI Library in C for Windows, Linux and MacOS but is easily adapted to C++.
See the "Graphapp and C PlusPlus.txt" file for info about using this library with C++.
This version of GraphApp has been updated to create 64-bit binaries on Windows, Linux and MacOS.
If you want 32-bit Windows binaries, then download the original archive from Enchantia and build your binaries from
those sources.  MacOS users will need to ensure that they've installed XQuartz or other X window manager
on the Mac in order to build and run the binaries properly.  Pre-compiled Windows binaries and a static library for
Windows users can be found in the "Windows Binaries" folder of this repository.  This fork has also
been successfully compiled using cygwin and Visual Studio versions 2010 thru 2019.  Successful builds have also been completed 
under MacOS using the XCode command line tools and on Ubuntu 20.  A progress-bar indicator has also been added
to this fork which is not present in the original library.  

A complete reference manual can be found online at:  http://enchantia.com/software/graphapp/doc/manual/index.html
or in the https://github.com/ferrellsl/Graphapp/blob/master/graphapp_manual.pdf file in the root of this repository.

Tutorials can be found here:  http://enchantia.com/software/graphapp/doc/tutorial/index.html

Windows Build:

To build Windows binaries from source using Visual Studio, open a Visual Studio x64 native tools command prompt.  Go to the GraphApp
directory and run the Build_MSVC.bat found there.  Successful builds have been tested on Visual Studio 2010 thru 2019.

Linux Build:

Linux users will need to open a terminal window and move to the GraphApp/src folder and type:  make


MacOS Build

MacOS users will need to install the XCode command line tools prior to compiling.  There is a MacOS makefile
located in the src folder.  Delete the Linux makefile and rename makefile.MacOS to makefile and then run make from the src folder.

GraphApp will use your system fonts by default but if you want UTF8 and custom soft fonts you'll want to copy the fonts
folders found in the GraphApp fonts folder to a folder called "graphappfonts" in the root of the drive where you expect
run your binaries.  This location can be changed/customized by editing line number 39 of fontutil.c found in the src/utility folder
and rebuilding the library.

![alt text](https://github.com/ferrellsl/Graphapp/blob/master/graphapp-demo.png?raw=true)
![alt text](https://github.com/ferrellsl/Graphapp/blob/master/progressbar.png?raw=true)
![alt text](https://github.com/ferrellsl/Graphapp/blob/master/textfields.png?raw=true)
![alt text](https://github.com/ferrellsl/Graphapp/blob/master/rainbow.png?raw=true)
![alt text](https://github.com/ferrellsl/Graphapp/blob/master/tabpane1.png?raw=true)
![alt text](https://github.com/ferrellsl/Graphapp/blob/master/tabpane2.png?raw=true)
![alt text](https://github.com/ferrellsl/Graphapp/blob/master/utf8edit.png?raw=true)
![alt text](https://github.com/ferrellsl/Graphapp/blob/master/unicode.png?raw=true)
