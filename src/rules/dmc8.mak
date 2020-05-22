
# Makefile for App (Symantec/DigitalMars C/C++, Static lib version):
# Use this makefile with make, not smake!

CC      = sc
CL      = sc
LD      = lib
CFLAGS  = -mn -o -WA -f -5 -a4 -Dmain=app_main -I. -Iwin32 -Iutility -Igui -Ilibgif -Ilibjpeg -Ilibpng -Ilibz -c
CLOPTSD = -mn -o -WA -f -5 -a4 -I. -L/EXET:NT -L/SU:WINDOWS -L/STUB:WINSTUB.EXE
CLOPTS  = -mn -o -WA -f -5 -a4 -Dmain=app_main -I. -L/EXET:NT -L/SU:WINDOWS -L/STUB:WINSTUB.EXE
LNKOPS  =
LNK     = link
LIBS    = kernel32.lib gdi32.lib user32.lib
RM      = deltree /y

APP_OBJECTS   = utility\apputil.obj  utility\array.obj    utility\border.obj   \
		utility\clipline.obj utility\compose.obj  \
		utility\control.obj  utility\deleting.obj utility\dispatch.obj \
		utility\drawimg.obj  utility\drawing.obj  utility\drawtext.obj \
		utility\fontutil.obj utility\image.obj    utility\imglist.obj  \
		utility\malloc.obj   utility\palette.obj  utility\point.obj    \
		utility\rect.obj     utility\region.obj   utility\resource.obj \
		utility\rgb.obj      utility\str.obj      utility\strtable.obj \
		utility\utf8.obj     utility\utf8regx.obj utility\winutil.obj  \
                gui\button.obj       gui\checkbox.obj     gui\dialog.obj    \
                gui\cursors.obj      gui\dropfld.obj      gui\droplist.obj  \
                gui\field.obj        gui\imagebtn.obj     gui\imgcheck.obj  \
                gui\imglabel.obj     gui\label.obj        gui\listbox.obj   \
                gui\manager.obj     \
                gui\menu.obj         gui\notebtn.obj      gui\passfld.obj   \
		gui\radiobtn.obj     gui\scroll.obj       gui\separat.obj   \
		gui\splitter.obj     gui\tabbtn.obj       gui\textbox.obj   \
                gui\textundo.obj     gui\tip.obj          \
                imgfmt\imgread.obj   imgfmt\imgwrite.obj  \
                imgfmt\readgif.obj   imgfmt\writegif.obj  \
                imgfmt\readh.obj     imgfmt\writeh.obj    \
                imgfmt\readjpg.obj   imgfmt\writejpg.obj  \
                imgfmt\readpng.obj   imgfmt\writepng.obj  \
                win32\bmap.obj       win32\bmapimg.obj    win32\clipbrd.obj  \
                win32\cursor.obj     win32\drawbmap.obj   win32\drawwin.obj  \
		win32\event.obj      win32\folder.obj     win32\font.obj     \
		win32\graphics.obj   win32\init.obj       win32\timer.obj    \
		win32\win.obj 

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

.c.obj:
	@if exist $*.err del $*.err
	$(CC) $(CFLAGS) -o$*.obj $*.c

static:	$(LIB)

$(LIB):	$(HEADERS) $(OBJECTS)
	$(RM) $(LIB)
	$(LD) /b @sc.lst

apptypes.h:	apptypes.c
	$(CL) $(CLOPTSD) apptypes.c
	apptypes.exe
	$(RM) apptypes.exe apptypes.obj apptypes.err

demos:	demo1 demo2 demo3 demo4 demo5

examples:	alldemo arcs blend calc char clock editdraw ellipses fastline fields imagedit imagine imgtest listtest moire nativfnt pizza polygons rainbow scribble scrolls smiley spectrum tabpane tester textclr utf8edit viewutf8 wintypes xortest xortest2 

demo1:	demo\imagine.c
	$(CL) $(CLOPTS) demo\imagine.c $(LIB) $(LIBS) -odemo\imagine.exe $(LNKOPS)

demo2:	demo\tester.c
	$(CL) $(CLOPTS) demo\tester.c $(LIB) $(LIBS) -odemo\tester.exe $(LNKOPS)

demo3:	demo\viewutf8.c
	$(CL) $(CLOPTS) demo\viewutf8.c $(LIB) $(LIBS) -odemo\viewutf8.exe $(LNKOPS)

demo4:	demo\imgtest.c
	$(CL) $(CLOPTS) demo\imgtest.c $(LIB) $(LIBS) -odemo\imgtest.exe $(LNKOPS)

demo5:	demo\blend.c
	$(CL) $(CLOPTS) demo\blend.c $(LIB) $(LIBS) -odemo\blend.exe $(LNKOPS)

alldemo:	..\examples\alldemo.c
	$(CL) $(CLOPTS) ..\examples\alldemo.c $(LIB) $(LIBS) -o..\examples\alldemo.exe $(LNKOPS)

arcs:	..\examples\arcs.c
	$(CL) $(CLOPTS) ..\examples\arcs.c $(LIB) $(LIBS) -o..\examples\arcs.exe $(LNKOPS)

blend:	..\examples\blend.c
	$(CL) $(CLOPTS) ..\examples\blend.c $(LIB) $(LIBS) -o..\examples\blend.exe $(LNKOPS)

calc:	..\examples\calc.c
	$(CL) $(CLOPTS) ..\examples\calc.c $(LIB) $(LIBS) -o..\examples\calc.exe $(LNKOPS)

char:	..\examples\char.c
	$(CL) $(CLOPTS) ..\examples\char.c $(LIB) $(LIBS) -o..\examples\char.exe $(LNKOPS)

