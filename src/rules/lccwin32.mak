
# Makefile for App (LccWin32, Static lib version):

CC      = lcc
CL      = lc
LD      = lcclib
CFLAGS  = -O -Zp4 -e5 -Dmain=app_main -I. -Iwin32 -Iutility -Igui -Iimgfmt -Ilibgif -Ilibjpeg -Ilibpng -Ilibz
CLOPTS  = -O -Zp4 -e5 -Dmain=app_main -I.
LNKOPS  = -s -subsystem windows
LNK     = lcclnk
RM      = del /Q /S

APP_OBJECTS   = utility/apputil.obj  utility/array.obj    utility/border.obj   \
                utility/clipline.obj utility/compose.obj  \
                utility/control.obj  utility/deleting.obj utility/dispatch.obj \
		utility/drawimg.obj  utility/drawing.obj  utility/drawtext.obj \
		utility/image.obj    utility/imglist.obj  utility/fontutil.obj \
		utility/malloc.obj   utility/palette.obj  utility/point.obj    \
		utility/rect.obj     utility/region.obj   utility/resource.obj \
		utility/rgb.obj      utility/str.obj      utility/strtable.obj \
		utility/utf8.obj     utility/utf8regx.obj utility/winutil.obj  \
                gui/button.obj       gui/checkbox.obj     gui/cursors.obj   \
                gui/dialog.obj       gui/dropfld.obj      gui/droplist.obj  \
                gui/field.obj        gui/imagebtn.obj     gui/imgcheck.obj  \
                gui/imglabel.obj     gui/label.obj        gui/listbox.obj   \
                gui/manager.obj     \
                gui/menu.obj         gui/notebtn.obj      gui/passfld.obj   \
                gui/radiobtn.obj     gui/scroll.obj       gui/separat.obj   \
                gui/splitter.obj     gui/tabbtn.obj       gui/textbox.obj   \
                gui/textundo.obj     gui/tip.obj          \
                imgfmt/imgread.obj   imgfmt/imgwrite.obj  \
                imgfmt/readgif.obj   imgfmt/writegif.obj  \
                imgfmt/readh.obj     imgfmt/writeh.obj    \
                imgfmt/readjpg.obj   imgfmt/writejpg.obj  \
                imgfmt/readpng.obj   imgfmt/writepng.obj  \
                win32/bmap.obj       win32/bmapimg.obj    win32/clipbrd.obj  \
                win32/cursor.obj     win32/drawbmap.obj   win32/drawwin.obj  \
                win32/event.obj      win32/folder.obj     win32/font.obj     \
                win32/graphics.obj   win32/init.obj       win32/timer.obj    \
                win32/win.obj

GIF_OBJECTS   = libgif\gif.obj

JPEG_OBJECTS  = libjpeg\jcapimin.obj libjpeg\jcapistd.obj libjpeg\jccoefct.obj \
                libjpeg\jccolor.obj  libjpeg\jcdctmgr.obj libjpeg\jchuff.obj   \
                libjpeg\jcinit.obj   libjpeg\jcmainct.obj libjpeg\jcmarker.obj \
                libjpeg\jcmaster.obj libjpeg\jcomapi.obj  libjpeg\jcparam.obj  \
                libjpeg\jcphuff.obj  libjpeg\jcprepct.obj libjpeg\jcsample.obj \
                libjpeg\jctrans.obj  libjpeg\jdapimin.obj libjpeg\jdapistd.obj \
                libjpeg\jdatadst.obj libjpeg\jdatasrc.obj libjpeg\jdcoefct.obj \
                libjpeg\jdcolor.obj  libjpeg\jddctmgr.obj libjpeg\jdhuff.obj   \
                libjpeg\jdinput.obj  libjpeg\jdmainct.obj libjpeg\jdmarker.obj \
                libjpeg\jdmaster.obj libjpeg\jdmerge.obj  libjpeg\jdphuff.obj  \
                libjpeg\jdpostct.obj libjpeg\jdsample.obj libjpeg\jdtrans.obj  \
                libjpeg\jerror.obj   libjpeg\jfdctflt.obj libjpeg\jfdctfst.obj \
                libjpeg\jfdctint.obj libjpeg\jidctflt.obj libjpeg\jidctfst.obj \
                libjpeg\jidctint.obj libjpeg\jidctred.obj libjpeg\jmemmgr.obj  \
                libjpeg\jmemnobs.obj libjpeg\jquant1.obj  libjpeg\jquant2.obj  \
                libjpeg\jutils.obj

