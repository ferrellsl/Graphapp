/*
 *  Region management utilities.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/07/16  Updated.
 *  Version: 3.56  2005/08/09  Silenced a size_t conversion warning.
 */

/* Copyright (c) the X Consortium and L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

/*
 *  A Region is a collection of non-intersecting rectangles, used
 *  to allow efficient drawing. Operations such as producing the
 *  intersection, union, or subtraction of regions and rectangles
 *  are allowed. The drawing operations, such as filling a rectangle
 *  or drawing text, are aware of regions and avoid drawing outside
 *  the currently selected clipping region. This not only speeds up
 *  drawing, but allows accurate clipping in a portable manner.
 *
 *  Most of these functions are modified from the X Consortium's
 *  Region code. All credit to them, but their code, as mine,
 *  has no warranty, and all faults are my own.
 *
 *  The main modification I've made to the Xlib Region code is
 *  to make a Region store an array of distinct Rect objects
 *  instead of Box objects (which is a bounding box x1,y1,x2,y2).
 *  This means the code does a few more additions and subtractions
 *  than is strictly necessary, but it has the advantage that
 *  the Region object can be made public to the programmer without
 *  introducing a new rectangle concept such as a Box type. It also
 *  makes drawing code which uses Regions very simple: just find
 *  the intersection of the rectangle you're drawing to, with each
 *  rectangle in the visible region, in order, and draw to that
 *  rectangular intersection.
 */

/*
 * A Region is simply an area, implemented as a "y-x-banded" array
 * of rectangles; each Region is made up of a certain number of
 * rectangles sorted by y coordinate first, and then by x coordinate.
 *
 * Furthermore, the rectangles are banded such that all rectangles with
 * a certain upper-left y coordinate (y1) will each have the same
 * lower-right y coordinate (y2) as each other, and vice versa.
 * If a rectangle has scanlines in a band, it will span the entire
 * vertical distance of the band. This means that some areas that could
 * be merged into a taller rectangle will be represented as several
 * shorter rectangles to account for shorter rectangles to its left or
 * right but within its "vertical scope".
 *
 * An added constraint on the rectangles is that they must cover as much
 * horizontal area as possible. Thus, no two rectangles in a band are
 * allowed to touch horizontally.
 *
 * Whenever possible, bands will be merged together to cover a greater
 * vertical distance (and thus reduce the number of rectangles). Two
 * bands can be merged only if the bottom of one touches the top of the
 * other and they have rectangles in the same places (of the same width,
 * of course). This maintains the y-x-banding that's so nice to have...
 */

#include "apputils.h"

#ifdef REGION_DEBUG
#define assert(expr) {if (!(expr)) fprintf(stderr,\
"Assertion failed file %s, line %d: expr\n", __FILE__, __LINE__); }
#else
#define assert(expr)
#endif

#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))


/*
 *  1 if two Rects overlap.
 *  0 if they do not overlap.
 */
#define RXR(r1,r2) \
	(((r1).x < (r2).x+(r2).width) && \
	 ((r2).x < (r1).x+(r1).width) && \
	 ((r1).y < (r2).y+(r2).height) && \
	 ((r2).y < (r1).y+(r1).height))

/*
 *  1 if the point (x,y) lies within the Rect r.
 *  0 if not.
 */
#define PTINRECT(r,px,py) \
	( ((px) >= (r).x) && \
	  ((py) >= (r).y) && \
	  ((px) < (r).x+(r).width) && \
	  ((py) < (r).y+(r).height) )

/*
 *  Check to see if there is enough memory in the present region.
 *  Reallocate the Region's rects list if necessary.
 */
#define MEMCHECK(rgn,r,rects) {\
		if ((rgn)->num_rects >= ((rgn)->size - 1)){\
			(rects) = app_realloc((rects), \
				(2 * sizeof(Rect) * ((rgn)->size)));\
			if ((rects) == NULL)\
				return 0;\
			(rgn)->size *= 2;\
			(r) = &(rects)[(rgn)->num_rects];\
		}\
	}

/*
 *  Create a new empty region.
 */
Region *app_new_region(void)
{
	Region *rgn;

	if ((rgn = app_alloc(sizeof(Region))) == NULL)
		return NULL;
	if ((rgn->rects = app_alloc(sizeof(Rect))) == NULL) {
		app_free(rgn);
		return NULL;
	}
	rgn->num_rects = 0;
	rgn->extents.x = 0;
	rgn->extents.y = 0;
	rgn->extents.width = 0;
	rgn->extents.height = 0;
	rgn->size = 1;
	return rgn;
}

/*
 *  Delete the region from memory.
 */
void app_del_region(Region *rgn)
{
	app_free(rgn->rects);
	app_free(rgn);
}

/*
 *  Determine the bounding rectangle of the region.
 */
Rect app_region_extents(Region *rgn)
{
	return rgn->extents;
}

/*
 *  Find the union of a region with a rectangle, as a region.
 */
Region * app_new_rect_region(Rect r)
{
	Region *rgn;

	rgn = app_new_region();
	if (rgn == NULL)
		return NULL;
	rgn->extents = r;
	rgn->num_rects = 1;
	rgn->rects[0] = r;
	return rgn;
}

