/*
 *  Reading and writing UTF-8 and ISO-Latin-1 streams.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/18  Updated.
 *  Version: 3.08  2001/11/11  Added utf8_to_latin1, utf8_is_latin1.
 *  Version: 3.11  2001/12/12  Added unicode_char_to_utf8 and vice versa.
 *  Version: 3.12  2001/12/13  Added utf8_length function.
 *  Version: 3.29  2002/08/22  Fixed bugs in utf8_to_latin1, write_latin1.
 *  Version: 3.32  2002/09/04  Added correct_utf8 function.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced some conversion warnings.
 *  Version: 3.62  2009/07/26  Bugfix in read_utf8_until.
 */

/* Copyright (c) L. Patrick and the Unicode organisation.

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.

   Portions of this code were developed by the Unicode organisation
   for free use by programmers, to promote the Unicode standard.
*/

/*
 *  UTF-8 is a way of reading and writing Unicode 32-bit characters
 *  to ordinary 8-bit communications streams.
 *
 *  The UTF-8 algorithm stores characters into variable-sized
 *  chunks. Characters in the range 0x00 to 0x7F fit into one
 *  byte, since these will be quite common (ASCII values).
 *  Characters with higher values fit into two, three, four,
 *  five, or six bytes, depending on the number of significant
 *  bits, according to the following pattern:
 *
 *  Bits  Pattern
 *  ----  -------
 *    7   0xxxxxxx
 *   11   110xxxxx 10xxxxxx
 *   16   1110xxxx 10xxxxxx 10xxxxxx
 *   21   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 *   26   111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 *   32   111111xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 *
 *  As can be seen from the table, at most 32 bits can be stored
 *  using this algorithm (the x's mark where the actual bits go,
 *  the numbers signify the padding bits). The padding "10" at
 *  the start of a byte uniquely identifies a continuation byte,
 *  which is never used as the start of a UTF-8 character sequence,
 *  so if a stream is broken for some reason, the algorithm can
 *  skip those bytes to find the next start of a character.
 *
 *  ASCII is a 7-bit encoding for the English language alphabet
 *  and various digits and symbols. Its values range from 0x00 to 0x7F.
 *
 *  A superset of ASCII is ISO-Latin-1 (code page 8859-1). This is
 *  an 8-bit encoding for Western European languages, with values
 *  in the range 0x00 to 0xFF. The lower half of this range is
 *  the same as ASCII, while the upper half includes many accented
 *  characters.
 *
 *  Unicode is a superset of ISO-Latin-1, which mostly fits into
 *  16-bits, but which is actually a 32-bit encoding for most
 *  language symbols on Earth, including Eastern European, African,
 *  Asian, and many other languages. It allows a single document
 *  to contain mixtures of all languages.
 *
 *  This file contains functions for reading and writing Unicode
 *  and ISO-Latin-1 streams, to and from an array of 32-bit
 *  Unicode values in memory. Each 32-bit value is called a Char.
 */

#include <stdio.h>
#include "apputils.h"

enum {
	Low6Bits = 0x3F,	/* 00111111 */
	High2Bits = 0xC0,	/* 11000000 */
	ByteMask = 0x00BF,	/* 10111111 */
	ContinueBits = 0x80	/* 10xxxxxx */
};

static const unsigned long ReplacementChar = 0x0000FFFDUL;
static const unsigned long MaximumChar     = 0x7FFFFFFFUL;

