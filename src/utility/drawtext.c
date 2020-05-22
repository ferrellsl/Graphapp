/*
 *  Portable text drawing functions.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/08/13  Fixed font handling.
 *  Version: 3.03  2001/10/18  Added text_line_length, changed text_height.
 *  Version: 3.05  2001/10/28  Fixed text clipping, added text_width.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced some pointer subtraction warnings.
 *  Version: 3.58  2005/09/28  Fixed possible bug in justified text.
 *  Version: 3.60  2007/06/06  app_text_width now examines entire text.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"

enum DrawTextConstants
{
	TAB_SIZE = 8,
	MAX_WORD = 256,
	MIN_BUF  = 192
};

#define IS_UTF8_START_BYTE(ch) ((((ch)>>7)&1)==1) /* is 1xxxxxxx */
#define IS_UTF8_CONT_BYTE(ch)  ((((ch)>>6)&3)==2) /* is 10xxxxxx */

#undef MAX
#define MAX(x,y) (((x)>(y))?(x):(y))

/*
 *  Internal function for finding a line of text to draw.
 *  A line is defined as whatever words fit in a certain
 *  pixel width, up to and not including a newline character.
 *  If half a word fits, the word will be wrapped to the
 *  next line, unless the word fills the entire line, in
 *  which case the word is truncated to fit.
 *
 *  If the pixel width runs out before reaching a newline,
 *  the line ends. The minimum number of characters on
 *  a line will be 1, even if an entire character will not
 *  fit in the given pixel width. This ensures that the
 *  function doesn't infinite loop on small inputs.
 *
 *  The line may be realloc'd larger during this function.
 *  To begin with, the line should be set to NULL and this
 *  function will allocate an appropriate block. After
 *  all calls to this function are finished, the calling
 *  function should call app_free to destroy the line.
 */
static int app_get_next_line(Font *f, int pixel_width,
	char *line[], int *line_bytes, const char *utf8[], int *nbytes)
{
	char ch, *glyph, *dst;
	const char *src;
	int max;
	int gb, lb, sb; /* byte sizes for glyph, line, source */
	int sword, dword; /* byte sizes for the word, in source or dest */
	int glyph_width, line_width; /* in pixels */
	int tab, column; /* for expanding tabs into spaces */
	int alloc_size; /* how many bytes allocated in line */

	ch = ' ';
	tab = column = sword = dword = lb = sb = 0;
	max = *nbytes;
	line_width = 0;
	src = *utf8;

	alloc_size = MIN_BUF;
	if (*line == NULL)
		dst = *line = app_alloc(MIN_BUF);
	else
		dst = *line;

	if (*nbytes == 0) {
		(*line)[0] = '\0';
		return 0;
	}

	do {
		/* grab a single glyph */
		gb = 0;
		glyph = dst;

		/* if there might be no space in dest line, resize it */
		if (lb + MAX(8,TAB_SIZE) > alloc_size) {
			alloc_size *= 2;
			dst = app_realloc(*line, alloc_size);
			if (dst == NULL)
				break;
			*line = dst;
			dst += lb;
		}

		/* whitespace and separators break words */
		if ((ch == ' ') || (ch == ',') || (ch == '-'))
			sword = dword = 0; /* reset length of word */

		ch = *src;

		/* whitespaces are treated as words on their own */
		if (ch == ' ') {
			sword = dword = 0; /* reset length of word */
		}
		else if (ch == '\t') { /* expand tabs in destination */
			ch = ' ';
			sword = dword = 0;
			for (tab=column%TAB_SIZE; tab < TAB_SIZE-1; tab++)
			{
				*dst++ = ch;
				lb++;
				dword++;
				column++;
			}
		}
		else if (ch == '\n') { /* newlines end the line */
			sword = dword = 0;
			*dst++ = ch; /* save the newline to dest */
			sb++; /* skip the newline in the source */
			break; /* but don't count the newline in lb */
		}

		/* grab first (and maybe only) byte of UTF-8 sequence */
		*dst++ = ch;
		src++;
		gb++;
		sb++;

		/* grab UTF-8 continuation bytes */
		if (IS_UTF8_START_BYTE(ch)) {
			while ((sb < max) && IS_UTF8_CONT_BYTE(*src)) {
				*dst++ = *src++;
				gb++;
				sb++;
			}
		}

		glyph_width = app_font_width(f, glyph, gb);

		if (tab) {
			glyph_width *= (dword+1); /* tabs = wide spaces */
			tab = 0;
		}

		if (glyph_width + line_width <= pixel_width)
		{
			/* if the glyph fits, keep it */
			line_width += glyph_width;
			lb += gb;
			sword += gb;
			dword += gb;
			column++;
		}
		else {
			/* the glyph doesn't fit on the line */
			if (line_width == 0) {
				/* put at least one glyph on the line! */
				line_width += glyph_width;
				lb += gb;
				sword += gb;
				dword += gb;
			}
			else {
				/* else, don't keep the glyph */
				sb -= gb;
			}
			break; /* either way, nothing else fits */
		}
	} while (sb < max);

	/* remove final word on line, unless it is the whole line */
	if ((sb < max) && (sword < sb)) {
		sb -= sword;
		lb -= dword;
	}

	*utf8 += sb;
	*nbytes -= sb;
	*line_bytes = lb;
	(*line)[lb] = '\0';
	return line_width;
}

