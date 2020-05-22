# Definitions for GNU tools with X11
OS       = x11
RM       = rm -f
RANLIB   = ranlib
OBJ      = o
EXE      =  
SEP      = /
INC      = -I
EXTRAINC = -I/usr/X11R6/include
GALIB    = libapp.a
COPTS    = -O2 -Wall
OSLIBS   = -L/usr/X11R6/lib -lX11 -lc -lm
LINK     = ar rc  
CL       = gcc -o 
CC       = gcc -c 
EXTRAS   =
include builds/common.mak
