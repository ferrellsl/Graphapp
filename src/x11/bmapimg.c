/*
 *  Image to Bitmap converter functions.
 *
 *  This file implements conversion from a platform-independent
 *  Image data format into the X-Windows Bitmap data format.
 *
 *  Platform: X-Windows.
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.43  2003/04/23  Added monochrome bitmap generation.
 *  Version: 3.45  2003/05/05  Added some conversions.
 *  Version: 3.58  2005/08/20  Fixed bugs. Added app_bitmap_to_image.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

/*
 *  Find a Pixmap format that will work:
 */

static void app_find_best_pixmap_format(Display *disp, int required_depth,
	int *actual_depth, int *bits_per_pixel, int *scanline_pad)
{
	int i, best, dist, count;
	XPixmapFormatValues *values;

	values = XListPixmapFormats(disp, &count);
	if (count == 0)
		return;
	best = 0;
	dist = 1000;

	for (i=0; i < count; i++) {
		if (values[i].depth == required_depth) {
			best = i;
			break;
		}
		else if (values[i].depth > required_depth) {
			if (values[i].depth - required_depth < dist) {
				dist = values[i].depth - required_depth;
				best = i;
			}
		}
	}

	*actual_depth = values[best].depth;
	*bits_per_pixel = values[best].bits_per_pixel;
	*scanline_pad = values[best].scanline_pad;
}

/*
 *  Functions which copy row data from an Image into an XImage scanline:
 */

static void app_put_8_1_1_MSB(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 1 significant bit, each packed
	 * into 1 bits-per-pixel, first pixel in most significant spot,
	 * i.e. abcdefgh ijklmnop ... */
	int i;

	for (i=0; i < width; i++,dst++)
	{
		*dst  = ((*src++    )& 0x80); if (++i == width) break;
		*dst |= ((*src++ >>1)& 0x40); if (++i == width) break;
		*dst |= ((*src++ >>2)& 0x20); if (++i == width) break;
		*dst |= ((*src++ >>3)& 0x10); if (++i == width) break;
		*dst |= ((*src++ >>4)& 0x08); if (++i == width) break;
		*dst |= ((*src++ >>5)& 0x04); if (++i == width) break;
		*dst |= ((*src++ >>6)& 0x02); if (++i == width) break;
		*dst |= ((*src++ >>7)& 0x01);
	}
}

static void app_put_8_1_1_LSB(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 1 significant bit, each packed
	 * into 1 bits-per-pixel, first pixel in least significant spot
	 * i.e. hgfedcba ponmlkji ... */
	int i;

	for (i=0; i < width; i++,dst++)
	{
		*dst  = ((*src++ >>7)& 0x01); if (++i == width) break;
		*dst |= ((*src++ >>6)& 0x02); if (++i == width) break;
		*dst |= ((*src++ >>5)& 0x04); if (++i == width) break;
		*dst |= ((*src++ >>4)& 0x08); if (++i == width) break;
		*dst |= ((*src++ >>3)& 0x10); if (++i == width) break;
		*dst |= ((*src++ >>2)& 0x20); if (++i == width) break;
		*dst |= ((*src++ >>1)& 0x40); if (++i == width) break;
		*dst |= ((*src++    )& 0x80);
	}
}

static void app_put_8_1_2_MSB(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 1 significant bit, each packed
	 * into 2 bits-per-pixel, first pixel in most significant spot
	 * i.e. 0a0b0c0d 0e0f0g0h ... */
	int i;

	for (i=0; i < width; i++,dst++)
	{
		*dst  = ((*src++ >>1)& 0x40); if (++i == width) break;
		*dst |= ((*src++ >>3)& 0x10); if (++i == width) break;
		*dst |= ((*src++ >>5)& 0x04); if (++i == width) break;
		*dst |= ((*src++ >>7)& 0x01);
	}
}

static void app_put_8_1_2_LSB(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 1 significant bit, each packed
	 * into 2 bits-per-pixel, first pixel in least significant spot
	 * i.e. 0d0c0b0a 0h0g0f0e ... */
	int i;

	for (i=0; i < width; i++,dst++)
	{
		*dst  = ((*src++ >>7)& 0x01); if (++i == width) break;
		*dst |= ((*src++ >>5)& 0x04); if (++i == width) break;
		*dst |= ((*src++ >>3)& 0x10); if (++i == width) break;
		*dst |= ((*src++ >>1)& 0x40);
	}
}

static void app_put_8_1_4_MSB(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 1 significant bit, each packed
	 * into 4 bits-per-pixel, first pixel in most significant spot
	 * i.e. 000a000b 000c000d ... */
	int i;

	for (i=0; i < width; i++,dst++)
	{
		*dst  = ((*src++ >>3)& 0x10); if (++i == width) break;
		*dst |= ((*src++ >>7)& 0x01);
	}
}

static void app_put_8_1_4_LSB(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 1 significant bit, each packed
	 * into 4 bits-per-pixel, first pixel in least significant spot
	 * i.e. 000b000a 000d000c ... */
	int i;

	for (i=0; i < width; i++,dst++)
	{
		*dst  = ((*src++ >>7)& 0x01); if (++i == width) break;
		*dst |= ((*src++ >>3)& 0x10);
	}
}

static void app_put_8_1_8(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 1 significant bit, each packed
	 * into 8 bits-per-pixel, i.e. 0000000a 0000000b ... */
	int i;

	for (i=0; i < width; i++,dst++)
	{
		*dst  = ((*src++ >>7)& 0x01);
	}
}

static void app_put_8_2_2_MSB(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 2 significant bits, each packed
	 * into 2 bits-per-pixel, first pixel in most significant spot,
	 * i.e. aabbccdd eeffgghh .... */
	int i;

	for (i=0; i < width; i++,dst++)
	{
		*dst  = ((*src++    )& 0xc0); if (++i == width) break;
		*dst |= ((*src++ >>2)& 0x30); if (++i == width) break;
		*dst |= ((*src++ >>4)& 0x0c); if (++i == width) break;
		*dst |= ((*src++ >>6)& 0x03);
	}
}

static void app_put_8_2_2_LSB(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 2 significant bits, each packed
	 * into 2 bits-per-pixel, first pixel in least significant spot,
	 * i.e. ddccbbaa hhggffee ... */
	int i;

	for (i=0; i < width; i++,dst++)
	{
		*dst  = ((*src++ >>6)& 0x03); if (++i == width) break;
		*dst |= ((*src++ >>4)& 0x0c); if (++i == width) break;
		*dst |= ((*src++ >>2)& 0x30); if (++i == width) break;
		*dst |= ((*src++    )& 0xc0);
	}
}

static void app_put_8_2_4_MSB(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 2 significant bits, each packed
	 * into 4 bits-per-pixel, first pixel in most significant spot,
	 * i.e. 00aa00bb 00cc00dd ... */
	int i;

	for (i=0; i < width; i++,dst++)
	{
		*dst  = ((*src++ >>2)& 0x30); if (++i == width) break;
		*dst |= ((*src++ >>6)& 0x03);
	}
}

static void app_put_8_2_4_LSB(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 2 significant bits, each packed
	 * into 4 bits-per-pixel, first pixel in least significant spot,
	 * i.e. 00bb00aa 00dd00cc ... */
	int i;

	for (i=0; i < width; i++,dst++)
	{
		*dst  = ((*src++ >>6)& 0x03); if (++i == width) break;
		*dst |= ((*src++ >>2)& 0x03);
	}
}

static void app_put_8_2_8(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 2 significant bits, each packed
	 * into 8 bits-per-pixel, i.e. 000000aa 000000bb ... */
	int i;

	for (i=0; i < width; i++,dst++)
	{
		*dst  = ((*src++ >>6)& 0x03);
	}
}

static void app_put_8_4_4_MSB(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 4 significant bits, each packed
	 * into 4 bits-per-pixel, first pixel in most significant spot,
	 * i.e. aaaabbbb ccccdddd ... */
	int i;

	for (i=0; i < width; i++,dst++)
	{
		*dst  = ((*src++    )& 0xf0); if (++i == width) break;
		*dst |= ((*src++ >>4)& 0x0f);
	}
}