PNG_OBJECTS   = libpng\png.obj       libpng\pngerror.obj  libpng\pngget.obj   \
                libpng\pngmem.obj    libpng\pngpread.obj  libpng\pngread.obj  \
                libpng\pngrio.obj    libpng\pngrtran.obj  libpng\pngrutil.obj \
                libpng\pngset.obj    libpng\pngtrans.obj  libpng\pngwio.obj   \
                libpng\pngwrite.obj  libpng\pngwtran.obj  libpng\pngwutil.obj

LIBZ_OBJECTS  = libz\adler32.obj     libz\compress.obj    libz\crc32.obj    \
                libz\deflate.obj     libz\infback.obj     libz\inffast.obj  \
                libz\inflate.obj     libz\inftrees.obj    libz\trees.obj    \
                libz\uncompr.obj     libz\zutil.obj

OBJECTS       = $(APP_OBJECTS) $(GIF_OBJECTS) $(JPEG_OBJECTS) \
        	$(PNG_OBJECTS) $(LIBZ_OBJECTS)

HEADERS       = apptypes.h app.h

LIB           = graphapp.lib
DLL           = graphapp.dll

.c.obj:
	@if exist $*.err del $*.err
	$(CC) $(CFLAGS) -errout=$*.err -Fo$*.obj $*.c

static:	$(LIB)

$(LIB):	$(HEADERS) $(OBJECTS)
	$(RM) $(LIB)
	mklcclib.bat $(LIB)

dynamic:	$(DLL)

$(DLL):	$(HEADERS) $(OBJECTS)
	$(CC) $(CFLAGS) -DCOMPILE_AS_A_DLL -errout=$win32\init.err -Fo$win32\init.obj $win32\init.c
	$(RM) $(DLL)
	mklcclib.bat $(DLL)

apptypes.h: apptypes.c
	lc -Zp4 apptypes.c
	apptypes.exe
	$(RM) apptypes.exe apptypes.obj apptypes.err

demos: demo1 demo2 demo3 demo4 demo5

examples: alldemo arcs blend calc char clock editdraw ellipses fastline fields imagedit imagine imgtest listtest moire nativfnt pizza polygons rainbow scribble scrolls smiley spectrum tabpane tester textclr utf8edit viewutf8 wintypes xortest xortest2 

demo1:	demo\imagine.obj
	$(CL) $(CLOPTS) demo\imagine.obj $(LIB) -o demo\imagine.exe $(LNKOPS)

demo2:	demo\tester.obj
	$(CL) $(CLOPTS) demo\tester.obj $(LIB) -o demo\tester.exe $(LNKOPS)

demo3:	demo\viewutf8.obj
	$(CL) $(CLOPTS) demo\viewutf8.obj $(LIB) -o demo\viewutf8.exe $(LNKOPS)

demo4:	demo\imgtest.obj
	$(CL) $(CLOPTS) demo\imgtest.obj $(LIB) -o demo\imgtest.exe $(LNKOPS)

demo5:	demo\blend.obj
	$(CL) $(CLOPTS) demo\blend.obj $(LIB) -o demo\blend.exe $(LNKOPS)

alldemo:	..\examples\alldemo.obj
	$(CL) $(CLOPTS) ..\examples\alldemo.obj $(LIB) -o ..\examples\alldemo.exe $(LNKOPS)

arcs:	..\examples\arcs.obj
	$(CL) $(CLOPTS) ..\examples\arcs.obj $(LIB) -o ..\examples\arcs.exe $(LNKOPS)

blend:	..\examples\blend.obj
	$(CL) $(CLOPTS) ..\examples\blend.obj $(LIB) -o ..\examples\blend.exe $(LNKOPS)

