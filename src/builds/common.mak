# Common rule file to compile GraphApp project.

# Path definitions

EXPL=src$(SEP)examples$(SEP)
LOC=.
SRC=src$(SEP)
DEMO=$(SRC)demo$(SEP)
UTIL=$(SRC)utility$(SEP)
GUI=$(SRC)gui$(SEP)
IMGFMT=$(SRC)imgfmt$(SEP)
LIBGIF=$(SRC)libgif$(SEP)
LIBJPEG=$(SRC)libjpeg$(SEP)
LIBPNG=$(SRC)libpng$(SEP)
LIBZ=$(SRC)libz$(SEP)
TOOLS=$(SRC)tools$(SEP)
OSDIR=$(SRC)$(OS)$(SEP)

# Option groups

INCLUDES = $(INC)$(LOC)          $(INC)$(SRC)     $(INC)$(GUI)     \
	   $(INC)$(IMGFMT)       $(INC)$(LIBGIF)  $(INC)$(LIBJPEG) \
	   $(INC)$(LIBPNG)       $(INC)$(LIBZ)    $(INC)$(OSDIR)   \
	   $(INC)$(UTIL) $(EXTRAINC)

CFLAGS = $(COPTS) $(INCLUDES)
ALLIBS = $(GALIB) $(OSLIBS)

# Object groups

UTIL_OBJECTS  = apputil.$(OBJ)   array.$(OBJ)     border.$(OBJ)   \
		clipline.$(OBJ)  compose.$(OBJ)  \
		control.$(OBJ)   deleting.$(OBJ)  dispatch.$(OBJ) \
		drawimg.$(OBJ)   drawing.$(OBJ)   drawtext.$(OBJ) \
		fontutil.$(OBJ)  image.$(OBJ)     imglist.$(OBJ)  \
		malloc.$(OBJ)    palette.$(OBJ)   point.$(OBJ)    \
		rect.$(OBJ)      region.$(OBJ)    resource.$(OBJ) \
		rgb.$(OBJ)       str.$(OBJ)       strtable.$(OBJ) \
		utf8.$(OBJ)      utf8regx.$(OBJ)  winutil.$(OBJ)

GUI_OBJECTS   = button.$(OBJ)    checkbox.$(OBJ)  cursors.$(OBJ)  \
		dialog.$(OBJ)    dropfld.$(OBJ)   droplist.$(OBJ) \
		field.$(OBJ)     imagebtn.$(OBJ)  imgcheck.$(OBJ) \
		imglabel.$(OBJ)  label.$(OBJ)     listbox.$(OBJ)  \
		manager.$(OBJ)   \
		menu.$(OBJ)      notebtn.$(OBJ)   passfld.$(OBJ)  \
		radiobtn.$(OBJ)  scroll.$(OBJ)    separat.$(OBJ)  \
		splitter.$(OBJ)  tabbtn.$(OBJ)    textbox.$(OBJ)  \
		textundo.$(OBJ)  tip.$(OBJ)

OS_OBJECTS    = bmap.$(OBJ)      bmapimg.$(OBJ)   clipbrd.$(OBJ)  \
		cursor.$(OBJ)    drawbmap.$(OBJ)  drawwin.$(OBJ)  \
		event.$(OBJ)     folder.$(OBJ)    font.$(OBJ)     \
		graphics.$(OBJ)  init.$(OBJ)      timer.$(OBJ)    \
		win.$(OBJ)       keys2ucs.$(OBJ)  clut.$(OBJ)

IMGFMT_OBJECTS= imgread.$(OBJ)   imgwrite.$(OBJ)  readgif.$(OBJ)  \
		readh.$(OBJ)     readjpg.$(OBJ)   readpng.$(OBJ)  \
		writegif.$(OBJ)  writeh.$(OBJ)    writejpg.$(OBJ) \
		writepng.$(OBJ)

GIF_OBJECTS   = gif.$(OBJ)

