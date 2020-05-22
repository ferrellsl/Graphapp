
For every system :
edit 'src/utility/fontutil.c' and change app_font_install_dir (line 33)

From GraphApp_N.MM directory:
- nmake /f builds\msvc.mak (Windows with MSVC)
- make -f builds/foo.mak (Unix-like, including Windows with GNU tools)

(Note some warnings when compiling examples)