/*
 *  Move the region by adding the point (x,y) to all rectangles in
 *  the region.
 */
void app_move_region(Region *rgn, int x, int y)
{
	int n;
	Rect *box;

	box = rgn->rects;
	n = rgn->num_rects;

	while (n--)
	{
		box->x += x;
		box->y += y;
		box++;
	}
	rgn->extents.x += x;
	rgn->extents.y += y;
}

/*======================================================================
 *	Generic Region Operator
 *====================================================================*/

/*
 *-----------------------------------------------------------------------
 * app_set_extents
 *	Change the extents of a region to what it should be.
 *	Called by app_subtract_region and app_intersect_region
 *	because they can't figure it out along the way or do so
 *	easily, as app_union_region can.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The region's 'extents' structure is changed.
 *
 *-----------------------------------------------------------------------
 */
static void app_set_extents(Region *rgn)
{
	Rect *box;
	Rect *end;
	Rect *extents;

	if (rgn->num_rects == 0)
	{
		rgn->extents.x = 0;
		rgn->extents.y = 0;
		rgn->extents.width = 0;
		rgn->extents.height = 0;
		return;
	}

	extents = &rgn->extents;
	box = rgn->rects;
	end = &box[rgn->num_rects - 1];

	/*
	 * Since box is the first rectangle in the region, it must have
	 * the smallest y and since boxend is the last rectangle in
	 * the region, it must have the largest y+height, due to banding.
	 * Initialize x and width from box and boxend, respectively,
	 * as good initial values.
	 */
	extents->x = box->x;
	extents->y = box->y;
	extents->width = end->x + end->width - extents->x;
	extents->height = end->y + end->height - extents->y;

	assert(extents->height > 0);
	while (box <= end)
	{
		if (box->x < extents->x) {
			extents->width += extents->x - box->x;
			extents->x = box->x;
		}
		if (box->x + box->width > extents->x + extents->width)
			extents->width = box->x + box->width - extents->x;
		box++;
	}
	assert(extents->width > 0);
}

/*
 *-----------------------------------------------------------------------
 * app_region_copy
 *	Make a copy of a region and place it in the dest region.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The dest region structure is changed.
 *
 *-----------------------------------------------------------------------
 */
static int app_region_copy(Region *dest, Region *rgn)
{
	Rect *boxes;

	if (dest == rgn) /*  don't want to copy to itself */
		return 1;

	if (dest->size < rgn->num_rects)
	{
		if (dest->rects)
		{
			boxes = app_realloc(dest->rects,
				rgn->num_rects * sizeof(Rect));
			if (boxes == NULL)
				return 0;
			dest->rects = boxes;
		}
		dest->size = rgn->num_rects;
	}
	dest->num_rects = rgn->num_rects;
	dest->extents.x = rgn->extents.x;
	dest->extents.y = rgn->extents.y;
	dest->extents.width = rgn->extents.width;
	dest->extents.height = rgn->extents.height;
	memcpy(dest->rects, rgn->rects, rgn->num_rects * sizeof(Rect));
	return 1;
}

Region *app_copy_region(Region *rgn)
{
	Region *dest;

	dest = app_new_region();
	if (dest == NULL)
		return NULL;
	if (! app_region_copy(dest, rgn)) {
		app_del_region(dest);
		return NULL;
	}
	return dest;
}

/*
 *-----------------------------------------------------------------------
 * app_coalesce --
 *	Attempt to merge the boxes in the current band with those in the
 *	previous one. Used only by app_region_op.
 *
 * Results:
 *	The new index for the previous band.
 *
 * Side Effects:
 *	If coalescing takes place:
 *		- rectangles in the previous band will have their y2
 *		  fields altered.
 *		- rgn->num_rects will be decreased.
 *
 *-----------------------------------------------------------------------
 */

