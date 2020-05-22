
# Makefile for App (Watcom C/C++ 11, 11c, OpenWatcom 1.0 Static lib version):

CC		=wcc386
CL		=wcl386
LD		=lib
CFLAGS	=/zq /5r /fpi87 /fp5 /oneatx /ei /zp4 /I. /Iwin32 /Iutility /Igui /Ilibgif /Ilibjpeg /Ilibpng /Ilibz /Dmain=app_main
CLOPTS	=/zq /5r /fpi87 /fp5 /oneatx /ei /zp4 /I. /Iwin32 /Iutility /Igui /Ilibgif /Ilibjpeg /Ilibpng /Ilibz /Dmain=app_main /"option quiet" /bt=nt /l=win95
RM		=deltree /y

APP_OBJECTS   = utility\apputil.obj  utility\array.obj    utility\border.obj   \
                utility\clipline.obj utility\compose.obj  \
                utility\control.obj  utility\deleting.obj utility\dispatch.obj \
		utility\drawimg.obj  utility\drawing.obj  utility\drawtext.obj \
		utility\image.obj    utility\imglist.obj  utility\fontutil.obj \
		utility\malloc.obj   utility\palette.obj  utility\point.obj    \
		utility\rect.obj     utility\region.obj   utility\resource.obj \
		utility\rgb.obj      utility\str.obj      utility\strtable.obj \
		utility\utf8.obj     utility\utf8regx.obj utility\winutil.obj  \
                gui\button.obj       gui\checkbox.obj     gui\cursors.obj   \
                gui\dialog.obj       gui\dropfld.obj      gui\droplist.obj  \
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
	$(CC) $(CFLAGS) -fo=$*.obj $*.c

static:	$(LIB)

$(LIB):	$(HEADERS) $(OBJECTS)
	$(RM) $(LIB)
	$(LD) -nologo -out:$(LIB) win32\*.obj utility\*.obj gui\*.obj imgfmt\*.obj libgif\*.obj libjpeg\*.obj libpng\*.obj libz\*.obj

apptypes.h: apptypes.c
	$(CL) /zq /"option quiet" /fo=apptypes.exe apptypes.c
	apptypes.exe
	$(RM) apptypes.exe apptypes.obj

demos:	demo1 demo2 demo3 demo4 demo5

examples: alldemo arcs blend calc char clock editdraw ellipses fastline fields imagedit imagine imgtest listtest moire nativfnt pizza polygons rainbow scribble scrolls smiley spectrum tester tabpane textclr utf8edit viewutf8 wintypes xortest xortest2 

demo1:	static demo\imagine.obj
	$(CL) $(CLOPTS) /fe=demo\imagine.exe demo\imagine.obj $(LIB)

demo2:	static demo\tester.obj
	$(CL) $(CLOPTS) /fe=demo\tester.exe demo\tester.obj $(LIB)

demo3:	static demo\viewutf8.obj
	$(CL) $(CLOPTS) /fe=demo\viewutf8.exe demo\viewutf8.obj $(LIB)

demo4:	static demo\imgtest.obj
	$(CL) $(CLOPTS) /fe=demo\imgtest.exe demo\imgtest.obj $(LIB)

demo5:	static demo\blend.obj
	$(CL) $(CLOPTS) /fe=demo\blend.exe demo\blend.obj $(LIB)

alldemo:	..\examples\alldemo.obj
	$(CL) $(CLOPTS) /fe=..\examples\alldemo.exe ..\examples\alldemo.obj $(LIB)

arcs:	..\examples\arcs.obj
	$(CL) $(CLOPTS) /fe=..\examples\arcs.exe ..\examples\arcs.obj $(LIB)

blend:	..\examples\blend.obj
	$(CL) $(CLOPTS) /fe=..\examples\blend.exe ..\examples\blend.obj $(LIB)

calc:	..\examples\calc.obj
	$(CL) $(CLOPTS) /fe=..\examples\calc.exe ..\examples\calc.obj $(LIB)

char:	..\examples\char.obj
	$(CL) $(CLOPTS) /fe=..\examples\char.exe ..\examples\char.obj $(LIB)

clock:	..\examples\clock.obj
	$(CL) $(CLOPTS) /fe=..\examples\clock.exe ..\examples\clock.obj $(LIB)

editdraw:	..\examples\editdraw.obj
	$(CL) $(CLOPTS) /fe=..\examples\editdraw.exe ..\examples\editdraw.obj $(LIB)

