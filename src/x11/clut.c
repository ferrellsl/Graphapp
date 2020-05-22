/*
 *  Colour Look-Up Table (CLUT) functions.
 *
 *  Platform: X-Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Updated.
 *  Version: 3.43  2003/04/21  Put get_direct_colour in window_find_colour.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

/*
 *  This file handles all of the X-Windows colour allocation.
 *  For colour-mapped displays it allocates each colour from
 *  the system palette, and if it cannot allocate any more
 *  it chooses the closest colour of those already in use.
 *
 *  For windows which request to have their own palette,
 *  the colours allocated are mapped to the nearest
 *  from the system palette, to reduce the 'flash' when the
 *  window's private palette is installed or uninstalled by
 *  the window manager.
 *
 *  For true-colour or direct-colour displays, a request for
 *  a colour is satisfied by a local calculation, rather than
 *  a round-trip to the X-server (which would just be doing the
 *  same calculation, only slower because of network time-lag).
 */

#include "appint.h"

/*
 *  Colour cell usage values.
 */

enum {
	UNUSED    = 0,	/* cells that appear to have never been used */
	AVAILABLE = 1,	/* cells that seem to have a colour in them */
	USED      = 2	/* cells that are being used by this program */
};

/*
 *  Colour matching exactness constants.
 */

enum {
	EXACT_MATCH = 2,
	CLOSE_MATCH = 1,
	POOR_MATCH  = 0
};

/*
 *  Given a bit-mask such as 00111000 and a RGB component value
 *  such as 10110101 (for instance), produce the corresponding
 *  32-bit direct pixval component (in this case it would be
 *  00111000 & (10110101>>2) == 00101000).
 *
 *  The algorithm is: start at the topmost bit of the mask and
 *  work our way down to the least significant bit, and every time
 *  we see a 1 bit in the mask, we grab a new bit from the source
 *  and place it in the result.
 */
static unsigned long app_form_pixval(unsigned long mask, unsigned short src)
{
	int src_shift, mask_shift;
	unsigned long src_bit, mask_bit;
	unsigned long result = 0UL;

	src_shift = 15;	/* start at bit 15 of the source */

	for (mask_shift = 31; mask_shift >= 0; mask_shift--) {
		mask_bit = (1 << mask_shift) & mask;
		if (mask_bit == 0)
			continue;
		src_bit = (1 << src_shift) & src;
		result |= ((src_bit >> src_shift) << mask_shift);
		src_shift--;
	}
	return result;
}

/*
 *  Obtain a direct colour value, using a Visual to calculate the
 *  pixel value without asking the X-server. If we cannot, just
 *  use XAllocColor.
 *  The given XColor's pixel field is filled in by this function.
 */

#ifdef DO_NOT_COMPILE

static void app_get_direct_colour(Display *disp, XColor *c)
{
	unsigned long pix;
	Colormap cmap;
	Visual *vis;

	vis = DefaultVisual(disp, DefaultScreen(disp));

	if (vis == NULL) {
		cmap = DefaultColormap(disp, DefaultScreen(disp));
		XAllocColor(disp, cmap, c);
	}
	else if ((vis->red_mask   == 0x00FF0000UL) &&
		 (vis->green_mask == 0x0000FF00UL) &&
		 (vis->blue_mask  == 0x000000FFUL))
	{
		pix  = c->red; pix <<= 8; pix &= 0x00FF0000UL;
		pix |= (c->green       & 0x0000FF00UL);
		pix |= ((c->blue >> 8) & 0x000000FFUL);
		c->pixel = pix;
	}
	else if ((vis->red_mask   == 0x0000F800UL) &&
		 (vis->green_mask == 0x000007E0UL) &&
		 (vis->blue_mask  == 0x0000001FUL))
	{
		pix  = (c->red           & 0x0000F800UL);
		pix |= ((c->green >>  5) & 0x000007E0UL);
		pix |= ((c->blue  >> 11) & 0x0000001FUL);
		c->pixel = pix;
	}
	else if ((vis->red_mask   == 0x00007C00UL) &&
		 (vis->green_mask == 0x000003E0UL) &&
		 (vis->blue_mask  == 0x0000001FUL))
	{
		pix  = ((c->red   >>  1) & 0x00007C00UL);
		pix |= ((c->green >>  6) & 0x000003E0UL);
		pix |= ((c->blue  >> 11) & 0x0000001FUL);
		c->pixel = pix;
	}
	else {
		pix = 0UL;
		pix |= app_form_pixval(vis->red_mask, c->red);
		pix |= app_form_pixval(vis->green_mask, c->green);
		pix |= app_form_pixval(vis->blue_mask, c->blue);
		c->pixel = pix;
	}
}