static int app_coalesce (Region *rgn, int prevstart, int curstart)
	/* prevstart is Index of start of previous band */
	/* curstart is Index of start of current band */
{
	Rect *prevbox;	/* Current box in previous band */
	Rect *curbox;	/* Current box in current band */
	Rect *end;	/* End of region */
	int curnum;	/* Number of rectangles in current band */
	int prevnum;	/* Number of rectangles in previous band */
	int band_y;	/* Y coordinate for current band */

	end = &rgn->rects[rgn->num_rects];

	prevbox = &rgn->rects[prevstart];
	prevnum = curstart - prevstart;

	/*
	 * Figure out how many rectangles are in the current band.
	 * Have to do this because multiple bands could have been
	 * added in app_region_op at the end when one region has
	 * been exhausted.
	 */
	curbox = &rgn->rects[curstart];
	band_y = curbox->y;
	for (curnum=0; (curbox != end) && (curbox->y == band_y); curnum++)
	{
		curbox++;
	}

	if (curbox != end)
	{
		/*
		 * If more than one band was added, we have to find the
		 * start of the last band added so the next coalescing
		 * job can start at the right place... (given when
		 * multiple bands are added, this may be pointless --
		 * see above).
		 */
		end--;
		while (end[-1].y == end->y)
		{
			end--;
		}
		curstart = (int) (end - rgn->rects);
		end = rgn->rects + rgn->num_rects;
	}
	
	if ((curnum == prevnum) && (curnum != 0)) {
		curbox -= curnum;
		/*
		 * The bands may only be coalesced if the bottom of the
		 * previous matches the top scanline of the current.
		 */
		if (prevbox->y + prevbox->height == curbox->y)
		{
			/*
			 * Make sure the bands have boxes in the same
			 * places. This assumes that boxes have been
			 * added in such a way that they cover the most
			 * area possible. I.e. two boxes in a band must
			 * have some horizontal space between them.
			 */
			do
			{
				if ((prevbox->x != curbox->x) ||
				    (prevbox->width != curbox->width))
				{
					/*
					 * The bands don't line up so
					 * they can't be coalesced.
					 */
					return curstart;
				}
				prevbox++;
				curbox++;
				prevnum -= 1;
			} while (prevnum != 0);

			rgn->num_rects -= curnum;
			curbox -= curnum;
			prevbox -= curnum;

			/*
			 * The bands may be merged, so set the bottom y
			 * of each box in the previous band to that of
			 * the corresponding box in the current band.
			 */
			do
			{
				prevbox->height = curbox->y +
					curbox->height - prevbox->y;
				prevbox++;
				curbox++;
				curnum -= 1;
			} while (curnum != 0);

			/*
			 * If only one band was added to the region,
			 * we have to backup curstart to the start of
			 * the previous band.
			 *
			 * If more than one band was added to the region,
			 * copy the other bands down. The assumption
			 * here is that the other bands came from the
			 * same region as the current one and no further
			 * coalescing can be done on them since it's all
			 * been done already... curstart is already in
			 * the right place.
			 */
			if (curbox == end)
			{
				curstart = prevstart;
			}
			else
			{
				do
				{
					*prevbox++ = *curbox++;
				} while (curbox != end);
			}
		}
	}
	return curstart;
}

/*
 *-----------------------------------------------------------------------
 * app_region_op --
 *	Apply an operation to two regions. Called by app_union_region,
 *	app_intersect_region, app_subtract_region, etc.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The new region is overwritten.
 *
 * Notes:
 *	The idea behind this function is to view the two regions as sets.
 *	Together they cover a rectangle of area that this function divides
 *	into horizontal bands where points are covered only by one region
 *	or by both. For the first case, the non_overlap_func is called
 *	with each of the band and the band's upper and lower extents.
 *	For the second, the overlap_func is called to process the entire
 *	band. It is responsible for clipping the rectangles in the band,
 *	though this function provides the boundaries.
 *	At the end of each band, the new region is coalesced, if possible,
 *	to reduce the number of rectangles in the region.
 *
 *-----------------------------------------------------------------------
 */
