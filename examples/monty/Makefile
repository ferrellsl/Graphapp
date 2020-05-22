# Monty Makefile

# You just have to compile monty.c really.
# You might have to change #include <app.h> into #include "app.h"
# if you can't get the compiler to automatically find app.h
# (and you'll have to copy app.h into the monty folder too).

# General settings:

CC	= gcc -g
APP	= /home/loki/apps/app/src
X11	= /usr/X11R6/lib

# Dynamic settings:

LINK	= -L$(APP) -L$(X11) -Xlinker -rpath -Xlinker $(APP) -lapp -lX11

# Static settings:

LINK	= -L$(APP) -L$(X11) $(APP)/libapp.a $(X11)/libX11.so.6 -lm

# Include files:

CFLAGS	= -I$(APP)

# Makefile settings:

SRC	= Makefile *.c *.h
MAIN	= monty.c
MONTY	= ./monty
DEST	= $(HOME)/bin
DOWNLOAD = ./download/

# Compression programs:

VERSION	= 3
MKDIR	= mkdir -p
COPY	= cp
TARDIR	= Monty$(VERSION)
TARFILE	= $(TARDIR).tar
GZFILE	= $(TARFILE).gz
TAR	= tar -cf
GZIP	= gzip -f
ZIP	= zip -kr
ZIPFILE	= $(TARDIR).zip

# Makefile rules:

test :	$(MONTY)
	$(MONTY)  # run the program as a test

$(MONTY) : $(OBJECTS) $(SRC)
	$(CC) $(CFLAGS) $(MAIN) -o $(MONTY) $(LINK)

.c.o :	
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(LIBS) $< -o $@

install : $(MONTY)
	cp $(MONTY) $(DEST)

tarfile :
	$(MKDIR) $(TARDIR)
	cp $(SRC) $(TARDIR)
	# Tar, gzip the program:
	$(TAR) $(TARFILE) $(TARDIR)
	$(GZIP) $(TARFILE)
	$(COPY) $(GZFILE) $(DOWNLOAD)
	# Copy the program in ZIP format:
	$(ZIP) $(ZIPFILE) $(TARDIR)
	$(COPY) $(ZIPFILE) $(DOWNLOAD)

clean :
	rm *.o core $(MONTY)

