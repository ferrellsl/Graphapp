LibJPEG Modifications


LibJPEG has been included in this package, in a
modified form. The original LibJPEG was produced
by the Independent JPEG Group, and can be obtained
from many mirror sites worldwide, including:

   ftp.uu.net

This code is based on Version 6b of LibJPEG, but
some modifications have been made, as listed here:

1. The local variables called 'main' in jcmainct.c
   and jdmainct.c have been renamed to 'jmain' to
   prevent some compilers from complaining.

2. Some of the extra programs and config files included
   with the original distribution have been removed,
   to slim the amount of code in this directory.

3. All documentation for LibJPEG has been omitted.
   Get the original version of the library if you need
   documentation.

4. The code is now made using the Makefile in the parent
   directory of this one, rather than using a special
   Makefile just for this library. So it is no longer a
   library on its own; rather this JPEG support code is
   linked directly into App.

5. The original LibJPEG includes a variety of memory
   managers. These have been removed in favour of one
   single memory manager file, jmemnobs.c, which is
   an ANSI-C compatible memory system which assumes the
   computer has enough RAM to process an entire image.
   The other memory managers used temporary files, limited
   memory blocks to 64K, etc. I feel these are not needed
   these days, since modern operating systems can use
   virtual memory instead of requiring user programs to
   create temporary files themselves.

6. The jmorecfg.h file has been simplified to use the
   apptypes.h header file for definitions of 8-bit, 16-bit
   and 32-bit signed and unsigned integer quantities.
   Since apptypes.h is generated by the same compiler as
   will be used to compile the JPEG code, this is the
   correct thing to do, and will work on all platforms.
   It avoids messing about with strange options in header
   files for every different platform.

7. The jconfig.h file has been simplified to remove a few
   options which were specific to the included demo JPEG
   programs. These programs have been removed from this
   distribution, so there is no point having definitions
   for them within the jconfig.h file. Also, the settings
   within this header file should work on all platforms.
   We assume, for instance, that right shifting is unsigned,
   to force OR-ing of 1's into the top of right shifted
   negative numbers. This will work on all platforms. On some
   platforms this will involve a little redundant work, but
   smart compilers should be able to remove the OR-ing.
   Really, the JPEG code shouldn't be using shifting if it
   actually meant to use division; the code writers assumed
   that right shifting is faster than division, then had to
   write little macros to fix shifting so it works like division.
   What they should have done is used division, and assumed
   the compiler is smart enough to implement division the
   fastest way, rather than second-guessing the compiler.
   Ah well, I can't be bothered to fix their code, so I'm just
   forcing it to make no bad assumptions about whether right
   shifting is signed or unsigned.

8. #if 0 was converted to #ifdef DO_NOT_COMPILE

Loki