static int app_region_op (Region *dest, Region *reg1, Region *reg2,
	int (*overlap_func)(Region *,Rect *,Rect *,Rect *,Rect *,int,int),
	int (*non_overlap_1_func)(Region *,Rect *,Rect *,int,int),
	int (*non_overlap_2_func)(Region *,Rect *,Rect *,int,int))
{
	/* overlap_func is the function to call for over-lapping bands */
	/* non_overlap_1_func is the function to call for */
	/* non-overlapping bands in region 1 */
	/* non_overlap_2_func is the function to call for */
	/* non-overlapping bands in region 2 */

	Rect *r1;	/* Pointer into first region */
	Rect *r2;	/* Pointer into 2d region */
	Rect *r1end;	/* End of 1st region */
	Rect *r2end;	/* End of 2d region */
	int ybot;	/* Bottom of intersection */
	int ytop;	/* Top of intersection */
	Rect *oldrects;	/* Old rects for dest */
	int prevband;	/* index of start of previous band in dest */
	int curband;	/* Index of start of current band in dest */
	Rect *r1bandend;/* End of current band in r1 */
	Rect *r2bandend;/* End of current band in r2 */
	int top;	/* Top of non-overlapping band */
	int bot;	/* Bottom of non-overlapping band */

	/*
	 * Initialization:
	 * set r1, r2, r1end and r2end appropriately, preserve the
	 * important parts of the destination region until the end
	 * in case it's one of the two source regions, then mark the
	 * "new" region empty, allocating another array of rectangles
	 * for it to use.
	 */
	r1 = reg1->rects;
	r2 = reg2->rects;
	r1end = r1 + reg1->num_rects;
	r2end = r2 + reg2->num_rects;

	oldrects = dest->rects;

	dest->num_rects = 0;

	/*
	 * Allocate a reasonable number of rectangles for the new region.
	 * The idea is to allocate enough so the individual functions
	 * don't need to reallocate and copy the array, which is time
	 * consuming, yet we don't have to worry about using too much
	 * memory. I hope to be able to remove the app_realloc() at the
	 * end of this function eventually.
	 */
	dest->size = MAX(reg1->num_rects,reg2->num_rects) * 2;

	if ((dest->rects = app_alloc((sizeof(Rect) * dest->size))) == NULL) {
		dest->size = 0;
		return 0;
	}

	/*
	 * Initialize ybot and ytop.
	 * In the upcoming loop, ybot and ytop serve different functions
	 * depending on whether the band being handled is an overlapping
	 * or non-overlapping band.
	 * In the case of a non-overlapping band (only one of the regions
	 * has points in the band), ybot is the bottom of the most recent
	 * intersection and thus clips the top of the rectangles in that
	 * band.
	 * ytop is the top of the next intersection between the two
	 * regions and serves to clip the bottom of the rectangles in the
	 * current band.
	 * For an overlapping band (where the two regions intersect),
	 * ytop clips the top of the rectangles of both regions and ybot
	 * clips the bottoms.
	 */
	if (reg1->extents.y < reg2->extents.y)
		ybot = reg1->extents.y;
	else
		ybot = reg2->extents.y;

	/*
	 * prevband serves to mark the start of the previous band so
	 * rectangles can be coalesced into larger rectangles. qv.
	 * app_coalesce, above.
	 * In the beginning, there is no previous band, so prevband ==
	 * curband (curband is set later on, of course, but the first
	 * band will always start at index 0). prevband and curband must
	 * be indices because of the possible expansion, and resultant
	 * moving, of the new region's array of rectangles.
	 */
	prevband = 0;

	do
	{
		curband = dest->num_rects;

		/*
		 * This algorithm proceeds one source-band (as opposed
		 * to a destination band, which is determined by where
		 * the two regions intersect) at a time. r1bandend and
		 * r2bandend serve to mark the rectangle after the last
		 * one in the current band for their respective regions.
		 */
		r1bandend = r1;
		while ((r1bandend != r1end) && (r1bandend->y == r1->y))
		{
			r1bandend++;
		}

		r2bandend = r2;
		while ((r2bandend != r2end) && (r2bandend->y == r2->y))
		{
			r2bandend++;
		}

		/*
		 * First handle the band that doesn't intersect, if any.
		 *
		 * Note that attention is restricted to one band in the
		 * non-intersecting region at once, so if a region has n
		 * bands between the current position and the next place
		 * it overlaps the other, this entire loop will be passed
		 * through n times.
		 */
		if (r1->y < r2->y)
		{
			top = MAX(r1->y,ybot);
			bot = MIN(r1->y+r1->height,r2->y);

			if ((top != bot) && (non_overlap_1_func != NULL))
			{
				if (non_overlap_1_func(dest,
					r1, r1bandend, top, bot) == 0)
					return 0;
			}

			ytop = r2->y;
		}
		else if (r2->y < r1->y)
		{
			top = MAX(r2->y,ybot);
			bot = MIN(r2->y+r2->height,r1->y);

			if ((top != bot) && (non_overlap_2_func != NULL))
			{
				if (non_overlap_2_func(dest,
					r2, r2bandend, top, bot) == 0)
					return 0;
			}

			ytop = r1->y;
		}
		else
		{
			ytop = r1->y;
		}

		/*
		 * If any rectangles got added to the region, try to
		 * coalesce them with rectangles from the previous band.
		 * Note we could just do this test in app_coalesce, but
		 * some machines incur a not inconsiderable cost for
		 * function calls, so...
		 */
		if (dest->num_rects != curband)
		{
			prevband = app_coalesce (dest, prevband, curband);
		}

		/*
		 * Now see if we've hit an intersecting band.
		 * The two bands only intersect if ybot > ytop
		 */
		ybot = MIN(r1->y+r1->height, r2->y+r2->height);
		curband = dest->num_rects;
		if (ybot > ytop)
		{
			if (overlap_func(dest, r1, r1bandend, r2,
				r2bandend, ytop, ybot) == 0)
				return 0;
		}
	
		if (dest->num_rects != curband)
		{
			prevband = app_coalesce (dest, prevband, curband);
		}

		/*
		 * If we've finished with a band (y+height == ybot)
		 * we skip forward in the region to the next band.
		 */
		if (r1->y+r1->height == ybot)
		{
			r1 = r1bandend;
		}
		if (r2->y+r2->height == ybot)
		{
			r2 = r2bandend;
		}
	} while ((r1 != r1end) && (r2 != r2end));

	/*
	 * Deal with whichever region still has rectangles left.
	 */
	curband = dest->num_rects;
	if (r1 != r1end)
	{
		if (non_overlap_1_func != NULL)
		{
			do
			{
				r1bandend = r1;
				while ((r1bandend < r1end) &&
					(r1bandend->y == r1->y))
				{
					r1bandend++;
				}
				if (non_overlap_1_func(dest,
					r1, r1bandend, MAX(r1->y,ybot),
					r1->y+r1->height) == 0)
					return 0;
				r1 = r1bandend;
			} while (r1 != r1end);
		}
	}
	else if ((r2 != r2end) && (non_overlap_2_func != NULL))
	{
		do
		{
			r2bandend = r2;
			while ((r2bandend < r2end) &&
				(r2bandend->y == r2->y))
			{
				r2bandend++;
			}
			if (non_overlap_2_func(dest, r2, r2bandend,
				MAX(r2->y,ybot), r2->y+r2->height) == 0)
				return 0;
			r2 = r2bandend;
		} while (r2 != r2end);
	}

	if (dest->num_rects != curband)
	{
		app_coalesce (dest, prevband, curband);
	}

	/*
	 * A bit of cleanup. To keep regions from growing without bound,
	 * we shrink the array of rectangles to match the new number of
	 * rectangles in the region. This never goes to 0, however...
	 *
	 * Only do this stuff if the number of rectangles allocated is
	 * more than twice the number of rectangles in the region
	 * (a simple optimization...).
	 */
	if (dest->num_rects < (dest->size >> 1))
	{
		if (dest->num_rects > 0)
		{
			Rect *prev_rects = dest->rects;
			dest->size = dest->num_rects;
			dest->rects = app_realloc (dest->rects,
					(sizeof(Rect) * dest->size));
			if (! dest->rects)
				dest->rects = prev_rects;
		}
		else
		{
			/*
			 * No point in doing the extra work involved in
			 * an app_realloc if the region is empty.
			 */
			dest->size = 1;
			app_free(dest->rects);
			dest->rects = app_alloc(sizeof(Rect));
		}
	}
	app_free(oldrects);
	return 1;
}