#endif

/*
 *  Allocate a new CLUT structure.
 */

static CLUT *
app_alloc_clut(Display *disp, Colormap cmap, int size, int private)
{
	CLUT *clut;
	XColor *table;
	char *in_use;
	int i;

	clut = app_zero_alloc(sizeof(struct CLUT));
	table = app_zero_alloc(size * sizeof(XColor));
	in_use = app_zero_alloc(size);

	if ((! clut) || (! table) || (! in_use)) {
		app_free(clut);
		app_free(table);
		app_free(in_use);
		return NULL;
	}

	clut->refcount = 1;
	clut->private = private;
	clut->size = size;
	clut->full = private;
	clut->disp = disp;
	clut->cmap = cmap;
	clut->table = table;
	clut->in_use = in_use;

	for (i = 0; i < size; i++) {
		in_use[i] = UNUSED;
		table[i].pixel = i;
		table[i].red = 0;
		table[i].green = 0;
		table[i].blue = 0;
		table[i].flags = DoRed | DoGreen | DoBlue;
	}

	return clut;
}

/*
 *  Delete a CLUT from memory if it is time.
 */
void app_del_clut(CLUT *clut)
{
	clut->refcount--;
	if (clut->refcount > 0)
		return;

	if (clut->private)
		XFreeColormap(clut->disp, clut->cmap);

	app_free(clut->in_use);
	app_free(clut->table);
	app_free(clut);
}

/*
 *  Create a new Palette using a CLUT's colours as a guide.
 */
Palette * app_new_palette_from_clut(CLUT *clut)
{
	Palette *pal;
	int i;

	if (clut == NULL)
		return NULL;

	pal = app_zero_alloc(sizeof(struct Palette));
	pal->size = clut->size;

	/* copy elements over to the new palette structure */

	pal->element = app_zero_alloc(sizeof(Colour) * pal->size);
	if (clut->table)
		for (i=0; i < pal->size; i++) {
			pal->element[i].red   = clut->table[i].red/256;
			pal->element[i].green = clut->table[i].green/256;
			pal->element[i].blue  = clut->table[i].blue/256;
		}

	return pal;
}

/*
 *  Fetch the shared system colours and store them in the CLUT.
 */
static void app_fetch_system_colours(CLUT *clut)
{
	int i;

	for (i = 0; i < clut->size; i++)
		clut->table[i].pixel = i;

	XQueryColors(clut->disp, clut->cmap, clut->table, clut->size);
	clut->full = 0;
}

/*
 *  Load and return the shared system CLUT.
 */
static CLUT * app_load_system_clut(Display *disp)
{
	int i, depth;
	Colormap cmap;
	CLUT *clut;

	if (app_is_true_colour_display(disp))
		return NULL;

	depth = DefaultDepth(disp, DefaultScreen(disp));
	cmap = DefaultColormap(disp, DefaultScreen(disp));
	if (cmap == None)
		return NULL;

	clut = app_alloc_clut(disp, cmap, 1<<depth, 0);
	if (! clut)
		return NULL;

	app_fetch_system_colours(clut);
	for (i = 0; i < clut->size; i++)
		clut->in_use[i] = UNUSED;

	return clut;
}

/*
 *  Return the index value of the best matching colour from a table.
 *  If the in_use array is given, the function will try for a
 *  cell which is not USED, and returns -1 if all the cells are USED.
 */
static int app_best_colour(XColor *xc, XColor *table, char *in_use,
				int size, int *match)
{
	int i, best;
	unsigned long min_dist, distance;
	long dr, dg, db;
	XColor *tbl;

	best = -1;
	min_dist = 0;
	for (i = 0; i < size; i++)
	{
		tbl = &table[i];

		/* RGB in decimal:
		dr = ((((long)xc->red   - (long)tbl->red)   ) *30) /100;
		dg = ((((long)xc->green - (long)tbl->green) ) *59) /100;
		db = ((((long)xc->blue  - (long)tbl->blue)  ) *11) /100;
		*/

		/* RGB match in hex: */
		dr = ((((long)xc->red   - (long)tbl->red)  /256)*77) /256;
		dg = ((((long)xc->green - (long)tbl->green)/256)*151)/256;
		db = ((((long)xc->blue  - (long)tbl->blue) /256)*28) /256;

		distance = dr * dr + dg * dg + db * db;

		if (distance < 1) { /* exact match */
			best = i;
			min_dist = distance;
			break;
		}

		/* if the in_use array is given, and this cell is taken */
		if (in_use && (in_use[i] == USED))
			continue;	/* skip this cell */

		if ((best < 0) || (distance < min_dist)) {
			best = i;
			min_dist = distance;
		}
	}
	if (min_dist < 1)
		*match = EXACT_MATCH;
	else if (min_dist < 40)
		*match = CLOSE_MATCH;
	else
		*match = POOR_MATCH;
	return best;
}

