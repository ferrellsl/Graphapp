This version of the ZLib library has been
slightly modified to work with GraphApp.

To see the original version, visit www.zlib.net.

I've removed some files from this version which
aren't needed to compile it, especially some
examples, build scripts and notes.

The file zconf.h has a new section added
to the top saying:

/*
 * Added by Loki:
 */

#include "apptypes.h"
#define Z_PREFIX
typedef app_uint16 z_ushort;
typedef app_uint8  z_byte;


The zutil.h file has been changed from:

typedef unsigned char  uch;
typedef uch FAR uchf;
typedef unsigned short ush;
typedef ush FAR ushf;
typedef unsigned long  ulg;

to say instead:

typedef app_uint8  uch;
typedef uch FAR uchf;
typedef app_uint16 ush;
typedef ush FAR ushf;
typedef app_uint32 ulg;


The reasons for these changes are that zlib
now uses the apptypes.h file for its definitions
of integer types to use. Also, it exports symbols
prefixed by z_ to avoid conflicts with other
software.

Also the types z_ushort and z_byte are used
in the older modified version of libpng which
was shipped with GraphApp in the past.
So for some backwards compatability, the z_ushort
and z_byte types are introduced. Those two types
are not actually used in the current version of
libpng which is included with GraphApp.

I've also changed #if 0 into #ifdef DO_NOT_COMPILE
in zconf.h and zutil.h because some compilers have
trouble handling #if 0 constructs.

See the libz README for more details of the
original authors of libz.

These are the files removed from this version:

-rw-r--r--        42928 Jul 18 06:26 ChangeLog
-rw-r--r--        15080 Nov 27  2004 FAQ
-rw-r--r--         1318 Oct  3  2004 INDEX
-rw-r--r--         4133 Aug  1 18:49 Makefile
-rw-r--r--         4133 Jul 17 19:25 Makefile.in
-rw-r--r--         9336 Jun  1  2003 algorithm.txt
drwxr-xr-x         4096 Aug  1 23:11 amiga
drwxr-xr-x         4096 Aug  1 23:11 as400
-rwxr-xr-x        13167 Jul 11 13:11 configure
drwxr-xr-x         4096 Aug  1 23:12 contrib
-rw-r--r--        16398 Oct 23  2004 example.c
drwxr-xr-x         4096 Aug  1 23:12 examples
-rw-r--r--        31103 Jul 11 13:31 gzio.c
-rw-------        13234 Jul 12 15:16 make_vms.com
-rw-r--r--         8018 May 27 23:40 minigzip.c
drwxr-xr-x         4096 Aug  1 23:19 msdos
drwxr-xr-x         4096 Aug  1 23:11 old
drwxr-xr-x         4096 Aug  1 23:19 projects
drwxr-xr-x         4096 Aug  1 23:19 qnx
drwxr-xr-x         4096 Aug  1 23:18 win32
-rw-r--r--         9560 Aug  1 22:58 zconf.in.h
-rw-r--r--         4486 Jul 17 19:34 zlib.3


Loki