JPEG_OBJECTS  = jcapimin.$(OBJ)  jcapistd.$(OBJ)  jccoefct.$(OBJ) \
		jccolor.$(OBJ)   jcdctmgr.$(OBJ)  jchuff.$(OBJ)   \
		jcinit.$(OBJ)    jcmainct.$(OBJ)  jcmarker.$(OBJ) \
		jcmaster.$(OBJ)  jcomapi.$(OBJ)   jcparam.$(OBJ)  \
		jcphuff.$(OBJ)   jcprepct.$(OBJ)  jcsample.$(OBJ) \
		jctrans.$(OBJ)   jdapimin.$(OBJ)  jdapistd.$(OBJ) \
		jdatadst.$(OBJ)  jdatasrc.$(OBJ)  jdcoefct.$(OBJ) \
		jdcolor.$(OBJ)   jddctmgr.$(OBJ)  jdhuff.$(OBJ)   \
		jdinput.$(OBJ)   jdmainct.$(OBJ)  jdmarker.$(OBJ) \
		jdmaster.$(OBJ)  jdmerge.$(OBJ)   jdphuff.$(OBJ)  \
		jdpostct.$(OBJ)  jdsample.$(OBJ)  jdtrans.$(OBJ)  \
		jerror.$(OBJ)    jfdctflt.$(OBJ)  jfdctfst.$(OBJ) \
		jfdctint.$(OBJ)  jidctflt.$(OBJ)  jidctfst.$(OBJ) \
		jidctint.$(OBJ)  jidctred.$(OBJ)  jmemmgr.$(OBJ)  \
		jmemnobs.$(OBJ)  jquant1.$(OBJ)   jquant2.$(OBJ)  \
		jutils.$(OBJ)

PNG_OBJECTS   = png.$(OBJ)        pngerror.$(OBJ)   pngget.$(OBJ)   \
		pngmem.$(OBJ)     pngpread.$(OBJ)   pngread.$(OBJ)  \
		pngrio.$(OBJ)     pngrtran.$(OBJ)   pngrutil.$(OBJ) \
		pngset.$(OBJ)     pngtrans.$(OBJ)   pngwio.$(OBJ)   \
		pngwrite.$(OBJ)   pngwtran.$(OBJ)   pngwutil.$(OBJ)

LIBZ_OBJECTS  = adler32.$(OBJ)    compress.$(OBJ)   crc32.$(OBJ)    \
		deflate.$(OBJ)    infback.$(OBJ)    inffast.$(OBJ)  \
		inflate.$(OBJ)    inftrees.$(OBJ)   trees.$(OBJ)    \
		uncompr.$(OBJ)    zutil.$(OBJ)

ALL_OBJECTS   = $(UTIL_OBJECTS)   $(GUI_OBJECTS)    \
		$(OS_OBJECTS)     $(IMGFMT_OBJECTS) \
		$(GIF_OBJECTS)    $(JPEG_OBJECTS)   \
		$(PNG_OBJECTS)    $(LIBZ_OBJECTS)

DEMO_PROGRAMS = demo1$(EXE)       demo2$(EXE)       demo3$(EXE)  \
		demo4$(EXE)       demo5$(EXE)

EXPL_PROGRAMS = alldemo$(EXE)     arcs$(EXE)        blend$(EXE)    \
		calc$(EXE)        char$(EXE)        clock$(EXE)    \
		diffimg$(EXE)     editdraw$(EXE)    ellipses$(EXE) \
		fastline$(EXE)    fields$(EXE)      imagedit$(EXE) \
		imagine$(EXE)     imgtest$(EXE)     listtest$(EXE) \
		moire$(EXE)       nativfnt$(EXE)    photoviz$(EXE) \
		pizza$(EXE)       polygons$(EXE)    rainbow$(EXE)  \
		scribble$(EXE)    scrolls$(EXE)     smiley$(EXE)   \
		spectrum$(EXE)    tabpane$(EXE)     tester$(EXE)   \
		textclr$(EXE)     trycurs$(EXE)     utf8edit$(EXE) \
		viewutf8$(EXE)    wintypes$(EXE)    xortest$(EXE)  \
		xortest2$(EXE)   progress_bar_demo$(EXE)

TOOL_PROGRAMS = addres$(EXE) getres$(EXE) seeres$(EXE)

# Building rules

all: apptypes.h $(GALIB) \
	$(DEMO_PROGRAMS) $(EXPL_PROGRAMS) $(TOOL_PROGRAMS)

apptypes.h: $(SRC)apptypes.c
	$(CL)apptypes$(EXE) $(COPTS) $(SRC)apptypes.c
	.$(SEP)apptypes$(EXE)
	$(RM) apptypes.$(OBJ)
	$(RM) apptypes$(EXE)

$(GALIB): $(ALL_OBJECTS)
	$(RM) $(GALIB)
	$(LINK)$(GALIB) $(ALL_OBJECTS)
	$(RANLIB) $(GALIB)

# Program files

progress_bar_demo$(EXE): $(GALIB) $(EXPL)progress_bar_demo.c
	$(CL)progress_bar_demo$(EXE) $(CFLAGS) $(EXPL)progress_bar_demo.c $(ALLIBS)

demo1$(EXE): $(GALIB) $(DEMO)imagine.c
	$(CL)demo1$(EXE) $(CFLAGS) $(DEMO)imagine.c $(ALLIBS)

demo2$(EXE): $(GALIB) $(DEMO)tester.c
	$(CL)demo2$(EXE) $(CFLAGS) $(DEMO)tester.c $(ALLIBS)

