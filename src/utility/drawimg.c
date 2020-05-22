/*
 *  Drawing to images (essential operations).
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.01  2001/09/09  Improved 8-bit drawing in copy_rect.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced a pointer subtraction warning.
 *  Version: 3.57  2002/08/13  Fixed an alpha-channel blending problem.
 *  Version: 3.58  2002/08/28  Now allows greyscale text blending.
 *  Version: 3.59  2005/10/10  Supports over-sized glyphs.
 *  Version: 3.62  2010/02/24  Non-black drawing of glyphs with alpha.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"

/*
 *  This file implements the essential drawing opertions needed
 *  to draw to images. These implement:
 *	app_fill_rect
 *	app_copy_rect
 *	app_draw_utf8
 *
 *  All valid image depths are handled in this file.
 */


/*
 *  app_image_fill_rect:
 *
 *  Fill a rectangle with colour, in an image.
 *
 *  We assume the correct colour and pixel value (pixval) are
 *  already set. If the image is 32-bit, we just fill with the
 *  colour, performing 'alpha' colour blending (unless it's
 *  transparent, if which case nothing needs to be done at all).
 *  If the image is 8-bit, we fill using the pixval, but we don't
 *  do alpha blending, since that would be too difficult
 *  (we'd have to find the best blended colour from the palette
 *  for each pixel - very slow).
 */

/*
 *  How to do alpha blending:
 *
 *  The destination colour value is the weighted average of the
 *  source and desintation (the weight is alpha). Here, we use
 *  alpha to measure opaqueness, not transparency, so these
 *  formulas are reversed with respect to other literature.
 *
 *  a = alpha, ranges from 0 (opaque) to 1 (transparent)
 *  r = red component, ranges from 0 (black) to 255 (bright red)
 *  g = green component, ranges from 0 (black) to 255 (bright green)
 *  b = blue component, ranges from 0 (black) to 255 (bright blue)
 *
 *  Dr = destination red, Sr = source red
 *  Da = destination alpha, Sa = source alpha
 *
 *  If Sa = 1, the source is completely transparent, so D = D
 *  If Sa = 0, the source is completely opaque, so D = S
 *  If Sa = 0.5, the source and destination are averaged
 *  If Sa = 0.9, the destination will be slightly tinted by the source
 *
 *  Formula: For all components, if a is the source alpha (Sa):
 *	D = D.a + S.(1-a)
 *	  = D.a + S - S.a
 *	  = S + (D-S).a
 *
 *  This formula works. Check it. If a = 1, D = D.1 + S.0 = D.
 *  If a = 0, D = D.0 + S.1 = S.
 *  If a = 0.9, D = 0.9D + 0.1S, a slight tint.
 *
 *  This involves an addition, a subtraction and multiplication for
 *  each component. Alpha is actually an unsigned byte, therefore
 *  we also must divide alpha by 256, but this can be done efficiently
 *  using a right shift by 8 bits, although we trust an optimising
 *  compiler to do this for us, since signed right shift is undefined
 *  in the C language, while signed division is defined correctly.
 *
 *  How to handle the case that the destination already has alpha:
 *  just multiply the alphas. Thus, opaque things stay opaque.
 *  This make two pieces of overlaid glass become darker than each
 *  on its own, which is correct. Overlay enough almost transparent
 *  glass and eventually it will become opaque.
 *
 *  Thus the formulas for alpha blending are:
 *
 *	Da = (Sa/256).Da = (Da.Sa)/256
 *	Dr = Sr + (Dr-Sr).Sa/256
 *	Dg = Sg + (Dg-Sg).Sa/256
 *	Db = Sb + (Db-Sb).Sa/256
 */