calc:	..\examples\calc.obj
	$(CL) $(CLOPTS) ..\examples\calc.obj $(LIB) -o ..\examples\calc.exe $(LNKOPS)

char:	..\examples\char.obj
	$(CL) $(CLOPTS) ..\examples\char.obj $(LIB) -o ..\examples\char.exe $(LNKOPS)

clock:	..\examples\clock.obj
	$(CL) $(CLOPTS) ..\examples\clock.obj $(LIB) -o ..\examples\clock.exe $(LNKOPS)

editdraw:	..\examples\editdraw.obj
	$(CL) $(CLOPTS) ..\examples\editdraw.obj $(LIB) -o ..\examples\editdraw.exe $(LNKOPS)

ellipses:	..\examples\ellipses.obj
	$(CL) $(CLOPTS) ..\examples\ellipses.obj $(LIB) -o ..\examples\ellipses.exe $(LNKOPS)

fastline:	..\examples\fastline.obj
	$(CL) $(CLOPTS) ..\examples\fastline.obj $(LIB) -o ..\examples\fastline.exe $(LNKOPS)

fields:	..\examples\fields.obj
	$(CL) $(CLOPTS) ..\examples\fields.obj $(LIB) -o ..\examples\fields.exe $(LNKOPS)

imagedit:	..\examples\imagedit.obj
	$(CL) $(CLOPTS) ..\examples\imagedit.obj $(LIB) -o ..\examples\imagedit.exe $(LNKOPS)

imagine:	..\examples\imagine.obj
	$(CL) $(CLOPTS) ..\examples\imagine.obj $(LIB) -o ..\examples\imagine.exe $(LNKOPS)

imgtest:	..\examples\imgtest.obj
	$(CL) $(CLOPTS) ..\examples\imgtest.obj $(LIB) -o ..\examples\imgtest.exe $(LNKOPS)

listtest:	..\examples\listtest.obj
	$(CL) $(CLOPTS) ..\examples\listtest.obj $(LIB) -o ..\examples\listtest.exe $(LNKOPS)

moire:	..\examples\moire.obj
	$(CL) $(CLOPTS) ..\examples\moire.obj $(LIB) -o ..\examples\moire.exe $(LNKOPS)

nativfnt:	..\examples\nativfnt.obj
	$(CL) $(CLOPTS) ..\examples\nativfnt.obj $(LIB) -o ..\examples\nativfnt.exe $(LNKOPS)

pizza:	..\examples\pizza.obj
	$(CL) $(CLOPTS) ..\examples\pizza.obj $(LIB) -o ..\examples\pizza.exe $(LNKOPS)

polygons:	..\examples\polygons.obj
	$(CL) $(CLOPTS) ..\examples\polygons.obj $(LIB) -o ..\examples\polygons.exe $(LNKOPS)

rainbow:	..\examples\rainbow.obj
	$(CL) $(CLOPTS) ..\examples\rainbow.obj $(LIB) -o ..\examples\rainbow.exe $(LNKOPS)

scribble:	..\examples\scribble.obj
	$(CL) $(CLOPTS) ..\examples\scribble.obj $(LIB) -o ..\examples\scribble.exe $(LNKOPS)

scrolls:	..\examples\scrolls.obj
	$(CL) $(CLOPTS) ..\examples\scrolls.obj $(LIB) -o ..\examples\scrolls.exe $(LNKOPS)

smiley:	..\examples\smiley.obj
	$(CL) $(CLOPTS) ..\examples\smiley.obj $(LIB) -o ..\examples\smiley.exe $(LNKOPS)

spectrum:	..\examples\spectrum.obj
	$(CL) $(CLOPTS) ..\examples\spectrum.obj $(LIB) -o ..\examples\spectrum.exe $(LNKOPS)

tabpane:	..\examples\tabpane.obj
	$(CL) $(CLOPTS) ..\examples\tabpane.obj $(LIB) -o ..\examples\tabpane.exe $(LNKOPS)

tester:	..\examples\tester.obj
	$(CL) $(CLOPTS) ..\examples\tester.obj $(LIB) -o ..\examples\tester.exe $(LNKOPS)

