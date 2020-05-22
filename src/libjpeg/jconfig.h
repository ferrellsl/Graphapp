/* jconfig.h.  Generated automatically by configure.  */
/* jconfig.cfg --- source file edited by configure script */
/* see jconfig.doc for explanations */

#define HAVE_PROTOTYPES 
#define HAVE_UNSIGNED_CHAR 
#define HAVE_UNSIGNED_SHORT 
#undef void
#undef const
#undef CHAR_IS_UNSIGNED
#define HAVE_STDDEF_H 
#define HAVE_STDLIB_H 
#undef NEED_BSD_STRINGS
#undef NEED_SYS_TYPES_H
#undef NEED_FAR_POINTERS
#undef NEED_SHORT_EXTERNAL_NAMES

/* Define this if you get warnings about undefined structures. */
#define INCOMPLETE_TYPES_BROKEN

#ifdef JPEG_INTERNALS

/* For safety, we assume right shifting is unsigned on all platforms. */
#define RIGHT_SHIFT_IS_UNSIGNED

/* Don't assume we have the keyword __inline__ */
#undef INLINE

/* These are for configuring the JPEG memory manager. */
#undef DEFAULT_MAX_MEM
#undef NO_MKTEMP

#endif /* JPEG_INTERNALS */