static void app_put_8_4_4_LSB(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 4 significant bits, each packed
	 * into 4 bits-per-pixel, first pixel in least significant spot,
	 * i.e. bbbbaaaa ddddcccc ... */
	int i;

	for (i=0; i < width; i++,dst++)
	{
		*dst  = ((*src++ >>4)& 0x0f); if (++i == width) break;
		*dst |= ((*src++    )& 0xf0);
	}
}

static void app_put_8_4_8(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 4 significant bits, each packed
	 * into 8 bits-per-pixel, i.e. 0000aaaa 0000bbbb ... */
	int i;

	for (i=0; i < width; i++,dst++)
	{
		*dst  = ((*src++ >>4)& 0x0f);
	}
}

static void app_put_8_8_8(byte *dst, byte *src, int width)
{
	/* copy 8-bit pixvals into 8 significant bits, each packed
	 * into 8 bits-per-pixel, i.e. aaaaaaaa bbbbbbbb ... */

	memcpy(dst, src, width);
}

static void app_put_12_16_rgb_MSB(byte *dst, Colour *src, int width)
{
	/* copy 12-bit RGBs into 16 bits-per-pixel, top byte first
	 * 4 bits per colour i.e. 0000rrrr ggggbbbb */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		*dst  = ((src->red>>4)  & 0x0f); dst++;
		*dst  = ((src->green)   & 0xf0);
		*dst |= ((src->blue>>4) & 0x0f);
	}
}

static void app_put_12_16_rgb_LSB(byte *dst, Colour *src, int width)
{
	/* copy 12-bit RGBs into 16 bits-per-pixel, top byte last
	 * 4 bits per colour i.e. ggggbbbb 0000rrrr */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		*dst  = ((src->green)   & 0xf0);
		*dst |= ((src->blue>>4) & 0x0f); dst++;
		*dst  = ((src->red>>4)  & 0x0f);
	}
}

static void app_put_15_16_rgb_MSB(byte *dst, Colour *src, int width)
{
	/* copy 15-bit RGBs into 16 bits-per-pixel, top byte first
	 * 5 bits per colour i.e. 0rrrrrgg gggbbbbb */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		*dst  = ((src->red>>1)   & 0x7c);
		*dst |= ((src->green>>6) & 0x03); dst++;
		*dst  = ((src->green<<2) & 0xe0);
		*dst |= ((src->blue>>3)  & 0x1f);
	}
}

static void app_put_15_16_rgb_LSB(byte *dst, Colour *src, int width)
{
	/* copy 15-bit RGBs into 16 bits-per-pixel, top byte last
	 * 5 bits per colour i.e. gggbbbbb 0rrrrrgg */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		*dst  = ((src->green<<2) & 0xe0);
		*dst |= ((src->blue>>3)  & 0x1f); dst++;
		*dst  = ((src->red>>1)   & 0x7c);
		*dst |= ((src->green>>6) & 0x03);
	}
}

static void app_put_16_rggb_MSB(byte *dst, Colour *src, int width)
{
	/* copy 16-bit RGBs into 16 bits-per-pixel, top byte first
	 * 5 bits per colour but 6 for green, i.e. rrrrrggg gggbbbbb */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		*dst  = ((src->red)      & 0xf8);
		*dst |= ((src->green>>5) & 0x07); dst++;
		*dst  = ((src->green<<3) & 0xe0);
		*dst |= ((src->blue>>3)  & 0x1f);
	}
}

static void app_put_16_rggb_LSB(byte *dst, Colour *src, int width)
{
	/* copy 16-bit RGBs into 16 bits-per-pixel, top byte last
	 * 5 bits per colour but 6 for green, i.e. gggbbbbb rrrrrggg */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		*dst  = ((src->green<<3) & 0xe0);
		*dst |= ((src->blue>>3)  & 0x1f); dst++;
		*dst  = ((src->red)      & 0xf8);
		*dst |= ((src->green>>5) & 0x07);
	}
}

static void app_put_24_rgb_MSB(byte *dst, Colour *src, int width)
{
	/* copy 24-bit RGBs into 24 bits-per-pixel, top byte first
	 * 8 bits per colour, i.e. rrrrrrrr gggggggg bbbbbbbb */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		*dst  = (src->red);   dst++;
		*dst  = (src->green); dst++;
		*dst  = (src->blue);
	}
}

static void app_put_24_rgb_LSB(byte *dst, Colour *src, int width)
{
	/* copy 24-bit RGBs into 24 bits-per-pixel, top byte last
	 * 8 bits per colour, i.e. bbbbbbbb gggggggg rrrrrrrr */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		*dst  = (src->blue);  dst++;
		*dst  = (src->green); dst++;
		*dst  = (src->red);
	}
}

static void app_put_24_32_rgb_MSB(byte *dst, Colour *src, int width)
{
	/* copy 24-bit RGBs into 32 bits-per-pixel, top byte first
	 * 8 bits per colour, i.e. aaaaaaaa rrrrrrrr gggggggg bbbbbbbb */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		*dst  = (src->alpha); dst++;
		*dst  = (src->red);   dst++;
		*dst  = (src->green); dst++;
		*dst  = (src->blue);
	}
}

static void app_put_24_32_rgb_LSB(byte *dst, Colour *src, int width)
{
	/* copy 24-bit RGBs into 32 bits-per-pixel, top byte last
	 * 8 bits per colour, i.e. bbbbbbbb gggggggg rrrrrrrr aaaaaaaa */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		*dst  = (src->blue);  dst++;
		*dst  = (src->green); dst++;
		*dst  = (src->red);   dst++;
		*dst  = (src->alpha);
	}
}

static void app_put_32_rgb_MSB(byte *dst, Colour *src, int width)
{
	/* copy 32-bit ARGBs into 32 bits-per-pixel, top byte first
	 * 8 bits per colour, i.e. aaaaaaaa rrrrrrrr gggggggg bbbbbbbb */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		*dst  = (src->alpha); dst++;
		*dst  = (src->red);   dst++;
		*dst  = (src->green); dst++;
		*dst  = (src->blue);
	}
}

static void app_put_32_rgb_LSB(byte *dst, Colour *src, int width)
{
	/* copy 32-bit ARGBs into 32 bits-per-pixel, top byte last
	 * 8 bits per colour, i.e. bbbbbbbb gggggggg rrrrrrrr aaaaaaaa */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		*dst  = (src->blue);  dst++;
		*dst  = (src->green); dst++;
		*dst  = (src->red);   dst++;
		*dst  = (src->alpha);
	}
}

/*
 *  Declare a data structure which 'knows' about the above
 *  fast blitting functions and allows quick searching for
 *  an appropriate function to use:
 */

typedef void (*AppBltFunc)(void *dst, void *src, int width);

typedef struct {
	int            depth, bpp, order, use_masks;
	unsigned long  rmask, gmask, bmask;
	void *         fn;
} AppBltFuncStruct;