int app_image_fill_rect(Graphics *dst, Rect dr)
{
	int i, num_rects;
	Rect clipped;
	Rect *rects;
	int x, y, end_y, pixval;
	byte *dst8;
	Colour *dst32, colour;
	int a, r, g, b;

	if (dst->colour.alpha == 0xFF)
		return 1; /* nothing to draw if colour is transparent */

	/* correct drawing displacement */
	dr.x += dst->offset.x;
	dr.y += dst->offset.y;

	/* fix negative spaces */
	if (dr.width < 0) {
		dr.x += dr.width;
		dr.width = 0 - dr.width;
	}
	if (dr.height < 0) {
		dr.y += dr.height;
		dr.height = 0 - dr.height;
	}

	/* set up clipping */

	if (dst->clip) {
		/* clip to the selected clipping region */
		num_rects = dst->clip->num_rects;
		rects = dst->clip->rects;
	}
	else {
		num_rects = 1;
		rects = & dst->area;
	}

	/* copy drawing colours to local variables for speed */

	pixval = dst->pixval;
	colour = dst->colour;

	/* draw the clipped rectangles */

	if ((dst->img->depth == 32) && (colour.alpha == 0))
	{
		/* just fill with the colour, fully opaque, no blending */

		for (i=0; i < num_rects; i++) {
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			/* fill pixels with colour */

			end_y = clipped.y + clipped.height;
			for (y=clipped.y; y < end_y; y++)
			{
				dst32 = & dst->img->data32[y][clipped.x];
				for (x=0; x < clipped.width; x++)
				{
					*dst32++ = colour;
				}
			}
		}
	}
	else if (dst->img->depth == 32)
	{
		/* blend with the partially transparent colour */

		a = colour.alpha;
		r = colour.red;
		g = colour.green;
		b = colour.blue;

		for (i=0; i < num_rects; i++) {
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			/* fill pixels with colour */

			end_y = clipped.y + clipped.height;
			for (y=clipped.y; y < end_y; y++)
			{
			  dst32 = & dst->img->data32[y][clipped.x];
			  for (x=0; x < clipped.width; x++)
			  {
			    dst32->alpha = ((dst32->alpha*a)/256);
			    dst32->red   = r+(((dst32->red  -r)*a)/256);
			    dst32->green = g+(((dst32->green-g)*a)/256);
			    dst32->blue  = b+(((dst32->blue -b)*a)/256);
			    dst32++;
			  }
			}
		}
	}
	else if (dst->img->depth == 8)
	{
		/* just fill with the pixval, fully opaque, no blending */

		for (i=0; i < num_rects; i++) {
			clipped = app_clip_rect(dr, rects[i]);
			if (clipped.width == 0)
				continue; /* nothing visible here */
			if (clipped.height == 0)
				continue; /* nothing visible here */

			/* fill pixels with pixval */

			end_y = clipped.y + clipped.height;
			for (y=clipped.y; y < end_y; y++)
			{
				dst8 = & dst->img->data8[y][clipped.x];
				memset(dst8, pixval, clipped.width);
			}
		}
	}

	return 1;
}

/*
 *  app_image_copy_rect:
 *
 *  Copy an area from an image to another image (or the same image).
 *
 *  There are a few cases to handle. The images could have different
 *  depths. Some pixels may be transparent. Copying may be from and
 *  to the same image (might need to copy data from bottom to top,
 *  and/or right to left). Clipping needs to occur.
 *
 *  For simplicity we assume no images share pixel data in memory,
 *  unless they are the same image. For example, it is possible
 *  to construct an image which points to some parts of another
 *  image. In that case it would be quite difficult to determine
 *  how to copy the pixels such that data isn't overwritten.
 *  So, for simplicity we assume either src->img and dst->img are
 *  completely distinct, or exactly the same. If they are distinct,
 *  there is no problem. If they are the same, we have to copy
 *  data carefully to avoid over-writing data.
 *
 *  To avoid over-writing data in the same image, use these rules:
 *  If the image is moving up, copy from the top to the bottom.
 *  If the image is moving down, copy from the bottom to the top.
 *  If the image is moving left, copy from the left to the right.
 *  If the image is moving right, copy from the right to the left.
 */
