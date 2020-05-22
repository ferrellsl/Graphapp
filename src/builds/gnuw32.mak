# Definitions for GNU tools with W32
OS       = win32
RM       = rm -f
RANLIB   = ranlib
OBJ      = o
EXE      = .exe
SEP      = \\
INC      = -I
EXTRAINC =
GALIB    = libappw32.a
COPTS    = -O2 -Wall -DW32GDI
OSLIBS   = -mwindows
LINK     = ar rc  
CL       = gcc -o 
CC       = gcc -c 
EXTRAS   =
include builds/common.mak