/*======================================================================
 *	Region Union
 *====================================================================*/

/*
 *-----------------------------------------------------------------------
 * app_union_non_o
 *	Handle a non-overlapping band for the union operation. Just
 *	adds the rectangles into the region. Doesn't have to check for
 *	subsumption or anything.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	rgn->num_rects is incremented and the final rectangles overwritten
 *	with the rectangles we're passed.
 *
 *-----------------------------------------------------------------------
 */
static int app_union_non_o(Region *rgn, Rect *r, Rect *end, int y1, int y2)
{
	Rect *nextrect;

	nextrect = &rgn->rects[rgn->num_rects];

	assert(y1 < y2);

	while (r != end)
	{
		assert(r->x < r->x + r->width);
		MEMCHECK(rgn, nextrect, rgn->rects);
		nextrect->x = r->x;
		nextrect->y = y1;
		nextrect->width = r->width;
		nextrect->height = y2 - y1;
		rgn->num_rects += 1;
		nextrect++;

		assert(rgn->num_rects <= rgn->size);
		r++;
	}
	return 1;
}

/*
 *-----------------------------------------------------------------------
 * app_union_o --
 *	Handle an overlapping band for the union operation. Picks the
 *	left-most rectangle each time and merges it into the region.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Rectangles are overwritten in rgn->rects and rgn->num_rects will
 *	be changed.
 *
 *-----------------------------------------------------------------------
 */

#define MERGERECT(r) \
	if ((rgn->num_rects != 0) && \
	    (nextrect[-1].y == y1) && \
	    (nextrect[-1].y+nextrect[-1].height == y2) && \
	    (nextrect[-1].x+nextrect[-1].width >= (r)->x)) \
	{  \
	  if (nextrect[-1].x+nextrect[-1].width < (r)->x+(r)->width)\
	  { \
	    nextrect[-1].width = (r)->x+(r)->width-nextrect[-1].x; \
	    assert(nextrect[-1].x < nextrect[-1].x+nextrect[-1].width); \
	  } \
	} \
	else \
	{ \
		MEMCHECK(rgn, nextrect, rgn->rects); \
		nextrect->y = y1; \
		nextrect->height = y2 - y1; \
		nextrect->x = (r)->x; \
		nextrect->width = (r)->width; \
		rgn->num_rects += 1; \
		nextrect += 1; \
	} \
	assert(rgn->num_rects< = rgn->size); \
	(r)++;

static int app_union_o (Region *rgn, Rect *r1, Rect *r1end,
	Rect *r2, Rect *r2end, int y1, int y2)
{
	Rect *nextrect;

	nextrect = &rgn->rects[rgn->num_rects];

	assert (y1 < y2);
	while ((r1 != r1end) && (r2 != r2end))
	{
		if (r1->x < r2->x)
		{
			MERGERECT(r1);
		}
		else
		{
			MERGERECT(r2);
		}
	}

	if (r1 != r1end)
	{
		do
		{
			MERGERECT(r1);
		} while (r1 != r1end);
	}
	else {
		while (r2 != r2end)
		{
			MERGERECT(r2);
		}
	}
	return 1;
}

#undef MERGERECT