static AppBltFuncStruct app_put_func_table[] =
{
 /* 8-bit pixvals to varying depth colour maps */

 { 1, 1,1,0,0UL,0UL,0UL,app_put_8_1_1_MSB},
 { 1, 1,0,0,0UL,0UL,0UL,app_put_8_1_1_LSB},

 { 1, 2,1,0,0UL,0UL,0UL,app_put_8_1_2_MSB},
 { 1, 2,0,0,0UL,0UL,0UL,app_put_8_1_2_LSB},

 { 1, 4,1,0,0UL,0UL,0UL,app_put_8_1_4_MSB},
 { 1, 4,0,0,0UL,0UL,0UL,app_put_8_1_4_LSB},

 { 1, 8,1,0,0UL,0UL,0UL,app_put_8_1_8},
 { 1, 8,0,0,0UL,0UL,0UL,app_put_8_1_8},

 { 2, 2,1,0,0UL,0UL,0UL,app_put_8_2_2_MSB},
 { 2, 2,0,0,0UL,0UL,0UL,app_put_8_2_2_LSB},

 { 2, 4,1,0,0UL,0UL,0UL,app_put_8_2_4_MSB},
 { 2, 4,0,0,0UL,0UL,0UL,app_put_8_2_4_LSB},

 { 2, 8,1,0,0UL,0UL,0UL,app_put_8_2_8},
 { 2, 8,0,0,0UL,0UL,0UL,app_put_8_2_8},

 { 4, 4,1,0,0UL,0UL,0UL,app_put_8_4_4_MSB},
 { 4, 4,0,0,0UL,0UL,0UL,app_put_8_4_4_LSB},

 { 4, 8,1,0,0UL,0UL,0UL,app_put_8_4_8},
 { 4, 8,0,0,0UL,0UL,0UL,app_put_8_4_8},

 { 8, 8,1,0,0UL,0UL,0UL,app_put_8_8_8},
 { 8, 8,0,0,0UL,0UL,0UL,app_put_8_8_8},

 /* 32-bit rgbs to true colour pixels using component masks */

 {12,16,1,1,0x0000f00UL,0x00000f0UL,0x000000fUL,app_put_12_16_rgb_MSB},
 {12,16,0,1,0x0000f00UL,0x00000f0UL,0x000000fUL,app_put_12_16_rgb_LSB},

 {15,16,1,1,0x0007c00UL,0x00003e0UL,0x000001fUL,app_put_15_16_rgb_MSB},
 {15,16,0,1,0x0007c00UL,0x00003e0UL,0x000001fUL,app_put_15_16_rgb_LSB},

 {16,16,1,1,0x000f800UL,0x00007e0UL,0x000001fUL,app_put_16_rggb_MSB},
 {16,16,0,1,0x000f800UL,0x00007e0UL,0x000001fUL,app_put_16_rggb_LSB},

 {24,24,1,1,0x0ff0000UL,0x000ff00UL,0x00000ffUL,app_put_24_rgb_MSB},
 {24,24,0,1,0x0ff0000UL,0x000ff00UL,0x00000ffUL,app_put_24_rgb_LSB},

 {24,32,1,1,0x0ff0000UL,0x000ff00UL,0x00000ffUL,app_put_24_32_rgb_MSB},
 {24,32,0,1,0x0ff0000UL,0x000ff00UL,0x00000ffUL,app_put_24_32_rgb_LSB},

 {32,32,1,1,0x0ff0000UL,0x000ff00UL,0x00000ffUL,app_put_32_rgb_MSB},
 {32,32,0,1,0x0ff0000UL,0x000ff00UL,0x00000ffUL,app_put_32_rgb_LSB},

 /* converse cases */

 {12,16,1,1,0x000000fUL,0x00000f0UL,0x0000f00UL,app_put_12_16_rgb_MSB},
 {12,16,0,1,0x000000fUL,0x00000f0UL,0x0000f00UL,app_put_12_16_rgb_LSB},

 {15,16,1,1,0x000001fUL,0x00003e0UL,0x0007c00UL,app_put_15_16_rgb_MSB},
 {15,16,0,1,0x000001fUL,0x00003e0UL,0x0007c00UL,app_put_15_16_rgb_LSB},

 {16,16,1,1,0x000001fUL,0x00007e0UL,0x000f800UL,app_put_16_rggb_MSB},
 {16,16,0,1,0x000001fUL,0x00007e0UL,0x000f800UL,app_put_16_rggb_LSB},

 {24,24,1,1,0x00000ffUL,0x000ff00UL,0x0ff0000UL,app_put_24_rgb_MSB},
 {24,24,0,1,0x00000ffUL,0x000ff00UL,0x0ff0000UL,app_put_24_rgb_LSB},

 {24,32,1,1,0x00000ffUL,0x000ff00UL,0x0ff0000UL,app_put_24_32_rgb_MSB},
 {24,32,0,1,0x00000ffUL,0x000ff00UL,0x0ff0000UL,app_put_24_32_rgb_LSB},

 {32,32,1,1,0x00000ffUL,0x000ff00UL,0x0ff0000UL,app_put_32_rgb_MSB},
 {32,32,0,1,0x00000ffUL,0x000ff00UL,0x0ff0000UL,app_put_32_rgb_LSB}
};

static int app_find_put_func(int depth,
	int bits_per_pixel, int order, int use_masks,
	unsigned long red_mask, unsigned long green_mask,
	unsigned long blue_mask)
{
	int i, nelem;

	nelem = sizeof(app_put_func_table)/sizeof(app_put_func_table[0]);

	for (i=0; i < nelem; i++) {
		if ((depth == app_put_func_table[i].depth) &&
		    (bits_per_pixel == app_put_func_table[i].bpp) &&
		    (order == app_put_func_table[i].order) &&
		    (use_masks == app_put_func_table[i].use_masks))
		{
			if (! use_masks) {
				return i+1;
			}
			else if
			   ((red_mask   == app_put_func_table[i].rmask) &&
			    (green_mask == app_put_func_table[i].gmask) &&
			    (blue_mask  == app_put_func_table[i].bmask))
			{
				return i+1;
			}
		}
	}
	return 0;	/* no matching row function found */
}

/*
 *  Determine if a display is using TrueColor or DirectColor.
 *  A TrueColor display uses the pixel value to directly determine
 *  the RGB intensities (e.g. 24-bit displays). DirectColor is
 *  similar, but the actual palette of colours is variable.
 *  Return zero if the display is using a colormap.
 */
int app_is_true_colour_display(Display *disp)
{
	Visual *vis;
	int vis_class;

	vis = DefaultVisual(disp, DefaultScreen(disp));
	if (! vis)
		return 0;

	#if defined(__cplusplus) || defined(c_plusplus)
         vis_class = vis->c_class;
	#else
         vis_class = vis->class;
	#endif

	if ((vis_class == TrueColor) || (vis_class == DirectColor))
		return 1;
	return 0;
}

/*
 *  If we cannot find an appropriate fast row blitting function,
 *  don't give up! We can use this slow, but correct, method
 *  of drawing the bits:
 */
static void app_draw_image_slowly(Display *disp, XID xid, GC gc,
	Graphics *g, int width, int height, Image *img)
{
	int x, y;
	int value;
	Colour col;

	if (img->depth == 8) {
	  for (y=0; y < height; y++) {
		for (x=0; x < width; x++) {
			value = img->data8[y][x];
			col = img->cmap[value];
			app_set_rgb(g, col);
			XFillRectangle(disp, xid, gc, x, y, 1, 1);
		}
	  }
	}
	else if (img->depth == 32) {
	  for (y=0; y < height; y++) {
		for (x=0; x < width; x++) {
			col = img->data32[y][x];
			app_set_rgb(g, col);
			XFillRectangle(disp, xid, gc, x, y, 1, 1);
		}
	  }
	}
}

/*
 *  Return a 'clipmask' Pixmap. This is a Pixmap of depth 1,
 *  with 1's where the image is opaque, 0's where it is transparent.
 *  We just make it completely transparent here.
 *  Return None on failure.
 */
XID app_new_clipmask(Display *disp, int width, int height)
{
	Pixmap bmap;
	GC gc;

	bmap = XCreatePixmap(disp, DefaultRootWindow(disp), width, height, 1);

	if (! app_bitmap_created(disp, bmap))
		return None;

	gc = XCreateGC(disp, bmap, 0, NULL);
	if (! gc) {
		XFreePixmap(disp, bmap);
		return None;
	}

	XSetForeground(disp, gc, 0); /* fill with zeros => transparent */
	XFillRectangle(disp, bmap, gc, 0, 0, width, height);
	XFreeGC(disp, gc);

	return bmap;
}

/*
 *  Copy transparent pixels to 0-bits, opaque to 1-bits.
 */
static void app_put_8_T_1(byte *dst, byte *src, int width, Colour *cmap,
	int order)
{
	int x, shift, value;
	Colour col;

	shift = 7;
	for (x=0; x < width; x++) {
		value = src[x];
		col = cmap[value];
		if (col.alpha <= 0x7F) {
			if (order == MSBFirst)
				*dst |= (1<<shift);
			else
				*dst |= (1<<(7-shift));
		}
		if (--shift < 0) {
			shift = 7;
			dst++;
		}
	}
}

