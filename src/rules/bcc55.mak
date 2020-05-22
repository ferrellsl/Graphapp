
# Makefile for App (Borland 5.5 C/C++, Static lib version):

CC      = bcc32
CL      = bcc32
LD      = tlib
CFLAGS  = -q -5 -a4 -O2 -OS -Oi -ff -W -c -I. -Iwin32 -Iutility -Igui -Ilibgif -Ilibjpeg -Ilibpng -Ilibz
CLOPTS  = -q -5 -a4 -O2 -OS -Oi -ff -W -I.
CCLOPTS = -q -5 -a4 -O2 -OS -Oi -ff -I.
LNKOPS  =
LNK     = ilink
LIBS    =
RM      = deltree /y
CP      = copy

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
                gui/menu.obj         gui/notebtnobj       gui/passfld.obj   \
                gui/radiobtn.obj     gui/scroll.obj       gui/separat.obj   \
                gui/splitter.obj     gui/tabbtn.obj       gui/textbox.obj   \
                gui/textundo.obj     gui/tip.c            \
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

.c.obj:
	@if exist $*.err del $*.err
	$(CC) $(CFLAGS) -o$*.obj $*.c

static:	$(LIB)

$(LIB):	$(HEADERS) $(OBJECTS)
	$(RM) $(LIB)
	$(LD) /C $(LIB) @bcc.lst

apptypes.h:	apptypes.c
	$(CL) $(CCLOPTS) apptypes.c
	apptypes.exe
	$(RM) apptypes.exe apptypes.obj apptypes.err apptypes.tds

demos:	demo1 demo2 demo3 demo4 demo5

examples:	alldemo arcs blend calc char clock editdraw ellipses fastline fields imagedit imagine imgtest listtest moire nativfnt pizza polygons rainbow scribble scrolls smiley spectrum tabpane tester textclr utf8edit viewutf8 wintypes xortest xortest2 

demo1:	demo\imagine.c
	$(CL) $(CLOPTS) demo\imagine.c $(LIB) $(LIBS) -eimagine.exe $(LNKOPS)
	$(CP) imagine.exe DEMO\imagine.exe
	$(RM) imagine.tds imagine.obj imagine.exe

demo2:	demo\tester.c
	$(CL) $(CLOPTS) demo\tester.c $(LIB) $(LIBS) -tester.exe $(LNKOPS)
	$(CP) tester.exe DEMO\tester.exe
	$(RM) tester.tds tester.obj tester.exe

demo3:	demo\viewutf8.c
	$(CL) $(CLOPTS) demo\viewutf8.c $(LIB) $(LIBS) -viewutf8.exe $(LNKOPS)
	$(CP) viewutf8.exe DEMO\viewutf8.exe
	$(RM) viewutf8.tds viewutf8.obj viewutf8.exe

demo4:	demo\imgtest.c
	$(CL) $(CLOPTS) demo\imgtest.c $(LIB) $(LIBS) -eimgtest.exe $(LNKOPS)
	$(CP) imgtest.exe DEMO\imgtest.exe
	$(RM) imgtest.tds imgtest.obj imgtest.exe

demo5:	demo\blend.c
	$(CL) $(CLOPTS) demo\blend.c $(LIB) $(LIBS) -eblend.exe $(LNKOPS)
	$(CP) blend.exe DEMO\blend.exe
	$(RM) blend.tds imgtest.obj imgtest.exe

alldemo:	..\examples\alldemo.c
	$(CL) $(CLOPTS) ..\examples\alldemo.c $(LIB) $(LIBS) -ealldemo.exe $(LNKOPS)
	$(CP) alldemo.exe ..\examples\alldemo.exe
	$(RM) alldemo.tds alldemo.obj alldemo.exe

arcs:	..\examples\arcs.c
	$(CL) $(CLOPTS) ..\examples\arcs.c $(LIB) $(LIBS) -earcs.exe $(LNKOPS)
	$(CP) arcs.exe ..\examples\arcs.exe
	$(RM) arcs.tds arcs.obj arcs.exe

blend:	..\examples\blend.c
	$(CL) $(CLOPTS) ..\examples\blend.c $(LIB) $(LIBS) -eblend.exe $(LNKOPS)
	$(CP) blend.exe ..\examples\blend.exe
	$(RM) blend.tds blend.obj blend.exe

calc:	..\examples\calc.c
	$(CL) $(CLOPTS) ..\examples\calc.c $(LIB) $(LIBS) -ecalc.exe $(LNKOPS)
	$(CP) calc.exe ..\examples\calc.exe
	$(RM) calc.tds calc.obj calc.exe

