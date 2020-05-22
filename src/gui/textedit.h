/*
 *  Common text editing structures and function definitions.
 *
 *  Platform: Neutral
 *
 *  Version: 3.50  2004/01/01  First release.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

typedef struct TextBox  TextBox;

struct TextBox {

	/* used by fields, password-fields, text-boxes: */

	int start;		/* first visible glyph's first byte */
	int caret;		/* insertion point byte-position */
	int selected;		/* selected length, 0=bar, else <0 >0 */
	char *allowed;		/* allowed Unicode characters, or NULL */
	char *disallowed;	/* disallowed characters, or NULL */
	int maxwidth;		/* maximum length in Chars, or 0 */
	int text_length;	/* running byte count of text length */

	/* used only by text boxes: (could use subclassing if C++!) */

	int num_lines;		/* number of lines in entire text */
	long *lines;		/* indices to start of each line */
	Control *parent;	/* enclosing region */
	Control *vert;		/* vertical scroll bar */
	Control *horz;		/* horizontal scroll bar */
	Control *box;		/* text canvas */
	int prev_start;		/* previous selection start */
	int prev_end;		/* previous selection end */
};

