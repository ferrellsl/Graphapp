# Definitions for MSVC
OS       = win32
RM       = -@erase 
RANLIB   = echo
OBJ      = obj
EXE      = .exe
SEP      = \\
INC      = /I
EXTRAINC =
GALIB    = graphapp.lib
COPTS    =  /w /EHs-c- /O2 /DWIN64 /DNDEBUG /D_MBCS /D_WINDOWS /D_LIB /FD
OSLIBS   = /link user32.lib gdi32.lib /subsystem:windows /incremental:no
LINK     = -@link /lib /nologo /out:
CL       = -@cl /nologo /Fe
CC       = -@cl /nologo /c 
EXTRAS   = vc140.idb vc140.pch
include src\builds\common.mak