static void app_put_32_T_1(byte *dst, Colour *src, int width, int order)
{
	int x, shift;
	Colour col;

	shift = 7;
	for (x=0; x < width; x++) {
		col = src[x];
		if (col.alpha <= 0x7F) {
			if (order == MSBFirst)
				*dst |= (1<<shift);
			else
				*dst |= (1<<(7-shift));
		}
		if (--shift < 0) {
			shift = 7;
			dst++;
		}
	}
}

/*
 *  Create a clipmask Pixmap.
 *
 *  This creates a clipmask that has 0's wherever the source
 *  image is transparent, 1's elsewhere. It checks that there is at
 *  least one transparent pixel in the image first, and if not,
 *  returns None which is equivalent to having no clipping.
 */

XID app_image_to_clipmask(App *app, Image *img)
{
	int width, height, depth, rowbytes;
	int y, bit_order;
	int bits_per_pixel, scanline_pad;
	byte *data;
	XImage *xi;
	Display *disp;
	GC gc;
	Pixmap clipmask;

	if (! img)
		return None;
	if ((img->depth != 8) && (img->depth != 32))
		return None;
	if (! app_image_has_transparent_pixels(img))
		return None;

	disp = app_extra(app)->display;

	width = img->width;
	height = img->height;

	app_find_best_pixmap_format(disp, 1, &depth,
		&bits_per_pixel, &scanline_pad);

	if ((depth != 1) || (bits_per_pixel != 1))
		return None;

	xi = XCreateImage(disp,
		DefaultVisual(disp, DefaultScreen(disp)),
		1, ZPixmap, 0, NULL, width, height, scanline_pad, 0);
	if (! xi) 
		return None;

	rowbytes = xi->bytes_per_line;
	bit_order = xi->bitmap_bit_order;

	data = app_zero_alloc(height * rowbytes);
	if (! data) {
		XDestroyImage(xi);
		return None;
	}

	if (img->depth == 8)
	{
		/* assign pixels into data array */
		for (y=0; y < height; y++)
			app_put_8_T_1(&data[y*rowbytes],
				img->data8[y], width,
				img->cmap, bit_order);
	}
	else if (img->depth == 32)
	{
		/* assign pixels into data array */
		for (y=0; y < height; y++)
			app_put_32_T_1(&data[y*rowbytes],
				img->data32[y], width, bit_order);
	}

	clipmask = app_new_clipmask(disp, width, height);
	if (clipmask == None) {
		app_free(data);
		XDestroyImage(xi);
		return None;
	}

	gc = XCreateGC(disp, clipmask, 0, NULL);
	if (! gc) {
		XFreePixmap(disp, clipmask);
		app_free(data);
		XDestroyImage(xi);
		return None;
	}

	xi->data = data;
	XPutImage(disp, clipmask, gc, xi, 0,0,0,0, width, height);
	xi->data = NULL;

	XFreeGC(disp, gc);
	app_free(data);
	XDestroyImage(xi);

	return clipmask;
}

/*
 *  Create a bitmap from an image.
 *  The bitmap will be a Pixmap with the bits of the image,
 *  and there may also be a clipmask Pixmap if the image
 *  contains any transparent pixels.
 */
enum RenderingTechnique {
	SlowRender               = 0,
	Fast8BitToSystemPalette  = 1,
	Fast32BitToSystemPalette = 2,
	Fast8BitToWindowPalette  = 3,
	Fast32BitToWindowPalette = 4,
	Fast8BitToDirectColour   = 5,
	Fast32BitToDirectColour  = 6
};

Bitmap *app_image_to_bitmap(Window *win, Image *img)
{
	int width, height, depth, rowbytes;
	int x, y, value;
	int bits_per_pixel, scanline_pad, order;
	int technique;
	AppBltFunc blt_func;
	Display *disp;
	XImage *xi;
	Bitmap *b;
	Graphics *g;
	byte *data = NULL;
	byte *translation = NULL;
	byte *byte_row = NULL;
	Colour *rgb_row = NULL;

	technique = SlowRender;
	disp = app_extra(win->app)->display;
	width = img->width;
	height = img->height;

	b = app_new_bitmap(win, width, height);
	if (! b) {
		/* could not create the bitmap! */
		return NULL;
	}

	g = app_get_bitmap_graphics(b);
	if (! g) {
		/* cannot draw the image if we can't get a GC! */
		app_del_bitmap(b);
		return NULL;
	}

	rowbytes = 0;
	depth = DefaultDepth(disp, DefaultScreen(disp));
	app_find_best_pixmap_format(disp, depth, &depth,
		&bits_per_pixel, &scanline_pad);

	xi = XCreateImage(disp,
		DefaultVisual(disp, DefaultScreen(disp)), depth,
		ZPixmap, 0, NULL, width, height, scanline_pad, 0);

	if (xi) {
		/* success */
		rowbytes = xi->bytes_per_line;
		depth = xi->depth;
		bits_per_pixel = xi->bits_per_pixel;

		if (depth <= 8)
			order = ((xi->bitmap_bit_order==MSBFirst) ?1:0);
		else
			order = ((xi->byte_order==MSBFirst) ?1:0);

		if (win_extra(win)->is_paletted) {
			int blit = app_find_put_func(depth,
				bits_per_pixel, order,
				0, 0UL, 0UL, 0UL);
			if (blit)
				blt_func = app_put_func_table[blit-1].fn;
			else
				blt_func = NULL;
			bitmap_extra(b)->blitter = blit;
		}
		else {
			int blit = app_find_put_func(depth,
				bits_per_pixel, order, 1, xi->red_mask,
				xi->green_mask, xi->blue_mask);
			if (blit)
				blt_func = app_put_func_table[blit-1].fn;
			else
				blt_func = NULL;
			bitmap_extra(b)->blitter = blit;
		}

		if (blt_func) {
			data = app_alloc(height * rowbytes);
			if (! data)
				technique = SlowRender;
		}
	}
	else {
		/* failed to allocate the XImage */
		blt_func = NULL;
	}

	if (blt_func == NULL) {
		/* could not find a fast row blitting function */
		/* fall back to a slow, correct drawing method */
		technique = SlowRender;
	}
	else if ((img->depth == 8) && (! win_extra(win)->is_paletted))
	{
		technique = Fast8BitToDirectColour;
		rgb_row = app_alloc(width * sizeof(Colour));
		if (! rgb_row)
			technique = SlowRender;
	}
	else if ((img->depth == 32) && (! win_extra(win)->is_paletted))
	{
		technique = Fast32BitToDirectColour;
	}
	else if ((img->depth == 8) && (win->pal))
	{
		technique = Fast8BitToWindowPalette;
		translation = app_alloc(img->cmap_size);
		byte_row = app_alloc(width);
		if ((! translation) || (! byte_row))
			technique = SlowRender;
	}
	else if ((img->depth == 32) && (win->pal))
	{
		technique = Fast32BitToWindowPalette;
		byte_row = app_alloc(width);
		if (! byte_row)
			technique = SlowRender;
	}
	else if (img->depth == 8)
	{
		technique = Fast8BitToSystemPalette;
		if (! app_extra(win->app)->pal)
			technique = SlowRender;
		else {
			translation = app_alloc(img->cmap_size);
			byte_row = app_alloc(width);
			if ((! translation) || (! byte_row))
				technique = SlowRender;
		}
	}
	else if (img->depth == 32)
	{
		technique = Fast32BitToSystemPalette;
		if (! app_extra(win->app)->pal)
			technique = SlowRender;
		else {
			byte_row = app_alloc(width);
			if (! byte_row)
				technique = SlowRender;
		}
	}
	else {
		technique = SlowRender;
	}

	/* now render using the correct technique */

	if (technique == Fast8BitToDirectColour)
	{
		/* expand image's pixvals to image's cmap colours */
		/* then blit from rgb_row array into data array */
		for (y=0; y < height; y++) {
			for (x=0; x < width; x++) {
				value = img->data8[y][x];
				rgb_row[x] = img->cmap[value];
			}
			blt_func(&data[y*rowbytes], rgb_row, width);
		}
	}
	else if (technique == Fast32BitToDirectColour)
	{
		/* just call the row blitting function on each row */
		for (y=0; y < height; y++) {
			blt_func(&data[y*rowbytes], img->data32[y], width);
		}
	}
	else if (technique == Fast8BitToWindowPalette)
	{
		/* translate img->cmap to win->pal once */
		/* then use it to translate each row's 8-bit values */

		app_palette_translation(win->pal, translation,
			img->cmap_size, img->cmap);

		/* assign pixels into data array */
		for (y=0; y < height; y++) {
			for (x=0; x < width; x++) {
				value = img->data8[y][x];
				byte_row[x] = translation[value];
			}
			blt_func(&data[y*rowbytes], byte_row, width);
		}
	}
	else if (technique == Fast32BitToWindowPalette)
	{
		/* translate each row of rgbs to win->pal */
		for (y=0; y < height; y++) {
			app_palette_translation(win->pal, byte_row,
				width, img->data32[y]);
			blt_func(&data[y*rowbytes], byte_row, width);
		}
	}
	else if (technique == Fast8BitToSystemPalette)
	{
		/* translate from img->cmap to win->app->pal once */
		/* then use it to translate each row's 8-bit values */

		app_palette_translation(app_extra(win->app)->pal,
			translation, img->cmap_size, img->cmap);

		/* assign pixels into data array */
		for (y=0; y < height; y++) {
			for (x=0; x < width; x++) {
				value = img->data8[y][x];
				byte_row[x] = translation[value];
			}
			blt_func(&data[y*rowbytes], byte_row, width);
		}
	}
	else if (technique == Fast32BitToSystemPalette)
	{
		/* translate each row of rgbs to win->app->pal */
		for (y=0; y < height; y++) {
			app_palette_translation(app_extra(win->app)->pal,
				byte_row, width, img->data32[y]);

			blt_func(&data[y*rowbytes], byte_row, width);
		}
	}
	else {
		/* just use the slow way of drawing */
		app_draw_image_slowly(disp, bitmap_extra(b)->handle,
			graphics_extra(g)->gc, g, width, height, img);
	}

	/* discard temporary data structures */

	if (byte_row)
		app_free(byte_row);
	if (rgb_row)
		app_free(rgb_row);
	if (translation)
		app_free(translation);

	/* copy bits over, if we haven't already done it the slow way */

	if (xi) {
		if (technique != SlowRender) {
			if (data) {
				xi->data = data;
				XPutImage(disp, bitmap_extra(b)->handle,
					graphics_extra(g)->gc, xi,
					0,0,0,0, width, height);
				xi->data = NULL;
			}
		}
		XDestroyImage(xi);
	}

	/* clean up and produce the transparency mask, if needed */

	if (data)
		app_free(data);
	app_del_graphics(g);
	if (bitmap_extra(b)->clipmask != None)
		XFreePixmap(app_extra(win->app)->display,
			bitmap_extra(b)->clipmask);
	bitmap_extra(b)->clipmask = app_image_to_clipmask(win->app, img);
	return b;
}