/*
 *  Measure the width in bytes of a single line of text, in the font.
 */
int app_text_line_length(Font *f, int pixel_width, const char *utf8, int nb)
{
	int nl, nbytes;
	char *line;

	nbytes = nb;
	line = NULL;

	app_get_next_line(f, pixel_width, &line, &nl, &utf8, &nb);

	app_free(line);
	return nbytes - nb;
}

/*
 *  Measure text width in pixels up to a maximum line width in pixels
 *  (expanding tabs).
 */
int app_text_width(Font *f, int pixel_width, const char *utf8, int nb)
{
	int nl, width;
	char *line;

	if (nb == 0)
		return 0;

	width = 0;
	line = NULL;

	while (nb > 0)
	{
		int w;
		w = app_get_next_line(f, pixel_width, &line, &nl, &utf8, &nb);
		if (width < w)
			width = w;
	}

	app_free(line);
	return width;
}

/*
 *  Measure text height in pixels (a multiple of the font's height).
 */
int app_text_height(Font *f, int pixel_width, const char *utf8, int nb)
{
	int y, nl, line_height;
	char *line;

	line_height = app_font_height(f);
	line = NULL;

	for (y=0; nb > 0; y += line_height)
	{
		app_get_next_line(f, pixel_width, &line, &nl, &utf8, &nb);
	}

	app_free(line);
	return y;
}

/*
 *  Drawing text functions, for various text alignments.
 */
static const char *app_draw_text_left(Graphics *g, Rect r, int line_height,
	const char *utf8, int nb)
{
	int nl;
	int right_to_left;
	char *line;
	Point p;
	Font *f;

	p.x = r.x;
	p.y = r.y;
	f = g->font;
	line = NULL;

	if (g->text_direction & RL_TB)
		right_to_left = 1;
	else
		right_to_left = 0;

	for (nl=0; (p.y <= r.y+r.height) && (nb > 0); p.y += line_height)
	{
		app_get_next_line(f, r.width, &line, &nl, &utf8, &nb);
		p.x = r.x;
		if (right_to_left)
			p.x += app_font_width(f, line, nl);
		app_draw_utf8(g, p, line, nl);
	}

	app_free(line);
	if (nb == 0)
		return NULL;
	return utf8;
}

static const char *app_draw_text_right(Graphics *g, Rect r, int line_height,
	const char *utf8, int nb)
{
	int nl;
	int right_to_left;
	char *line;
	Point p;
	Font *f;

	p.x = r.x;
	p.y = r.y;
	f = g->font;
	line = NULL;

	if (g->text_direction & RL_TB)
		right_to_left = 1;
	else
		right_to_left = 0;

	for (nl=0; (p.y <= r.y+r.height) && (nb > 0); p.y += line_height)
	{
		app_get_next_line(f, r.width, &line, &nl, &utf8, &nb);
		p.x = r.x + r.width;
		if (! right_to_left)
			p.x -= app_font_width(f, line, nl);
		app_draw_utf8(g, p, line, nl);
	}

	app_free(line);
	if (nb == 0)
		return NULL;
	return utf8;
}

static const char *app_draw_text_centered(Graphics *g, Rect r, int line_height,
	const char *utf8, int nb)
{
	int nl, w;
	int right_to_left;
	char *line;
	Point p;
	Font *f;

	p.x = r.x;
	p.y = r.y;
	f = g->font;
	line = NULL;

	if (g->text_direction & RL_TB)
		right_to_left = 1;
	else
		right_to_left = 0;

	for (nl=0; (p.y <= r.y+r.height) && (nb > 0); p.y += line_height)
	{
		app_get_next_line(f, r.width, &line, &nl, &utf8, &nb);
		w = app_font_width(f, line, nl);
		p.x = r.x + (r.width - w) / 2;
		if (right_to_left)
			p.x += w;
		app_draw_utf8(g, p, line, nl);
	}

	app_free(line);
	if (nb == 0)
		return NULL;
	return utf8;
}

static int app_find_word_size(const char *utf8, int nbytes)
{
	int i;

	if (nbytes > 0)
		if (*utf8 == ' ')
			return 1; /* spaces are words 1 byte wide */

	for (i=0; i < nbytes; i++) {
		if (*utf8++ == ' ')
			return i; /* a word is all bytes up to a space */
	}
	return i;
}

