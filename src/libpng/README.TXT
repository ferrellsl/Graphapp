This is a modified version of LibPNG for use
with the GraphApp library.

This version of LibPNG is based on version 1.2.18
of LibPNG, available from www.libpng.org.

The only modifications are that I've changed the
#if 0 construct into #ifdef DO_NOT_COMPILE in
pngconf.h (since some compilers have trouble with
#if 0 for some reason), and I've also made some
small modifications to zLib, upon which LibPNG
relies. See the README.TXT file in libz/ for
more details.

Since the real version of LibPNG is available
from the net, you may upgrade to the latest
version by downloading it from www.libpng.org
and extracting it into this folder and compiling.

I've taken the liberty of removing some unused
files and documentation and subfolders. See the
real version for these removed files or folders:

-rw-r--r--         1476 May 15  2007 ANNOUNCE
-rw-r--r--        79441 May 15  2007 CHANGES
-rw-r--r--         7366 May 15  2007 INSTALL
-rw-r--r--          402 May 15  2007 KNOWNBUG
-rw-r--r--         1182 Dec 23  2000 TODO
-rw-r--r--         2321 May 15  2007 Y2KINFO
-rwxr-xr-x          388 May 15  2007 configure
drwxr-xr-x         4096 May 15  2007 contrib/
-rw-r--r--        29793 May 15  2007 example.c
-rw-r--r--       164387 May 15  2007 libpng.3
-rw-r--r--       127770 May 15  2007 libpng-1.2.18.txt
-rw-r--r--        24981 May 15  2007 libpngpf.3
-rw-r--r--         2381 May 15  2007 png.5
-rw-r--r--         2498 Jul 12  2000 pngbar.jpg
-rw-r--r--         2399 Jul 12  2000 pngbar.png
-rw-r--r--       235080 May 15  2007 pnggccrd.c
-rw-r--r--         2069 Aug 25  2004 pngnow.png
-rw-r--r--        46652 May 15  2007 pngtest.c
-rw-r--r--         8574 Aug 25  2004 pngtest.png
-rw-r--r--       143414 May 15  2007 pngvcrd.c
drwxr-xr-x         4096 May 15  2007 projects/
drwxr-xr-x         4096 May 15  2007 scripts/

Loki