int app_union_region(Region *reg1, Region *reg2, Region *dest)
{
	int result = 1;

	/* check all the simple cases */

	/*
	 * Region 1 and 2 are the same or region 1 is empty
	 */
	if ((reg1 == reg2) || (reg1->num_rects == 0))
	{
		if (dest != reg2)
			result = app_region_copy(dest, reg2);
		return result;
	}

	/*
	 * if nothing to union (region 2 empty)
	 */
	if (reg2->num_rects == 0)
	{
		if (dest != reg1)
			result = app_region_copy(dest, reg1);
		return result;
	}

	/*
	 * Region 1 completely subsumes region 2
	 */
	if ((reg1->num_rects == 1) && 
	    (reg1->extents.x <= reg2->extents.x) &&
	    (reg1->extents.y <= reg2->extents.y) &&
	    (reg1->extents.x+reg1->extents.width >=
	     reg2->extents.x+reg2->extents.width) &&
	    (reg1->extents.y+reg1->extents.height >=
	     reg2->extents.y+reg2->extents.height))
	{
		if (dest != reg1)
			result = app_region_copy(dest, reg1);
		return result;
	}

	/*
	 * Region 2 completely subsumes region 1
	 */
	if ((reg2->num_rects == 1) && 
	    (reg2->extents.x <= reg1->extents.x) &&
	    (reg2->extents.y <= reg1->extents.y) &&
	    (reg2->extents.x+reg2->extents.width >=
	     reg1->extents.x+reg1->extents.width) &&
	    (reg2->extents.y+reg2->extents.height >=
	     reg1->extents.y+reg1->extents.height))
	{
		if (dest != reg2)
			result = app_region_copy(dest, reg2);
		return result;
	}

	result = app_region_op (dest, reg1, reg2,
		app_union_o, app_union_non_o, app_union_non_o);

	dest->extents.x = MIN(reg1->extents.x, reg2->extents.x);
	dest->extents.y = MIN(reg1->extents.y, reg2->extents.y);
	dest->extents.width = MAX(reg1->extents.x+reg1->extents.width,
				reg2->extents.x+reg2->extents.width) -
				dest->extents.x;
	dest->extents.height = MAX(reg1->extents.y+reg1->extents.height,
				reg2->extents.y+reg2->extents.height) -
				dest->extents.y;
	return result;
}

/*
 *  Find the union of a region with a rectangle, as a region.
 */
int app_union_region_with_rect(Region *source, Rect r, Region *dest)
{
	Region reg;

	if ((r.width == 0) || (r.height == 0))
		return 1;
	reg.rects = &reg.extents;
	reg.num_rects = 1;
	reg.extents.x = r.x;
	reg.extents.y = r.y;
	reg.extents.width = r.width;
	reg.extents.height = r.height;
	reg.size = 1;

	return app_union_region(&reg, source, dest);
}

/*======================================================================
 *	Region Intersection
 *====================================================================*/
/*
 *-----------------------------------------------------------------------
 * app_intersect_o
 *	Handle an overlapping band for app_ntersect_region.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Rectangles may be added to the region.
 *
 *-----------------------------------------------------------------------
 */
static int app_intersect_o (Region *rgn, Rect *r1, Rect *r1end,
	Rect *r2, Rect *r2end, int y1, int y2)
{
	int x1;
	int x2;
	Rect *nextrect;

	nextrect = &rgn->rects[rgn->num_rects];

	while ((r1 != r1end) && (r2 != r2end))
	{
		x1 = MAX(r1->x,r2->x);
		x2 = MIN(r1->x+r1->width,r2->x+r2->width);

		/*
		 * If there's any overlap between the two rectangles,
		 * add that overlap to the new region.
		 * There's no need to check for subsumption because the
		 * only way such a need could arise is if some region
		 * has two rectangles right next to each other.
		 * Since that should never happen, we don't need to check.
		 */
		if (x1 < x2)
		{
			assert(y1 < y2);

			MEMCHECK(rgn, nextrect, rgn->rects);
			nextrect->x = x1;
			nextrect->y = y1;
			nextrect->width = x2 - x1;
			nextrect->height = y2 - y1;
			rgn->num_rects += 1;
			nextrect++;
			assert(rgn->num_rects <= rgn->size);
		}

		/*
		 * Need to advance the pointers. Shift the one that
		 * extends to the right the least, since the other still
		 * has a chance to overlap with that region's next
		 * rectangle.
		 */
		if (r1->x+r1->width < r2->x+r2->width)
			r1++;
		else if (r2->x+r2->width < r1->x+r1->width)
			r2++;
		else {
			r1++;
			r2++;
		}
	}
	return 1;
}

int app_intersect_region(Region *reg1, Region *reg2, Region *dest)
{
	int result = 1;

	/* check for trivial reject */
	if ((reg1->num_rects == 0) || (reg2->num_rects == 0)  ||
	    (!RXR(reg1->extents, reg2->extents)))
		dest->num_rects = 0;
	else
		result = app_region_op(dest, reg1, reg2,
			app_intersect_o, NULL, NULL);

	/*
	 * Can't alter dest's extents before we call app_region_op because
	 * it might be one of the source regions and app_region_op depends
	 * on the extents of those regions being the same. Besides, this
	 * way there's no checking against rectangles that will be removed
	 * due to coalescing, so we have fewer rectangles to examine.
	 */
	app_set_extents(dest);
	return result;
}

/*
 *  Find the intersection of a region with a rectangle, as a region.
 */
int app_intersect_region_with_rect(Region *source, Rect r, Region *dest)
{
	Region reg;

	if ((r.width == 0) || (r.height == 0))
		return 1;
	reg.rects = &reg.extents;
	reg.num_rects = 1;
	reg.extents.x = r.x;
	reg.extents.y = r.y;
	reg.extents.width = r.width;
	reg.extents.height = r.height;
	reg.size = 1;

	return app_intersect_region(&reg, source, dest);
}