/*
 *  Generate monochrome (black and white) Bitmaps:
 *  This code generates a Bitmap with depth 1 and a clipmask with depth 1.
 *  The Bitmap is generated using a slow but correct algorithm,
 *  which accumulates adjacent colours and draws each row in pieces.
 *  The clipmask is always present even if the image is fully opaque.
 */
static Colour app_monochrome(Colour c)
{
	int min, max, g, b;

	max = min = c.red;
	g = c.green;
	if      (g < min) min = g;
	else if (g > max) max = g;
	b = c.blue;
	if      (b < min) min = b;
	else if (b > max) max = b;

	if (min > 0xE0) 	c.red = c.green = c.blue = 255;
	else if (max < 0x10)	c.red = c.green = c.blue = 0;
	else if (max < 0x60)	c.red = c.green = c.blue = 0;
	else if (max < 0xD0)	c.red = c.green = c.blue = 0;
	else			c.red = c.green = c.blue = 255;

	return c;
}

static Colour app_get_monochrome_pixel_at(Image *img, int x, int y)
{
	if (img->depth == 8)
		return app_monochrome(img->cmap[img->data8[y][x]]);
	else
		return app_monochrome(img->data32[y][x]);
}

Bitmap *app_image_to_monochrome_bitmap(Window *win, Image *img)
{
	Bitmap *bmp;
	int x, y, xstart, xwidth;
	Colour col, prev;
	Display *disp;
	XID dst_id, dst_mask;
	GC dst_gc, dst_mask_gc = 0;

	bmp = app_new_monochrome_bitmap(win, img->width, img->height);
	if (bmp == NULL)
		return NULL;

	disp = app_extra(bmp->win->app)->display;
	dst_id = bitmap_extra(bmp)->handle;
	dst_mask = bitmap_extra(bmp)->clipmask;
	dst_gc = XCreateGC(disp, dst_id, 0, NULL);
	if (! dst_gc) {
		app_del_bitmap(bmp);
		return NULL;
	}
	dst_mask_gc = XCreateGC(disp, dst_mask, 0, NULL);
	if (! dst_mask_gc) {
		XFreeGC(disp, dst_gc);
		app_del_bitmap(bmp);
		return NULL;
	}
	XSetForeground(disp, dst_gc, 0);
	XFillRectangle(disp, dst_id, dst_gc, 0, 0, img->width, img->height);
	XSetForeground(disp, dst_gc, ~0);
	XSetForeground(disp, dst_mask_gc, ~0);

	for (y=0; y < img->height; y++) {
		for (x=xstart=xwidth=0; x <= img->width; x++) {
			if (x < img->width)
				col = app_get_monochrome_pixel_at(img,x,y);
			if (x == img->width) {
				/* fall through to output the row */
			}
			else if (xwidth == 0) {
				/* start output row with this pixel */
				xwidth = 1;
				prev = col;
				continue;
			}
			else if (app_colours_equal(prev, col)) {
				/* add this pixel to the output row */
				xwidth++;
				continue;
			}
			/* output the row since this colour differs */
			if (prev.blue <= 0x7F)
				XFillRectangle(disp, dst_id, dst_gc,
						xstart, y, xwidth, 1);
			if (prev.alpha <= 0x7F)
				XFillRectangle(disp, dst_mask, dst_mask_gc,
						xstart, y, xwidth, 1);
			xstart = x;
			xwidth = 1;
			prev = col;
		}
	}
	XFreeGC(disp, dst_mask_gc);
	XFreeGC(disp, dst_gc);

	return bmp;
}


/*
 *  Functions which copy row data from an XImage into an Image scanline.
 */

/*
 *  These 8-bit expansions are paletted, so we keep the pixel values.
 */

static void app_get_8_1_1_MSB(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 1 significant bit, each packed
	 * into 1 bits-per-pixel, first pixel in most significant spot,
	 * i.e. abcdefgh ijklmnop ... => 0000000a 0000000b ... */
	int i;

	for (i=0; i < width; i++,src++)
	{
		*dst++ = (*src & 0x80)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x40)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x20)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x10)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x08)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x04)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x02)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x01)? 0x01 : 0;
	}
}

static void app_get_8_1_1_LSB(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 1 significant bit, each packed
	 * into 1 bits-per-pixel, first pixel in least significant spot
	 * i.e. hgfedcba ponmlkji ... => 0000000a 0000000b ... */
	int i;

	for (i=0; i < width; i++,src++)
	{
		*dst++ = (*src & 0x01)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x02)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x04)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x08)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x10)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x20)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x40)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x80)? 0x01 : 0;
	}
}

static void app_get_8_1_2_MSB(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 1 significant bit, each packed
	 * into 2 bits-per-pixel, first pixel in most significant spot
	 * i.e. 0a0b0c0d 0e0f0g0h ... => 0000000a 0000000b ... */
	int i;

	for (i=0; i < width; i++,src++)
	{
		*dst++ = (*src & 0x40)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x10)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x04)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x01)? 0x01 : 0;
	}
}

static void app_get_8_1_2_LSB(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 1 significant bit, each packed
	 * into 2 bits-per-pixel, first pixel in least significant spot
	 * i.e. 0d0c0b0a 0h0g0f0e ... => 0000000a 0000000b ... */
	int i;

	for (i=0; i < width; i++,src++)
	{
		*dst++ = (*src & 0x01)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x04)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x10)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x40)? 0x01 : 0;
	}
}

