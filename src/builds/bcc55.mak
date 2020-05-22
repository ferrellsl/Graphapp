# Definitions for BCC5
OS       = win32
RM       = deltree /y
RANLIB   = echo
OBJ      = obj
EXE      = .exe
SEP      = \\
INC      = -I
EXTRAINC = 
GALIB    = graphapp.lib
COPTS    = -q -5 -a4 -O2 -OS -Oi -ff -W
OSLIBS   =
LINK     = ilink -e 
CL       = bcc32 -e 
CC       = bcc32 -c 
EXTRAS   =
include builds\common.mak