textclr:	..\examples\textclr.obj
	$(CL) $(CLOPTS) ..\examples\textclr.obj $(LIB) -o ..\examples\textclr.exe $(LNKOPS)

utf8edit:	..\examples\utf8edit.obj
	$(CL) $(CLOPTS) ..\examples\utf8edit.obj $(LIB) -o ..\examples\utf8edit.exe $(LNKOPS)

viewutf8:	..\examples\viewutf8.obj
	$(CL) $(CLOPTS) ..\examples\viewutf8.obj $(LIB) -o ..\examples\viewutf8.exe $(LNKOPS)

wintypes:	..\examples\wintypes.obj
	$(CL) $(CLOPTS) ..\examples\wintypes.obj $(LIB) -o ..\examples\wintypes.exe $(LNKOPS)

xortest:	..\examples\xortest.obj
	$(CL) $(CLOPTS) ..\examples\xortest.obj $(LIB) -o ..\examples\xortest.exe $(LNKOPS)

xortest2:	..\examples\xortest2.obj
	$(CL) $(CLOPTS) ..\examples\xortest2.obj $(LIB) -o ..\examples\xortest2.exe $(LNKOPS)

monty:	..\examples\monty\monty.obj
	$(CL) $(CLOPTS) ..\examples\monty\monty.obj $(LIB) -o ..\examples\monty.exe $(LNKOPS)

tidy:
	$(RM) utility\*.obj
	$(RM) gui\*.obj
	$(RM) imgfmt\*.obj
	$(RM) win32\*.obj
	$(RM) libgif\*.obj
	$(RM) libjpeg\*.obj
	$(RM) libpng\*.obj
	$(RM) libz\*.obj
	$(RM) demo\*.obj
	$(RM) ..\examples\*.obj
	$(RM) ..\examples\monty\*.obj
	$(RM) ..\examples\*.err
	$(RM) ..\examples\monty\*.err
	$(RM) *.obj
	$(RM) *.tds
	$(RM) utility\*.err
	$(RM) gui\*.err
	$(RM) imgfmt\*.err
	$(RM) win32\*.err
	$(RM) libgif\*.err
	$(RM) libjpeg\*.err
	$(RM) libpng\*.err
	$(RM) libz\*.err
	$(RM) demo\*.err
	$(RM) ..\examples\*.err
	$(RM) ..\examples\monty\*.err
	$(RM) *.err
	$(RM) *.map

clean:
	$(RM) utility\*.obj
	$(RM) gui\*.obj
	$(RM) imgfmt\*.obj
	$(RM) win32\*.obj
	$(RM) libgif\*.obj
	$(RM) libjpeg\*.obj
	$(RM) libpng\*.obj
	$(RM) libz\*.obj
	$(RM) demo\*.obj
	$(RM) ..\examples\*.obj
	$(RM) ..\examples\monty\*.obj
	$(RM) ..\examples\*.err
	$(RM) ..\examples\monty\*.err
	$(RM) *.obj
	$(RM) utility\*.err
	$(RM) gui\*.err
	$(RM) imgfmt\*.err
	$(RM) win32\*.err
	$(RM) libgif\*.err
	$(RM) libjpeg\*.err
	$(RM) libpng\*.err
	$(RM) libz\*.err
	$(RM) demo\*.err
	$(RM) *.err
	$(RM) *.map
	$(RM) *.tds
	$(RM) $(LIB)
	$(RM) demo\*.exe
	$(RM) ..\examples\*.exe
	$(RM) ..\examples\monty\*.exe
	$(RM) ..\examples\*.obj
	$(RM) ..\examples\monty\*.obj
	$(RM) ..\examples\*.err
	$(RM) ..\examples\monty\*.err
	$(RM) *.exe
	$(RM) apptypes.h
	$(RM) utility\*.bak
	$(RM) gui\*.bak
	$(RM) imgfmt\*.bak
	$(RM) win32\*.bak
	$(RM) libgif\*.bak
	$(RM) libjpeg\*.bak
	$(RM) libpng\*.bak
	$(RM) libz\*.bak
	$(RM) demo\*.bak
	$(RM) ..\examples\*.bak
	$(RM) ..\examples\monty\*.bak
	$(RM) *.bak