int app_image_copy_rect(Graphics *dst, Point dp, Graphics *src, Rect sr)
{
	int i, num_rects;
	Rect dr, clipped;
	Rect *rects;
	int ydiff, xdiff;
	int x, y, end_y;
	byte *src8, *dst8, *translation;
	Colour *src32, *dst32, *cmap, colour;
	int a, r, g, b;
	Palette p;

	/* correct drawing displacement */
	dp.x += dst->offset.x;
	dp.y += dst->offset.y;
	sr.x += src->offset.x;
	sr.y += src->offset.y;

	/* fix negative spaces */
	if (sr.width < 0) {
		sr.x += sr.width;
		sr.width = 0 - sr.width;
	}
	if (sr.height < 0) {
		sr.y += sr.height;
		sr.height = 0 - sr.height;
	}

	/* set up clipping */

	if (dst->clip) {
		/* clip to the selected clipping region */
		num_rects = dst->clip->num_rects;
		rects = dst->clip->rects;
	}
	else {
		num_rects = 1;
		rects = & dst->area;
	}

	/* are pixels moving up, down, left or right? */

	ydiff = sr.y - dp.y; /* positive is up, negative is down */
	xdiff = sr.x - dp.x; /* positive is left, negative is right */

	/* note, source y = clipped.y + sr.y - dp.y = clipped.y + ydiff */
	/* and   source x = clipped.x + sr.x - dp.x = clipped.x + xdiff */

	/* copy clipped rectangles */

	dr = rect(dp.x, dp.y, sr.width, sr.height);

	if ((src->img->depth == 32) && (dst->img->depth == 32))
	{
	  /* perform full alpha blending for each 32-bit pixel */

	  for (i=0; i < num_rects; i++) {
		clipped = app_clip_rect(dr, rects[i]);
		if (clipped.width == 0)
			continue; /* nothing visible here */
		if (clipped.height == 0)
			continue; /* nothing visible here */

		/* copy pixels with alpha blending */

		end_y = clipped.y + clipped.height;

		if (ydiff >= 0) {	/* upwards: copy top to bottom */
		  if (xdiff >= 0) {	/* leftwards: copy left to right */
		    for (y=clipped.y; y < end_y; y++)
		    {
		      src32 = & src->img->data32[y+ydiff][clipped.x+xdiff];
		      dst32 = & dst->img->data32[y][clipped.x];
		      for (x=0; x < clipped.width; x++)
		      {
			a = src32->alpha;
			r = src32->red;
			g = src32->green;
			b = src32->blue;
			src32++; /* move right */

			dst32->alpha = ((dst32->alpha*a)/256);
			dst32->red   = r+(((dst32->red  -r)*a)/256);
			dst32->green = g+(((dst32->green-g)*a)/256);
			dst32->blue  = b+(((dst32->blue -b)*a)/256);
			dst32++; /* move right */
		      }
		    }
		  }
		  else {		/* rightwards: copy right to left */
		    for (y=clipped.y; y < end_y; y++)
		    {
		      src32 = & src->img->data32[y+ydiff][clipped.x+xdiff];
		      dst32 = & dst->img->data32[y][clipped.x];
		      src32 += clipped.width - 1; /* start on right */
		      dst32 += clipped.width - 1; /* start on right */
		      for (x=0; x < clipped.width; x++)
		      {
			a = src32->alpha;
			r = src32->red;
			g = src32->green;
			b = src32->blue;
			src32--; /* move left */

			dst32->alpha = ((dst32->alpha*a)/256);
			dst32->red   = r+(((dst32->red  -r)*a)/256);
			dst32->green = g+(((dst32->green-g)*a)/256);
			dst32->blue  = b+(((dst32->blue -b)*a)/256);
			dst32--; /* move left */
		      }
		    }
		  }
		}
		else {			/* downwards: copy bottom to top */
		  if (xdiff >= 0) {	/* leftwards: copy left to right */
		    for (y=end_y-1; y >= clipped.y; y--)
		    {
		      src32 = & src->img->data32[y+ydiff][clipped.x+xdiff];
		      dst32 = & dst->img->data32[y][clipped.x];
		      for (x=0; x < clipped.width; x++)
		      {
			a = src32->alpha;
			r = src32->red;
			g = src32->green;
			b = src32->blue;
			src32++; /* move right */

			dst32->alpha = ((dst32->alpha*a)/256);
			dst32->red   = r+(((dst32->red  -r)*a)/256);
			dst32->green = g+(((dst32->green-g)*a)/256);
			dst32->blue  = b+(((dst32->blue -b)*a)/256);
			dst32++; /* move right */
		      }
		    }
		  }
		  else {		/* rightwards: copy right to left */
		    for (y=end_y-1; y >= clipped.y; y--)
		    {
		      src32 = & src->img->data32[y+ydiff][clipped.x+xdiff];
		      dst32 = & dst->img->data32[y][clipped.x];
		      src32 += clipped.width - 1; /* start on right */
		      dst32 += clipped.width - 1; /* start on right */
		      for (x=0; x < clipped.width; x++)
		      {
			a = src32->alpha;
			r = src32->red;
			g = src32->green;
			b = src32->blue;
			src32--; /* move left */

			dst32->alpha = ((dst32->alpha*a)/256);
			dst32->red   = r+(((dst32->red  -r)*a)/256);
			dst32->green = g+(((dst32->green-g)*a)/256);
			dst32->blue  = b+(((dst32->blue -b)*a)/256);
			dst32--; /* move left */
		      }
		    }
		  }
		}
	  }
	}
	else if ((src->img->depth == 8) && (dst->img->depth == 32))
	{
	  /* expand 8-bit pixvals as we go, and do alpha blending */

	  /* since the images have different depths, the code in this */
	  /* section has been simplified to assume the data is distinct */
	  /* - no need to copy right to left or bottom to top */

	  /* make a local reference to the source colour map */
	  cmap = src->img->cmap;

	  for (i=0; i < num_rects; i++) {
		clipped = app_clip_rect(dr, rects[i]);
		if (clipped.width == 0)
			continue; /* nothing visible here */
		if (clipped.height == 0)
			continue; /* nothing visible here */

		/* copy pixels with alpha blending */

		end_y = clipped.y + clipped.height;
		for (y=clipped.y; y < end_y; y++)
		{
		      src8  = & src->img->data8[y+ydiff][clipped.x+xdiff];
		      dst32 = & dst->img->data32[y][clipped.x];
		      for (x=0; x < clipped.width; x++)
		      {
			colour = cmap[*src8++]; /* move right */
			a = colour.alpha;
			r = colour.red;
			g = colour.green;
			b = colour.blue;

			dst32->alpha = ((dst32->alpha*a)/256);
			dst32->red   = r+(((dst32->red  -r)*a)/256);
			dst32->green = g+(((dst32->green-g)*a)/256);
			dst32->blue  = b+(((dst32->blue -b)*a)/256);
			dst32++; /* move right */
		      }
		}
	  }
	}
	else if ((src->img->depth == 32) && (dst->img->depth == 8))
	{
	  /* map 32-bit pixels to 8-bit pixvals as we go, no blending */

	  /* since the images have different depths, the code in this */
	  /* section has been simplified to assume the data is distinct */
	  /* - no need to copy right to left or bottom to top */

	  /* make a local palette to use when translating 32-bit pixels */
	  p.size = dst->img->cmap_size;
	  p.element = dst->img->cmap;

	  for (i=0; i < num_rects; i++) {
		clipped = app_clip_rect(dr, rects[i]);
		if (clipped.width == 0)
			continue; /* nothing visible here */
		if (clipped.height == 0)
			continue; /* nothing visible here */

		/* map pixels to pixvals, ignore alpha */

		end_y = clipped.y + clipped.height;
		for (y=clipped.y; y < end_y; y++)
		{
		  src32 = & src->img->data32[y+ydiff][clipped.x+xdiff];
		  dst8  = & dst->img->data8[y][clipped.x];
		  app_palette_translation(&p, dst8, clipped.width, src32);
		}
	  }
	}
	else if ((src->img->depth == 8) && (src->img == dst->img)
		&& (! app_image_has_transparent_pixels(src->img)))
	{
	  /* fast copy 8-bit pixvals with memmove, no alpha blending */

	  for (i=0; i < num_rects; i++) {
		clipped = app_clip_rect(dr, rects[i]);
		if (clipped.width == 0)
			continue; /* nothing visible here */
		if (clipped.height == 0)
			continue; /* nothing visible here */

		/* copy pixvals without alpha blending */

		/* note, memmove can copy bytes safely left or right, */
		/* so we don't need to handle xdiff specially here */

		if (ydiff >= 0) {	/* upwards: copy top to bottom */
		  end_y = clipped.y + clipped.height;
		  for (y=clipped.y; y < end_y; y++)
		  {
			src8 = & src->img->data8[y+ydiff][clipped.x+xdiff];
			dst8 = & dst->img->data8[y][clipped.x];
			memmove(dst8, src8, clipped.width);
		  }
		}
		else {			/* downwards: copy bottom to top */
		  end_y = clipped.y + clipped.height;
		  for (y=end_y-1; y >= clipped.y; y--)
		  {
			src8 = & src->img->data8[y+ydiff][clipped.x+xdiff];
			dst8 = & dst->img->data8[y][clipped.x];
			memmove(dst8, src8, clipped.width);
		  }
		}
	  }
	}
	else if ((src->img->depth == 8) && (dst->img->depth == 8))
	{
	  /* slow copy 8-bit pixvals, avoid copying transparent pixels */

	  /* make a local reference to the source colour map */
	  cmap = src->img->cmap;

	  /* make a local palette to translate colour maps */
	  p.size = dst->img->cmap_size;
	  p.element = dst->img->cmap;
	  translation = app_alloc(src->img->cmap_size);
	  app_palette_translation(&p, translation,
			src->img->cmap_size, src->img->cmap);

	  for (i=0; i < num_rects; i++) {
		clipped = app_clip_rect(dr, rects[i]);
		if (clipped.width == 0)
			continue; /* nothing visible here */
		if (clipped.height == 0)
			continue; /* nothing visible here */

		/* copy pixels avoiding transparent pixels */

		end_y = clipped.y + clipped.height;

		if (ydiff >= 0) {	/* upwards: copy top to bottom */
		  if (xdiff >= 0) {	/* leftwards: copy left to right */
		    for (y=clipped.y; y < end_y; y++)
		    {
		      src8 = & src->img->data8[y+ydiff][clipped.x+xdiff];
		      dst8 = & dst->img->data8[y][clipped.x];
		      for (x=0; x < clipped.width; x++)
		      {
			if (cmap[*src8].alpha <= 0x7f) /* opaque */
				*dst8 = translation[*src8];
			src8++; /* move right */
			dst8++; /* move right */
		      }
		    }
		  }
		  else {		/* rightwards: copy right to left */
		    for (y=clipped.y; y < end_y; y++)
		    {
		      src8 = & src->img->data8[y+ydiff][clipped.x+xdiff];
		      dst8 = & dst->img->data8[y][clipped.x];
		      src8 += clipped.width - 1; /* start on right */
		      dst8 += clipped.width - 1; /* start on right */
		      for (x=0; x < clipped.width; x++)
		      {
			if (cmap[*src8].alpha <= 0x7f) /* opaque */
				*dst8 = translation[*src8];
			src8--; /* move left */
			dst8--; /* move left */
		      }
		    }
		  }
		}
		else {			/* downwards: copy bottom to top */
		  if (xdiff >= 0) {	/* leftwards: copy left to right */
		    for (y=end_y-1; y >= clipped.y; y--)
		    {
		      src8 = & src->img->data8[y+ydiff][clipped.x+xdiff];
		      dst8 = & dst->img->data8[y][clipped.x];
		      for (x=0; x < clipped.width; x++)
		      {
			if (cmap[*src8].alpha <= 0x7f) /* opaque */
				*dst8 = translation[*src8];
			src8++; /* move right */
			dst8++; /* move right */
		      }
		    }
		  }
		  else {		/* rightwards: copy right to left */
		    for (y=end_y-1; y >= clipped.y; y--)
		    {
		      src8 = & src->img->data8[y+ydiff][clipped.x+xdiff];
		      dst8 = & dst->img->data8[y][clipped.x];
		      src8 += clipped.width - 1; /* start on right */
		      dst8 += clipped.width - 1; /* start on right */
		      for (x=0; x < clipped.width; x++)
		      {
			if (cmap[*src8].alpha <= 0x7f) /* opaque */
				*dst8 = translation[*src8];
			src8--; /* move left */
			dst8--; /* move left */
		      }
		    }
		  }
		}
	  }
	  app_free(translation);
	}
	else {
		return 0; /* error: strange depths */
	}

	return 1;
}