static void app_get_8_1_4_MSB(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 1 significant bit, each packed
	 * into 4 bits-per-pixel, first pixel in most significant spot
	 * i.e. 000a000b 000c000d ... => 0000000a 0000000b ... */
	int i;

	for (i=0; i < width; i++,src++)
	{
		*dst++ = (*src & 0x10)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x01)? 0x01 : 0;
	}
}

static void app_get_8_1_4_LSB(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 1 significant bit, each packed
	 * into 4 bits-per-pixel, first pixel in least significant spot
	 * i.e. 000b000a 000d000c ... => 0000000a 0000000b ... */
	int i;

	for (i=0; i < width; i++,src++)
	{
		*dst++ = (*src & 0x01)? 0x01 : 0; if (++i == width) break;
		*dst++ = (*src & 0x10)? 0x01 : 0;
	}
}

static void app_get_8_1_8(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 1 significant bit, each packed
	 * into 8 bits-per-pixel, i.e. 0000000a 0000000b ... */
	int i;

	for (i=0; i < width; i++,src++)
	{
		*dst++ = (*src & 0x01)? 0x01 : 0;
	}
}

static void app_get_8_2_2_MSB(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 2 significant bits, each packed
	 * into 2 bits-per-pixel, first pixel in most significant spot,
	 * i.e. aabbccdd eeffgghh .... => 000000aa 000000bb */
	int i;

	for (i=0; i < width; i++,src++)
	{
		*dst++ = (*src >> 6) & 0x03;
		if (++i == width) break;

		*dst++ = (*src >> 4) & 0x03;
		if (++i == width) break;

		*dst++ = (*src >> 2) & 0x03;
		if (++i == width) break;

		*dst++ = (*src     ) & 0x03;
	}
}

static void app_get_8_2_2_LSB(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 2 significant bits, each packed
	 * into 2 bits-per-pixel, first pixel in least significant spot,
	 * i.e. ddccbbaa hhggffee ... => 000000aa 000000bb */
	int i;

	for (i=0; i < width; i++,src++)
	{
		*dst++ = (*src     ) & 0x03;
		if (++i == width) break;

		*dst++ = (*src >> 2) & 0x03;
		if (++i == width) break;

		*dst++ = (*src >> 4) & 0x03;
		if (++i == width) break;

		*dst++ = (*src >> 6) & 0x03;
	}
}

static void app_get_8_2_4_MSB(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 2 significant bits, each packed
	 * into 4 bits-per-pixel, first pixel in most significant spot,
	 * i.e. 00aa00bb 00cc00dd ... => 000000aa 000000bb */
	int i;

	for (i=0; i < width; i++,src++)
	{
		*dst++ = (*src >> 4) & 0x03;
		if (++i == width) break;

		*dst++ = (*src     ) & 0x03;
	}
}

static void app_get_8_2_4_LSB(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 2 significant bits, each packed
	 * into 4 bits-per-pixel, first pixel in least significant spot,
	 * i.e. 00bb00aa 00dd00cc ... => 000000aa 000000bb */
	int i;

	for (i=0; i < width; i++,src++)
	{
		*dst++ = (*src     ) & 0x03;
		if (++i == width) break;

		*dst++ = (*src >> 4) & 0x03;
	}
}

static void app_get_8_2_8(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 2 significant bits, each packed
	 * into 8 bits-per-pixel, i.e. 000000aa 000000bb ... */
	int i;

	for (i=0; i < width; i++,src++)
	{
		*dst++ = (*src     ) & 0x03;
	}
}

static void app_get_8_4_4_MSB(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 4 significant bits, each packed
	 * into 4 bits-per-pixel, first pixel in most significant spot,
	 * i.e. aaaabbbb ccccdddd ... => 0000aaaa 0000bbbb */
	int i;

	for (i=0; i < width; i++,src++)
	{
		*dst++ = (*src >> 4) & 0x0f;
		if (++i == width) break;

		*dst++ = (*src     ) & 0x03;
	}
}

static void app_get_8_4_4_LSB(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 4 significant bits, each packed
	 * into 4 bits-per-pixel, first pixel in least significant spot,
	 * i.e. bbbbaaaa ddddcccc ... => 0000aaaa 0000bbbb */
	int i;

	for (i=0; i < width; i++,src++)
	{
		*dst++ = (*src     ) & 0x0f;
		if (++i == width) break;

		*dst++ = (*src >> 4) & 0x03;
	}
}

static void app_get_8_4_8(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 4 significant bits, each packed
	 * into 8 bits-per-pixel, i.e. 0000aaaa 0000bbbb ... */
	int i;

	for (i=0; i < width; i++,src++)
	{
		*dst++ = (*src     ) & 0x0f;
	}
}

static void app_get_8_8_8(byte *dst, byte *src, int width)
{
	/* make 8-bit pixvals from 8 significant bits, each packed
	 * into 8 bits-per-pixel, i.e. aaaaaaaa bbbbbbbb ... */

	memcpy(dst, src, width);
}

/*
 *  The general trick where bits are lacking is to repeat the pattern,
 *  e.g. the 4-bit pattern 0011 becomes the 8-bit pixval 00110011.
 *  Thus we map 0 -> 00000000, 1-> 11111111, 01 -> 01010101, etc.
 *  This evenly spaces out the created values within each channel's space.
 */

static void app_get_12_16_rgb_MSB(Colour *dst, byte *src, int width)
{
	/* make 12-bit RGBs from 16 bits-per-pixel, top byte first
	 * 4 bits per colour i.e. 0000rrrr ggggbbbb */
	int i;
	byte bits;

	for (i=0; i < width; i++,dst++,src++)
	{
		dst->alpha = 0;

		bits = (*src++   ) & 0x0f;
		dst->red   = (bits << 4) | bits;

		bits = (*src >> 4) & 0x0f;
		dst->green = (bits << 4) | bits;

		bits = (*src     ) & 0x0f;
		dst->blue  = (bits << 4) | bits;
	}
}

static void app_get_12_16_rgb_LSB(Colour *dst, byte *src, int width)
{
	/* make 12-bit RGBs from 16 bits-per-pixel, top byte last
	 * 4 bits per colour i.e. ggggbbbb 0000rrrr */
	int i;
	byte bits;

	for (i=0; i < width; i++,dst++,src++)
	{
		bits = (*src >> 4) & 0x0f;
		dst->green = (bits << 4) | bits;

		bits = (*src++   ) & 0x0f;
		dst->blue  = (bits << 4) | bits;

		bits = (*src     ) & 0x0f;
		dst->red   = (bits << 4) | bits;

		dst->alpha = 0;
	}
}

static void app_get_15_16_rgb_MSB(Colour *dst, byte *src, int width)
{
	/* make 15-bit RGBs from 16 bits-per-pixel, top byte first
	 * 5 bits per colour i.e. 0rrrrrgg gggbbbbb */
	int i;
	byte bits;

	for (i=0; i < width; i++,dst++,src++)
	{
		dst->alpha = 0;

		bits  =  (*src >> 2) & 0x1f;
		dst->red   = (bits << 3) | (bits >> 2);

		bits  = ((*src++   ) & 0x03) << 3; /* low 2 bits */
		bits |= ((*src >> 5) & 0x07); /* OR high 3 bits */
		dst->green = (bits << 3) | (bits >> 2);

		bits  =  (*src     ) & 0x1f;
		dst->blue  = (bits << 3) | (bits >> 2);
	}
}

static void app_get_15_16_rgb_LSB(Colour *dst, byte *src, int width)
{
	/* make 15-bit RGBs from 16 bits-per-pixel, top byte last
	 * 5 bits per colour i.e. gggbbbbb 0rrrrrgg */
	int i;
	byte bits;

	for (i=0; i < width; i++,dst++,src++)
	{
		bits  =  (*src     ) & 0x1f;
		dst->blue  = (bits << 3) | (bits >> 2);

		bits  = ((*src++ >> 5) & 0x07); /* high 3 bits */
		bits |= ((*src   ) & 0x03) << 3; /* OR low 2 bits */
		dst->green = (bits << 3) | (bits >> 2);

		bits  =  (*src >> 2) & 0x1f;
		dst->red   = (bits << 3) | (bits >> 2);

		dst->alpha = 0;
	}
}

