
# Makefile for compiling the examples.
# Type "make all" to see a demonstration of all the examples.
# Type "make alldemo", "make fields" etc for other examples.
# Type "make static" for a static linking test.
# Type "make dynamic" to see if dynamic linking is working.

CC       = gcc -g -fno-pic -no-pie  -Wall

APP_PATH = /home/steve/Desktop/graphapp/src
APP_LIB  = app

X11      = 
#X11LIB   = $(X11)/lib
X11LIB   = /usr/lib/x86_64-linux-gnu/

LIBS     = -lX11 -lc -lm
#LIBS     = $(X11LIB)/libX11.dll.a -lc -lm
LIBS     = $(X11LIB)/libX11.so -lc -lm

DYNALINK = -Xlinker -rpath -Xlinker $(APP_PATH)

CFLAGS   = -I$(APP_PATH)
DYNAMIC  = -L$(APP_PATH) -l$(APP_LIB) -L$(X11LIB) $(LIBS) $(DYNALINK)
STATIC   = $(APP_PATH)/lib$(APP_LIB).a -L$(X11LIB) $(LIBS)

TARGETS  = progress_bar_demo alldemo arcs blend calc char clock diffimg editdraw ellipses fakemenu fastline fields fontdraw imagedit imagine imgtest listtest moire nativfnt photoviz pizza pizza2 polygons rainbow scribble scrolls smiley spectrum tabpane tester textclr to_bw to_half trycurs utf8edit viewutf8 wintypes xortest xortest2


static:	progress_bar_demo.o
	# Creating the static version.
	# Try "make dynamic" for a smaller executable size.
	$(CC) $(CFLAGS) progress_bar_demo.o $(STATIC) -o progress_bar_demo

dynamic: progress_bar_demo.o
	# Creating the dynamic version.
	# Try "make static" if this has problems.
	$(CC) $(CFLAGS) progress_bar_demo.o $(DYNAMIC) -o progress_bar_demo

static:	smiley.o
	# Creating the static version.
	# Try "make dynamic" for a smaller executable size.
	$(CC) $(CFLAGS) smiley.o $(STATIC) -o smiley

dynamic: smiley.o
	# Creating the dynamic version.
	# Try "make static" if this has problems.
	$(CC) $(CFLAGS) smiley.o $(DYNAMIC) -o smiley

all:	$(TARGETS)
	./RunAll.bat


progress_bar_demo: progress_bar_demo.o
	$(CC) $(CFLAGS) progress_bar_demo.o $(DYNAMIC) -o progress_bar_demo

alldemo: alldemo.o icons.h
	$(CC) $(CFLAGS) alldemo.o $(DYNAMIC) -o alldemo

arcs: arcs.o
	$(CC) $(CFLAGS) arcs.o $(DYNAMIC) -o arcs

blend: blend.o
	$(CC) $(CFLAGS) blend.o $(DYNAMIC) -o blend

calc: calc.o
	$(CC) $(CFLAGS) calc.o $(DYNAMIC) -o calc

char: char.o
	$(CC) $(CFLAGS) char.o $(DYNAMIC) -o char

clock: clock.o
	$(CC) $(CFLAGS) clock.o $(DYNAMIC) -o clock

diffimg: diffimg.o
	$(CC) $(CFLAGS) diffimg.o $(DYNAMIC) -o diffimg

editdraw: editdraw.o
	$(CC) $(CFLAGS) editdraw.o $(DYNAMIC) -o editdraw

ellipses: ellipses.o
	$(CC) $(CFLAGS) ellipses.o $(DYNAMIC) -o ellipses

fakemenu: fakemenu.o
	$(CC) $(CFLAGS) fakemenu.o $(DYNAMIC) -o fakemenu

fastline: fastline.o
	$(CC) $(CFLAGS) fastline.o $(DYNAMIC) -o fastline

fields: fields.o
	$(CC) $(CFLAGS) fields.o $(DYNAMIC) -o fields

fontdraw: fontdraw.o
	$(CC) $(CFLAGS) fontdraw.o $(DYNAMIC) -o fontdraw

imagedit: imagedit.o
	$(CC) $(CFLAGS) imagedit.o $(DYNAMIC) -o imagedit

imagine: imagine.o
	$(CC) $(CFLAGS) imagine.o $(DYNAMIC) -o imagine

imgtest: imgtest.o
	$(CC) $(CFLAGS) imgtest.o $(DYNAMIC) -o imgtest

img2gif: img2gif.o
	$(CC) $(CFLAGS) img2gif.o $(DYNAMIC) -o img2gif

listtest: listtest.o
	$(CC) $(CFLAGS) listtest.o $(DYNAMIC) -o listtest

moire: moire.o
	$(CC) $(CFLAGS) moire.o $(DYNAMIC) -o moire

nativfnt: nativfnt.o icons.h
	$(CC) $(CFLAGS) nativfnt.o $(DYNAMIC) -o nativfnt

photoviz: photoviz.o
	$(CC) $(CFLAGS) photoviz.o $(DYNAMIC) -o photoviz

photovi2: photovi2.o
	$(CC) $(CFLAGS) photovi2.o $(DYNAMIC) -o photovi2

pizza: pizza.o
	$(CC) $(CFLAGS) pizza.o $(DYNAMIC) -o pizza

pizza2: pizza2.o
	$(CC) $(CFLAGS) pizza2.o $(DYNAMIC) -o pizza2

polygons: polygons.o
	$(CC) $(CFLAGS) polygons.o $(DYNAMIC) -o polygons

rainbow: rainbow.o
	$(CC) $(CFLAGS) rainbow.o $(DYNAMIC) -o rainbow

scribble: scribble.o
	$(CC) $(CFLAGS) scribble.o $(DYNAMIC) -o scribble

scrolls: scrolls.o
	$(CC) $(CFLAGS) scrolls.o $(DYNAMIC) -o scrolls

smiley: smiley.o
	$(CC) $(CFLAGS) smiley.o $(DYNAMIC) -o smiley

spectrum: spectrum.o
	$(CC) $(CFLAGS) spectrum.o $(DYNAMIC) -o spectrum

tabpane: tabpane.o
	$(CC) $(CFLAGS) tabpane.o $(DYNAMIC) -o tabpane

tester: tester.o
	$(CC) $(CFLAGS) tester.o $(DYNAMIC) -o tester

textclr: textclr.o
	$(CC) $(CFLAGS) textclr.o $(DYNAMIC) -o textclr

to_bw: to_bw.o
	$(CC) $(CFLAGS) to_bw.o $(DYNAMIC) -o to_bw

to_half: to_half.o
	$(CC) $(CFLAGS) to_half.o $(DYNAMIC) -o to_half

trycurs: trycurs.o
	$(CC) $(CFLAGS) trycurs.o $(DYNAMIC) -o trycurs

utf8edit: utf8edit.o
	$(CC) $(CFLAGS) utf8edit.o $(DYNAMIC) -o utf8edit

viewutf8: viewutf8.o
	$(CC) $(CFLAGS) viewutf8.o $(DYNAMIC) -o viewutf8

wintypes: wintypes.o
	$(CC) $(CFLAGS) wintypes.o $(DYNAMIC) -o wintypes

xortest: xortest.o
	$(CC) $(CFLAGS) xortest.o $(DYNAMIC) -o xortest

xortest2: xortest2.o
	$(CC) $(CFLAGS) xortest2.o $(DYNAMIC) -o xortest2

clean:
	rm -f core *.o $(TARGETS)

