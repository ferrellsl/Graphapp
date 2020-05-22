# Definitions for Tru64
OS       = x11
RM       = rm -f
RANLIB   = ranlib
OBJ      = o
EXE      = .x
SEP      = /
INC      = -I
EXTRAINC = 
GALIB    = libapp.a
COPTS    = -O -fast
OSLIBS   = -lX11 -lc -lm
LINK     = ar rc 
CL       = cc -o 
CC       = cc -c 
EXTRAS   =
include builds/common.mak