/*
 *  For sorting the colours into an 'importance' order,
 *  brightest colours first:
 */
static int app_cmp_intensity(const void *a, const void *b)
{
	Colour c1, c2;
	long dr, dg, db;
	unsigned long dist1, dist2;

	c1 = *((Colour *) a);
	c2 = *((Colour *) b);
/*
	dr = (((long)c1.red)   * 54)  /256;
	dg = (((long)c1.green) * 184) /256;
	db = (((long)c1.blue)  * 18)  /256;
*/
	dr = (((long)c1.red)   * 77)  /256;
	dg = (((long)c1.green) * 151) /256;
	db = (((long)c1.blue)  * 28)  /256;
	dist1 = dr * dr + dg * dg + db * db;

	dr = (((long)c2.red)   * 77)  /256;
	dg = (((long)c2.green) * 151) /256;
	db = (((long)c2.blue)  * 28)  /256;
	dist2 = dr * dr + dg * dg + db * db;

	if (dist1 < dist2)
		return +1;
	else if (dist1 > dist2)
		return -1;
	else
		return 0;
}

/*
 *  Store a given set of colours into a CLUT.
 *  To reduce the 'flash' that occurs when window focus changes
 *  into a window with a private palette, we map the required
 *  colours onto the nearest colour from the shared system palette.
 */
static void app_store_colours(CLUT *clut, int size, Colour *colours)
{
	CLUT * sys;
	int i, best, close;
	Colour c;
	Colour *sorted;
	XColor xcol;
	unsigned long black, white;

	/* Get a hold of the current system palette for this display. */
	sys = app_load_system_clut(clut->disp);
	if (! sys) {
		/* Hmm, couldn't get the shared system palette... */
		return;
	}

	/* Fix black and white in place. */
	black = BlackPixel(clut->disp, DefaultScreen(clut->disp));
	white = WhitePixel(clut->disp, DefaultScreen(clut->disp));

	if (black < clut->size)
		clut->in_use[black] = USED;
	if (white < clut->size)
		clut->in_use[white] = USED;

	/* Sort the required colours by intensity, so that the */
	/* most important (brightest) colours are first */

	sorted = app_alloc(sizeof(Colour) * size);
	if (! sorted)
		sorted = colours;
	else {
		for (i=0; i < size; i++)
			sorted[i] = colours[i];
		qsort(sorted, size, sizeof(Colour), app_cmp_intensity);
	}

	/* Find good matches for the required colours */
	/* from the system palette */

	for (i=0; i < size; i++) {
		c = sorted[i];
		xcol.red   = ((int) c.red) * 257;
		xcol.green = ((int) c.green) * 257;
		xcol.blue  = ((int) c.blue) * 257;
		xcol.flags = DoRed | DoGreen | DoBlue;

		best = app_best_colour(&xcol, sys->table, clut->in_use,
				clut->size, &close);
		if (best < 0)
			continue; /* ignore this strange error */

		/* save the required colour into the table */
		xcol.pixel = best;
		clut->table[best] = xcol;
		clut->in_use[best] = USED;
	}

	/* Store the required colours to the right spots on the server */
	XStoreColors(clut->disp, clut->cmap, clut->table, clut->size);

	/* Mark unused, non-black colours as available */
	for (i=0; i < clut->size; i++)
	{
		if ((clut->in_use[i] == UNUSED)
		    && ((clut->table[i].red   != 0)
		     || (clut->table[i].green != 0)
		     || (clut->table[i].blue  != 0)))
			clut->in_use[i] = AVAILABLE;
	}

	/* This leaves unused black cells as the preferred cells to use */

	if (sorted != colours)
		app_free(sorted);

	app_del_clut(sys);
}