char:	..\examples\char.c
	$(CL) $(CLOPTS) ..\examples\char.c $(LIB) $(LIBS) -echar.exe $(LNKOPS)
	$(CP) char.exe ..\examples\char.exe
	$(RM) char.tds char.obj char.exe

clock:	..\examples\clock.c
	$(CL) $(CLOPTS) ..\examples\clock.c $(LIB) $(LIBS) -eclock.exe $(LNKOPS)
	$(CP) clock.exe ..\examples\clock.exe
	$(RM) clock.tds clock.obj clock.exe

editdraw:	..\examples\editdraw.c
	$(CL) $(CLOPTS) ..\examples\editdraw.c $(LIB) $(LIBS) -eeditdraw.exe $(LNKOPS)
	$(CP) editdraw.exe ..\examples\editdraw.exe
	$(RM) editdraw.tds editdraw.obj editdraw.exe

ellipses:	..\examples\ellipses.c
	$(CL) $(CLOPTS) ..\examples\ellipses.c $(LIB) $(LIBS) -eellipses.exe $(LNKOPS)
	$(CP) ellipses.exe ..\examples\ellipses.exe
	$(RM) ellipses.tds ellipses.obj ellipses.exe

fastline:	..\examples\fastline.c
	$(CL) $(CLOPTS) ..\examples\fastline.c $(LIB) $(LIBS) -efastline.exe $(LNKOPS)
	$(CP) fastline.exe ..\examples\fastline.exe
	$(RM) fastline.tds fastline.obj fastline.exe

fields:	..\examples\fields.c
	$(CL) $(CLOPTS) ..\examples\fields.c $(LIB) $(LIBS) -efields.exe $(LNKOPS)
	$(CP) fields.exe ..\examples\fields.exe
	$(RM) fields.tds fields.obj fields.exe

fontedit:	..\examples\fontedit.c
	$(CL) $(CLOPTS) ..\examples\fontedit.c $(LIB) $(LIBS) -efontedit.exe $(LNKOPS)
	$(CP) fontedit.exe ..\examples\fontedit.exe
	$(RM) fontedit.tds fontedit.obj fontedit.exe

fontsave:	..\examples\fontsave.c
	$(CL) $(CLOPTS) ..\examples\fontsave.c $(LIB) $(LIBS) -efontsave.exe $(LNKOPS)
	$(CP) fontsave.exe ..\examples\fontsave.exe
	$(RM) fontsave.tds fontsave.obj fontsave.exe

imagedit:	..\examples\imagedit.c
	$(CL) $(CLOPTS) ..\examples\imagedit.c $(LIB) $(LIBS) -eimagedit.exe $(LNKOPS)
	$(CP) imagedit.exe ..\examples\imagedit.exe
	$(RM) imagedit.tds imagedit.obj imagedit.exe

imagine:	..\examples\imagine.c
	$(CL) $(CLOPTS) ..\examples\imagine.c $(LIB) $(LIBS) -eimagine.exe $(LNKOPS)
	$(CP) imagine.exe ..\examples\imagine.exe
	$(RM) imagine.tds imagine.obj imagine.exe

imgtest:	..\examples\imgtest.c
	$(CL) $(CLOPTS) ..\examples\imgtest.c $(LIB) $(LIBS) -eimgtest.exe $(LNKOPS)
	$(CP) imgtest.exe ..\examples\imgtest.exe
	$(RM) imgtest.tds imgtest.obj imgtest.exe

listtest:	..\examples\listtest.c
	$(CL) $(CLOPTS) ..\examples\listtest.c $(LIB) $(LIBS) -elisttest.exe $(LNKOPS)
	$(CP) listtest.exe ..\examples\listtest.exe
	$(RM) listtest.tds listtest.obj listtest.exe

moire:	..\examples\moire.c
	$(CL) $(CLOPTS) ..\examples\moire.c $(LIB) $(LIBS) -emoire.exe $(LNKOPS)
	$(CP) moire.exe ..\examples\moire.exe
	$(RM) moire.tds moire.obj moire.exe

nativfnt:	..\examples\nativfnt.c
	$(CL) $(CLOPTS) ..\examples\nativfnt.c $(LIB) $(LIBS) -enativfnt.exe $(LNKOPS)
	$(CP) nativfnt.exe ..\examples\nativfnt.exe
	$(RM) nativfnt.tds nativfnt.obj nativfnt.exe

pizza:	..\examples\pizza.c
	$(CL) $(CLOPTS) ..\examples\pizza.c $(LIB) $(LIBS) -epizza.exe $(LNKOPS)
	$(CP) pizza.exe ..\examples\pizza.exe
	$(RM) pizza.tds pizza.obj pizza.exe