static const byte UTF8ExtraBytes[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

static const byte FirstByteBits[7] = {
	0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
};

static const unsigned long FirstByteMask[6] = {
	0xFF, 0x1F, 0x0F, 0x07, 0x03, 0x03
};


int app_unicode_to_utf8(const Char ** src_start, const Char * src_end,
			char ** dst_start, const char * dst_end)
{
	ConversionResult result = ConversionSuccess;
	register const Char * src = *src_start;
	register char * dst = *dst_start;

	while (src < src_end) {
		register unsigned long ch;
		register unsigned short bytes_to_write;
		register unsigned short extra_bytes;

		ch = *src++;

		if (ch < 0x80) {                bytes_to_write = 1;
		} else if (ch < 0x800) {        bytes_to_write = 2;
		} else if (ch < 0x10000) {      bytes_to_write = 3;
		} else if (ch < 0x200000) {     bytes_to_write = 4;
		} else if (ch < 0x4000000) {    bytes_to_write = 5;
		} else if (ch <= MaximumChar){  bytes_to_write = 6;
		} else {
			bytes_to_write = 2;
			ch = ReplacementChar;
		}

		dst += bytes_to_write;
		if (dst > dst_end) {
			dst -= bytes_to_write;
			result = TargetExhausted;
			break;
		}
		extra_bytes = bytes_to_write;
		while (--extra_bytes) {
			*--dst = (char) ((ch | ContinueBits) & ByteMask);
			ch >>= 6;
		}
		*--dst = (char) (ch | FirstByteBits[bytes_to_write]);

		dst += bytes_to_write;
	}
	*src_start = src;
	*dst_start = dst;
	return result;
}


int app_utf8_to_unicode(const char ** src_start, const char * src_end,
			Char ** dst_start, const Char * dst_end)
{
	ConversionResult result = ConversionSuccess;
	register const char * src = *src_start;
	register Char * dst = *dst_start;

	while (src < src_end) {
		register Char ch;
		register unsigned short extra_bytes;

		extra_bytes = UTF8ExtraBytes[(unsigned char)*src];
		/*
		if (src + extra_bytes > src_end) {
			result |= SourceExhausted;
			break;
		}
		*/
		if (dst >= dst_end) {
			result |= TargetExhausted;
			break;
		}

		ch = (unsigned char)*src++;
		if (extra_bytes) {
			if (src >= src_end) {
				result |= SourceCorrupt;
				result |= SourceExhausted;
			}
			else if ((*src & High2Bits) == ContinueBits) {
				ch &= FirstByteMask[extra_bytes];
				do {
					ch <<= 6;
					ch |= ((*src++) & Low6Bits);
					if (--extra_bytes == 0)
						break;
					if (src >= src_end) {
						result |= SourceCorrupt;
						result |= SourceExhausted;
						break;
					}
					if ((*src & High2Bits) != ContinueBits) {
						result |= SourceCorrupt;
						break;
					}
				} while (1);
			} else {
				result |= SourceCorrupt;
			}
		}

		if (ch <= MaximumChar) {
			*dst++ = ch;
		} else {
			*dst++ = ReplacementChar;
		}
	}
	*src_start = src;
	*dst_start = dst;
	return result;
}

/*
 *  Convert a single Unicode character into a UTF-8 byte buffer.
 *  The buffer should be at least 7 bytes wide.
 */

int app_unicode_char_to_utf8(Char ch, char *utf8)
{
	char *bp;
	const Char *cp;

	cp = &ch;
	bp = utf8;
	memset(utf8, 0, 7);
	return app_unicode_to_utf8(&cp, cp+1, &bp, bp+7);
}

/*
 *  Convert a single UTF-8 character into a Unicode value.
 */

unsigned long app_utf8_char_to_unicode(const char *utf8)
{
	const char *bp;
	Char ch;
	Char *cp;
	int result;

	cp = &ch;
	bp = utf8;
	result = app_utf8_to_unicode(&bp, bp+7, &cp, cp+1);
	return ch;
}

/*
 *  Return the number of Unicode characters within a UTF-8 string.
 *  For ASCII strings this will return the same number as strlen.
 */

int app_utf8_length(const char *utf8)
{
	int length = 0;

	while (*utf8 != '\0') {
		length++;
		if ((*utf8 & High2Bits) == High2Bits) {
			utf8++;
			while ((*utf8 & High2Bits) == ContinueBits)
				utf8++;
		}
		else
			utf8++;
	}
	return length;
}

/*
 *  Read a Unicode value from a UTF-8 file. The file must
 *  be open in binary mode to read. Errors are reported
 *  in the return value, as a ConversionResult.
 */

int app_read_utf8_char(FILE *f, Char * dst)
{
	ConversionResult result = ConversionSuccess;
	Char ch;
	unsigned short extra_bytes;
	int c;

	c = getc(f);
	if (c == EOF) {
		*dst = '\0';
		return SourceExhausted;
	}

	extra_bytes = UTF8ExtraBytes[c];

	ch = c;
	if (extra_bytes) {
		c = getc(f);
		if (c == EOF) {
			*dst = ch;
			return SourceExhausted;
		}
		if ((c & High2Bits) == ContinueBits) {
			ch &= FirstByteMask[extra_bytes];
			do {
				ch <<= 6;
				ch |= (c & Low6Bits);
				if (--extra_bytes == 0)
					break;
				c = getc(f);
				if (c == EOF) {
					result = SourceExhausted;
					break;
				}
				if ((c & High2Bits) != ContinueBits) {
					ungetc(c, f);
					result = SourceCorrupt;
					break;
				}
			} while (1);
		} else {
			result = SourceCorrupt;
		}
	}

	if (ch <= MaximumChar) {
		*dst = ch;
	} else {
		*dst = ReplacementChar;
	}
	return result;
}


/*
 *  Read a file into a UTF-8 char array, up to and including
 *  the 'stop' character (or an EOF will end input).
 *  This function returns the app_alloc'd UTF-8 encoded string.
 *  The number of bytes in the returned string is placed in *nbytes.
 *  The number of characters in the returned string is placed in *nchars.
 *  If EOF is encountered immediately, the function returns NULL.
 *  If the 'stop' character is EOF, this function reads the
 *  entire file.
 */

char *app_read_latin1_until(FILE *f, long *nbytes, long *nchars, long stop)
{
	long pos, nch, max;
	char *line;
	char *c;
	int ch;

	if (feof(f)) {
		*nbytes = 0;
		*nchars = 0;
		return NULL;
	}

	pos = 0;
	nch = 0;
	max = 112; /* big enough for many input lines; not a power of 2 */
	line = app_alloc(max);
	c = & line[pos];

	while ((ch = getc(f)) != EOF) {
		if (ch & 0x80) {
			/* character will occupy two bytes */
			if (pos+2 >= max) {
				/* realloc buffer twice the size */
				max += max;
				line = app_realloc(line, max);
				c = & line[pos];
			}
			*c = (0xC0 | ((ch >> 6) & 0x03)); /* 110---xx */
			c++;
			pos++;

			*c = (0x80 | (ch & 0x3F)); /* 10xxxxxx */
			c++;
			pos++;

			nch++;
		}
		else {
			/* character will only occupy one byte */
			*c = (ch & 0x7F);
			c++;
			pos++;
			if (pos == max) {
				/* realloc buffer twice the size */
				max += max;
				line = app_realloc(line, max);
				c = & line[pos];
			}
			nch++;
		}
		if (ch == stop)
			break;
	}

	/* shrink array to smallest required space */
	line = app_realloc(line, pos+1);
	line[pos] = '\0';

	*nbytes = pos;
	*nchars = nch;
	return line;
}

char *app_read_utf8_until(FILE *f, long *nbytes, long *nchars, long stop)
{
	long pos, nch, max;
	char *line;
	char *c;
	int ch;

	if (feof(f)) {
		*nbytes = 0;
		*nchars = 0;
		return NULL;
	}

	pos = 0;
	nch = 0;
	max = 112; /* big enough for many input lines; not a power of 2 */
	line = app_alloc(max);
	c = & line[pos];

	while ((ch = getc(f)) != EOF) {
		*c = ch;
		c++;
		pos++;
		if (pos == max) {
			/* realloc buffer twice the size */
			max += max;
			line = app_realloc(line, max);
			c = & line[pos];
		}
		if ((ch & 0xC0) != 0x80) /* i.e. not 10xxxxxx */
			nch++;
		if (ch == stop)
			break;
	}

	/* shrink array to smallest required space */
	line = app_realloc(line, pos+1);
	line[pos] = '\0';

	*nbytes = pos;
	*nchars = nch;
	return line;
}


/*
 *  Read an entire file into a memory char array.
 *  Return NULL if the file is empty.
 */

char *app_read_latin1_file(FILE *f, long *nbytes, long *nchars)
{
	return app_read_latin1_until(f, nbytes, nchars, EOF);
}

char *app_read_utf8_file(FILE *f, long *nbytes, long *nchars)
{
	return app_read_utf8_until(f, nbytes, nchars, EOF);
}

/*
 *  A function for reading one line of input from a file.
 *  This function returns the app_alloc'd string, and any
 *  terminating newline character is included in the line.
 *  The length of the returned string is placed into *length.
 *  If EOF is encountered immediately, the function returns NULL.
 *  If EOF is encountered before a newline character, the string
 *  is returned without any terminating newline.
 *  Otherwise, a newline character will be the last character
 *  in the char array.
 */
char *app_read_latin1_line(FILE *f, long *nbytes, long *nchars)
{
	return app_read_latin1_until(f, nbytes, nchars, '\n');
}

char *app_read_utf8_line(FILE *f, long *nbytes, long *nchars)
{
	return app_read_utf8_until(f, nbytes, nchars, '\n');
}

/*
 *  Write a UTF-8 char array to a file as ISO Latin 1
 *  (non ISO-Latin-1 characters will be distorted by this process).
 *  Assume the UTF-8 char array is correct.
 */
int app_write_latin1(FILE *f, const char *utf8, long nbytes)
{
	Char buf[1];
	Char *bp;
	char ch;
	const char *sp;
	long total = 0L;

	sp = utf8;
	while (nbytes > 0) {
		/* write one UTF-8 sequence into a Char buffer */
		bp = buf;
		app_utf8_to_unicode(&sp, sp+7, &bp, bp+1);
		/* determine what happened */
		nbytes -= (long) (sp - utf8);
		utf8 = sp;
		/* force Unicode Char into a Latin-1 char */
		ch = (char) (buf[0] & 0x00FF);
		putc(ch, f);
		total++;
	}
	fflush(f);
	return total;
}

/*
 *  Write a UTF-8 char array to a file as UTF-8.
 *  Assume the UTF-8 char array is correct.
 */
int app_write_utf8(FILE *f, const char *utf8, long nbytes)
{
	long n, total = 0L;

	while (nbytes > 256) {
		n = (long) fwrite(utf8, 1, 256, f);
		total += n;
		nbytes -= n;
	}
	total += (long) fwrite(utf8, 1, nbytes, f);
	fflush(f);
	return total;
}

/*
 *  Convert a UTF-8 char array to an ISO Latin 1 char array
 *  (non ISO Latin 1 characters will be distorted by this process).
 *  Assume the UTF-8 char array is correct.
 *  This function creates a new string containing the ISO Latin 1 data.
 *  It returns NULL if it runs out of memory.
 */
char *app_utf8_to_latin1(const char *utf8, int *bytes)
{
	Char buf[1];
	Char *bp;
	const char *sp;
	long total = 0L;
	long nbytes = *bytes;
	char *dp;
	char *dest;

	dest = app_alloc(nbytes + 1);
	if (dest == NULL) {
		*bytes = 0;
		return NULL;
	}
	dp = dest;

	sp = utf8;
	while (nbytes > 0) {
		/* write one UTF-8 sequence into a Char buffer */
		bp = buf;
		app_utf8_to_unicode(&sp, sp+7, &bp, bp+1);
		/* determine what happened */
		nbytes -= (long) (sp - utf8);
		utf8 = sp;
		/* force Unicode Char into a Latin-1 char */
		*dp++ = (char) (buf[0] & 0x00FF);
		total++;
	}
	*dp = '\0';
	*bytes = total;
	return dest;
}

/*
 *  Convert a (possibly ISO Latin 1) char array to a UTF-8 char array,
 *  as best we can. If it is already correctly UTF 8 encoded, return
 *  the input string unchanged.
 *  This function may create a new string containing the UTF-8 data.
 *  It returns NULL if it runs out of memory.
 */
char *app_correct_utf8(const char *s, int *bytes)
{
	Char buf[1];
	Char *bp;
	const Char *cbp;
	const char *sp;
	long total;
	long nbytes = *bytes;
	char *dp;
	char *dest;
	char tmp[8];
	char *tp;
	const char *original;
	int i;
	int diff = 0;

	total = nbytes;
	dest = app_alloc(total + 1);
	if (dest == NULL) {
		*bytes = 0;
		return NULL;
	}
	dp = dest;

	original = s;
	sp = s;
	while (nbytes > 0) {
		/* write one UTF-8 sequence into a Char buffer */
		bp = buf;
		app_utf8_to_unicode(&sp, sp+7, &bp, bp+1);

		/* determine what happened */
		nbytes -= (long) (sp - s);

		/* check for any difference, copy string also */
		cbp = buf;
		tp = tmp;
		app_unicode_to_utf8(&cbp, cbp+1, &tp, tp+7);
		i = (int) (dp-dest);
		if (total-i < tp-tmp) {
			total = i + (long) (tp-tmp);
			dest = app_realloc(dest, total+1);
			dp = dest + i;
		}
		for (i=0; i < (tp-tmp); i++) {
			if (s[i] != tmp[i])
				diff = 1;
			*dp++ = tmp[i];
		}

		/* move on to next char */
		s = sp;
	}
	*dp = '\0';
	*bytes = total;
	if (diff == 0) {
		app_free(dest);
		return (char *) original; /* cast away const */
	}
	return dest;
}

/*
 *  Return non-zero (true) if the given UTF-8 char array contains
 *  only ASCII characters, otherwise return zero. 
 */
int app_utf8_is_ascii(const char *utf8, long nbytes)
{
	long i;

	for (i=0; i < nbytes; i++) {
		if (*utf8 & 0x80)
			return 0;
		utf8++;
	}
	return 1;
}

/*
 *  Return non-zero (true) if the given UTF-8 char array contains
 *  only ASCII and ISO Latin-1 characters, otherwise return zero. 
 */
int app_utf8_is_latin1(const char *utf8, long nbytes)
{
	long i;

	for (i=0; i < nbytes; i++) {
		if (*utf8 & 0x80) {
			if ((*utf8 & 0xFC) != 0xC0) /* not 110-000xx */
				return 0;
			utf8++;
			i++;
			if (i >= nbytes)
				return 0;
			if ((*utf8 & 0xC0) != 0x80) /* not 10-xxxxxx */
				return 0;
		}
		utf8++;
	}
	return 1;
}

