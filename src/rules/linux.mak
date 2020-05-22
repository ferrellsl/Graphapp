
# Makefile for App (Linux/gcc X11 version):

CC               = /usr/bin/gcc
AR               = /usr/bin/ar
RANLIB           = /usr/bin/ranlib
LD               = /usr/bin/ld
MAKE_STATIC_LIB  = /usr/bin/ar rc
MAKE_DYNAMIC_LIB = /usr/bin/ld -shared -fPIC -Bdynamic -o
X11_LIB          = /usr/X11R6/lib/libX11.so
X11_LIB_DIR      = /usr/X11R6/lib
X11_INC_DIR      = /usr/X11R6/include


# Makefile rules (Linux X11 version):

CFLAGS        = -O2 -Wall -I. -Ix11 -Iutility -Igui -Ilibz -Ilibpng -Ilibjpeg -Ilibgif -I$(X11_INC_DIR)
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

tidy:
	$(RM) */*.o core

clean:	tidy
	$(RM) $(LIB) $(DLL)

