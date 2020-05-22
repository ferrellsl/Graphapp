
# Makefile for App (MingW version):

CC               = gcc
AR               = ar
RANLIB           = ranlib
LD               = ld
MAKE_STATIC_LIB  = ar rc

# Makefile rules (MingW version):

CFLAGS        = -O2 -Wall -I. -Iwin32 -Iutility -Igui -Ilibgif -Ilibjpeg -Ilibpng -Ilibz
WLIBS         = -mwindows

# Add the flags below to CFLAGS if you are using the CygWin gcc compiler
# (with no cygwin1.dll dependence):

EXTRA_CFLAGS  = -I/usr/include -mno-cygwin

# Rules:

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
		win32/bmap.o       win32/bmapimg.o    win32/clipbrd.o  \
		win32/cursor.o     win32/drawbmap.o   win32/drawwin.o  \
		win32/event.o      win32/folder.o     win32/font.o     \
		win32/graphics.o   win32/init.o       win32/timer.o    \
		win32/win.o

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

HEADERS       = apptypes.h app.h win32/appint.h utility/apputils.h gui/appgui.h gui/cursors.h

LIB           = libapp.a

static:	apptypes.h $(LIB)

$(LIB):	$(OBJECTS) $(HEADERS)
	$(exist $(LIB), del $(LIB))
	$(MAKE_STATIC_LIB) $(LIB) win32/*.o utility/*.o gui/*.o imgfmt/*.o libgif/*.o libjpeg/*.o libpng/*.o libz/*.o
	$(RANLIB) $(LIB)

apptypes.h: apptypes.o
	$(CC) $(CFLAGS) -o apptypes.exe apptypes.o
	./apptypes
	$(exist apptypes.exe, del apptypes.exe)

ga2:	ga2.o $(LIB) $(DLL)
	# Create GraphApp version 2 compatability library
	$(RM) libga2.a
	$(MAKE_STATIC_LIB) libga2.a x11/*.o utility/*.o gui/*.o imgfmt/*.o libgif/*.o libjpeg/*.o libpng/*.o libz/*.o ga2.o
	$(RANLIB) libga2.a

demos: demo1 demo2 demo3 demo4 demo5

demo1:	static demo/imagine.o
	$(CC) $(CFLAGS) -o demo/imagine.exe demo/imagine.o $(LIB) $(WLIBS)

demo2:	static demo/tester.o
	$(CC) $(CFLAGS) -o demo/tester.exe demo/tester.o $(LIB) $(WLIBS)

demo3:	static demo/viewutf8.o
	$(CC) $(CFLAGS) -o demo/viewutf8.exe demo/viewutf8.o $(LIB) $(WLIBS)

demo4:	static demo/imgtest.o
	$(CC) $(CFLAGS) -o demo/imgtest.exe demo/imgtest.o $(LIB) $(WLIBS)

demo5:	static demo/blend.o
	$(CC) $(CFLAGS) -o demo/blend.exe demo/blend.o $(LIB) $(WLIBS)

tidy:
	$(exist *.o, del *.o)
	$(exist demo\*.o, del demo\*.o)
	$(exist utility\*.o, del utility\*.o)
	$(exist gui\*.o, del gui\*.o)
	$(exist imgfmt\*.o, del imgfmt\*.o)
	$(exist libgif\*.o, del libgif\*.o)
	$(exist libjpeg\*.o, del libjpeg\*.o)
	$(exist libpng\*.o, del libpng\*.o)
	$(exist libz\*.o, del libz\*.o)
	$(exist win32\*.o, del win32\*.o)

clean:	tidy
	$(exist $(LIB), del $(LIB))

.c.o:
	$(CC) -c $(CFLAGS) $*.c -o $*.o