static void app_get_16_rggb_MSB(Colour *dst, byte *src, int width)
{
	/* make 16-bit RGBs from 16 bits-per-pixel, top byte first
	 * 5 bits per colour but 6 for green, i.e. rrrrrggg gggbbbbb */
	int i;
	byte bits;

	for (i=0; i < width; i++,dst++,src++)
	{
		dst->alpha = 0;

		bits  =  (*src >> 3) & 0x1f;
		dst->red   = (bits << 3) | (bits >> 2);

		bits  = ((*src++   ) & 0x07) << 3; /* low 3 bits */
		bits |= ((*src >> 5) & 0x07); /* OR high 3 bits */
		dst->green = (bits << 3) | (bits >> 2);

		bits  =  (*src     ) & 0x1f;
		dst->blue  = (bits << 3) | (bits >> 2);
	}
}

static void app_get_16_rggb_LSB(Colour *dst, byte *src, int width)
{
	/* make 16-bit RGBs from 16 bits-per-pixel, top byte last
	 * 5 bits per colour but 6 for green, i.e. gggbbbbb rrrrrggg */
	int i;
	byte bits;

	for (i=0; i < width; i++,dst++,src++)
	{
		bits  =  (*src     ) & 0x1f;
		dst->blue  = (bits << 3) | (bits >> 2);

		bits  = ((*src++ >> 5) & 0x07); /* high 3 bits */
		bits |= ((*src   ) & 0x07) << 3; /* OR low 3 bits */
		dst->green = (bits << 3) | (bits >> 2);

		bits  =  (*src >> 3) & 0x1f;
		dst->red   = (bits << 3) | (bits >> 2);

		dst->alpha = 0;
	}
}

static void app_get_24_rgb_MSB(Colour *dst, byte *src, int width)
{
	/* make 24-bit RGBs from 24 bits-per-pixel, top byte first
	 * 8 bits per colour, i.e. rrrrrrrr gggggggg bbbbbbbb */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		dst->alpha = 0;
		dst->red   = *src++;
		dst->green = *src++;
		dst->blue  = *src;
	}
}

static void app_get_24_rgb_LSB(Colour *dst, byte *src, int width)
{
	/* make 24-bit RGBs from 24 bits-per-pixel, top byte last
	 * 8 bits per colour, i.e. bbbbbbbb gggggggg rrrrrrrr */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		dst->alpha = 0;
		dst->blue  = *src++;
		dst->green = *src++;
		dst->red   = *src;
	}
}

static void app_get_24_32_rgb_MSB(Colour *dst, byte *src, int width)
{
	/* make 24-bit RGBs from 32 bits-per-pixel, top byte first
	 * 8 bits per colour, i.e. aaaaaaaa rrrrrrrr gggggggg bbbbbbbb */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		dst->alpha = 0; src++; /* don't trust the alpha byte */
		dst->red   = *src++;
		dst->green = *src++;
		dst->blue  = *src;
	}
}

static void app_get_24_32_rgb_LSB(Colour *dst, byte *src, int width)
{
	/* make 24-bit RGBs from 32 bits-per-pixel, top byte last
	 * 8 bits per colour, i.e. bbbbbbbb gggggggg rrrrrrrr aaaaaaaa */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		dst->blue  = *src++;
		dst->green = *src++;
		dst->red   = *src++;
		dst->alpha = 0; /* don't trust the alpha byte */
	}
}

static void app_get_32_rgb_MSB(Colour *dst, byte *src, int width)
{
	/* make 32-bit ARGBs from 32 bits-per-pixel, top byte first
	 * 8 bits per colour, i.e. aaaaaaaa rrrrrrrr gggggggg bbbbbbbb */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		dst->alpha = *src++;
		dst->red   = *src++;
		dst->green = *src++;
		dst->blue  = *src;
	}
}

static void app_get_32_rgb_LSB(Colour *dst, byte *src, int width)
{
	/* make 32-bit ARGBs from 32 bits-per-pixel, top byte last
	 * 8 bits per colour, i.e. bbbbbbbb gggggggg rrrrrrrr aaaaaaaa */
	int i;

	for (i=0; i < width; i++,dst++,src++)
	{
		dst->blue  = *src++;
		dst->green = *src++;
		dst->red   = *src++;
		dst->alpha = *src;
	}
}

/*
 *  Declare a data structure which 'knows' about the above
 *  fast reverse blitting functions and allows quick searching for
 *  an appropriate function to use:
 */

static AppBltFuncStruct app_get_func_table[] =
{
 /* 8-bit pixvals to varying depth colour maps */

 { 1, 1,1,0,0UL,0UL,0UL,app_get_8_1_1_MSB},
 { 1, 1,0,0,0UL,0UL,0UL,app_get_8_1_1_LSB},

 { 1, 2,1,0,0UL,0UL,0UL,app_get_8_1_2_MSB},
 { 1, 2,0,0,0UL,0UL,0UL,app_get_8_1_2_LSB},

 { 1, 4,1,0,0UL,0UL,0UL,app_get_8_1_4_MSB},
 { 1, 4,0,0,0UL,0UL,0UL,app_get_8_1_4_LSB},

 { 1, 8,1,0,0UL,0UL,0UL,app_get_8_1_8},
 { 1, 8,0,0,0UL,0UL,0UL,app_get_8_1_8},

 { 2, 2,1,0,0UL,0UL,0UL,app_get_8_2_2_MSB},
 { 2, 2,0,0,0UL,0UL,0UL,app_get_8_2_2_LSB},

 { 2, 4,1,0,0UL,0UL,0UL,app_get_8_2_4_MSB},
 { 2, 4,0,0,0UL,0UL,0UL,app_get_8_2_4_LSB},

 { 2, 8,1,0,0UL,0UL,0UL,app_get_8_2_8},
 { 2, 8,0,0,0UL,0UL,0UL,app_get_8_2_8},

 { 4, 4,1,0,0UL,0UL,0UL,app_get_8_4_4_MSB},
 { 4, 4,0,0,0UL,0UL,0UL,app_get_8_4_4_LSB},

 { 4, 8,1,0,0UL,0UL,0UL,app_get_8_4_8},
 { 4, 8,0,0,0UL,0UL,0UL,app_get_8_4_8},

 { 8, 8,1,0,0UL,0UL,0UL,app_get_8_8_8},
 { 8, 8,0,0,0UL,0UL,0UL,app_get_8_8_8},

 /* 32-bit rgbs to true colour pixels using component masks */

 {12,16,1,1,0x0000f00UL,0x00000f0UL,0x000000fUL,app_get_12_16_rgb_MSB},
 {12,16,0,1,0x0000f00UL,0x00000f0UL,0x000000fUL,app_get_12_16_rgb_LSB},

 {15,16,1,1,0x0007c00UL,0x00003e0UL,0x000001fUL,app_get_15_16_rgb_MSB},
 {15,16,0,1,0x0007c00UL,0x00003e0UL,0x000001fUL,app_get_15_16_rgb_LSB},

 {16,16,1,1,0x000f800UL,0x00007e0UL,0x000001fUL,app_get_16_rggb_MSB},
 {16,16,0,1,0x000f800UL,0x00007e0UL,0x000001fUL,app_get_16_rggb_LSB},

 {24,24,1,1,0x0ff0000UL,0x000ff00UL,0x00000ffUL,app_get_24_rgb_MSB},
 {24,24,0,1,0x0ff0000UL,0x000ff00UL,0x00000ffUL,app_get_24_rgb_LSB},

 {24,32,1,1,0x0ff0000UL,0x000ff00UL,0x00000ffUL,app_get_24_32_rgb_MSB},
 {24,32,0,1,0x0ff0000UL,0x000ff00UL,0x00000ffUL,app_get_24_32_rgb_LSB},

 {32,32,1,1,0x0ff0000UL,0x000ff00UL,0x00000ffUL,app_get_32_rgb_MSB},
 {32,32,0,1,0x0ff0000UL,0x000ff00UL,0x00000ffUL,app_get_32_rgb_LSB},

 /* converse cases */

 {12,16,1,1,0x000000fUL,0x00000f0UL,0x0000f00UL,app_get_12_16_rgb_MSB},
 {12,16,0,1,0x000000fUL,0x00000f0UL,0x0000f00UL,app_get_12_16_rgb_LSB},

 {15,16,1,1,0x000001fUL,0x00003e0UL,0x0007c00UL,app_get_15_16_rgb_MSB},
 {15,16,0,1,0x000001fUL,0x00003e0UL,0x0007c00UL,app_get_15_16_rgb_LSB},

 {16,16,1,1,0x000001fUL,0x00007e0UL,0x000f800UL,app_get_16_rggb_MSB},
 {16,16,0,1,0x000001fUL,0x00007e0UL,0x000f800UL,app_get_16_rggb_LSB},

 {24,24,1,1,0x00000ffUL,0x000ff00UL,0x0ff0000UL,app_get_24_rgb_MSB},
 {24,24,0,1,0x00000ffUL,0x000ff00UL,0x0ff0000UL,app_get_24_rgb_LSB},

 {24,32,1,1,0x00000ffUL,0x000ff00UL,0x0ff0000UL,app_get_24_32_rgb_MSB},
 {24,32,0,1,0x00000ffUL,0x000ff00UL,0x0ff0000UL,app_get_24_32_rgb_LSB},

 {32,32,1,1,0x00000ffUL,0x000ff00UL,0x0ff0000UL,app_get_32_rgb_MSB},
 {32,32,0,1,0x00000ffUL,0x000ff00UL,0x0ff0000UL,app_get_32_rgb_LSB}
};