clock:	..\examples\clock.c
	$(CL) $(CLOPTS) ..\examples\clock.c $(LIB) $(LIBS) -o..\examples\clock.exe $(LNKOPS)

editdraw:	..\examples\editdraw.c
	$(CL) $(CLOPTS) ..\examples\editdraw.c $(LIB) $(LIBS) -o..\examples\editdraw.exe $(LNKOPS)

ellipses:	..\examples\ellipses.c
	$(CL) $(CLOPTS) ..\examples\ellipses.c $(LIB) $(LIBS) -o..\examples\ellipses.exe $(LNKOPS)

fastline:	..\examples\fastline.c
	$(CL) $(CLOPTS) ..\examples\fastline.c $(LIB) $(LIBS) -o..\examples\fastline.exe $(LNKOPS)

fields:	..\examples\fields.c
	$(CL) $(CLOPTS) ..\examples\fields.c $(LIB) $(LIBS) -o..\examples\fields.exe $(LNKOPS)

imagedit:	..\examples\imagedit.c
	$(CL) $(CLOPTS) ..\examples\imagedit.c $(LIB) $(LIBS) -o..\examples\imagedit.exe $(LNKOPS)

imagine:	..\examples\imagine.c
	$(CL) $(CLOPTS) ..\examples\imagine.c $(LIB) $(LIBS) -o..\examples\imagine.exe $(LNKOPS)

imgtest:	..\examples\imgtest.c
	$(CL) $(CLOPTS) ..\examples\imgtest.c $(LIB) $(LIBS) -o..\examples\imgtest.exe $(LNKOPS)

listtest:	..\examples\listtest.c
	$(CL) $(CLOPTS) ..\examples\listtest.c $(LIB) $(LIBS) -o..\examples\listtest.exe $(LNKOPS)

moire:	..\examples\moire.c
	$(CL) $(CLOPTS) ..\examples\moire.c $(LIB) $(LIBS) -o..\examples\moire.exe $(LNKOPS)

nativfnt:	..\examples\nativfnt.c
	$(CL) $(CLOPTS) ..\examples\nativfnt.c $(LIB) $(LIBS) -o..\examples\nativfnt.exe $(LNKOPS)

pizza:	..\examples\pizza.c
	$(CL) $(CLOPTS) ..\examples\pizza.c $(LIB) $(LIBS) -o..\examples\pizza.exe $(LNKOPS)

polygons:	..\examples\polygons.c
	$(CL) $(CLOPTS) ..\examples\polygons.c $(LIB) $(LIBS) -o..\examples\polygons.exe $(LNKOPS)

rainbow:	..\examples\rainbow.c
	$(CL) $(CLOPTS) ..\examples\rainbow.c $(LIB) $(LIBS) -o..\examples\rainbow.exe $(LNKOPS)

scribble:	..\examples\scribble.c
	$(CL) $(CLOPTS) ..\examples\scribble.c $(LIB) $(LIBS) -o..\examples\scribble.exe $(LNKOPS)

scrolls:	..\examples\scrolls.c
	$(CL) $(CLOPTS) ..\examples\scrolls.c $(LIB) $(LIBS) -o..\examples\scrolls.exe $(LNKOPS)

smiley:	..\examples\smiley.c
	$(CL) $(CLOPTS) ..\examples\smiley.c $(LIB) $(LIBS) -o..\examples\smiley.exe $(LNKOPS)

spectrum:	..\examples\spectrum.c
	$(CL) $(CLOPTS) ..\examples\spectrum.c $(LIB) $(LIBS) -o..\examples\spectrum.exe $(LNKOPS)

tabpane:	..\examples\tabpane.c
	$(CL) $(CLOPTS) ..\examples\tabpane.c $(LIB) $(LIBS) -o..\examples\tabpane.exe $(LNKOPS)

tester:	..\examples\tester.c
	$(CL) $(CLOPTS) ..\examples\tester.c $(LIB) $(LIBS) -o..\examples\tester.exe $(LNKOPS)

textclr:	..\examples\textclr.c
	$(CL) $(CLOPTS) ..\examples\textclr.c $(LIB) $(LIBS) -o..\examples\textclr.exe $(LNKOPS)

utf8edit:	..\examples\utf8edit.c
	$(CL) $(CLOPTS) ..\examples\utf8edit.c $(LIB) $(LIBS) -o..\examples\utf8edit.exe $(LNKOPS)

viewutf8:	..\examples\viewutf8.c
	$(CL) $(CLOPTS) ..\examples\viewutf8.c $(LIB) $(LIBS) -o..\examples\viewutf8.exe $(LNKOPS)

wintypes:	..\examples\wintypes.c
	$(CL) $(CLOPTS) ..\examples\wintypes.c $(LIB) $(LIBS) -o..\examples\wintypes.exe $(LNKOPS)

xortest:	..\examples\xortest.c
	$(CL) $(CLOPTS) ..\examples\xortest.c $(LIB) $(LIBS) -o..\examples\xortest.exe $(LNKOPS)

xortest2:	..\examples\xortest2.c
	$(CL) $(CLOPTS) ..\examples\xortest2.c $(LIB) $(LIBS) -o..\examples\xortest2.exe $(LNKOPS)

monty:	..\examples\monty\monty.c
	$(CL) $(CLOPTS) ..\examples\monty\monty.c $(LIB) $(LIBS) -o..\examples\monty\monty.exe $(LNKOPS)

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
	$(RM) $(LIB)
	$(RM) demo\*.exe
	$(RM) ..\examples\*.exe
	$(RM) ..\examples\monty\*.exe
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