polygons:	..\examples\polygons.c
	$(CL) $(CLOPTS) ..\examples\polygons.c $(LIB) $(LIBS) -epolygons.exe $(LNKOPS)
	$(CP) polygons.exe ..\examples\polygons.exe
	$(RM) polygons.tds polygons.obj polygons.exe

rainbow:	..\examples\rainbow.c
	$(CL) $(CLOPTS) ..\examples\rainbow.c $(LIB) $(LIBS) -erainbow.exe $(LNKOPS)
	$(CP) rainbow.exe ..\examples\rainbow.exe
	$(RM) rainbow.tds rainbow.obj rainbow.exe

scribble:	..\examples\scribble.c
	$(CL) $(CLOPTS) ..\examples\scribble.c $(LIB) $(LIBS) -escribble.exe $(LNKOPS)
	$(CP) scribble.exe ..\examples\scribble.exe
	$(RM) scribble.tds scribble.obj scribble.exe

scrolls:	..\examples\scrolls.c
	$(CL) $(CLOPTS) ..\examples\scrolls.c $(LIB) $(LIBS) -escrolls.exe $(LNKOPS)
	$(CP) scrolls.exe ..\examples\scrolls.exe
	$(RM) scrolls.tds scrolls.obj scrolls.exe

smiley:	..\examples\smiley.c
	$(CL) $(CLOPTS) ..\examples\smiley.c $(LIB) $(LIBS) -esmiley.exe $(LNKOPS)
	$(CP) smiley.exe ..\examples\smiley.exe
	$(RM) smiley.tds smiley.obj smiley.exe

spectrum:	..\examples\spectrum.c
	$(CL) $(CLOPTS) ..\examples\spectrum.c $(LIB) $(LIBS) -espectrum.exe $(LNKOPS)
	$(CP) spectrum.exe ..\examples\spectrum.exe
	$(RM) spectrum.tds spectrum.obj spectrum.exe

tabpane:	..\examples\tabpane.c
	$(CL) $(CLOPTS) ..\examples\tabpane.c $(LIB) $(LIBS) -etabpane.exe $(LNKOPS)
	$(CP) tabpane.exe ..\examples\tabpane.exe
	$(RM) tabpane.tds tabpane.obj tabpane.exe

tester:	..\examples\tester.c
	$(CL) $(CLOPTS) ..\examples\tester.c $(LIB) $(LIBS) -etester.exe $(LNKOPS)
	$(CP) tester.exe ..\examples\tester.exe
	$(RM) tester.tds tester.obj tester.exe

textclr:	..\examples\textclr.c
	$(CL) $(CLOPTS) ..\examples\textclr.c $(LIB) $(LIBS) -etextclr.exe $(LNKOPS)
	$(CP) textclr.exe ..\examples\textclr.exe
	$(RM) textclr.tds textclr.obj textclr.exe

utf8edit:	..\examples\utf8edit.c
	$(CL) $(CLOPTS) ..\examples\utf8edit.c $(LIB) $(LIBS) -eutf8edit.exe $(LNKOPS)
	$(CP) utf8edit.exe ..\examples\utf8edit.exe
	$(RM) utf8edit.tds utf8edit.obj utf8edit.exe

viewutf8:	..\examples\viewutf8.c
	$(CL) $(CLOPTS) ..\examples\viewutf8.c $(LIB) $(LIBS) -eviewutf8.exe $(LNKOPS)
	$(CP) viewutf8.exe ..\examples\viewutf8.exe
	$(RM) viewutf8.tds viewutf8.obj viewutf8.exe

wintypes:	..\examples\wintypes.c
	$(CL) $(CLOPTS) ..\examples\wintypes.c $(LIB) $(LIBS) -ewintypes.exe $(LNKOPS)
	$(CP) wintypes.exe ..\examples\wintypes.exe
	$(RM) wintypes.tds wintypes.obj wintypes.exe

xortest:	..\examples\xortest.c
	$(CL) $(CLOPTS) ..\examples\xortest.c $(LIB) $(LIBS) -exortest.exe $(LNKOPS)
	$(CP) xortest.exe ..\examples\xortest.exe
	$(RM) xortest.tds xortest.obj xortest.exe

xortest2:	..\examples\xortest2.c
	$(CL) $(CLOPTS) ..\examples\xortest2.c $(LIB) $(LIBS) -exortest2.exe $(LNKOPS)
	$(CP) xortest2.exe ..\examples\xortest2.exe
	$(RM) xortest2.tds xortest2.obj xortest2.exe

monty:	..\examples\monty\monty.c
	$(CL) $(CLOPTS) ..\examples\monty\monty.c $(LIB) $(LIBS) -emonty.exe $(LNKOPS)
	$(CP) monty.exe ..\examples\monty\monty.exe
	$(RM) monty.tds monty.obj monty.exe

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