demo3$(EXE): $(GALIB) $(DEMO)viewutf8.c
	$(CL)demo3$(EXE) $(CFLAGS) $(DEMO)viewutf8.c $(ALLIBS)

demo4$(EXE): $(GALIB) $(DEMO)imgtest.c
	$(CL)demo4$(EXE) $(CFLAGS) $(DEMO)imgtest.c $(ALLIBS)

demo5$(EXE): $(GALIB) $(DEMO)blend.$(OBJ)
	$(CL)demo5$(EXE) $(CFLAGS) $(DEMO)blend.c $(ALLIBS)

alldemo$(EXE): $(GALIB) $(EXPL)alldemo.c
	$(CL)alldemo$(EXE) $(CFLAGS) $(EXPL)alldemo.c $(ALLIBS)

arcs$(EXE): $(GALIB) $(EXPL)arcs.c
	$(CL)arcs$(EXE) $(CFLAGS) $(EXPL)arcs.c $(ALLIBS)

blend$(EXE): $(GALIB) $(EXPL)blend.c
	$(CL)blend$(EXE) $(CFLAGS) $(EXPL)blend.c $(ALLIBS)

calc$(EXE): $(GALIB) $(EXPL)calc.c
	$(CL)calc$(EXE) $(CFLAGS) $(EXPL)calc.c $(ALLIBS)

char$(EXE): $(GALIB) $(EXPL)char.c
	$(CL)char$(EXE) $(CFLAGS) $(EXPL)char.c $(ALLIBS)

clock$(EXE): $(GALIB) $(EXPL)clock.c
	$(CL)clock$(EXE) $(CFLAGS) $(EXPL)clock.c $(ALLIBS)

diffimg$(EXE): $(GALIB) $(EXPL)diffimg.c
	$(CL)diffimg$(EXE) $(CFLAGS) $(EXPL)diffimg.c $(ALLIBS)

editdraw$(EXE): $(GALIB) $(EXPL)editdraw.c
	$(CL)editdraw$(EXE) $(CFLAGS) $(EXPL)editdraw.c $(ALLIBS)

ellipses$(EXE): $(GALIB) $(EXPL)ellipses.c
	$(CL)ellipses$(EXE) $(CFLAGS) $(EXPL)ellipses.c $(ALLIBS)

fastline$(EXE): $(GALIB) $(EXPL)fastline.c
	$(CL)fastline$(EXE) $(CFLAGS) $(EXPL)fastline.c $(ALLIBS)

fields$(EXE): $(GALIB) $(EXPL)fields.c
	$(CL)fields$(EXE) $(CFLAGS) $(EXPL)fields.c $(ALLIBS)

fontedit$(EXE): $(GALIB) $(EXPL)fontedit.c
	$(CL)fields$(EXE) $(CFLAGS) $(EXPL)fontedit.c $(ALLIBS)

fontsave$(EXE): $(GALIB) $(EXPL)fontsave.c
	$(CL)fields$(EXE) $(CFLAGS) $(EXPL)fontsave.c $(ALLIBS)

imagedit$(EXE): $(GALIB) $(EXPL)imagedit.c
	$(CL)imagedit$(EXE) $(CFLAGS) $(EXPL)imagedit.c $(ALLIBS)

imagine$(EXE): $(GALIB) $(EXPL)imagine.c
	$(CL)imagine$(EXE) $(CFLAGS) $(EXPL)imagine.c $(ALLIBS)

imgtest$(EXE): $(GALIB) $(EXPL)imgtest.c
	$(CL)imgtest$(EXE) $(CFLAGS) $(EXPL)imgtest.c $(ALLIBS)

listtest$(EXE): $(GALIB) $(EXPL)listtest.c
	$(CL)listtest$(EXE) $(CFLAGS) $(EXPL)listtest.c $(ALLIBS)

moire$(EXE): $(GALIB) $(EXPL)moire.c
	$(CL)moire$(EXE) $(CFLAGS) $(EXPL)moire.c $(ALLIBS)

nativfnt$(EXE): $(GALIB) $(EXPL)nativfnt.c
	$(CL)nativfnt$(EXE) $(CFLAGS) $(EXPL)nativfnt.c $(ALLIBS)

photoviz$(EXE): $(GALIB) $(EXPL)photoviz.c
	$(CL)photoviz$(EXE) $(CFLAGS) $(EXPL)photoviz.c $(ALLIBS)

pizza$(EXE): $(GALIB) $(EXPL)pizza.c
	$(CL)pizza$(EXE) $(CFLAGS) $(EXPL)pizza.c $(ALLIBS)

polygons$(EXE): $(GALIB) $(EXPL)polygons.c
	$(CL)polygons$(EXE) $(CFLAGS) $(EXPL)polygons.c $(ALLIBS)