/*
 *  Create and return a new CLUT for a window.
 */
CLUT * app_new_clut(Display *disp, XID xid, int size, Colour *colours)
{
	int depth;
	Visual *vis;
	Colormap cmap;
	CLUT *clut;
	int max;

	if (app_is_true_colour_display(disp))
		return NULL;

	vis = DefaultVisual(disp, DefaultScreen(disp));
	if ((vis == NULL) || (xid == None))
		return NULL;

	depth = DefaultDepth(disp, DefaultScreen(disp));
	cmap = XCreateColormap(disp, xid, vis, AllocAll);
	if (cmap == None)
		return NULL;

	max = 1<<depth;
	if (max > size)
		max = size;
	clut = app_alloc_clut(disp, cmap, max, 1);
	if (! clut)
		return NULL;

	app_store_colours(clut, size, colours);
	XSetWindowColormap(disp, xid, cmap);
	return clut;
}

/*
 *  Return the index value of the last UNUSED colour in the table.
 *  Returns -1 if there is no such colour cell.
 */
static int app_unused_colour(char *in_use, int size)
{
	int i;

	for (i = size - 1; i >= 0; i--) {
		if (in_use[i] == UNUSED)
			return i;
	}
	return -1;
}

/*
 *  If all of the colours in the table are USED, return 1.
 *  Otherwise return 0.
 */
static int app_clut_is_full(char *in_use, int size)
{
	int i;

	for (i = 0; i < size; i++) {
		if (in_use[i] != USED)
			return 0;
	}
	return 1;
}

/*
 *  Grab a copy of a colour from a table, and store it.
 */
static void app_copy_colour(XColor *dest, int i, XColor *table)
{
	dest->pixel = (unsigned long) i;
	dest->red   = table[i].red;
	dest->green = table[i].green;
	dest->blue  = table[i].blue;
	dest->flags = DoRed | DoGreen | DoBlue;
}

/*
 *  Get a private colour cell, and modify it to be the requested colour.
 */
static void app_get_private_colour(CLUT *clut, XColor *xc)
{
	int best, nextbest, close;

	/* Find the best matching colour from the entire table. */
	best = app_best_colour(xc, clut->table, NULL, clut->size, &close);

	/* Return the result if we found an exact match */
	if (close == EXACT_MATCH)
	{
		app_copy_colour(xc, best, clut->table);
		clut->in_use[best] = USED;
		return;
	}

	/* Modify a close match in an unused cell. */
	if ((close == CLOSE_MATCH) && (clut->in_use[best] < USED))
	{
		xc->pixel = best;
		xc->flags = DoRed | DoGreen | DoBlue;
		XStoreColor(clut->disp, clut->cmap, xc);
		clut->table[best] = *xc;
		clut->in_use[best] = USED;
		return;
	}

	/* Poor match, so find an unused cell. */
	nextbest = app_unused_colour(clut->in_use, clut->size);
	if (nextbest != -1) {
		xc->pixel = nextbest;
		xc->flags = DoRed | DoGreen | DoBlue;
		XStoreColor(clut->disp, clut->cmap, xc);
		clut->table[nextbest] = *xc;
		clut->in_use[nextbest] = USED;
		return;
	}

	/* Find the next best cell which is available. */
	nextbest = app_best_colour(xc, clut->table, clut->in_use,
				clut->size, &close);
	if (nextbest != -1) {
		xc->pixel = nextbest;
		xc->flags = DoRed | DoGreen | DoBlue;
		XStoreColor(clut->disp, clut->cmap, xc);
		clut->table[nextbest] = *xc;
		clut->in_use[nextbest] = USED;
		return;
	}

	/* Return the best match from the colour table. */
	app_copy_colour(xc, best, clut->table);
}

/*
 *  Find a colour cell in a shared colourmap.
 */