static AppBltFunc app_find_get_func(int depth,
	int bits_per_pixel, int order, int use_masks,
	unsigned long red_mask, unsigned long green_mask,
	unsigned long blue_mask)
{
	int i, nelem;

	nelem = sizeof(app_get_func_table)/sizeof(app_get_func_table[0]);

	for (i=0; i < nelem; i++) {
		if ((depth == app_get_func_table[i].depth) &&
		    (bits_per_pixel == app_get_func_table[i].bpp) &&
		    (order == app_get_func_table[i].order) &&
		    (use_masks == app_get_func_table[i].use_masks))
		{
			if (! use_masks) {
				return app_get_func_table[i].fn;
			}
			else if
			   ((red_mask   == app_get_func_table[i].rmask) &&
			    (green_mask == app_get_func_table[i].gmask) &&
			    (blue_mask  == app_get_func_table[i].bmask))
			{
				return app_get_func_table[i].fn;
			}
		}
	}
	return NULL;	/* no matching get function found */
}

/*
 *  Copy 0-bits to transparent pixels, 1-bits to opaque.
 */
static void app_get_32_T_1(Colour *dst, byte *src, int width, int order)
{
	int x, shift;

	shift = 7;
	for (x=0; x < width; x++) {
		if (order == MSBFirst)
		{
			if (*src & (1<<shift))
				dst->alpha = 0;
			else
				dst->red = dst->green = dst->blue =
				dst->alpha = 0xff;
		}
		else {
			if (*src & (1<<(7-shift)))
				dst->alpha = 0;
			else
				dst->red = dst->green = dst->blue =
				dst->alpha = 0xff;
		}
		dst++;
		if (--shift < 0) {
			shift = 7;
			src++;
		}
	}
}

/*
 *  Retrieve the pixels within a Bitmap and construct a
 *  new portable image from them.
 */
Image *app_bitmap_to_image(Bitmap *bmp)
{
	Image *img;
	XID src_id, src_mask;
	Window *win;
	Display *disp;
	XImage *xi;
	int width, height, depth, rowbytes;
	int bits_per_pixel, order;
	int y;
	AppBltFunc blt_func;
	byte *data;
	Image *tmpi;
	Bitmap *tmpb;
	int blit = 0;
	int failure = 0;

	win = bmp->win;
	disp = app_extra(bmp->win->app)->display;
	src_id = bitmap_extra(bmp)->handle;
	src_mask = bitmap_extra(bmp)->clipmask;
	width = bmp->area.width;
	height = bmp->area.height;

	xi = XGetImage(disp, src_id, 0, 0, width, height, ~0UL, ZPixmap);
	if (! xi) {
		return NULL; /* error */
	}

	rowbytes = xi->bytes_per_line;
	depth = xi->depth;
	bits_per_pixel = xi->bits_per_pixel;
	data = xi->data;

	if (depth <= 8)
		order = ((xi->bitmap_bit_order==MSBFirst) ?1:0);
	else
		order = ((xi->byte_order==MSBFirst) ?1:0);

	if (! failure) {
		img = app_new_image(width, height, (depth <= 8) ? 8 : 32);

		if (! img) {
			failure = 1;
		}
	}
	else {
		img = NULL;
		failure = 1;
	}

	if (! failure)
	{
		/* Make a temporary image, convert it to a bitmap,
		 * then check what blit function was used to do that. */

		Colour clr = BLACK;

		tmpi = app_new_image(1, 1, img->depth);

		if (tmpi) {
			if (tmpi->depth == 8)
			{
				tmpi->cmap_size = 1;
				tmpi->cmap = &clr;
				tmpi->data8[0][0] = 0;
			}
			else {
				tmpi->data32[0][0] = BLACK;
			}

			tmpb = app_image_to_bitmap(bmp->win, tmpi);

			if (tmpb)
			{
				blit = bitmap_extra(tmpb)->blitter;
				app_del_bitmap(tmpb);
			}

			tmpi->cmap_size = 0;
			tmpi->cmap = NULL;
			app_del_image(tmpi);
		}
	}

	if (blit != 0)
	{
		blt_func = app_get_func_table[blit-1].fn;
	}
	else if ((blit = bitmap_extra(bmp)->blitter) != 0)
	{
		blt_func = app_get_func_table[blit-1].fn;
	}
	else if (win && win_extra(win)->is_paletted)
	{
		blt_func = app_find_get_func(depth,
			bits_per_pixel, order,
			0, 0UL, 0UL, 0UL);
	}
	else {
		blt_func = app_find_get_func(depth,
			bits_per_pixel, order, 1, xi->red_mask,
			xi->green_mask, xi->blue_mask);
	}

	if (! blt_func) {
		failure = 1;
	}

	if (failure)
	{
		/* nothing can be done */
	}
	else if (win->pal)
	{
		Palette *pal = win->pal;
		img->cmap_size = pal->size;
		img->cmap = app_alloc(sizeof(Colour) * img->cmap_size);
		for (y=0; y < img->cmap_size; y++)
			img->cmap[y] = pal->element[y];
	}
	else if (app_extra(win->app)->pal)
	{
		Palette *pal = app_extra(win->app)->pal;
		img->cmap_size = pal->size;
		img->cmap = app_alloc(sizeof(Colour) * img->cmap_size);
		for (y=0; y < img->cmap_size; y++)
			img->cmap[y] = pal->element[y];
	}

	if ((! failure) && img)
	{
		if (img->depth == 8)
		{
			for (y=0; y < height; y++) {
				blt_func(img->data8[y],
					&data[y*rowbytes], width);
			}
		}
		else /* img->depth == 32 */
		{
			for (y=0; y < height; y++) {
				blt_func(img->data32[y],
					&data[y*rowbytes], width);
			}
		}
	}

	XDestroyImage(xi);

	/* Now examine the bitmap's clipmask, if any, to set alpha. */

	if (failure || (! src_mask) || (! img) || (img->depth < 32))
		return img;
	/* There's no easy way to map transparent 8-bit pixels
	 * onto a palette, which is why we don't bother if
	 * the image's depth is less than 32. */

	xi = XGetImage(disp, src_mask, 0, 0, width, height,
				~0UL, ZPixmap);
	if (! xi)
		return img;

	data = xi->data;
	for (y=0; y < height; y++) {
		app_get_32_T_1(img->data32[y],
				&data[y*xi->bytes_per_line],
				width, order);
	}

	XDestroyImage(xi);

	return img;
}