rainbow$(EXE): $(GALIB) $(EXPL)rainbow.c
	$(CL)rainbow$(EXE) $(CFLAGS) $(EXPL)rainbow.c $(ALLIBS)

scribble$(EXE): $(GALIB) $(EXPL)scribble.c
	$(CL)scribble$(EXE) $(CFLAGS) $(EXPL)scribble.c $(ALLIBS)

scrolls$(EXE): $(GALIB) $(EXPL)scrolls.c
	$(CL)scrolls$(EXE) $(CFLAGS) $(EXPL)scrolls.c $(ALLIBS)

smiley$(EXE): $(GALIB) $(EXPL)smiley.c
	$(CL)smiley$(EXE) $(CFLAGS) $(EXPL)smiley.c $(ALLIBS)

spectrum$(EXE): $(GALIB) $(EXPL)spectrum.c
	$(CL)spectrum$(EXE) $(CFLAGS) $(EXPL)spectrum.c $(ALLIBS)

tabpane$(EXE): $(GALIB) $(EXPL)tabpane.c
	$(CL)tabpane$(EXE) $(CFLAGS) $(EXPL)tabpane.c $(ALLIBS)

tester$(EXE): $(GALIB) $(EXPL)tester.c
	$(CL)tester$(EXE) $(CFLAGS) $(EXPL)tester.c $(ALLIBS)

textclr$(EXE): $(GALIB) $(EXPL)textclr.c
	$(CL)textclr$(EXE) $(CFLAGS) $(EXPL)textclr.c $(ALLIBS)

trycurs$(EXE): $(GALIB) $(EXPL)trycurs.c
	$(CL)trycurs$(EXE) $(CFLAGS) $(EXPL)trycurs.c $(ALLIBS)

utf8edit$(EXE): $(GALIB) $(EXPL)utf8edit.c
	$(CL)utf8edit$(EXE) $(CFLAGS) $(EXPL)utf8edit.c $(ALLIBS)

viewutf8$(EXE): $(GALIB) $(EXPL)viewutf8.c
	$(CL)viewutf8$(EXE) $(CFLAGS) $(EXPL)viewutf8.c $(ALLIBS)

wintypes$(EXE): $(GALIB) $(EXPL)wintypes.c
	$(CL)wintypes$(EXE) $(CFLAGS) $(EXPL)wintypes.c $(ALLIBS)

xortest$(EXE): $(GALIB) $(EXPL)xortest.c
	$(CL)xortest$(EXE) $(CFLAGS) $(EXPL)xortest.c $(ALLIBS)

xortest2$(EXE): $(GALIB) $(EXPL)xortest2.c
	$(CL)xortest2$(EXE) $(CFLAGS) $(EXPL)xortest2.c $(ALLIBS)

monty$(EXE): $(GALIB) $(EXPL)monty$(SEP)monty.c
	$(CL)monty$(EXE) $(CFLAGS) $(EXPL)monty$(SEP)monty.c $(ALLIBS)

addres$(EXE): $(GALIB) $(TOOLS)addres.c
	$(CL)addres$(EXE) $(CFLAGS) $(TOOLS)addres.c $(ALLIBS)

getres$(EXE): $(GALIB) $(TOOLS)getres.c
	$(CL)getres$(EXE) $(CFLAGS) $(TOOLS)getres.c $(ALLIBS)

seeres$(EXE): $(GALIB) $(TOOLS)seeres.c
	$(CL)seeres$(EXE) $(CFLAGS) $(TOOLS)seeres.c $(ALLIBS)


# Source files

apputil.$(OBJ): $(UTIL)apputil.c
	$(CC) $(CFLAGS) $(UTIL)apputil.c

array.$(OBJ): $(UTIL)array.c
	$(CC) $(CFLAGS) $(UTIL)array.c

border.$(OBJ): $(UTIL)border.c
	$(CC) $(CFLAGS) $(UTIL)border.c

clipline.$(OBJ): $(UTIL)clipline.c
	$(CC) $(CFLAGS) $(UTIL)clipline.c

compose.$(OBJ): $(UTIL)compose.c
	$(CC) $(CFLAGS) $(UTIL)compose.c

control.$(OBJ): $(UTIL)control.c
	$(CC) $(CFLAGS) $(UTIL)control.c

deleting.$(OBJ): $(UTIL)deleting.c
	$(CC) $(CFLAGS) $(UTIL)deleting.c

dispatch.$(OBJ): $(UTIL)dispatch.c
	$(CC) $(CFLAGS) $(UTIL)dispatch.c

drawimg.$(OBJ): $(UTIL)drawimg.c
	$(CC) $(CFLAGS) $(UTIL)drawimg.c

drawing.$(OBJ): $(UTIL)drawing.c
	$(CC) $(CFLAGS) $(UTIL)drawing.c