/*
 *  The following function is a specialised, modified form of
 *  app_image_copy_rect, for use when drawing glyphs in the
 *  app_image_draw_utf8 fuction below.
 *
 *  To draw text we could just perform a copy_rect operation
 *  from the subfont's image, because it will only have two
 *  colours: black and transparent. This would work for black
 *  text, but not for any other colour.
 *
 *  Therefore the function below determines whether it's about
 *  to draw a black or transparent pixel, and if it's black,
 *  draws the current drawing colour or pixval instead.
 *
 *  The code has also been simplified by assuming the glyph
 *  image is distinct from the destination. (We would not want
 *  to draw directly into a font!) Thus, the safe copying
 *  used above is not needed here, reducing the code size,
 *  and speeding things up a bit too.
 */
static
int app_image_draw_glyph(Graphics *dst, Point dp, Image *img, Rect sr)
{
	int i, num_rects;
	Rect dr, clipped;
	Rect *rects;
	int ydiff, xdiff;
	int x, y, end_y;
	byte *src8, *dst8, *translation;
	Colour *src32, *dst32, colour, *cmap;
	int pixval, blackval, whiteval;
	int a, r, g, b;
	Palette p;

	/* correct drawing displacement */

	dp.x += dst->offset.x;
	dp.y += dst->offset.y;

	/* set up clipping */

	if (dst->clip) {
		/* clip to the selected clipping region */
		num_rects = dst->clip->num_rects;
		rects = dst->clip->rects;
	}
	else {
		num_rects = 1;
		rects = & dst->area;
	}

	/* determine motion of pixels */

	ydiff = sr.y - dp.y;
	xdiff = sr.x - dp.x;

	/* note, source y = clipped.y + sr.y - dp.y = clipped.y + ydiff */
	/* and   source x = clipped.x + sr.x - dp.x = clipped.x + xdiff */

	/* copy drawing colour into local variables */

	colour = dst->colour;
	pixval = dst->pixval;

	r = colour.red;
	g = colour.green;
	b = colour.blue;

	/* copy clipped rectangles */

	dr = rect(dp.x, dp.y, sr.width, sr.height);

	if ((img->depth == 32) && (dst->img->depth == 32))
	{
	  /* copy drawing colour if source pixel is opaque */

	  for (i=0; i < num_rects; i++) {
		clipped = app_clip_rect(dr, rects[i]);
		if (clipped.width == 0)
			continue; /* nothing visible here */
		if (clipped.height == 0)
			continue; /* nothing visible here */

		end_y = clipped.y + clipped.height;
		for (y=clipped.y; y < end_y; y++)
		{
		      src32 = & img->data32[y+ydiff][clipped.x+xdiff];
		      dst32 = & dst->img->data32[y][clipped.x];
		      for (x=0; x < clipped.width; x++)
		      {
			if (src32->alpha == 0) {
				*dst32 = colour;
			}
			else if (src32->alpha == 255) {
				/* draw nothing */
			}
			else {
				a = src32->alpha;
				/*
				r = src32->red;
				g = src32->green;
				b = src32->blue;
				*/

				dst32->alpha = ((dst32->alpha*a)/256);
				dst32->red   = r+(((dst32->red  -r)*a)/256);
				dst32->green = g+(((dst32->green-g)*a)/256);
				dst32->blue  = b+(((dst32->blue -b)*a)/256);
			}
			src32++;
			dst32++;
		      }
		}
	  }
	}
	else if ((img->depth == 8) && (dst->img->depth == 32))
	{
	  /* check 8-bit pixels against the black-value */

	  for (blackval=0; blackval < img->cmap_size; blackval++)
		if (img->cmap[blackval].alpha == 0)
			break;

	  for (whiteval=0; whiteval < img->cmap_size; whiteval++)
		if (img->cmap[whiteval].alpha == 255)
			break;

	  for (i=0; i < num_rects; i++) {
		clipped = app_clip_rect(dr, rects[i]);
		if (clipped.width == 0)
			continue; /* nothing visible here */
		if (clipped.height == 0)
			continue; /* nothing visible here */

		end_y = clipped.y + clipped.height;
		for (y=clipped.y; y < end_y; y++)
		{
		      src8  = & img->data8[y+ydiff][clipped.x+xdiff];
		      dst32 = & dst->img->data32[y][clipped.x];
		      for (x=0; x < clipped.width; x++)
		      {
			if (*src8 == blackval) {
				*dst32 = colour;
			}
			else if (*src8 == whiteval) {
				/* draw nothing */
			}
			else {
				a = img->cmap[*src8].alpha;
				/*
				r = img->cmap[*src8].red;
				g = img->cmap[*src8].green;
				b = img->cmap[*src8].blue;
				*/

				dst32->alpha = ((dst32->alpha*a)/256);
				dst32->red   = r+(((dst32->red  -r)*a)/256);
				dst32->green = g+(((dst32->green-g)*a)/256);
				dst32->blue  = b+(((dst32->blue -b)*a)/256);
			}
			src8++;
			dst32++;
		      }
		}
	  }
	}
	else if ((img->depth == 32) && (dst->img->depth == 8))
	{
	  /* check 32-bit pixels, copy drawing pixval if opaque */

	  /* make a local palette to use when translating 32-bit pixels */
	  p.size = dst->img->cmap_size;
	  p.element = dst->img->cmap;
	  translation = app_alloc(dst->img->width);

	  for (i=0; i < num_rects; i++) {
		clipped = app_clip_rect(dr, rects[i]);
		if (clipped.width == 0)
			continue; /* nothing visible here */
		if (clipped.height == 0)
			continue; /* nothing visible here */

		/* map pixels to pixvals, ignore alpha */

		end_y = clipped.y + clipped.height;
		for (y=clipped.y; y < end_y; y++)
		{
		      src32 = & img->data32[y+ydiff][clipped.x+xdiff];
		      dst8  = & dst->img->data8[y][clipped.x];
		      app_palette_translation(&p, translation,
						clipped.width, src32);
		      for (x=0; x < clipped.width; x++)
		      {
			if (src32->alpha == 0) {
				*dst8 = pixval;
			}
			else if (src32->alpha == 255) {
				/* draw nothing */
			}
			else {
				*dst8 = translation[x];
			}
			src32++;
			dst8++;
		      }
		}
	  }

	  app_free(translation);
	}
	else if ((img->depth == 8) && (dst->img->depth == 8))
	{
	  /* check 8-bit pixels against the black-value */

	  for (blackval=0; blackval < img->cmap_size; blackval++)
		if (img->cmap[blackval].alpha == 0)
			break;

	  for (whiteval=0; whiteval < img->cmap_size; whiteval++)
		if (img->cmap[whiteval].alpha == 255)
			break;

	  /* make a local reference to the source colour map */
	  cmap = img->cmap;

	  /* make a local palette to translate colour maps */
	  p.size = dst->img->cmap_size;
	  p.element = dst->img->cmap;
	  translation = app_alloc(img->cmap_size);
	  app_palette_translation(&p, translation,
				img->cmap_size, img->cmap);

	  for (i=0; i < num_rects; i++) {
		clipped = app_clip_rect(dr, rects[i]);
		if (clipped.width == 0)
			continue; /* nothing visible here */
		if (clipped.height == 0)
			continue; /* nothing visible here */

		end_y = clipped.y + clipped.height;
		for (y=clipped.y; y < end_y; y++)
		{
		      src8 = & img->data8[y+ydiff][clipped.x+xdiff];
		      dst8 = & dst->img->data8[y][clipped.x];
		      for (x=0; x < clipped.width; x++)
		      {
			if (*src8 == blackval) {
				*dst8 = pixval;
			}
			else if (*src8 == whiteval) {
				/* draw nothing */
			}
			else {
				if (cmap[*src8].alpha <= 0x7f) /* opaque */
					*dst8 = translation[*src8];
			}
			src8++;
			dst8++;
		      }
		}
	  }

	  app_free(translation);
	}
	else {
		return 0; /* error: strange depths */
	}

	return 1;
}