static const char *app_draw_text_justified(Graphics *g, Rect r, int line_height,
	const char *utf8, int nb)
{
	int nl, w, ns, sw, xw;
	int width, spaces, last;
	int right_to_left;
	char *line, *s, *start, *end;
	Point p;
	Font *f;

	p.x = r.x;
	p.y = r.y;
	f = g->font;
	line = NULL;

	if (g->text_direction & RL_TB)
		right_to_left = 1;
	else
		right_to_left = 0;

	for (nl=0; (p.y <= r.y+r.height) && (nb > 0); p.y += line_height)
	{
		app_get_next_line(f, r.width, &line, &nl, &utf8, &nb);

		p.x = r.x;
		width = r.width;

		/* if we have reached a newline line, don't justify it */
		if (line[nl] == '\n')	/* yes, line[nl] looks wrong */
			last = 1;	/* but it's correct */
		else
			last = 0;

		/* discard spaces at start of line */
		end = line+nl;
		for (s=line; (s < end) && (*s == ' '); s++)
			continue;
		start = s;

		/* discard spaces at the end of the line */
		for (s=line+nl-1; (s >= start) && (*s == ' '); s--)
			continue;
		end = s+1;

		/* count the number of scalable spaces */
		for (spaces=0, s=start; s < end; s++)
			if (*s == ' ')
				spaces++;

		if ((spaces == 0) || last || (nb == 0))
		{
			/* align this line normally, don't justify it */
			if (right_to_left)
				p.x += r.width;
			app_draw_utf8(g, p, line, nl);
			continue; /* go to next line */
		}

		/* otherwise, each word must be drawn individually */

		w = app_font_width(f, start, (int)(end-start));
		sw = (width-w) / spaces;
		xw = (width-w) % spaces;

		if (right_to_left) {
			p.x += r.width; /* draw from right-most point */
			while (start < end)
			{
				ns = app_find_word_size(start, (int)(end-start));
				w = app_font_width(f, start, ns);

				app_draw_utf8(g, p, start, ns);
				p.x -= w;
				if (*start == ' ')
					p.x -= sw;
				if (xw) {
					p.x--;
					xw--;
				}
				start += ns;
			}
		}
		else { /* draw left to right */
			while (start < end)
			{
				ns = app_find_word_size(start, (int)(end-start));
				w = app_font_width(f, start, ns);

				app_draw_utf8(g, p, start, ns);
				p.x += w;
				if (*start == ' ')
					p.x += sw;
				if (xw) {
					p.x++;
					xw--;
				}
				start += ns;
			}
		}
	}

	app_free(line);
	if (nb == 0)
		return NULL;
	return utf8;
}

/*
 *  The function which calls the above alignment-specific functions.
 */
char *app_draw_text(Graphics *g, Rect r, int align,
	const char *utf8, int nbytes)
{
	int h, lh, nlines, td;
	const char *remains;
	Region *old, *clip;

	if (g->font == NULL)
		app_set_default_font(g);
	if (g->font == NULL)
		return NULL; /* error */

	old = g->clip;
	if (old != NULL) {	//!!
	g->clip = NULL;
		clip = app_copy_region(old);
		app_move_region(clip, -g->offset.x, -g->offset.y);
		app_intersect_region_with_rect(clip, r, clip);
		app_set_clip_region(g, clip);
		app_del_region(clip);
	} else
		app_set_clip_rect(g, r);

	lh = app_font_height(g->font);

	if ((align & VALIGN_CENTER) == VALIGN_CENTER) {
		h = app_text_height(g->font, r.width, utf8, nbytes);
		if (h < r.height)
			r.y += (r.height-h)/2;
	}
	else if ((align & VALIGN_JUSTIFY) == VALIGN_JUSTIFY) {
		h = app_text_height(g->font, r.width, utf8, nbytes);
		if (h < r.height) {
			nlines = h / lh;
			if (nlines > 1)
				lh += ((r.height-h) / (nlines-1));
		}
	}
	else if ((align & VALIGN_BOTTOM) == VALIGN_BOTTOM) {
		h = app_text_height(g->font, r.width, utf8, nbytes);
		if (h < r.height)
			r.y += (r.height-h);
	}

	td = g->text_direction;
	if (align & LR_TB) {
		g->text_direction &= ~RL_TB;
		g->text_direction |= LR_TB;
	}
	else if (align & RL_TB) {
		g->text_direction &= ~LR_TB;
		g->text_direction |= RL_TB;
	}

	if ((align & ALIGN_CENTER) == ALIGN_CENTER)
		remains = app_draw_text_centered(g, r, lh, utf8, nbytes);
	else if ((align & ALIGN_JUSTIFY) == ALIGN_JUSTIFY)
		remains = app_draw_text_justified(g, r, lh, utf8, nbytes);
	else if ((align & ALIGN_RIGHT) == ALIGN_RIGHT)
		remains = app_draw_text_right(g, r, lh, utf8, nbytes);
	else
		remains = app_draw_text_left(g, r, lh, utf8, nbytes);

	app_del_region(g->clip);
	g->clip = old;
	g->text_direction = td;

	return (char *) remains; /* cast away const */
}