static void app_get_shared_colour(CLUT *clut, XColor *xc)
{
	int best, close;

	/* Find the best matching colour from the table we have. */
	best = app_best_colour(xc, clut->table, NULL, clut->size, &close);

	/* Reasonable match in a cell this application is using? */
	if ((close != POOR_MATCH) && (clut->in_use[best] == USED)) {
		app_copy_colour(xc, best, clut->table);
		return;
	}

	/* Try XAllocColor and see if that works. */
	if (! clut->full) {
		if (XAllocColor(clut->disp, clut->cmap, xc)) {
			clut->in_use[xc->pixel] = USED;
			return;
		}
		else {
			if (app_clut_is_full(clut->in_use, clut->size))
				clut->full = 1;
		}
	}

	/* Alloc failed, just use the best match. */

	/* Make a copy of the best colour from the table. */
	app_copy_colour(xc, best, clut->table);

	/* Try to use that closest matching colour. This operation
	 * fails if someone else has just changed the cell we wanted.
	 * We just use the best colour in that case. */
	if (XAllocColor(clut->disp, clut->cmap, xc)) {
		clut->in_use[xc->pixel] = USED;
		return;
	}
	app_copy_colour(xc, best, clut->table);
}

/*
 *  Find/allocate the closest matching colour for a window,
 *  assuming it is using an indexed colour system (is paletted).
 */
static void app_get_window_colour(Window *win, Display *disp, XColor *xc)
{
	App *app;
	CLUT *clut;

	clut = win_extra(win)->clut; /* try window's private palette */
	if (! clut) {
		app = win->app;
		clut = app_extra(app)->clut; /* try system shared palette */
		if (! clut) {
			app_extra(app)->clut = app_load_system_clut(disp);
			clut = app_extra(app)->clut;
		}
		if (! clut) {
			XAllocColor(disp, DefaultColormap(disp,
				DefaultScreen(disp)), xc);
			return;
		}
		else if ((clut->full) && (! app_extra(app)->pal)) {
			app_extra(app)->pal = app_new_palette_from_clut(app_extra(app)->clut);
		}
	}

	if (clut->private)
		app_get_private_colour(clut, xc);
	else
		app_get_shared_colour(clut, xc);
}

/*
 *  Find the best colour pixel value for a given window.
 */
long app_window_find_colour(Graphics *g, Window *win, Colour c)
{
	XColor xcol;
	Display *disp;
	unsigned long pix;
	Colormap cmap;
	Visual *vis;

	disp = app_extra(win->app)->display;

	xcol.red   = ((int) c.red) * 257;	/* scale of 0 to 65535 */
	xcol.green = ((int) c.green) * 257;	/* scale of 0 to 65535 */
	xcol.blue  = ((int) c.blue) * 257;	/* scale of 0 to 65535 */

	/* if the window uses indexed colour, handle it elsewhere */

	if (win_extra(win)->is_paletted) {
		app_get_window_colour(win, disp, &xcol);
		return xcol.pixel;
	}

	/* otherwise it is a direct colour window, */
	/* so perform a local calculation if possible */

	vis = DefaultVisual(disp, DefaultScreen(disp));

	if (vis == NULL) {
		cmap = DefaultColormap(disp, DefaultScreen(disp));
		XAllocColor(disp, cmap, &xcol);
	}
	else if ((vis->red_mask   == 0x00FF0000UL) &&
		 (vis->green_mask == 0x0000FF00UL) &&
		 (vis->blue_mask  == 0x000000FFUL))
	{
		pix  = xcol.red; pix <<= 8; pix &= 0x00FF0000UL;
		pix |= (xcol.green       & 0x0000FF00UL);
		pix |= ((xcol.blue >> 8) & 0x000000FFUL);
		xcol.pixel = pix;
	}
	else if ((vis->red_mask   == 0x0000F800UL) &&
		 (vis->green_mask == 0x000007E0UL) &&
		 (vis->blue_mask  == 0x0000001FUL))
	{
		pix  = (xcol.red           & 0x0000F800UL);
		pix |= ((xcol.green >>  5) & 0x000007E0UL);
		pix |= ((xcol.blue  >> 11) & 0x0000001FUL);
		xcol.pixel = pix;
	}
	else if ((vis->red_mask   == 0x00007C00UL) &&
		 (vis->green_mask == 0x000003E0UL) &&
		 (vis->blue_mask  == 0x0000001FUL))
	{
		pix  = ((xcol.red   >>  1) & 0x00007C00UL);
		pix |= ((xcol.green >>  6) & 0x000003E0UL);
		pix |= ((xcol.blue  >> 11) & 0x0000001FUL);
		xcol.pixel = pix;
	}
	else {
		pix = 0UL;
		pix |= app_form_pixval(vis->red_mask, xcol.red);
		pix |= app_form_pixval(vis->green_mask, xcol.green);
		pix |= app_form_pixval(vis->blue_mask, xcol.blue);
		xcol.pixel = pix;
	}

	return xcol.pixel;
}
