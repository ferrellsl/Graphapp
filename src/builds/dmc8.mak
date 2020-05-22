# Definitions for DMC8
OS       = win32
RM       = deltree /y
RANLIB   = echo
OBJ      = obj
EXE      = .exe
SEP      = \\
INC      = -I
EXTRAINC =
GALIB    = graphapp.lib
COPTS    = -mn -o -WA -f -5 -a4 -Dmain=app_main
OSLIBS   = kernel32.lib gdi32.lib user32.lib
LINK     = link -o 
CL       = sc -L/EXET:NT -L/SU:WINDOWS -L/STUB:WINSTUB.EXE -o 
CC       = sc -c 
EXTRAS   =
include builds\common.mak