drawtext.$(OBJ): $(UTIL)drawtext.c
	$(CC) $(CFLAGS) $(UTIL)drawtext.c

fontutil.$(OBJ): $(UTIL)fontutil.c
	$(CC) $(CFLAGS) $(UTIL)fontutil.c

image.$(OBJ): $(UTIL)image.c
	$(CC) $(CFLAGS) $(UTIL)image.c

imglist.$(OBJ): $(UTIL)imglist.c
	$(CC) $(CFLAGS) $(UTIL)imglist.c

malloc.$(OBJ): $(UTIL)malloc.c
	$(CC) $(CFLAGS) $(UTIL)malloc.c

palette.$(OBJ): $(UTIL)palette.c
	$(CC) $(CFLAGS) $(UTIL)palette.c

point.$(OBJ): $(UTIL)point.c
	$(CC) $(CFLAGS) $(UTIL)point.c

rect.$(OBJ): $(UTIL)rect.c
	$(CC) $(CFLAGS) $(UTIL)rect.c

region.$(OBJ): $(UTIL)region.c
	$(CC) $(CFLAGS) $(UTIL)region.c

resource.$(OBJ): $(UTIL)resource.c
	$(CC) $(CFLAGS) $(UTIL)resource.c

rgb.$(OBJ): $(UTIL)rgb.c
	$(CC) $(CFLAGS) $(UTIL)rgb.c

str.$(OBJ): $(UTIL)str.c
	$(CC) $(CFLAGS) $(UTIL)str.c

strtable.$(OBJ): $(UTIL)strtable.c
	$(CC) $(CFLAGS) $(UTIL)strtable.c

utf8.$(OBJ): $(UTIL)utf8.c
	$(CC) $(CFLAGS) $(UTIL)utf8.c

utf8regx.$(OBJ): $(UTIL)utf8regx.c
	$(CC) $(CFLAGS) $(UTIL)utf8regx.c

winutil.$(OBJ): $(UTIL)winutil.c
	$(CC) $(CFLAGS) $(UTIL)winutil.c


button.$(OBJ): $(GUI)button.c
	$(CC) $(CFLAGS) $(GUI)button.c

checkbox.$(OBJ): $(GUI)checkbox.c
	$(CC) $(CFLAGS) $(GUI)checkbox.c

cursors.$(OBJ): $(GUI)cursors.c
	$(CC) $(CFLAGS) $(GUI)cursors.c

dialog.$(OBJ): $(GUI)dialog.c
	$(CC) $(CFLAGS) $(GUI)dialog.c

dropfld.$(OBJ): $(GUI)dropfld.c
	$(CC) $(CFLAGS) $(GUI)dropfld.c

droplist.$(OBJ): $(GUI)droplist.c
	$(CC) $(CFLAGS) $(GUI)droplist.c

field.$(OBJ): $(GUI)field.c
	$(CC) $(CFLAGS) $(GUI)field.c

imagebtn.$(OBJ): $(GUI)imagebtn.c
	$(CC) $(CFLAGS) $(GUI)imagebtn.c

imgcheck.$(OBJ): $(GUI)imgcheck.c
	$(CC) $(CFLAGS) $(GUI)imgcheck.c

imglabel.$(OBJ): $(GUI)imglabel.c
	$(CC) $(CFLAGS) $(GUI)imglabel.c

label.$(OBJ): $(GUI)label.c
	$(CC) $(CFLAGS) $(GUI)label.c

listbox.$(OBJ): $(GUI)listbox.c
	$(CC) $(CFLAGS) $(GUI)listbox.c

manager.$(OBJ): $(GUI)manager.c
	$(CC) $(CFLAGS) $(GUI)manager.c

menu.$(OBJ): $(GUI)menu.c
	$(CC) $(CFLAGS) $(GUI)menu.c

notebtn.$(OBJ): $(GUI)notebtn.c
	$(CC) $(CFLAGS) $(GUI)notebtn.c

passfld.$(OBJ): $(GUI)passfld.c
	$(CC) $(CFLAGS) $(GUI)passfld.c

radiobtn.$(OBJ): $(GUI)radiobtn.c
	$(CC) $(CFLAGS) $(GUI)radiobtn.c

scroll.$(OBJ): $(GUI)scroll.c
	$(CC) $(CFLAGS) $(GUI)scroll.c

separat.$(OBJ): $(GUI)separat.c
	$(CC) $(CFLAGS) $(GUI)separat.c

splitter.$(OBJ): $(GUI)splitter.c
	$(CC) $(CFLAGS) $(GUI)splitter.c

tabbtn.$(OBJ): $(GUI)tabbtn.c
	$(CC) $(CFLAGS) $(GUI)tabbtn.c

