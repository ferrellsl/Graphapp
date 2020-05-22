# Definitions for LCC
OS       = win32
RM       = deltree /y
RANLIB   = echo
OBJ      = obj
EXE      = .exe
SEP      = \\
INC      = -I
EXTRAINC =
GALIB    = graphapp.lib
COPTS    = -O -Zp4 -e5 -Dmain=app_main
OSLIBS   = -s -subsystem windows
LINK     = lcclnk 
CL       = lc -o  
CC       = lcc    
EXTRAS   =
include builds\common.mak
