
# Makefile for App:

CC               = /bin/gcc
AR               = /bin/ar
RANLIB           = /bin/ranlib
LD               = /bin/ld
MAKE_STATIC_LIB  = /bin/ar rc
MAKE_DYNAMIC_LIB = /bin/ld -shared -o
X11_LIB          = /usr/lib/libX11.dll.a
X11_LIB_DIR      = /usr/lib
X11_INC_DIR      = /usr/include


# Makefile rules (Linux X11 version):

CFLAGS        = -DPNG_NO_MMX_CODE -fno-pic -no-pie -O2 -Wall -I. -Ix11 -Iutility -Igui -Ilibz -Ilibpng -Ilibjpeg -Ilibgif -I$(X11_INC_DIR)
RM            = rm -f

XLIBS         = -L$(X11_LIB_DIR) -lX11 -lc -lm

APP_OBJECTS   = utility/apputil.o  utility/array.o    utility/border.o   \
		utility/clipline.o utility/compose.o  \
		utility/control.o  utility/deleting.o utility/dispatch.o \
		utility/drawimg.o  utility/drawing.o  utility/drawtext.o \
		utility/image.o    utility/imglist.o  utility/fontutil.o \
		utility/malloc.o   utility/palette.o  utility/point.o    \
		utility/rect.o     utility/region.o   utility/resource.o \
		utility/rgb.o      utility/str.o      utility/strtable.o \
		utility/utf8.o     utility/utf8regx.o utility/winutil.o  \
		gui/button.o       gui/checkbox.o     gui/cursors.o   \
		gui/dialog.o       gui/dropfld.o      gui/droplist.o  \
		gui/field.o        gui/imagebtn.o     gui/imgcheck.o  \
		gui/imglabel.o     gui/label.o        gui/listbox.o   \
		gui/manager.o     \
		gui/menu.o         gui/notebtn.o      gui/passfld.o   \
		gui/radiobtn.o     gui/scroll.o       gui/separat.o   \
		gui/splitter.o     gui/tabbtn.o       gui/textbox.o   \
		gui/textundo.o     gui/tip.o          \
		imgfmt/imgread.o   imgfmt/imgwrite.o  \
		imgfmt/readgif.o   imgfmt/writegif.o  \
		imgfmt/readh.o     imgfmt/writeh.o    \
		imgfmt/readjpg.o   imgfmt/writejpg.o  \
		imgfmt/readpng.o   imgfmt/writepng.o  \
		x11/bmap.o         x11/bmapimg.o      x11/clipbrd.o   \
		x11/clut.o         x11/cursor.o       x11/drawbmap.o  \
		x11/drawwin.o      x11/event.o        x11/folder.o    \
		x11/font.o         x11/graphics.o     x11/init.o      \
		x11/keys2ucs.o     x11/timer.o        x11/win.o

GIF_OBJECTS   = libgif/gif.o

JPEG_OBJECTS  = libjpeg/jcapimin.o libjpeg/jcapistd.o libjpeg/jccoefct.o \
		libjpeg/jccolor.o  libjpeg/jcdctmgr.o libjpeg/jchuff.o   \
		libjpeg/jcinit.o   libjpeg/jcmainct.o libjpeg/jcmarker.o \
		libjpeg/jcmaster.o libjpeg/jcomapi.o  libjpeg/jcparam.o  \
		libjpeg/jcphuff.o  libjpeg/jcprepct.o libjpeg/jcsample.o \
		libjpeg/jctrans.o  libjpeg/jdapimin.o libjpeg/jdapistd.o \
		libjpeg/jdatadst.o libjpeg/jdatasrc.o libjpeg/jdcoefct.o \
		libjpeg/jdcolor.o  libjpeg/jddctmgr.o libjpeg/jdhuff.o   \
		libjpeg/jdinput.o  libjpeg/jdmainct.o libjpeg/jdmarker.o \
		libjpeg/jdmaster.o libjpeg/jdmerge.o  libjpeg/jdphuff.o  \
		libjpeg/jdpostct.o libjpeg/jdsample.o libjpeg/jdtrans.o  \
		libjpeg/jerror.o   libjpeg/jfdctflt.o libjpeg/jfdctfst.o \
		libjpeg/jfdctint.o libjpeg/jidctflt.o libjpeg/jidctfst.o \
		libjpeg/jidctint.o libjpeg/jidctred.o libjpeg/jmemmgr.o  \
		libjpeg/jmemnobs.o libjpeg/jquant1.o  libjpeg/jquant2.o  \
		libjpeg/jutils.o

PNG_OBJECTS   = libpng/png.o       libpng/pngerror.o  libpng/pngget.o   \
		libpng/pngmem.o    libpng/pngpread.o  libpng/pngread.o  \
		libpng/pngrio.o    libpng/pngrtran.o  libpng/pngrutil.o \
		libpng/pngset.o    libpng/pngtrans.o  libpng/pngwio.o   \
		libpng/pngwrite.o  libpng/pngwtran.o  libpng/pngwutil.o