/*
 *  app_image_draw_utf8:
 *
 *  Draw the characters from a UTF-8 string, loading subfonts as needed.
 *
 *  This function implements client-side clipping to allow partial
 *  glyphs to be drawn.
 */
int app_image_draw_utf8(Graphics *dst, Point dp, const char *s, int nbytes)
{
	int gw; /* glyph's width, by which the cursor is stepped */
	int right_to_left;
	unsigned long ch;
	unsigned long *cp;
	const char *sp;
	const char *src_end;
	Rect clipped;
	Subfont *sub;
	Font *f;
	Rect gr;
	Point gp;
	int temp = 0;

	if (dst->colour.alpha == 0xFF)
		return 1; /* nothing to draw if colour is transparent */

	/* correct drawing displacement */
	dp.x += dst->offset.x;
	dp.y += dst->offset.y;

	/* obtain font to use when drawing */
	f = dst->font;
	if (f == NULL) {
		if (dst->app == NULL)
			temp = 1;
		f = app_find_default_font(dst->app);
	}
	if (f == NULL)
		return 0;

	/* correct drawing direction */
	if (dst->text_direction & RL_TB)
		right_to_left = 1;
	else
		right_to_left = 0;

	/* set up clipping */

	if (dst->clip) {
		/* clip to the selected clipping region */
		clipped = dst->clip->extents;
	}
	else {
		clipped = dst->area;
	}

	/* check if we are completely outside the clipping region */

	if (dp.y + f->height <= clipped.y)
		return 1;	/* completely above */
	if (dp.y >= clipped.y + clipped.height)
		return 1;	/* completely below */
	if (right_to_left) {
		if (dp.x <= clipped.x)
			return 1;	/* completely to the left */
	}
	else {
		if (dp.x >= clipped.x + clipped.width)
			return 1;	/* completely to the right */
	}

	/* draw glyphs */

	sp = s;
	src_end = s + nbytes;

	while (nbytes > 0) {
		cp = &ch;
		if (app_utf8_to_unicode(&sp, src_end, &cp, cp+1)
		    & SourceExhausted)
			break;
		nbytes -= (int) (sp - s);
		s = sp;

		sub = app_font_char_info(f, ch, &gw);

		if ((sub == NULL) || (gw < 0)) {
			if (right_to_left)
				dp.x -= 6;

			/* character glyph not found, draw a box */
			app_draw_rect(dst, rect(dp.x - dst->offset.x + 1,
						dp.y - dst->offset.y + 1,
						4, f->height-2));

			if (! right_to_left)
				dp.x += 6;

			continue; /* go to next character */
		}
		/* else, character glyph exists */

		gr.width = sub->img->width / 32;
		gr.height = sub->img->height / 8;
		gr.x = (ch%32) * gr.width;
		gr.y = ((ch/32)%8) * gr.height;

		if (right_to_left)
			dp.x -= gw;

		if (gr.height > f->height) {
			/* glyphs are centered within each glyph box */
			gp.x = dp.x - (gr.width - gw) / 2;
			gp.y = dp.y - (gr.height - f->height) / 2;
		}
		else {
			/* glyphs are in the top left of each glyph box */
			gp = dp;
			gr.width = gw;
		}

		/* draw the glyph, clipping within the function */
		app_image_draw_glyph(dst, gp, sub->img, gr);

		if (! right_to_left)
			dp.x += gw;

		/* go to next character */
	}

	if (temp)
		app_del_font(f);

	return 1;
}
