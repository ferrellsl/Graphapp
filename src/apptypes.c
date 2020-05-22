/*
 *  AppTypes
 *
 *  Generates consistent type definitions, and
 *  checks the word alignment on this machine.
 *
 *  This program writes a file called "apptypes.h"
 *  which contains platform-specific information
 *  defining numeric types and determining
 *  how data should be aligned within structures.
 *
 *  This program attempts to find 8-bit, 16-bit
 *  and 32-bit integer types to use in definitions
 *  of app_uint8, app_int16 and app_int32.
 *  We don't just assume C's "int" is 32-bits in size;
 *  this would be non-portable. Instead, this program
 *  uses the C language sizeof() operator to determine
 *  the sizes of built-in C types such as "int". It
 *  then writes a header file which is used in
 *  compiling other programs. We can then safely
 *  assume that app_int32 is 32 bits in size, no more,
 *  no less.
 *
 *  This program also tries to make app_float32 a
 *  floating point type of 32-bits in size, and
 *  app_float64 a floating point type of 64-bits size.
 *  These may not succeed, and even if they do,
 *  there is no guarantee that the resulting type
 *  has that many bits of precision on the hardware.
 *  But at least we can try to get the structure
 *  alignment and packing correct.
 *
 *  The "apptypes.h" header file is used when
 *  compiling other files below this directory.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *filename = "apptypes.h";
char *progname = "apptypes";

struct AlignChar {
	char	dummy;
	char	data;
};

struct AlignShort {
	char	dummy;
	short	data;
};

struct AlignInt {
	char	dummy;
	int	data;
};

struct AlignLong {
	char	dummy;
	long	data;
};

struct AlignFloat {
	char	dummy;
	float	data;
};

struct AlignDouble {
	char	dummy;
	double	data;
};

struct AlignLongDouble {
	char		dummy;
	long double	data;
};

struct AlignPointer {
	char	dummy;
	void *	data;
};

int main(int argc, char *argv[])
{
	FILE *f;
	struct AlignChar align_char;
	struct AlignShort align_short;
	struct AlignInt align_int;
	struct AlignLong align_long;
	struct AlignFloat align_float;
	struct AlignDouble align_double;
	struct AlignLongDouble align_long_double;
	struct AlignPointer align_pointer;
	int size_app_float32 = 4;
	int size_app_float64 = 8;
	int size_app_pointer = 0;
	int offset_app_int8 = 0;
	int offset_app_int16 = 0;
	int offset_app_int32 = 0;
	int offset_app_float32 = 0;
	int offset_app_float64 = 0;
	int offset_app_pointer = 0;
	int offset_maximum = 0;

	f = fopen(filename, "w");
	if (f == NULL) {
		fprintf(stderr, "%s: warning: couldn't create file %s\n",
				progname, filename);
		fprintf(stderr, "%s: warning: using stdout instead\n",
				progname);
		f = stdout;
	}
	fprintf(f, "/*\n");
	fprintf(f, " *  %s\n", filename);
	fprintf(f, " *  Contains platform and compiler-specific definitions.\n");
	fprintf(f, " *  Do not edit this machine-generated file.\n");
	fprintf(f, " *  Re-compile and run %s.c instead.\n", progname);
	fprintf(f, " */\n\n");

	/*
	 *  Print standard types.
	 */
	fprintf(f, "/* Standard type names: */\n");

	if (sizeof(char) == 1) {
		fprintf(f, "typedef signed char     app_int8;    ");
		fprintf(f, "/* 8-bit signed integer. */\n");
		fprintf(f, "typedef unsigned char   app_uint8;   ");
		fprintf(f, "/* 8-bit unsigned integer. */\n");
		offset_app_int8 = sizeof(align_char) - sizeof(char);
	}
	else {
		/* This is a serious error. We need char to be 8 bits */
		/* otherwise most C programs will break. */
		/* So, we send an error into the file to alert the user */
		/* to find an 8-bit int. */
		fprintf(f, "Could not find an 8-bit integer type!\n");
		fprintf(f, "typedef signed ?!?!     app_int8;\n");
		fprintf(f, "typedef unsigned ?!?!   app_uint8;\n");
	}

	/*
	 *  Find a 16-bit integer type.
	 */
	if (sizeof(short) == 2) {
		fprintf(f, "typedef short           app_int16;   ");
		fprintf(f, "/* 16-bit signed integer. */\n");
		fprintf(f, "typedef unsigned short  app_uint16;  ");
		fprintf(f, "/* 16-bit unsigned integer. */\n");
		offset_app_int16 = sizeof(align_short) - sizeof(short);
	}
	else {
		/* We could use wchar_t, but that type is somewhat */
		/* obsolete and may not be supported by all compilers. */
		/* So, we send an error into the file to alert the user */
		/* to find a 16-bit int. */
		fprintf(f, "Could not find a 16-bit integer type!\n");
		fprintf(f, "typedef ?!?!?           app_int16;\n");
		fprintf(f, "typedef unsigned ?!?!?  app_uint16;\n");
	}

	/*
	 *  Find a 32-bit integer type.
	 */
	if (sizeof(int) == 4) {
		fprintf(f, "typedef int             app_int32;   ");
		fprintf(f, "/* 32-bit signed integer. */\n");
		fprintf(f, "typedef unsigned int    app_uint32;  ");
		fprintf(f, "/* 32-bit unsigned integer. */\n");
		offset_app_int32 = sizeof(align_int) - sizeof(int);
	}
	else if (sizeof(long) == 4) {
		fprintf(f, "typedef long            app_int32;   ");
		fprintf(f, "/* 32-bit signed integer. */\n");
		fprintf(f, "typedef unsigned long   app_uint32;  ");
		fprintf(f, "/* 32-bit unsigned integer. */\n");
		offset_app_int32 = sizeof(align_long) - sizeof(long);
	}
	else if (sizeof(short) == 4) {
		/* Rather unlikely to be true. */
		fprintf(f, "typedef short           app_int32;   ");
		fprintf(f, "/* 32-bit signed integer. */\n");
		fprintf(f, "typedef unsigned short  app_uint32;  ");
		fprintf(f, "/* 32-bit unsigned integer. */\n");
		offset_app_int32 = sizeof(align_short) - sizeof(short);
	}
	else {
		/* Send an error into the file to alert the user */
		/* to find a 32-bit int. */
		fprintf(f, "Could not find a 32-bit integer type!\n");
		fprintf(f, "typedef ?!?!            app_int32;\n");
		fprintf(f, "typedef unsigned ?!?!   app_uint32;\n");
	}

	/*
	 *  Find the 32-bit float and 64-bit real types.
	 */
	if (sizeof(float) == 4) {
		fprintf(f, "typedef float           app_float32; ");
		offset_app_float32 = sizeof(align_float) - sizeof(float);
	}
	else if (sizeof(double) == 4) {
		fprintf(f, "typedef double          app_float32; ");
		offset_app_float32 = sizeof(align_double) - sizeof(double);
	}
	else if (sizeof(long double) == 4) {
		/* Very strange indeed. */
		fprintf(f, "typedef long double     app_float32; ");
		offset_app_float32 = sizeof(align_long_double) - sizeof(long double);
	}
	else {
		/* Give up, and use float. */
		fprintf(f, "typedef float           app_float32; ");
		offset_app_float32 = sizeof(align_float) - sizeof(float);
		size_app_float32 = sizeof(float);
	}
	fprintf(f, "/* Small floating-point number. */\n");

	if (sizeof(float) == 8) {
		fprintf(f, "typedef float           app_float64; ");
		offset_app_float64 = sizeof(align_float) - sizeof(float);
	}
	else if (sizeof(double) == 8) {
		fprintf(f, "typedef double          app_float64; ");
		offset_app_float64 = sizeof(align_double) - sizeof(double);
	}
	else if (sizeof(long double) == 8) {
		fprintf(f, "typedef long double     app_float64; ");
		offset_app_float64 = sizeof(align_long_double) - sizeof(long double);
	}
	else {
		/* Give up, and use double. */
		fprintf(f, "typedef double          app_float64; ");
		offset_app_float64 = sizeof(align_double) - sizeof(double);
		size_app_float64 = sizeof(double);
	}
	fprintf(f, "/* Large floating-point number. */\n");

	/*
	 *  Print the pointer type.
	 */
	fprintf(f, "typedef void *          app_pointer; ");
	fprintf(f, "/* Largest pointer. */\n");
	fprintf(f, "\n");
	offset_app_pointer = sizeof(align_pointer) - sizeof(void *);
	size_app_pointer = sizeof(void *);

	/*
	 *  Define some other common type names.
	 */
	fprintf(f, "/* Common type names: */\n");

	fprintf(f, "typedef app_uint8       app_byte;    ");
	fprintf(f, "/* 8-bit unsigned integer. */\n");
	fprintf(f, "typedef app_int8        app_sbyte;   ");
	fprintf(f, "/* 8-bit signed integer. */\n");

	fprintf(f, "typedef app_int16       app_short;   ");
	fprintf(f, "/* 16-bit signed integer. */\n");
	fprintf(f, "typedef app_uint16      app_ushort;  ");
	fprintf(f, "/* 16-bit unsigned integer. */\n");

	fprintf(f, "typedef app_int32       app_int;     ");
	fprintf(f, "/* 32-bit signed integer. */\n");
	fprintf(f, "typedef app_uint32      app_uint;    ");
	fprintf(f, "/* 32-bit unsigned integer. */\n");

	fprintf(f, "typedef app_float32     app_float;   ");
	fprintf(f, "/* 32-bit floating point number. */\n");
	fprintf(f, "typedef app_float64     app_real;    ");
	fprintf(f, "/* 64-bit floating point number. */\n");

	fprintf(f, "typedef app_uint32      app_char;    ");
	fprintf(f, "/* 32-bit Unicode Char. */\n");

	fprintf(f, "\n");

	/*
	 *  Write printf formats to use.
	 */
	fprintf(f, "/* The printf length modifier format strings. */\n");
	if (sizeof(char) == 1)
		fprintf(f, "#define APP_BYTE_PRINTF_FMT   \n");
	else
		fprintf(f, "#define APP_BYTE_PRINTF_FMT   ???\n");

	if (sizeof(short) == 2)
		fprintf(f, "#define APP_SHORT_PRINTF_FMT  \"h\"\n");
	else
		fprintf(f, "#define APP_SHORT_PRINTF_FMT  ???\n");

	if (sizeof(int) == 4)
		fprintf(f, "#define APP_INT_PRINTF_FMT    \n");
	else if (sizeof(long) == 4)
		fprintf(f, "#define APP_INT_PRINTF_FMT    \"l\"\n");
	else if (sizeof(short) == 4)
		fprintf(f, "#define APP_INT_PRINTF_FMT    \n");
	else
		fprintf(f, "#define APP_INT_PRINTF_FMT    ???\n");

	if (sizeof(float) == 4)
		fprintf(f, "#define APP_FLOAT_PRINTF_FMT  \n");
	else if (sizeof(double) == 4)
		fprintf(f, "#define APP_FLOAT_PRINTF_FMT  \n");
	else if (sizeof(long double) == 4)
		fprintf(f, "#define APP_FLOAT_PRINTF_FMT  \"L\"\n");
	else
		fprintf(f, "#define APP_FLOAT_PRINTF_FMT  \n");

	if (sizeof(float) == 8)
		fprintf(f, "#define APP_REAL_PRINTF_FMT   \n");
	else if (sizeof(double) == 8)
		fprintf(f, "#define APP_REAL_PRINTF_FMT   \n");
	else if (sizeof(long double) == 8)
		fprintf(f, "#define APP_REAL_PRINTF_FMT   \"L\"\n");
	else
		fprintf(f, "#define APP_REAL_PRINTF_FMT   \n");

	fprintf(f, "\n");

	/*
	 *  Write scanf formats to use.
	 */
	fprintf(f, "/* The scanf length modifier format strings. */\n");
	if (sizeof(char) == 1)
		fprintf(f, "#define APP_BYTE_SCANF_FMT    \n");
	else
		fprintf(f, "#define APP_BYTE_SCANF_FMT    ???\n");

	if (sizeof(short) == 2)
		fprintf(f, "#define APP_SHORT_SCANF_FMT   \"h\"\n");
	else
		fprintf(f, "#define APP_SHORT_SCANF_FMT   ???\n");

	if (sizeof(int) == 4)
		fprintf(f, "#define APP_INT_SCANF_FMT     \n");
	else if (sizeof(long) == 4)
		fprintf(f, "#define APP_INT_SCANF_FMT     \"l\"\n");
	else if (sizeof(short) == 4)
		fprintf(f, "#define APP_INT_SCANF_FMT     \n");
	else
		fprintf(f, "#define APP_INT_SCANF_FMT     ???\n");

	if (sizeof(float) == 4)
		fprintf(f, "#define APP_FLOAT_SCANF_FMT   \n");
	else if (sizeof(double) == 4)
		fprintf(f, "#define APP_FLOAT_SCANF_FMT   \"l\"\n");
	else if (sizeof(long double) == 4)
		fprintf(f, "#define APP_FLOAT_SCANF_FMT   \"L\"\n");
	else
		fprintf(f, "#define APP_FLOAT_SCANF_FMT   \n");

	if (sizeof(float) == 8)
		fprintf(f, "#define APP_REAL_SCANF_FMT    \n");
	else if (sizeof(double) == 8)
		fprintf(f, "#define APP_REAL_SCANF_FMT    \"l\"\n");
	else if (sizeof(long double) == 8)
		fprintf(f, "#define APP_REAL_SCANF_FMT    \"L\"\n");
	else
		fprintf(f, "#define APP_REAL_SCANF_FMT    \"l\"\n");

	fprintf(f, "\n");

	/*
	 *  Print type-sizes.
	 */
	fprintf(f, "/* Type sizes (in bytes). */\n");
	fprintf(f, "enum {\n");
	fprintf(f, "\tAPP_BYTE_SIZE       = 1,\n");
	fprintf(f, "\tAPP_SHORT_SIZE      = 2,\n");
	fprintf(f, "\tAPP_INT_SIZE        = 4,\n");
	fprintf(f, "\tAPP_FLOAT_SIZE      = %d,\n", size_app_float32);
	fprintf(f, "\tAPP_REAL_SIZE       = %d,\n", size_app_float64);
	fprintf(f, "\tAPP_POINTER_SIZE    = %d\n", size_app_pointer);
	fprintf(f, "};\n\n");

	/*
	 *  Write alignment specifiers, by examining struct offsets.
	 */
	fprintf(f, "/* Type packing alignment within structures. */\n");
	fprintf(f, "enum {\n");
	fprintf(f, "\tAPP_BYTE_PACKING    = %d,\n", offset_app_int8);
	fprintf(f, "\tAPP_SHORT_PACKING   = %d,\n", offset_app_int16);
	fprintf(f, "\tAPP_INT_PACKING     = %d,\n", offset_app_int32);
	fprintf(f, "\tAPP_FLOAT_PACKING   = %d,\n", offset_app_float32);
	fprintf(f, "\tAPP_REAL_PACKING    = %d,\n", offset_app_float64);
	fprintf(f, "\tAPP_POINTER_PACKING = %d,\n", offset_app_pointer);
	offset_maximum = offset_app_int8;
	if (offset_maximum < offset_app_int16)
		offset_maximum = offset_app_int16;
	if (offset_maximum < offset_app_int32)
		offset_maximum = offset_app_int32;
	if (offset_maximum < offset_app_float32)
		offset_maximum = offset_app_float32;
	if (offset_maximum < offset_app_float64)
		offset_maximum = offset_app_float64;
	if (offset_maximum < offset_app_pointer)
		offset_maximum = offset_app_pointer;
	fprintf(f, "\tAPP_MAXIMUM_PACKING = %d\n", offset_maximum);
	fprintf(f, "};\n\n");

	/*
	 *  Write arithmetic limits. Note, these are fixed.
	 */
	fprintf(f, "/* Arithmetic limits. */\n");
	fprintf(f, "#define APP_BYTE_MIN    (0)\n");
	fprintf(f, "#define APP_BYTE_MAX    (255)\n");
	fprintf(f, "#define APP_SBYTE_MIN   (-128)\n");
	fprintf(f, "#define APP_SBYTE_MAX   (127)\n");
	fprintf(f, "\n");
	fprintf(f, "#define APP_SHORT_MIN   (-32768)\n");
	fprintf(f, "#define APP_SHORT_MAX   (32767)\n");
	fprintf(f, "#define APP_USHORT_MIN  (0)\n");
	fprintf(f, "#define APP_USHORT_MAX  (65535)\n");
	fprintf(f, "\n");
	if ((sizeof(int) != 4) && (sizeof(long) == 4)) {
		fprintf(f, "#define APP_INT_MIN     (-2147483648L)\n");
		fprintf(f, "#define APP_INT_MAX     (2147483647L)\n");
		fprintf(f, "#define APP_UINT_MIN    (0UL)\n");
		fprintf(f, "#define APP_UINT_MAX    (4294967295UL)\n");
		fprintf(f, "\n");
		fprintf(f, "#define APP_CHAR_MIN    (0UL)\n");
		fprintf(f, "#define APP_CHAR_MAX    (4294967295UL)\n");
	}
	else {
		fprintf(f, "#define APP_INT_MIN     (-2147483648)\n");
		fprintf(f, "#define APP_INT_MAX     (2147483647)\n");
		fprintf(f, "#define APP_UINT_MIN    (0)\n");
		fprintf(f, "#define APP_UINT_MAX    (4294967295)\n");
		fprintf(f, "\n");
		fprintf(f, "#define APP_CHAR_MIN    (0)\n");
		fprintf(f, "#define APP_CHAR_MAX    (4294967295)\n");
	}
	fprintf(f, "\n");

	/*
	 *  Close the file and finish.
	 */
	fclose(f);
	return 0;
}