LIBZ_OBJECTS  = libz/adler32.o     libz/compress.o    libz/crc32.o    \
		libz/deflate.o     libz/infback.o     libz/inffast.o  \
		libz/inflate.o     libz/inftrees.o    libz/trees.o    \
		libz/uncompr.o     libz/zutil.o

OBJECTS       = $(APP_OBJECTS) $(GIF_OBJECTS) $(JPEG_OBJECTS) \
		$(PNG_OBJECTS) $(LIBZ_OBJECTS)

HEADERS       = apptypes.h app.h x11/appint.h utility/apputils.h gui/appgui.h gui/cursors.h

LIB           = libapp.a
DLL           = libapp.so

static:	$(LIB)

dynamic: $(DLL)

$(LIB):	$(OBJECTS) $(HEADERS)
	$(RM) $(LIB)
	$(MAKE_STATIC_LIB) $(LIB) x11/*.o utility/*.o gui/*.o imgfmt/*.o libgif/*.o libjpeg/*.o libpng/*.o libz/*.o
	$(RANLIB) $(LIB)

$(DLL):	$(OBJECTS) $(HEADERS)
	$(RM) $(DLL)
	$(MAKE_DYNAMIC_LIB) $(DLL) x11/*.o utility/*.o gui/*.o imgfmt/*.o libgif/*.o libjpeg/*.o libpng/*.o libz/*.o $(XLIBS)

apptypes.h: apptypes.c
	# Regenerating apptypes.h file
	$(CC) $(CFLAGS) -o apptypes apptypes.c
	./apptypes
	$(RM) apptypes
	# apptypes.h created

ga2:	ga2.o $(LIB) $(DLL)
	# Create GraphApp version 2 compatability library
	$(RM) libga2.a
	$(MAKE_STATIC_LIB) libga2.a x11/*.o utility/*.o gui/*.o imgfmt/*.o libgif/*.o libjpeg/*.o libpng/*.o libz/*.o ga2.o
	$(RANLIB) libga2.a
	$(RM) libga2.so
	$(MAKE_DYNAMIC_LIB) libga2.so x11/*.o utility/*.o gui/*.o imgfmt/*.o libgif/*.o libjpeg/*.o libpng/*.o libz/*.o $(XLIBS) ga2.o

demo:	static demo/imagine.o
	$(CC) $(CFLAGS) -o demo/imagine demo/imagine.o $(LIB) $(XLIBS)

demo2:	static demo/tester.o
	$(CC) $(CFLAGS) -o demo/tester demo/tester.o $(LIB) $(XLIBS)

demo3:	static demo/viewutf8.o
	$(CC) $(CFLAGS) -o demo/viewutf8 demo/viewutf8.o $(LIB) $(XLIBS)

demo4:	static demo/imgtest.o
	$(CC) $(CFLAGS) -o demo/imgtest demo/imgtest.o $(LIB) $(XLIBS)

demo5:	static demo/blend.o
	$(CC) $(CFLAGS) -o demo/blend demo/blend.o $(LIB) $(XLIBS)

progress_bar_demo: static examples/progress_bar_demo.o

	$(CC) $(CFLAGS) -o examples/progress_bar_demo examples/progress_bar_demo.o $(LIB) $(XLIBS)

smiley: static examples/smiley.o

	$(CC) $(CFLAGS) -o examples/smiley examples/smiley.o $(LIB) $(XLIBS)

alldemo: static examples/alldemo.o

	$(CC) $(CFLAGS) -o examples/alldemo examples/alldemo.o $(LIB) $(XLIBS)

arcs: static examples/arcs.o

	$(CC) $(CFLAGS) -o examples/arcs examples/arcs.o $(LIB) $(XLIBS)
	
trycurs: static examples/trycurs.o

	$(CC) $(CFLAGS) -o examples/trycurs examples/trycurs.o $(LIB) $(XLIBS)

blend: static examples/blend.o

	$(CC) $(CFLAGS) -o examples/blend examples/blend.o $(LIB) $(XLIBS)

calc: static examples/calc.o

	$(CC) $(CFLAGS) -o examples/calc examples/calc.o $(LIB) $(XLIBS)

char: static examples/char.o

	$(CC) $(CFLAGS) -o examples/char examples/char.o $(LIB) $(XLIBS)

clock: static examples/clock.o

	$(CC) $(CFLAGS) -o examples/clock examples/clock.o $(LIB) $(XLIBS)

diffimg: static examples/diffimg.o

	$(CC) $(CFLAGS) -o examples/diffimg examples/diffimg.o $(LIB) $(XLIBS)

editdraw: static examples/editdraw.o

	$(CC) $(CFLAGS) -o examples/editdraw examples/editdraw.o $(LIB) $(XLIBS)

ellipses: static examples/ellipses.o

	$(CC) $(CFLAGS) -o examples/ellipses examples/ellipses.o $(LIB) $(XLIBS)

fakemenu: static examples/fakemenu.o

	$(CC) $(CFLAGS) -o examples/fakemenuexamples/fakemenu.o $(LIB) $(XLIBS)

fastline: static examples/fastline.o

	$(CC) $(CFLAGS) -o examples/fastline examples/fastline.o $(LIB) $(XLIBS)

fields: static examples/fields.o

	$(CC) $(CFLAGS) -o examples/fields examples/fields.o $(LIB) $(XLIBS)

fontdraw: static examples/fontdraw.o

	$(CC) $(CFLAGS) -o examples/fontdraw examples/fontdraw.o $(LIB) $(XLIBS)

imagedit: static examples/imagedit.o

	$(CC) $(CFLAGS) -o examples/imagedit examples/imagedit.o $(LIB) $(XLIBS)

imagine: static examples/imagine.o

	$(CC) $(CFLAGS) -o examples/imagine examples/imagine.o $(LIB) $(XLIBS)

img2gif: static examples/img2gif.o

	$(CC) $(CFLAGS) -o examples/img2gif examples/img2gif.o $(LIB) $(XLIBS)

imgtest: static examples/imgtest.o

	$(CC) $(CFLAGS) -o examples/imgtest examples/imgtest.o $(LIB) $(XLIBS)

listtest: static examples/listtest.o

	$(CC) $(CFLAGS) -o examples/listtest examples/listtest.o $(LIB) $(XLIBS)

moire: static examples/moire.o

	$(CC) $(CFLAGS) -o examples/moire examples/moire.o $(LIB) $(XLIBS)

nativfnt: static examples/nativfnt.o

	$(CC) $(CFLAGS) -o examples/nativfnt examples/nativfnt.o $(LIB) $(XLIBS)

photovi2: static examples/photovi2.o

	$(CC) $(CFLAGS) -o examples/photovi2 examples/photovi2.o $(LIB) $(XLIBS)

photoviz: static examples/photoviz.o

	$(CC) $(CFLAGS) -o examples/photoviz examples/photoviz.o $(LIB) $(XLIBS)

pizza: static examples/pizza.o

	$(CC) $(CFLAGS) -o examples/pizza examples/pizza.o $(LIB) $(XLIBS)

pizza2: static examples/pizza2.o

	$(CC) $(CFLAGS) -o examples/pizza2 examples/pizza2.o $(LIB) $(XLIBS)

polygons: static examples/polygons.o

	$(CC) $(CFLAGS) -o examples/polygons examples/polygons.o $(LIB) $(XLIBS)

rainbow: static examples/rainbow.o

	$(CC) $(CFLAGS) -o examples/rainbow examples/rainbow.o $(LIB) $(XLIBS)

scrolls: static examples/scrolls.o

	$(CC) $(CFLAGS) -o examples/scrolls examples/scrolls.o $(LIB) $(XLIBS)

scribble: static examples/scribble.o

	$(CC) $(CFLAGS) -o examples/scribble examples/scribble.o $(LIB) $(XLIBS)

spectrum: static examples/spectrum.o

	$(CC) $(CFLAGS) -o examples/spectrum examples/spectrum.o $(LIB) $(XLIBS)

utf8edit: static examples/utf8edit.o

	$(CC) $(CFLAGS) -o examples/utf8edit examples/utf8edit.o $(LIB) $(XLIBS)

viewutf8: static examples/viewutf8.o

	$(CC) $(CFLAGS) -o examples/viewutf8 examples/viewutf8.o $(LIB) $(XLIBS)

monty: static examples/monty/monty.o

	$(CC) $(CFLAGS) -o examples/monty/monty examples/monty/monty.o $(LIB) $(XLIBS)

tester: static examples/tester.o

	$(CC) $(CFLAGS) -o examples/tester examples/tester.o $(LIB) $(XLIBS)

xortest: static examples/xortest.o

	$(CC) $(CFLAGS) -o examples/xortest examples/xortest.o $(LIB) $(XLIBS)
	
xortest2: static examples/xortest2.o

	$(CC) $(CFLAGS) -o examples/xortest2 examples/xortest2.o $(LIB) $(XLIBS)
	
textclr: static examples/textclr.o

	$(CC) $(CFLAGS) -o examples/textclr examples/textclr.o $(LIB) $(XLIBS)
	
char: static examples/char.o

	$(CC) $(CFLAGS) -o examples/char examples/char.o $(LIB) $(XLIBS)	
	
showutf8: static examples/showutf8.o

	$(CC) $(CFLAGS) -o examples/showutf8 examples/showutf8.o $(LIB) $(XLIBS)
	
tabpane: static examples/tabpane.o

	$(CC) $(CFLAGS) -o examples/tabpane examples/tabpane.o $(LIB) $(XLIBS)
	
fakemenu: static examples/fakemenu.o

	$(CC) $(CFLAGS) -o examples/fakemenu examples/fakemenu.o $(LIB) $(XLIBS)	
	
wintypes: static examples/wintypes.o

	$(CC) $(CFLAGS) -o examples/wintypes examples/wintypes.o $(LIB) $(XLIBS)	

photovi2: static examples/photovi2.o

	$(CC) $(CFLAGS) -o examples/photovi2 examples/photovi2.o $(LIB) $(XLIBS)
	

tidy:
	$(RM) */*.o core

clean:	tidy
	$(RM) $(LIB) $(DLL)

