/*
 *  Memory allocation checker.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/06/28  Better debugging handling.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"

/*
 *  Some internal global variables:
 */

static long memory_usage = 0L;
static void * last_pointer_allocated = NULL;
static int  error_printed = 0;
static long alloc_count = 0L;
static long realloc_count = 0L;
static long free_count = 0L;

/*
 *  Internal definitions:
 */

#define USED_START 0x5AFEC0DEL
#define USED_END   0x5AFE1DEAL
#define FREE_START 0xDECEA5EDL
#define FREE_END   0xDEADD0D0L
#define FILL_BYTE  0x55
#define MAX_BLOCK  (1L<<27)

/*
 *  Print a message on stderr. The format of this function is
 *  the same as printf.
 */
static void app_mem_error(char *fmt, ...)
{
	va_list v;
	FILE *f;
	time_t t;

	va_start(v, fmt);
	f = fopen("apperror.txt", "a");
	if (! error_printed)
	{
		error_printed = 1;
		fprintf(f, "\nApp memory error happened at ");
		t = time(NULL);
		fprintf(f, "%s\n", asctime(localtime(&t)));
		fprintf(f, "  app_alloc has been called:   %ld times\n",
			alloc_count);
		fprintf(f, "  app_realloc has been called: %ld times\n",
			realloc_count);
		fprintf(f, "  app_free has been called:    %ld times\n",
			free_count);
	}
	/*
	else {
		f = fopen("apperror.txt", "a");
	}
	*/
	fprintf(f, "  ");
	vfprintf(f, fmt, v);
	fflush(f);
	fclose(f);
	va_end(v);
}

/*
 *  Find out how much memory is being used.
 */
long app_memory_used(void)
{
	return memory_usage;
}

/*
 *  Safe memory allocators:
 */

static void * app_alloc_safe(long nbytes)
{
	void *ptr;

	if (nbytes <= 0)
		return NULL;
	ptr = malloc(nbytes);
	if (ptr == NULL) {
		app_mem_error("app_alloc: ran out of memory.\n");
		exit(1);
	}
	memory_usage += nbytes;	/* approximation */
	return ptr;
}

static void app_free_safe(void *ptr)
{
	if (ptr)
		free(ptr);
}

static void * app_realloc_safe(void *old, long nbytes)
{
	void *ptr;

	if (nbytes <= 0) {
		app_free_safe(old);
		return NULL;
	}
	else if (old == NULL)
		return app_alloc_safe(nbytes);
	ptr = realloc(old, nbytes);
	if (ptr == NULL) {
		app_mem_error("app_realloc: ran out of memory.\n");
		exit(1);
	}
	return ptr;
}

/*
 *  Debugging memory allocators:
 */

/*
 *  Check that a pointer is a validly allocated pointer returned from
 *  app_alloc_debug, and that its buffers are intact. This function
 *  prints the given message and aborts if there is something wrong.
 *  NULL is assumed to be a valid pointer.
 */
