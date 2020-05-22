
CC       = gcc -g

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

TARGETS  = addres getres seeres

all:	$(TARGETS)

addres: addres.c $(APP_PATH)/lib$(APP_LIB).a
	$(CC) $(INCLUDE) addres.c $(STATIC) -o addres

getres: getres.c $(APP_PATH)/lib$(APP_LIB).a
	$(CC) $(INCLUDE) getres.c $(STATIC) -o getres

seeres: seeres.c $(APP_PATH)/lib$(APP_LIB).a
	$(CC) $(INCLUDE) seeres.c $(STATIC) -o seeres

clean:
	rm -f *.o core $(TARGETS)