/*======================================================================
 * 	Region Subtraction
 *====================================================================*/

/*
 *-----------------------------------------------------------------------
 * app_subtract_non_o
 *	Deal with non-overlapping band for subtraction. Any parts from
 *	region 2 we discard. Anything from region 1 we add to the region.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	rgn may be affected.
 *
 *-----------------------------------------------------------------------
 */

static int app_subtract_non_o(Region *rgn, Rect *r, Rect *end, int y1, int y2)
{
	Rect *nextrect;
	
	nextrect = &rgn->rects[rgn->num_rects];
	
	assert(y1 < y2);

	while (r != end)
	{
		assert(r->x < r->x+r->width);
		MEMCHECK(rgn, nextrect, rgn->rects);
		nextrect->x = r->x;
		nextrect->y = y1;
		nextrect->width = r->width;
		nextrect->height = y2 - y1;
		rgn->num_rects += 1;
		nextrect++;

		assert(rgn->num_rects <= rgn->size);

		r++;
	}
	return 1;
}

/*
 *-----------------------------------------------------------------------
 * app_subtract_o
 *	Overlapping band subtraction. x1 is the left-most point not yet
 *	checked.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	rgn may have rectangles added to it.
 *
 *-----------------------------------------------------------------------
 */

static int app_subtract_o (Region *rgn, Rect *r1, Rect *r1end,
	Rect *r2, Rect *r2end, int y1, int y2)
{
	Rect *nextrect;
	int x1;

	x1 = r1->x;

	assert(y1 < y2);
	nextrect = &rgn->rects[rgn->num_rects];

	while ((r1 != r1end) && (r2 != r2end))
	{
		if (r2->x+r2->width <= x1)
		{
			/*
			 * Subtrahend missed: go to next subtrahend.
			 */
			r2++;
		}
		else if (r2->x <= x1)
		{
			/*
			 * Subtrahend preceeds minuend:
			 * remove left edge of minuend.
			 */
			x1 = r2->x+r2->width;
			if (x1 >= r1->x+r1->width)
			{
				/*
				 * Minuend completely covered:
				 * advance to next minuend and reset
				 * left fence to edge of new minuend.
				 */
				r1++;
				if (r1 != r1end)
					x1 = r1->x;
			}
			else
			{
				/*
				 * Subtrahend now used up since it
				 * doesn't extend beyond minuend
				 */
				r2++;
			}
		}
		else if (r2->x < r1->x+r1->width)
		{
			/*
			 * Left part of subtrahend covers part of minuend:
			 * add uncovered part of minuend to region and
			 * skip to next subtrahend.
			 */
			assert(x1 < r2->x);
			MEMCHECK(rgn, nextrect, rgn->rects);
			nextrect->x = x1;
			nextrect->y = y1;
			nextrect->width = r2->x - x1;
			nextrect->height = y2 - y1;
			rgn->num_rects += 1;
			nextrect++;

			assert(rgn->num_rects <= rgn->size);

			x1 = r2->x + r2->width;
			if (x1 >= r1->x + r1->width)
			{
				/*
				 * Minuend used up: advance to new...
				 */
				r1++;
				if (r1 != r1end)
					x1 = r1->x;
			}
			else
			{
				/*
				 * Subtrahend used up
				 */
				r2++;
			}
		}
		else
		{
			/*
			 * Minuend used up:
			 * add any remaining piece before advancing.
			 */
			if (r1->x+r1->width > x1)
			{
				MEMCHECK(rgn, nextrect, rgn->rects);
				nextrect->x = x1;
				nextrect->y = y1;
				nextrect->width = r1->x + r1->width - x1;
				nextrect->height = y2 - y1;
				rgn->num_rects += 1;
				nextrect++;
				assert(rgn->num_rects <= rgn->size);
			}
			r1++;
			x1 = r1->x;
		}
	}

	/*
	 * Add remaining minuend rectangles to region.
	 */
	while (r1 != r1end)
	{
		assert(x1 < r1->x+r1->width);
		MEMCHECK(rgn, nextrect, rgn->rects);
		nextrect->x = x1;
		nextrect->y = y1;
		nextrect->width = r1->x + r1->width - x1;
		nextrect->height = y2 - y1;
		rgn->num_rects += 1;
		nextrect++;

		assert(rgn->num_rects <= rgn->size);

		r1++;
		if (r1 != r1end)
		{
			x1 = r1->x;
		}
	}
	return 1;
}
	
/*
 *-----------------------------------------------------------------------
 * app_subtract_region
 *	Subtract regS from regM and leave the result in dest.
 *	S stands for subtrahend, M for minuend.
 *
 * Results:
 *	Returns 1 on succes, 0 on failure.
 *
 * Side Effects:
 *	dest is overwritten.
 *
 *-----------------------------------------------------------------------
 */