static void app_mem_check(void *v, char *msg)
{
	char * ptr;
	long nbytes, size;
	long i, start, end;

	if (v == NULL)
		return;
	ptr = (char *) v;

	start = *(long *)(ptr - sizeof(long));
	if (start == FREE_START) {
		app_mem_error(msg);
		app_mem_error("app_mem_check: block has already been freed.\n");
		app_mem_error(" Start buffer is 0x%-08.8lX\n", start);
		abort();
	}
	else if (start != USED_START) {
		app_mem_error(msg);
		app_mem_error("app_mem_check: block start buffer is wrong.\n");
		app_mem_error(" Start buffer is 0x%-08.8lX\n", start);
		abort();
	}

	nbytes = *(long *)(ptr - sizeof(long) * 2);
	if (nbytes < 0) {
		app_mem_error(msg);
		app_mem_error("app_mem_check: block size is negative (size is corrupt?).\n");
		app_mem_error(" Block length is %ld.\n", nbytes);
		app_mem_error(" Start buffer is 0x%-08.8lX\n", start);
		abort();
	}
	else if (nbytes > MAX_BLOCK) {
		app_mem_error(msg);
		app_mem_error("app_mem_check: block is too big (size is corrupt?).\n");
		app_mem_error(" Block length is %ld.\n", nbytes);
		app_mem_error(" Start buffer is 0x%-08.8lX\n", start);
		abort();
	}

	size = (nbytes / sizeof(long) + 1) * sizeof(long); /* round up */
	end = *(long *)(ptr + size);
	if (end == FREE_END) {
		app_mem_error(msg);
		app_mem_error("app_mem_check: block buffers disagree.\n");
		app_mem_error(" Block length is %ld.\n", nbytes);
		app_mem_error(" Start buffer is 0x%-08.8lX.\n", start);
		app_mem_error(" End buffer is 0x%-08.8lX.\n",  end);
		abort();
	}
	else if (end != USED_END) {
		app_mem_error(msg);
		app_mem_error("app_mem_check: end of block was overwritten.\n");
		app_mem_error(" Block length is %ld.\n", nbytes);
		app_mem_error(" Start buffer is 0x%-08.8lX.\n", start);
		app_mem_error(" End buffer is 0x%-08.8lX.\n",  end);
		abort();
	}
	for (i=nbytes; i < size; i++) {
		if (ptr[i] == FILL_BYTE)
			continue;
		app_mem_error(msg);
		app_mem_error("app_mem_check: end buffer corrupted at offset %d.\n", i);
		app_mem_error(" Block length is %ld.\n", nbytes);
		app_mem_error(" Start buffer is 0x%-08.8lX.\n", start);
		app_mem_error(" End buffer is 0x%-08.8lX.\n",  end);
		abort();
	}
}

/*
 *  Return the length in bytes of the usable portion of this block.
 *  A NULL pointer is defined to have a length of 0 bytes.
 *  This function performs a complete integrity check of the pointer.
 */
long app_mem_length(void *v)
{
	char * ptr;
	long nbytes;

	if (v == NULL)
		return 0;

	app_mem_check(v, "app_mem_length: error with the pointer passed in\n");

	ptr = (char *) v;

	nbytes = *(long *)(ptr - sizeof(long) * 2);
	return nbytes;
}

/*
 *  Return a block of memory for use by the program. The routine
 *  uses malloc to obtain the block of memory, and then surrounds
 *  the block with a buffer zone of unlikely 'magic numbers' which
 *  are checked when the block is freed, to ensure memory has not
 *  been overwritten accidentally.
 *
 *  The returned block is not initialised and so may contain random,
 *  data, and if malloc fails, this routine causes the program to abort,
 *  so you can assume app_alloc_debug always returns a valid pointer.
 *
 *  Allocating a zero-sized block returns NULL.
 */
static void * app_alloc_debug(long nbytes)
{
	char * ptr;
	long size;

	alloc_count++;

	if (last_pointer_allocated != NULL)
		app_mem_check(last_pointer_allocated,
			"app_alloc: error when checking last allocated pointer\n");

	if (nbytes == 0)
		return NULL;
	else if (nbytes < 0) {
		app_mem_error("app_alloc: requested size is negative (%ld)\n", nbytes);
		abort();
	}
	else if (nbytes > MAX_BLOCK) {
		app_mem_error("app_alloc: requested size is too big (%ld)\n", nbytes);
		abort();
	}
	size = (nbytes / sizeof(long) + 1) * sizeof(long); /* round up */

	memory_usage += nbytes;

	ptr = (char *) malloc(sizeof(long) * 3 + size);
	if (ptr == NULL) {
		app_mem_error("app_alloc: ran out of memory.\n");
		abort();
	}
	*(long *)(ptr) = nbytes;
	*(long *)(ptr+sizeof(long)) = USED_START;
	ptr += sizeof(long) * 2;	/* point to user block */
	memset(ptr+nbytes, FILL_BYTE, size-nbytes);
	*(long *)(ptr+size) = USED_END;

	last_pointer_allocated = ptr;
	return ptr;
}

/*
 *  Free the block of memory previously returned by app_alloc_debug.
 *  Checks that the block was created by app_alloc_debug, and has not
 *  already been freed. Checks that the boundaries have not been
 *  overwritten.
 *
 *  Before freeing the block, we write an unlikely value into the
 *  memory of the block and mark it as having been freed, to prevent
 *  mistaken reuse of this block afterwards.
 *
 *  This function will abort if there is a problem.
 *
 *  Freeing null pointers is allowed and does nothing.
 */
