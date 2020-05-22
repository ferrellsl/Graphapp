# Definition for Watcom11
OS       = win32
RM       = deltree /y
RANLIB   = echo
OBJ      = obj
EXE      = .exe
SEP      = \\
INC      = /I
EXTRAINC =
GALIB    = graphapp.lib
COPTS    = /zq /5r /fpi87 /fp5 /oneatx /ei /zp4 /Dmain=app_main
OSLIBS   =
LINK     = lib -nologo -out:
CL       = wcl386 /"option quiet" /bt=nt /l=win95 /fe=
CC       = wcc386 
EXTRAS   =
include builds\common.mak