int app_subtract_region(Region *regM, Region *regS, Region *dest)
{
	int result;

	/* check for trivial reject */
	if ( (regM->num_rects == 0) || (regS->num_rects == 0)  ||
		(!RXR(regM->extents, regS->extents)) )
	{
		app_region_copy(dest, regM);
		return 1;
	}
 
	result = app_region_op(dest, regM, regS,
		app_subtract_o, app_subtract_non_o, NULL);

	/*
	 * Can't alter dest's extents before we call app_region_op
	 * because it might be one of the source regions and
	 * app_region_op depends on the extents of those regions being
	 * the unaltered. Besides, this way there's no checking against
	 * rectangles that will be removed due to coalescing, so we
	 * have to examine fewer rectangles.
	 */
	app_set_extents(dest);
	return result;
}

/*
 *  Find all rectangles in one region or the other, but not both.
 */
int app_xor_region(Region *src1, Region *src2, Region *dest)
{
	int result = 0;
	Region *tmp1;
	Region *tmp2;

	tmp1 = app_new_region();
	tmp2 = app_new_region();
	if ((tmp1 == NULL) || (tmp2 == NULL))
		return 0;
	result += app_subtract_region(src1, src2, tmp1);
	result += app_subtract_region(src2, src1, tmp2);
	result += app_union_region(tmp1, tmp2, dest);
	app_del_region(tmp1);
	app_del_region(tmp2);
	if (result == 3)
		return 1;
	return 0;
}

/*
 *  Check to see if the region is empty.
 */
int app_region_is_empty(Region *rgn)
{
	if (rgn->num_rects == 0)
		return 1;
	else
		return 0;
}

/*
 *  Check to see if two regions are equal.
 */
int app_regions_equal(Region *r1, Region *r2)
{
	int i;

	if (r1->num_rects != r2->num_rects)
		return 0;
	else if (r1->num_rects == 0)
		return 1;
	else if (r1->extents.x != r2->extents.x)
		return 0;
	else if (r1->extents.width != r2->extents.width)
		return 0;
	else if (r1->extents.y != r2->extents.y)
		return 0;
	else if (r1->extents.height != r2->extents.height)
		return 0;
	else for (i=0; i < r1->num_rects; i++) {
		if (r1->rects[i].x != r2->rects[i].x)
			return 0;
		else if (r1->rects[i].width != r2->rects[i].width)
			return 0;
		else if (r1->rects[i].y != r2->rects[i].y)
			return 0;
		else if (r1->rects[i].height != r2->rects[i].height)
			return 0;
	}
	return 1;
}

/*
 *  Is a point within a region?
 */
int app_point_in_region(Point p, Region *rgn)
{
	int i;

	if (rgn->num_rects == 0)
		return 0;
	if (!PTINRECT(rgn->extents, p.x, p.y))
		return 0;
	for (i=0; i < rgn->num_rects; i++)
	{
		if (PTINRECT (rgn->rects[i], p.x, p.y))
			return 1;
	}
	return 0;
}

/*
 *  Rectangle and region intersection:
 */

enum {
	RectOut = 0,
	RectIn = 1,
	RectPart = -1
};

int app_rect_intersects_region(Rect r, Region *rgn)
{
	Rect *box;
	Rect *end;
	Rect *pr = &r;
	int x, y;
	int partIn, partOut; /* 1 for true, 0 for false */

	x = r.x;
	y = r.y;

	/* this is (just) a useful optimization */
	if ((rgn->num_rects == 0) || !RXR(rgn->extents, r))
		return RectOut;

	partOut = 0; /* false */
	partIn = 0; /* false */

	/* can stop when both partOut and partIn are true, or if */
	/* we reach pr->y2 */
	for (box=rgn->rects, end=box+rgn->num_rects; box < end; box++)
	{

		if (box->y+box->height <= y)
			continue; /* getting up to speed or skipping remainder of band */

		if (box->y > y)
		{
			partOut = 1;	/* missed part of rect above */
			if (partIn || (box->y >= pr->y+pr->height))
				break;
			y = box->y;	/* x guaranteed to be == pr->x */
		}

		if (box->x+box->width <= x)
			continue; /* not far enough over yet */

		if (box->x > x)
		{
			partOut = 1;	/* missed part of rect to left */
			if (partIn)
				break;
		}

		if (box->x < pr->x+pr->width)
		{
			partIn = 1;	/* definitely overlap */
			if (partOut)
				break;
		}

		if (box->x+box->width >= pr->x+pr->width)
		{
			y = box->y+box->height;	/* done with this band */
			if (y >= pr->y+pr->height)
				break;
			x = pr->x;	/* reset x out to left again */
		}
		else
		{
			/*
			 * Because boxes in a band are maximal width,
			 * if the first box to overlap the rectangle
			 * doesn't completely cover it in that band,
			 * the rectangle must be partially out, since
			 * some of it will be uncovered in that band.
			 * partIn will have been set true by now...
			 */
			break;
		}
	}

	if (partIn) {
		if (y < pr->y+pr->height)
			return RectPart;
		else
			return RectIn;
	}
	else
		return RectOut;
}

/*
 *  Is a rectangle wholly contained within a region?
 */
int app_rect_in_region(Rect r, Region *rgn)
{
	if (app_rect_intersects_region(r, rgn) == RectIn)
		return 1;
	else
		return 0;
}