textbox.$(OBJ): $(GUI)textbox.c
	$(CC) $(CFLAGS) $(GUI)textbox.c

textundo.$(OBJ): $(GUI)textundo.c
	$(CC) $(CFLAGS) $(GUI)textundo.c

tip.$(OBJ): $(GUI)tip.c
	$(CC) $(CFLAGS) $(GUI)tip.c


imgread.$(OBJ): $(IMGFMT)imgread.c
	$(CC) $(CFLAGS) $(IMGFMT)imgread.c

imgwrite.$(OBJ): $(IMGFMT)imgwrite.c
	$(CC) $(CFLAGS) $(IMGFMT)imgwrite.c

readgif.$(OBJ): $(IMGFMT)readgif.c
	$(CC) $(CFLAGS) $(IMGFMT)readgif.c

readh.$(OBJ): $(IMGFMT)readh.c
	$(CC) $(CFLAGS) $(IMGFMT)readh.c

readjpg.$(OBJ): $(IMGFMT)readjpg.c
	$(CC) $(CFLAGS) $(IMGFMT)readjpg.c

readpng.$(OBJ): $(IMGFMT)readpng.c
	$(CC) $(CFLAGS) $(IMGFMT)readpng.c

writegif.$(OBJ): $(IMGFMT)writegif.c
	$(CC) $(CFLAGS) $(IMGFMT)writegif.c

writeh.$(OBJ): $(IMGFMT)writeh.c
	$(CC) $(CFLAGS) $(IMGFMT)writeh.c

writejpg.$(OBJ): $(IMGFMT)writejpg.c
	$(CC) $(CFLAGS) $(IMGFMT)writejpg.c

writepng.$(OBJ): $(IMGFMT)writepng.c
	$(CC) $(CFLAGS) $(IMGFMT)writepng.c


gif.$(OBJ): $(LIBGIF)gif.c
	$(CC) $(CFLAGS) $(LIBGIF)gif.c


jcapimin.$(OBJ): $(LIBJPEG)jcapimin.c
	$(CC) $(CFLAGS) $(LIBJPEG)jcapimin.c

jcapistd.$(OBJ): $(LIBJPEG)jcapistd.c
	$(CC) $(CFLAGS) $(LIBJPEG)jcapistd.c

jccoefct.$(OBJ): $(LIBJPEG)jccoefct.c
	$(CC) $(CFLAGS) $(LIBJPEG)jccoefct.c

jccolor.$(OBJ): $(LIBJPEG)jccolor.c
	$(CC) $(CFLAGS) $(LIBJPEG)jccolor.c

jcdctmgr.$(OBJ): $(LIBJPEG)jcdctmgr.c
	$(CC) $(CFLAGS) $(LIBJPEG)jcdctmgr.c

jchuff.$(OBJ): $(LIBJPEG)jchuff.c
	$(CC) $(CFLAGS) $(LIBJPEG)jchuff.c

jcinit.$(OBJ): $(LIBJPEG)jcinit.c
	$(CC) $(CFLAGS) $(LIBJPEG)jcinit.c

jcmainct.$(OBJ): $(LIBJPEG)jcmainct.c
	$(CC) $(CFLAGS) $(LIBJPEG)jcmainct.c

jcmarker.$(OBJ): $(LIBJPEG)jcmarker.c
	$(CC) $(CFLAGS) $(LIBJPEG)jcmarker.c

jcmaster.$(OBJ): $(LIBJPEG)jcmaster.c
	$(CC) $(CFLAGS) $(LIBJPEG)jcmaster.c

jcomapi.$(OBJ): $(LIBJPEG)jcomapi.c
	$(CC) $(CFLAGS) $(LIBJPEG)jcomapi.c

jcparam.$(OBJ): $(LIBJPEG)jcparam.c
	$(CC) $(CFLAGS) $(LIBJPEG)jcparam.c

jcphuff.$(OBJ): $(LIBJPEG)jcphuff.c
	$(CC) $(CFLAGS) $(LIBJPEG)jcphuff.c

jcprepct.$(OBJ): $(LIBJPEG)jcprepct.c
	$(CC) $(CFLAGS) $(LIBJPEG)jcprepct.c

jcsample.$(OBJ): $(LIBJPEG)jcsample.c
	$(CC) $(CFLAGS) $(LIBJPEG)jcsample.c

jctrans.$(OBJ): $(LIBJPEG)jctrans.c
	$(CC) $(CFLAGS) $(LIBJPEG)jctrans.c

jdapimin.$(OBJ): $(LIBJPEG)jdapimin.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdapimin.c

jdapistd.$(OBJ): $(LIBJPEG)jdapistd.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdapistd.c

jdatadst.$(OBJ): $(LIBJPEG)jdatadst.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdatadst.c

