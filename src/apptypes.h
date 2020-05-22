/*
 *  apptypes.h
 *  Contains platform and compiler-specific definitions.
 *  Do not edit this machine-generated file.
 *  Re-compile and run apptypes.c instead.
 */

/* Standard type names: */
typedef signed char     app_int8;    /* 8-bit signed integer. */
typedef unsigned char   app_uint8;   /* 8-bit unsigned integer. */
typedef short           app_int16;   /* 16-bit signed integer. */
typedef unsigned short  app_uint16;  /* 16-bit unsigned integer. */
typedef int             app_int32;   /* 32-bit signed integer. */
typedef unsigned int    app_uint32;  /* 32-bit unsigned integer. */
typedef float           app_float32; /* Small floating-point number. */
typedef double          app_float64; /* Large floating-point number. */
typedef void *          app_pointer; /* Largest pointer. */

/* Common type names: */
typedef app_uint8       app_byte;    /* 8-bit unsigned integer. */
typedef app_int8        app_sbyte;   /* 8-bit signed integer. */
typedef app_int16       app_short;   /* 16-bit signed integer. */
typedef app_uint16      app_ushort;  /* 16-bit unsigned integer. */
typedef app_int32       app_int;     /* 32-bit signed integer. */
typedef app_uint32      app_uint;    /* 32-bit unsigned integer. */
typedef app_float32     app_float;   /* 32-bit floating point number. */
typedef app_float64     app_real;    /* 64-bit floating point number. */
typedef app_uint32      app_char;    /* 32-bit Unicode Char. */

/* The printf length modifier format strings. */
#define APP_BYTE_PRINTF_FMT   
#define APP_SHORT_PRINTF_FMT  "h"
#define APP_INT_PRINTF_FMT    
#define APP_FLOAT_PRINTF_FMT  
#define APP_REAL_PRINTF_FMT   

/* The scanf length modifier format strings. */
#define APP_BYTE_SCANF_FMT    
#define APP_SHORT_SCANF_FMT   "h"
#define APP_INT_SCANF_FMT     
#define APP_FLOAT_SCANF_FMT   
#define APP_REAL_SCANF_FMT    "l"

/* Type sizes (in bytes). */
enum {
	APP_BYTE_SIZE       = 1,
	APP_SHORT_SIZE      = 2,
	APP_INT_SIZE        = 4,
	APP_FLOAT_SIZE      = 4,
	APP_REAL_SIZE       = 8,
	APP_POINTER_SIZE    = 8
};

/* Type packing alignment within structures. */
enum {
	APP_BYTE_PACKING    = 1,
	APP_SHORT_PACKING   = 2,
	APP_INT_PACKING     = 4,
	APP_FLOAT_PACKING   = 4,
	APP_REAL_PACKING    = 8,
	APP_POINTER_PACKING = 8,
	APP_MAXIMUM_PACKING = 8
};

/* Arithmetic limits. */
#define APP_BYTE_MIN    (0)
#define APP_BYTE_MAX    (255)
#define APP_SBYTE_MIN   (-128)
#define APP_SBYTE_MAX   (127)

#define APP_SHORT_MIN   (-32768)
#define APP_SHORT_MAX   (32767)
#define APP_USHORT_MIN  (0)
#define APP_USHORT_MAX  (65535)

#define APP_INT_MIN     (-2147483648)
#define APP_INT_MAX     (2147483647)
#define APP_UINT_MIN    (0)
#define APP_UINT_MAX    (4294967295)

#define APP_CHAR_MIN    (0)
#define APP_CHAR_MAX    (4294967295)