ellipses:	..\examples\ellipses.obj
	$(CL) $(CLOPTS) /fe=..\examples\ellipses.exe ..\examples\ellipses.obj $(LIB)

fastline:	..\examples\fastline.obj
	$(CL) $(CLOPTS) /fe=..\examples\fastline.exe ..\examples\fastline.obj $(LIB)

fields:	..\examples\fields.obj
	$(CL) $(CLOPTS) /fe=..\examples\fields.exe ..\examples\fields.obj $(LIB)

fontedit:	..\examples\fontedit.obj
	$(CL) $(CLOPTS) /fe=..\examples\fontedit.exe ..\examples\fontedit.obj $(LIB)

fontsave:	..\examples\fontsave.obj
	$(CL) $(CLOPTS) /fe=..\examples\fontsave.exe ..\examples\fontsave.obj $(LIB)

imagedit:	..\examples\imagedit.obj
	$(CL) $(CLOPTS) /fe=..\examples\imagedit.exe ..\examples\imagedit.obj $(LIB)

imagine:	..\examples\imagine.obj
	$(CL) $(CLOPTS) /fe=..\examples\imagine.exe ..\examples\imagine.obj $(LIB)

imgtest:	..\examples\imgtest.obj
	$(CL) $(CLOPTS) /fe=..\examples\imgtest.exe ..\examples\imgtest.obj $(LIB)

listtest:	..\examples\listtest.obj
	$(CL) $(CLOPTS) /fe=..\examples\listtest.exe ..\examples\listtest.obj $(LIB)

moire:	..\examples\moire.obj
	$(CL) $(CLOPTS) /fe=..\examples\moire.exe ..\examples\moire.obj $(LIB)

nativfnt:	..\examples\nativfnt.obj
	$(CL) $(CLOPTS) /fe=..\examples\nativfnt.exe ..\examples\nativfnt.obj $(LIB)

pizza:	..\examples\pizza.obj
	$(CL) $(CLOPTS) /fe=..\examples\pizza.exe ..\examples\pizza.obj $(LIB)

polygons:	..\examples\polygons.obj
	$(CL) $(CLOPTS) /fe=..\examples\polygons.exe ..\examples\polygons.obj $(LIB)

rainbow:	..\examples\rainbow.obj
	$(CL) $(CLOPTS) /fe=..\examples\rainbow.exe ..\examples\rainbow.obj $(LIB)

scribble:	..\examples\scribble.obj
	$(CL) $(CLOPTS) /fe=..\examples\scribble.exe ..\examples\scribble.obj $(LIB)

scrolls:	..\examples\scrolls.obj
	$(CL) $(CLOPTS) /fe=..\examples\scrolls.exe ..\examples\scrolls.obj $(LIB)

smiley:	..\examples\smiley.obj
	$(CL) $(CLOPTS) /fe=..\examples\smiley.exe ..\examples\smiley.obj $(LIB)

spectrum:	..\examples\spectrum.obj
	$(CL) $(CLOPTS) /fe=..\examples\spectrum.exe ..\examples\spectrum.obj $(LIB)

tabpane:	..\examples\tabpane.obj
	$(CL) $(CLOPTS) /fe=..\examples\tabpane.exe ..\examples\tabpane.obj $(LIB)

tester:	..\examples\tester.obj
	$(CL) $(CLOPTS) /fe=..\examples\tester.exe ..\examples\tester.obj $(LIB)

textclr:	..\examples\textclr.obj
	$(CL) $(CLOPTS) /fe=..\examples\textclr.exe ..\examples\textclr.obj $(LIB)

utf8edit:	..\examples\utf8edit.obj
	$(CL) $(CLOPTS) /fe=..\examples\utf8edit.exe ..\examples\utf8edit.obj $(LIB)

viewutf8:	..\examples\viewutf8.obj
	$(CL) $(CLOPTS) /fe=..\examples\viewutf8.exe ..\examples\viewutf8.obj $(LIB)

wintypes:	..\examples\wintypes.obj
	$(CL) $(CLOPTS) /fe=..\examples\wintypes.exe ..\examples\wintypes.obj $(LIB)

xortest:	..\examples\xortest.obj
	$(CL) $(CLOPTS) /fe=..\examples\xortest.exe ..\examples\xortest.obj $(LIB)

xortest2:	..\examples\xortest2.obj
	$(CL) $(CLOPTS) /fe=..\examples\xortest2.exe ..\examples\xortest2.obj $(LIB)

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
