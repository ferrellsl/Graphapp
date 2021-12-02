
TARGET   = imagine
SRC      = imagine.c

CC       = gcc -g -no-pie

APP_PATH = ..
APP_LIB  = app

X11      = /usr/X11R6
X11INC   = $(X11)/include
X11LIB   = $(X11)/lib

LIBS     = -lX11 -lc -lm
DYNALINK = -Xlinker -rpath -Xlinker $(APP_PATH)

INCLUDE  = -I$(APP_PATH)
DYNAMIC  = -L$(APP_PATH) -l$(APP_LIB) -L$(X11LIB) $(LIBS) $(DYNALINK)
STATIC   = $(APP_PATH)/lib$(APP_LIB).a -L$(X11LIB) $(LIBS)

all:	imagine tester viewutf8 imgtest blend

static:	$(SRC)
	# Creating the static version, try "make dynamic" if this has problems
	$(CC) $(INCLUDE) $(SRC) $(STATIC) -o $(TARGET)

dynamic: $(SRC)
	# Creating the dynamic version, try "make static" if this has problems
	$(CC) $(INCLUDE) $(SRC) $(DYNAMIC) -o $(TARGET)

imagine: imagine.c $(APP_PATH)/lib$(APP_LIB).a
	# Creating the imagine program, static version.
	$(CC) $(INCLUDE) imagine.c $(STATIC) -o imagine

tester: tester.c $(APP_PATH)/lib$(APP_LIB).a
	# Creating the tester program, static version.
	$(CC) $(INCLUDE) tester.c $(STATIC) -o tester

viewutf8: viewutf8.c $(APP_PATH)/lib$(APP_LIB).a
	# Creating the viewutf8 program, static version.
	$(CC) $(INCLUDE) viewutf8.c $(STATIC) -o viewutf8

imgtest: imgtest.c $(APP_PATH)/lib$(APP_LIB).a
	# Creating the imgtest program, static version.
	$(CC) $(INCLUDE) imgtest.c $(STATIC) -o imgtest

blend: blend.c $(APP_PATH)/lib$(APP_LIB).a
	# Creating the blend program, static version.
	$(CC) $(INCLUDE) blend.c $(STATIC) -o blend

clean:
	rm -f *.o core imagine tester viewutf8 imgtest blend