static void app_free_debug(void *v)
{
	char * ptr;
	long nbytes, size;

	free_count++;

	if (last_pointer_allocated != NULL)
		app_mem_check(last_pointer_allocated,
			"app_free: error when checking last allocated pointer\n");

	if (v == NULL)
		return;

	if (v == last_pointer_allocated)
		last_pointer_allocated = NULL;
	else
		app_mem_check(v, "app_free: error with pointer passed in\n");

	ptr = (char *) v;

	nbytes = *(long *)(ptr - sizeof(long) * 2);
	size = (nbytes / sizeof(long) + 1) * sizeof(long); /* round up */

	memory_usage = memory_usage - nbytes;

	*(long *)(ptr - sizeof(long)) = FREE_START;
	*(long *)(ptr + size) = FREE_END;
	memset(ptr, FILL_BYTE, size);
	ptr -= sizeof(long) * 2;	/* point to malloc'd block */
	free(ptr);
}

/*
 *  Realloc a block after checking the buffers are intact.
 *  Reallocing a NULL pointer is equivalent to using app_alloc_debug,
 *  and reallocing to a size of zero bytes is the same as
 *  using app_free_debug.
 */
static void * app_realloc_debug(void *v, long nbytes)
{
	char *ptr;
	long oldsize, size;

	realloc_count++;

	if (last_pointer_allocated != NULL)
		app_mem_check(last_pointer_allocated,
			"app_realloc: error when checking last allocated pointer\n");

	if (v == NULL)
		return app_alloc_debug(nbytes);
	else if (nbytes == 0) {
		app_free_debug(v);
		return NULL;
	}
	else if (nbytes < 0) {
		app_mem_error("app_realloc: requested size is negative (%ld)\n", nbytes);
		abort();
	}
	else if (nbytes > MAX_BLOCK) {
		app_mem_error("app_realloc: requested size is too big (%ld)\n", nbytes);
		abort();
	}

	app_mem_check(v, "app_realloc: error with pointer passed in\n");

	ptr = ((char *) v) - sizeof(long) * 2;
	oldsize = *(long *)(ptr);

	size = (nbytes / sizeof(long) + 1) * sizeof(long); /* round up */

	memory_usage = memory_usage + nbytes - oldsize;

	ptr = (char *) realloc(ptr, sizeof(long) * 3 + size);
	if (ptr == NULL) {
		app_mem_error("app_realloc: ran out of memory.\n");
		abort();
	}
	*(long *)(ptr) = nbytes;	/* USED_START already there */
	ptr += sizeof(long) * 2;	/* point to user block */
	memset(ptr+nbytes, FILL_BYTE, size-nbytes);
	*(long *)(ptr+size) = USED_END;

	last_pointer_allocated = ptr;
	return ptr;
}

/*
 *  Exported function pointers:
 */

static void * (*app_alloc_function)(long)           = app_alloc_safe;
static void * (*app_realloc_function)(void *, long) = app_realloc_safe;
static void   (*app_free_function)(void *)          = app_free_safe;

/*
 *  Call memory allocators through function pointers:
 */

/*
 *  Control which set of functions to call:
 */

void app_debug_memory(int on)
{
	if (on) {
		app_alloc_function = app_alloc_debug;
		app_realloc_function = app_realloc_debug;
		app_free_function = app_free_debug;
	} else {
		app_alloc_function = app_alloc_safe;
		app_realloc_function = app_realloc_safe;
		app_free_function = app_free_safe;
	}
}

/*
 *  Public allocator functions:
 */

void * app_alloc(long nbytes)
{
	return app_alloc_function(nbytes);
}

void * app_zero_alloc(long nbytes)
{
	void * ptr = app_alloc_function(nbytes);
	memset(ptr, 0, nbytes);
	return ptr;
}

void * app_realloc(void *ptr, long nbytes)
{
	return app_realloc_function(ptr, nbytes);
}

void app_free(void *ptr)
{
	app_free_function(ptr);
}