jdatasrc.$(OBJ): $(LIBJPEG)jdatasrc.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdatasrc.c

jdcoefct.$(OBJ): $(LIBJPEG)jdcoefct.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdcoefct.c

jdcolor.$(OBJ): $(LIBJPEG)jdcolor.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdcolor.c

jddctmgr.$(OBJ): $(LIBJPEG)jddctmgr.c
	$(CC) $(CFLAGS) $(LIBJPEG)jddctmgr.c

jdhuff.$(OBJ): $(LIBJPEG)jdhuff.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdhuff.c

jdinput.$(OBJ): $(LIBJPEG)jdinput.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdinput.c

jdmainct.$(OBJ): $(LIBJPEG)jdmainct.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdmainct.c

jdmarker.$(OBJ): $(LIBJPEG)jdmarker.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdmarker.c

jdmaster.$(OBJ): $(LIBJPEG)jdmaster.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdmaster.c

jdmerge.$(OBJ): $(LIBJPEG)jdmerge.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdmerge.c

jdphuff.$(OBJ): $(LIBJPEG)jdphuff.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdphuff.c

jdpostct.$(OBJ): $(LIBJPEG)jdpostct.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdpostct.c

jdsample.$(OBJ): $(LIBJPEG)jdsample.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdsample.c

jdtrans.$(OBJ): $(LIBJPEG)jdtrans.c
	$(CC) $(CFLAGS) $(LIBJPEG)jdtrans.c

jerror.$(OBJ): $(LIBJPEG)jerror.c
	$(CC) $(CFLAGS) $(LIBJPEG)jerror.c

jfdctflt.$(OBJ): $(LIBJPEG)jfdctflt.c
	$(CC) $(CFLAGS) $(LIBJPEG)jfdctflt.c

jfdctfst.$(OBJ): $(LIBJPEG)jfdctfst.c
	$(CC) $(CFLAGS) $(LIBJPEG)jfdctfst.c

jfdctint.$(OBJ): $(LIBJPEG)jfdctint.c
	$(CC) $(CFLAGS) $(LIBJPEG)jfdctint.c

jidctflt.$(OBJ): $(LIBJPEG)jidctflt.c
	$(CC) $(CFLAGS) $(LIBJPEG)jidctflt.c

jidctfst.$(OBJ): $(LIBJPEG)jidctfst.c
	$(CC) $(CFLAGS) $(LIBJPEG)jidctfst.c

jidctint.$(OBJ): $(LIBJPEG)jidctint.c
	$(CC) $(CFLAGS) $(LIBJPEG)jidctint.c

jidctred.$(OBJ): $(LIBJPEG)jidctred.c
	$(CC) $(CFLAGS) $(LIBJPEG)jidctred.c

jmemmgr.$(OBJ): $(LIBJPEG)jmemmgr.c
	$(CC) $(CFLAGS) $(LIBJPEG)jmemmgr.c

jmemnobs.$(OBJ): $(LIBJPEG)jmemnobs.c
	$(CC) $(CFLAGS) $(LIBJPEG)jmemnobs.c

jquant1.$(OBJ): $(LIBJPEG)jquant1.c
	$(CC) $(CFLAGS) $(LIBJPEG)jquant1.c

jquant2.$(OBJ): $(LIBJPEG)jquant2.c
	$(CC) $(CFLAGS) $(LIBJPEG)jquant2.c

jutils.$(OBJ): $(LIBJPEG)jutils.c
	$(CC) $(CFLAGS) $(LIBJPEG)jutils.c


png.$(OBJ): $(LIBPNG)png.c
	$(CC) $(CFLAGS) $(LIBPNG)png.c

pngerror.$(OBJ): $(LIBPNG)pngerror.c
	$(CC) $(CFLAGS) $(LIBPNG)pngerror.c

pngget.$(OBJ): $(LIBPNG)pngget.c
	$(CC) $(CFLAGS) $(LIBPNG)pngget.c

pngmem.$(OBJ): $(LIBPNG)pngmem.c
	$(CC) $(CFLAGS) $(LIBPNG)pngmem.c

pngpread.$(OBJ): $(LIBPNG)pngpread.c
	$(CC) $(CFLAGS) $(LIBPNG)pngpread.c

pngread.$(OBJ): $(LIBPNG)pngread.c
	$(CC) $(CFLAGS) $(LIBPNG)pngread.c

pngrio.$(OBJ): $(LIBPNG)pngrio.c
	$(CC) $(CFLAGS) $(LIBPNG)pngrio.c

pngrtran.$(OBJ): $(LIBPNG)pngrtran.c
	$(CC) $(CFLAGS) $(LIBPNG)pngrtran.c

pngrutil.$(OBJ): $(LIBPNG)pngrutil.c
	$(CC) $(CFLAGS) $(LIBPNG)pngrutil.c

pngset.$(OBJ): $(LIBPNG)pngset.c
	$(CC) $(CFLAGS) $(LIBPNG)pngset.c

pngtrans.$(OBJ): $(LIBPNG)pngtrans.c
	$(CC) $(CFLAGS) $(LIBPNG)pngtrans.c

pngwio.$(OBJ): $(LIBPNG)pngwio.c
	$(CC) $(CFLAGS) $(LIBPNG)pngwio.c

pngwrite.$(OBJ): $(LIBPNG)pngwrite.c
	$(CC) $(CFLAGS) $(LIBPNG)pngwrite.c

pngwtran.$(OBJ): $(LIBPNG)pngwtran.c
	$(CC) $(CFLAGS) $(LIBPNG)pngwtran.c

pngwutil.$(OBJ): $(LIBPNG)pngwutil.c
	$(CC) $(CFLAGS) $(LIBPNG)pngwutil.c


adler32.$(OBJ): $(LIBZ)adler32.c
	$(CC) $(CFLAGS) $(LIBZ)adler32.c

compress.$(OBJ): $(LIBZ)compress.c
	$(CC) $(CFLAGS) $(LIBZ)compress.c

crc32.$(OBJ): $(LIBZ)crc32.c
	$(CC) $(CFLAGS) $(LIBZ)crc32.c

deflate.$(OBJ): $(LIBZ)deflate.c
	$(CC) $(CFLAGS) $(LIBZ)deflate.c

infback.$(OBJ): $(LIBZ)infback.c
	$(CC) $(CFLAGS) $(LIBZ)infback.c

inffast.$(OBJ): $(LIBZ)inffast.c
	$(CC) $(CFLAGS) $(LIBZ)inffast.c

inflate.$(OBJ): $(LIBZ)inflate.c
	$(CC) $(CFLAGS) $(LIBZ)inflate.c

inftrees.$(OBJ): $(LIBZ)inftrees.c
	$(CC) $(CFLAGS) $(LIBZ)inftrees.c

trees.$(OBJ): $(LIBZ)trees.c
	$(CC) $(CFLAGS) $(LIBZ)trees.c

uncompr.$(OBJ): $(LIBZ)uncompr.c
	$(CC) $(CFLAGS) $(LIBZ)uncompr.c

zutil.$(OBJ): $(LIBZ)zutil.c
	$(CC) $(CFLAGS) $(LIBZ)zutil.c


bmap.$(OBJ): $(OSDIR)bmap.c
	$(CC) $(CFLAGS) $(OSDIR)bmap.c

bmapimg.$(OBJ): $(OSDIR)bmapimg.c
	$(CC) $(CFLAGS) $(OSDIR)bmapimg.c

clipbrd.$(OBJ): $(OSDIR)clipbrd.c
	$(CC) $(CFLAGS) $(OSDIR)clipbrd.c

clut.$(OBJ): $(OSDIR)clut.c
	$(CC) $(CFLAGS) $(OSDIR)clut.c

cursor.$(OBJ): $(OSDIR)cursor.c
	$(CC) $(CFLAGS) $(OSDIR)cursor.c

drawbmap.$(OBJ): $(OSDIR)drawbmap.c
	$(CC) $(CFLAGS) $(OSDIR)drawbmap.c

drawwin.$(OBJ): $(OSDIR)drawwin.c
	$(CC) $(CFLAGS) $(OSDIR)drawwin.c

event.$(OBJ): $(OSDIR)event.c
	$(CC) $(CFLAGS) $(OSDIR)event.c

folder.$(OBJ): $(OSDIR)folder.c
	$(CC) $(CFLAGS) $(OSDIR)folder.c

font.$(OBJ): $(OSDIR)font.c
	$(CC) $(CFLAGS) $(OSDIR)font.c

graphics.$(OBJ): $(OSDIR)graphics.c
	$(CC) $(CFLAGS) $(OSDIR)graphics.c

init.$(OBJ): $(OSDIR)init.c
	$(CC) $(CFLAGS) $(OSDIR)init.c

keys2ucs.$(OBJ): $(OSDIR)keys2ucs.c
	$(CC) $(CFLAGS) $(OSDIR)keys2ucs.c

timer.$(OBJ): $(OSDIR)timer.c
	$(CC) $(CFLAGS) $(OSDIR)timer.c

win.$(OBJ): $(OSDIR)win.c
	$(CC) $(CFLAGS) $(OSDIR)win.c


# Remove everything

tidy:
	$(RM) *.$(OBJ) $(EXTRAS)

clean: tidy
	$(RM) apptypes.h $(GALIB) \
	$(DEMO_PROGRAMS) $(EXPL_PROGRAMS) $(TOOL_PROGRAMS)

