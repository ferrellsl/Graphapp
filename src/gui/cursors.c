/*
 *  Standard cursor shapes.
 *
 *  Platform: Neutral
 *
 *  Version: 3.43  2003/04/25  First release.
 *  Version: 3.44  2003/04/28  Improved app_pointing cursor.
 *  Version: 3.57  2005/08/16  Added sizeLR and sizeTB cursors.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>
#include "app.h"

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 16 */
/* height = 16 */
/* cmap_size = 3 */
static Colour app_arrow_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_arrow_pixels [] = {
	2,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,1,0,1,2,2,2,2,2,2,2,2,2,2,2,2,
	2,1,0,0,1,2,2,2,2,2,2,2,2,2,2,2,
	2,1,0,0,0,1,2,2,2,2,2,2,2,2,2,2,
	2,1,0,0,0,0,1,2,2,2,2,2,2,2,2,2,
	2,1,0,0,0,0,0,1,2,2,2,2,2,2,2,2,
	2,1,0,0,0,0,0,0,1,2,2,2,2,2,2,2,
	2,1,0,0,0,0,0,0,0,1,2,2,2,2,2,2,
	2,1,0,0,0,0,0,0,0,0,1,2,2,2,2,2,
	2,1,0,0,0,0,0,1,1,1,1,2,2,2,2,2,
	2,1,0,0,1,0,0,1,2,2,2,2,2,2,2,2,
	2,1,0,1,2,1,0,0,1,2,2,2,2,2,2,2,
	2,1,1,2,2,1,0,0,1,2,2,2,2,2,2,2,
	2,2,2,2,2,2,1,0,0,1,2,2,2,2,2,2,
	2,2,2,2,2,2,1,0,0,1,2,2,2,2,2,2,
	2,2,2,2,2,2,2,1,1,2,2,2,2,2,2,2
};
static byte * app_arrow_data8 [] = {
	&app_arrow_pixels[16*0],
	&app_arrow_pixels[16*1],
	&app_arrow_pixels[16*2],
	&app_arrow_pixels[16*3],
	&app_arrow_pixels[16*4],
	&app_arrow_pixels[16*5],
	&app_arrow_pixels[16*6],
	&app_arrow_pixels[16*7],
	&app_arrow_pixels[16*8],
	&app_arrow_pixels[16*9],
	&app_arrow_pixels[16*10],
	&app_arrow_pixels[16*11],
	&app_arrow_pixels[16*12],
	&app_arrow_pixels[16*13],
	&app_arrow_pixels[16*14],
	&app_arrow_pixels[16*15]
};
static Image app_arrow_imagedata = {
	8,	/* depth */
	16,	/* width */
	16,	/* height */
	3,	/* cmap_size */
	app_arrow_cmap,
	app_arrow_data8,
	(Colour **) 0
};
Image * app_arrow_image = & app_arrow_imagedata;
Point app_arrow_hotspot = {2,1};

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 16 */
/* height = 16 */
/* cmap_size = 3 */
static Colour app_blank_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_blank_pixels [] = {
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
};
static byte * app_blank_data8 [] = {
	&app_blank_pixels[16*0],
	&app_blank_pixels[16*1],
	&app_blank_pixels[16*2],
	&app_blank_pixels[16*3],
	&app_blank_pixels[16*4],
	&app_blank_pixels[16*5],
	&app_blank_pixels[16*6],
	&app_blank_pixels[16*7],
	&app_blank_pixels[16*8],
	&app_blank_pixels[16*9],
	&app_blank_pixels[16*10],
	&app_blank_pixels[16*11],
	&app_blank_pixels[16*12],
	&app_blank_pixels[16*13],
	&app_blank_pixels[16*14],
	&app_blank_pixels[16*15]
};
static Image app_blank_imagedata = {
	8,	/* depth */
	16,	/* width */
	16,	/* height */
	3,	/* cmap_size */
	app_blank_cmap,
	app_blank_data8,
	(Colour **) 0
};
Image * app_blank_image = & app_blank_imagedata;
Point app_blank_hotspot = {7,7};

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 18 */
/* height = 18 */
/* cmap_size = 3 */
static Colour app_caret_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_caret_pixels [] = {
	2,2,2,2,2,2,1,1,2,1,1,2,2,2,2,2,2,2,
	2,2,2,2,2,1,0,0,1,0,0,1,2,2,2,2,2,2,
	2,2,2,2,2,2,1,1,0,1,1,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,1,1,0,1,1,2,2,2,2,2,2,2,
	2,2,2,2,2,1,0,0,1,0,0,1,2,2,2,2,2,2,
	2,2,2,2,2,2,1,1,2,1,1,2,2,2,2,2,2,2
};
static byte * app_caret_data8 [] = {
	&app_caret_pixels[18*0],
	&app_caret_pixels[18*1],
	&app_caret_pixels[18*2],
	&app_caret_pixels[18*3],
	&app_caret_pixels[18*4],
	&app_caret_pixels[18*5],
	&app_caret_pixels[18*6],
	&app_caret_pixels[18*7],
	&app_caret_pixels[18*8],
	&app_caret_pixels[18*9],
	&app_caret_pixels[18*10],
	&app_caret_pixels[18*11],
	&app_caret_pixels[18*12],
	&app_caret_pixels[18*13],
	&app_caret_pixels[18*14],
	&app_caret_pixels[18*15],
	&app_caret_pixels[18*16],
	&app_caret_pixels[18*17]
};
static Image app_caret_imagedata = {
	8,	/* depth */
	18,	/* width */
	18,	/* height */
	3,	/* cmap_size */
	app_caret_cmap,
	app_caret_data8,
	(Colour **) 0
};
Image * app_caret_image = & app_caret_imagedata;
Point app_caret_hotspot = {8,13};

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 16 */
/* height = 16 */
/* cmap_size = 3 */
static Colour app_cross_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_cross_pixels [] = {
	2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,
	2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,
	2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,
	2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,
	2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,
	2,1,1,1,1,1,1,0,1,1,1,1,1,1,2,2,
	1,0,0,0,0,0,0,2,0,0,0,0,0,0,1,2,
	2,1,1,1,1,1,1,0,1,1,1,1,1,1,2,2,
	2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,
	2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,
	2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,
	2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,
	2,2,2,2,2,2,1,0,1,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
};
static byte * app_cross_data8 [] = {
	&app_cross_pixels[16*0],
	&app_cross_pixels[16*1],
	&app_cross_pixels[16*2],
	&app_cross_pixels[16*3],
	&app_cross_pixels[16*4],
	&app_cross_pixels[16*5],
	&app_cross_pixels[16*6],
	&app_cross_pixels[16*7],
	&app_cross_pixels[16*8],
	&app_cross_pixels[16*9],
	&app_cross_pixels[16*10],
	&app_cross_pixels[16*11],
	&app_cross_pixels[16*12],
	&app_cross_pixels[16*13],
	&app_cross_pixels[16*14],
	&app_cross_pixels[16*15]
};
static Image app_cross_imagedata = {
	8,	/* depth */
	16,	/* width */
	16,	/* height */
	3,	/* cmap_size */
	app_cross_cmap,
	app_cross_data8,
	(Colour **) 0
};
Image * app_cross_image = & app_cross_imagedata;
Point app_cross_hotspot = {7,7};

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 18 */
/* height = 18 */
/* cmap_size = 3 */
static Colour app_dropper_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_dropper_pixels [] = {
	2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,1,2,
	2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0,1,
	2,2,2,2,2,2,2,2,2,1,1,1,0,0,0,0,0,1,
	2,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0,0,1,
	2,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0,1,2,
	2,2,2,2,2,2,2,2,1,0,1,0,0,0,1,1,2,2,
	2,2,2,2,2,2,2,1,0,1,1,1,0,0,1,2,2,2,
	2,2,2,2,2,2,1,0,1,1,1,0,1,0,1,2,2,2,
	2,2,2,2,2,1,0,1,1,1,0,1,2,1,2,2,2,2,
	2,2,2,2,1,0,1,1,1,0,1,2,2,2,2,2,2,2,
	2,2,2,1,0,1,1,1,0,1,2,2,2,2,2,2,2,2,
	2,2,1,0,1,1,1,0,1,2,2,2,2,2,2,2,2,2,
	2,1,0,1,1,1,0,1,2,2,2,2,2,2,2,2,2,2,
	2,1,0,1,1,0,1,2,2,2,2,2,2,2,2,2,2,2,
	2,0,1,0,0,1,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,0,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
};
static byte * app_dropper_data8 [] = {
	&app_dropper_pixels[18*0],
	&app_dropper_pixels[18*1],
	&app_dropper_pixels[18*2],
	&app_dropper_pixels[18*3],
	&app_dropper_pixels[18*4],
	&app_dropper_pixels[18*5],
	&app_dropper_pixels[18*6],
	&app_dropper_pixels[18*7],
	&app_dropper_pixels[18*8],
	&app_dropper_pixels[18*9],
	&app_dropper_pixels[18*10],
	&app_dropper_pixels[18*11],
	&app_dropper_pixels[18*12],
	&app_dropper_pixels[18*13],
	&app_dropper_pixels[18*14],
	&app_dropper_pixels[18*15],
	&app_dropper_pixels[18*16],
	&app_dropper_pixels[18*17]
};
static Image app_dropper_imagedata = {
	8,	/* depth */
	18,	/* width */
	18,	/* height */
	3,	/* cmap_size */
	app_dropper_cmap,
	app_dropper_data8,
	(Colour **) 0
};
Image * app_dropper_image = & app_dropper_imagedata;
Point app_dropper_hotspot = {1,16};

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 16 */
/* height = 16 */
/* cmap_size = 3 */
static Colour app_grab_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_grab_pixels [] = {
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,0,0,2,0,0,2,0,0,2,2,2,2,
	2,2,2,0,1,1,0,1,1,0,1,1,0,0,2,2,
	2,2,2,0,1,1,1,1,1,1,1,1,0,1,0,2,
	2,2,2,2,0,1,1,1,1,1,1,1,1,1,0,2,
	2,2,2,0,0,1,1,1,1,1,1,1,1,1,0,2,
	2,2,0,1,1,1,1,1,1,1,1,1,1,1,0,2,
	2,2,0,1,1,1,1,1,1,1,1,1,1,0,2,2,
	2,2,2,0,1,1,1,1,1,1,1,1,1,0,2,2,
	2,2,2,2,0,1,1,1,1,1,1,1,0,2,2,2,
	2,2,2,2,2,0,1,1,1,1,1,1,0,2,2,2,
	2,2,2,2,2,0,1,1,1,1,1,1,0,2,2,2
};
static byte * app_grab_data8 [] = {
	&app_grab_pixels[16*0],
	&app_grab_pixels[16*1],
	&app_grab_pixels[16*2],
	&app_grab_pixels[16*3],
	&app_grab_pixels[16*4],
	&app_grab_pixels[16*5],
	&app_grab_pixels[16*6],
	&app_grab_pixels[16*7],
	&app_grab_pixels[16*8],
	&app_grab_pixels[16*9],
	&app_grab_pixels[16*10],
	&app_grab_pixels[16*11],
	&app_grab_pixels[16*12],
	&app_grab_pixels[16*13],
	&app_grab_pixels[16*14],
	&app_grab_pixels[16*15]
};
static Image app_grab_imagedata = {
	8,	/* depth */
	16,	/* width */
	16,	/* height */
	3,	/* cmap_size */
	app_grab_cmap,
	app_grab_data8,
	(Colour **) 0
};
Image * app_grab_image = & app_grab_imagedata;
Point app_grab_hotspot = {8,8};

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 16 */
/* height = 16 */
/* cmap_size = 3 */
static Colour app_hand_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_hand_pixels [] = {
	2,2,2,2,2,2,2,0,0,2,2,2,2,2,2,2,
	2,2,2,0,0,2,0,1,1,0,0,0,2,2,2,2,
	2,2,0,1,1,0,0,1,1,0,1,1,0,2,2,2,
	2,2,0,1,1,0,0,1,1,0,1,1,0,2,0,2,
	2,2,2,0,1,1,0,1,1,0,1,1,0,0,1,0,
	2,2,2,0,1,1,0,1,1,0,1,1,0,1,1,0,
	2,0,0,2,0,1,1,1,1,1,1,1,0,1,1,0,
	0,1,1,0,0,1,1,1,1,1,1,1,1,1,1,0,
	0,1,1,1,0,1,1,1,1,1,1,1,1,1,0,2,
	2,0,1,1,1,1,1,1,1,1,1,1,1,1,0,2,
	2,2,0,1,1,1,1,1,1,1,1,1,1,1,0,2,
	2,2,0,1,1,1,1,1,1,1,1,1,1,0,2,2,
	2,2,2,0,1,1,1,1,1,1,1,1,1,0,2,2,
	2,2,2,2,0,1,1,1,1,1,1,1,0,2,2,2,
	2,2,2,2,2,0,1,1,1,1,1,1,0,2,2,2,
	2,2,2,2,2,0,1,1,1,1,1,1,0,2,2,2
};
static byte * app_hand_data8 [] = {
	&app_hand_pixels[16*0],
	&app_hand_pixels[16*1],
	&app_hand_pixels[16*2],
	&app_hand_pixels[16*3],
	&app_hand_pixels[16*4],
	&app_hand_pixels[16*5],
	&app_hand_pixels[16*6],
	&app_hand_pixels[16*7],
	&app_hand_pixels[16*8],
	&app_hand_pixels[16*9],
	&app_hand_pixels[16*10],
	&app_hand_pixels[16*11],
	&app_hand_pixels[16*12],
	&app_hand_pixels[16*13],
	&app_hand_pixels[16*14],
	&app_hand_pixels[16*15]
};
static Image app_hand_imagedata = {
	8,	/* depth */
	16,	/* width */
	16,	/* height */
	3,	/* cmap_size */
	app_hand_cmap,
	app_hand_data8,
	(Colour **) 0
};
Image * app_hand_image = & app_hand_imagedata;
Point app_hand_hotspot = {8,8};

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 18 */
/* height = 18 */
/* cmap_size = 3 */
static Colour app_lasso_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_lasso_pixels [] = {
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,1,1,1,1,1,1,1,2,2,2,2,
	2,2,2,2,1,1,1,0,0,0,0,0,0,0,1,1,2,2,
	2,2,2,1,0,0,0,1,1,1,1,1,1,1,0,0,1,2,
	2,2,1,0,1,1,1,2,2,2,2,2,2,2,1,1,0,1,
	2,1,0,1,2,2,2,2,2,2,2,2,2,2,2,1,0,1,
	1,0,1,2,2,2,2,2,2,2,2,2,2,2,1,1,0,1,
	1,0,1,2,2,2,2,2,2,2,2,1,1,1,0,0,1,2,
	1,0,1,1,1,2,2,2,1,1,1,0,0,0,1,1,2,2,
	2,1,0,0,0,1,1,1,0,0,0,1,1,1,2,2,2,2,
	1,0,0,1,1,0,0,0,1,1,1,2,2,2,2,2,2,2,
	1,0,1,0,1,0,1,1,2,2,2,2,2,2,2,2,2,2,
	2,1,0,0,0,1,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,1,1,0,1,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,1,0,1,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,1,0,1,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
};
static byte * app_lasso_data8 [] = {
	&app_lasso_pixels[18*0],
	&app_lasso_pixels[18*1],
	&app_lasso_pixels[18*2],
	&app_lasso_pixels[18*3],
	&app_lasso_pixels[18*4],
	&app_lasso_pixels[18*5],
	&app_lasso_pixels[18*6],
	&app_lasso_pixels[18*7],
	&app_lasso_pixels[18*8],
	&app_lasso_pixels[18*9],
	&app_lasso_pixels[18*10],
	&app_lasso_pixels[18*11],
	&app_lasso_pixels[18*12],
	&app_lasso_pixels[18*13],
	&app_lasso_pixels[18*14],
	&app_lasso_pixels[18*15],
	&app_lasso_pixels[18*16],
	&app_lasso_pixels[18*17]
};
static Image app_lasso_imagedata = {
	8,	/* depth */
	18,	/* width */
	18,	/* height */
	3,	/* cmap_size */
	app_lasso_cmap,
	app_lasso_data8,
	(Colour **) 0
};
Image * app_lasso_image = & app_lasso_imagedata;
Point app_lasso_hotspot = {2,16};

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 18 */
/* height = 18 */
/* cmap_size = 3 */
static Colour app_magminus_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_magminus_pixels [] = {
	2,2,2,2,2,1,1,1,1,2,2,2,2,2,2,2,2,2,
	2,2,2,1,1,0,0,0,0,1,1,2,2,2,2,2,2,2,
	2,2,1,0,0,1,1,1,1,0,0,1,2,2,2,2,2,2,
	2,1,0,1,1,1,1,1,1,1,1,0,1,2,2,2,2,2,
	2,1,0,1,1,1,1,1,1,1,1,0,1,2,2,2,2,2,
	1,0,1,1,1,1,1,1,1,1,1,1,0,1,2,2,2,2,
	1,0,1,1,0,0,0,0,0,0,1,1,0,1,2,2,2,2,
	1,0,1,1,0,0,0,0,0,0,1,1,0,1,2,2,2,2,
	1,0,1,1,1,1,1,1,1,1,1,1,0,1,2,2,2,2,
	2,1,0,1,1,1,1,1,1,1,1,0,1,2,2,2,2,2,
	2,1,0,1,1,1,1,1,1,1,1,0,1,2,2,2,2,2,
	2,2,1,0,0,1,1,1,1,0,0,0,0,1,2,2,2,2,
	2,2,2,1,1,0,0,0,0,1,1,0,0,0,1,2,2,2,
	2,2,2,2,2,1,1,1,1,2,2,1,0,0,0,1,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,1,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,1,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2
};
static byte * app_magminus_data8 [] = {
	&app_magminus_pixels[18*0],
	&app_magminus_pixels[18*1],
	&app_magminus_pixels[18*2],
	&app_magminus_pixels[18*3],
	&app_magminus_pixels[18*4],
	&app_magminus_pixels[18*5],
	&app_magminus_pixels[18*6],
	&app_magminus_pixels[18*7],
	&app_magminus_pixels[18*8],
	&app_magminus_pixels[18*9],
	&app_magminus_pixels[18*10],
	&app_magminus_pixels[18*11],
	&app_magminus_pixels[18*12],
	&app_magminus_pixels[18*13],
	&app_magminus_pixels[18*14],
	&app_magminus_pixels[18*15],
	&app_magminus_pixels[18*16],
	&app_magminus_pixels[18*17]
};
static Image app_magminus_imagedata = {
	8,	/* depth */
	18,	/* width */
	18,	/* height */
	3,	/* cmap_size */
	app_magminus_cmap,
	app_magminus_data8,
	(Colour **) 0
};
Image * app_magminus_image = & app_magminus_imagedata;
Point app_magminus_hotspot = {6,6};

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 18 */
/* height = 18 */
/* cmap_size = 3 */
static Colour app_magnify_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_magnify_pixels [] = {
	2,2,2,2,2,1,1,1,1,2,2,2,2,2,2,2,2,2,
	2,2,2,1,1,0,0,0,0,1,1,2,2,2,2,2,2,2,
	2,2,1,0,0,1,1,1,1,0,0,1,2,2,2,2,2,2,
	2,1,0,1,1,1,1,1,1,1,1,0,1,2,2,2,2,2,
	2,1,0,1,1,1,1,1,1,1,1,0,1,2,2,2,2,2,
	1,0,1,1,1,1,1,1,1,1,1,1,0,1,2,2,2,2,
	1,0,1,1,1,1,1,1,1,1,1,1,0,1,2,2,2,2,
	1,0,1,1,1,1,1,1,1,1,1,1,0,1,2,2,2,2,
	1,0,1,1,1,1,1,1,1,1,1,1,0,1,2,2,2,2,
	2,1,0,1,1,1,1,1,1,1,1,0,1,2,2,2,2,2,
	2,1,0,1,1,1,1,1,1,1,1,0,1,2,2,2,2,2,
	2,2,1,0,0,1,1,1,1,0,0,0,0,1,2,2,2,2,
	2,2,2,1,1,0,0,0,0,1,1,0,0,0,1,2,2,2,
	2,2,2,2,2,1,1,1,1,2,2,1,0,0,0,1,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,1,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,1,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2
};
static byte * app_magnify_data8 [] = {
	&app_magnify_pixels[18*0],
	&app_magnify_pixels[18*1],
	&app_magnify_pixels[18*2],
	&app_magnify_pixels[18*3],
	&app_magnify_pixels[18*4],
	&app_magnify_pixels[18*5],
	&app_magnify_pixels[18*6],
	&app_magnify_pixels[18*7],
	&app_magnify_pixels[18*8],
	&app_magnify_pixels[18*9],
	&app_magnify_pixels[18*10],
	&app_magnify_pixels[18*11],
	&app_magnify_pixels[18*12],
	&app_magnify_pixels[18*13],
	&app_magnify_pixels[18*14],
	&app_magnify_pixels[18*15],
	&app_magnify_pixels[18*16],
	&app_magnify_pixels[18*17]
};
static Image app_magnify_imagedata = {
	8,	/* depth */
	18,	/* width */
	18,	/* height */
	3,	/* cmap_size */
	app_magnify_cmap,
	app_magnify_data8,
	(Colour **) 0
};
Image * app_magnify_image = & app_magnify_imagedata;
Point app_magnify_hotspot = {6,6};

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 18 */
/* height = 18 */
/* cmap_size = 3 */
static Colour app_magplus_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_magplus_pixels [] = {
	2,2,2,2,2,1,1,1,1,2,2,2,2,2,2,2,2,2,
	2,2,2,1,1,0,0,0,0,1,1,2,2,2,2,2,2,2,
	2,2,1,0,0,1,1,1,1,0,0,1,2,2,2,2,2,2,
	2,1,0,1,1,1,1,1,1,1,1,0,1,2,2,2,2,2,
	2,1,0,1,1,1,0,0,1,1,1,0,1,2,2,2,2,2,
	1,0,1,1,1,1,0,0,1,1,1,1,0,1,2,2,2,2,
	1,0,1,1,0,0,0,0,0,0,1,1,0,1,2,2,2,2,
	1,0,1,1,0,0,0,0,0,0,1,1,0,1,2,2,2,2,
	1,0,1,1,1,1,0,0,1,1,1,1,0,1,2,2,2,2,
	2,1,0,1,1,1,0,0,1,1,1,0,1,2,2,2,2,2,
	2,1,0,1,1,1,1,1,1,1,1,0,1,2,2,2,2,2,
	2,2,1,0,0,1,1,1,1,0,0,0,0,1,2,2,2,2,
	2,2,2,1,1,0,0,0,0,1,1,0,0,0,1,2,2,2,
	2,2,2,2,2,1,1,1,1,2,2,1,0,0,0,1,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,1,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,1,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2
};
static byte * app_magplus_data8 [] = {
	&app_magplus_pixels[18*0],
	&app_magplus_pixels[18*1],
	&app_magplus_pixels[18*2],
	&app_magplus_pixels[18*3],
	&app_magplus_pixels[18*4],
	&app_magplus_pixels[18*5],
	&app_magplus_pixels[18*6],
	&app_magplus_pixels[18*7],
	&app_magplus_pixels[18*8],
	&app_magplus_pixels[18*9],
	&app_magplus_pixels[18*10],
	&app_magplus_pixels[18*11],
	&app_magplus_pixels[18*12],
	&app_magplus_pixels[18*13],
	&app_magplus_pixels[18*14],
	&app_magplus_pixels[18*15],
	&app_magplus_pixels[18*16],
	&app_magplus_pixels[18*17]
};
static Image app_magplus_imagedata = {
	8,	/* depth */
	18,	/* width */
	18,	/* height */
	3,	/* cmap_size */
	app_magplus_cmap,
	app_magplus_data8,
	(Colour **) 0
};
Image * app_magplus_image = & app_magplus_imagedata;
Point app_magplus_hotspot = {6,6};

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 16 */
/* height = 16 */
/* cmap_size = 3 */
static Colour app_pencil_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_pencil_pixels [] = {
	2,2,2,2,2,1,1,1,1,2,2,2,2,2,2,2,
	2,2,2,2,1,0,0,0,0,1,2,2,2,2,2,2,
	2,2,2,2,1,0,1,1,1,0,1,2,2,2,2,2,
	2,2,2,1,0,1,1,1,1,0,1,2,2,2,2,2,
	2,2,2,1,0,0,1,1,0,1,2,2,2,2,2,2,
	2,2,1,0,1,1,0,0,0,1,2,2,2,2,2,2,
	2,2,1,0,1,1,1,0,1,2,2,2,2,2,2,2,
	2,1,0,1,1,1,1,0,1,2,2,2,2,2,2,2,
	2,1,0,1,1,1,0,1,2,2,2,2,2,2,2,2,
	1,0,1,1,1,1,0,1,2,2,2,2,2,2,2,2,
	1,0,1,1,1,0,1,2,2,2,2,2,2,2,2,2,
	1,0,0,1,1,0,1,2,2,2,2,2,2,2,2,2,
	1,0,0,0,0,1,2,2,2,2,2,2,2,2,2,2,
	1,0,0,0,1,2,2,2,2,2,2,2,2,2,2,2,
	1,0,0,1,2,2,2,2,2,2,2,2,2,2,2,2,
	1,0,1,2,2,2,2,2,2,2,2,2,2,2,2,2
};
static byte * app_pencil_data8 [] = {
	&app_pencil_pixels[16*0],
	&app_pencil_pixels[16*1],
	&app_pencil_pixels[16*2],
	&app_pencil_pixels[16*3],
	&app_pencil_pixels[16*4],
	&app_pencil_pixels[16*5],
	&app_pencil_pixels[16*6],
	&app_pencil_pixels[16*7],
	&app_pencil_pixels[16*8],
	&app_pencil_pixels[16*9],
	&app_pencil_pixels[16*10],
	&app_pencil_pixels[16*11],
	&app_pencil_pixels[16*12],
	&app_pencil_pixels[16*13],
	&app_pencil_pixels[16*14],
	&app_pencil_pixels[16*15]
};
static Image app_pencil_imagedata = {
	8,	/* depth */
	16,	/* width */
	16,	/* height */
	3,	/* cmap_size */
	app_pencil_cmap,
	app_pencil_data8,
	(Colour **) 0
};
Image * app_pencil_image = & app_pencil_imagedata;
Point app_pencil_hotspot = {1,15};

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 16 */
/* height = 16 */
/* cmap_size = 3 */
static Colour app_pointing_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_pointing_pixels [] = {
	2,2,2,2,2,2,0,0,2,2,2,2,2,2,2,2,
	2,2,2,2,2,0,1,1,0,2,2,2,2,2,2,2,
	2,2,2,2,2,0,1,1,0,2,2,2,2,2,2,2,
	2,2,2,2,2,0,1,1,0,2,2,2,2,2,2,2,
	2,2,2,2,2,0,1,1,0,0,2,2,2,2,2,2,
	2,2,2,2,2,0,1,1,0,1,0,0,2,2,2,2,
	2,0,0,2,2,0,1,1,0,1,0,1,0,0,2,2,
	0,1,1,0,2,0,1,1,1,1,0,1,0,1,0,2,
	2,0,1,1,0,0,1,1,1,1,1,1,0,1,0,2,
	2,2,0,1,1,0,1,1,1,1,1,1,1,1,0,2,
	2,2,0,1,1,0,1,1,1,1,1,1,1,1,0,2,
	2,2,2,0,1,1,1,1,1,1,1,1,1,1,0,2,
	2,2,2,0,1,1,1,1,1,1,1,1,1,0,2,2,
	2,2,2,2,0,1,1,1,1,1,1,1,1,0,2,2,
	2,2,2,2,2,0,1,1,1,1,1,1,0,2,2,2,
	2,2,2,2,2,0,1,1,1,1,1,1,0,2,2,2
};
static byte * app_pointing_data8 [] = {
	&app_pointing_pixels[16*0],
	&app_pointing_pixels[16*1],
	&app_pointing_pixels[16*2],
	&app_pointing_pixels[16*3],
	&app_pointing_pixels[16*4],
	&app_pointing_pixels[16*5],
	&app_pointing_pixels[16*6],
	&app_pointing_pixels[16*7],
	&app_pointing_pixels[16*8],
	&app_pointing_pixels[16*9],
	&app_pointing_pixels[16*10],
	&app_pointing_pixels[16*11],
	&app_pointing_pixels[16*12],
	&app_pointing_pixels[16*13],
	&app_pointing_pixels[16*14],
	&app_pointing_pixels[16*15]
};
static Image app_pointing_imagedata = {
	8,	/* depth */
	16,	/* width */
	16,	/* height */
	3,	/* cmap_size */
	app_pointing_cmap,
	app_pointing_data8,
	(Colour **) 0
};
Image * app_pointing_image = & app_pointing_imagedata;
Point app_pointing_hotspot = {6,0};

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 16 */
/* height = 16 */
/* cmap_size = 3 */
static Colour app_wait_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_wait_pixels [] = {
	2,2,2,2,1,0,0,0,0,0,1,2,2,2,2,2,
	2,2,2,2,1,0,0,0,0,0,1,2,2,2,2,2,
	2,2,2,2,1,0,0,0,0,0,1,2,2,2,2,2,
	2,2,2,2,1,0,0,0,0,0,1,2,2,2,2,2,
	2,2,2,1,0,1,1,1,1,1,0,1,2,2,2,2,
	2,2,1,0,1,1,1,0,1,1,1,0,1,2,2,2,
	2,1,0,1,1,1,1,0,1,1,1,1,0,1,2,2,
	2,1,0,1,1,1,1,0,1,1,1,1,0,0,1,2,
	2,1,0,1,1,0,0,0,1,1,1,1,0,0,1,2,
	2,1,0,1,1,1,1,1,1,1,1,1,0,1,2,2,
	2,2,1,0,1,1,1,1,1,1,1,0,1,2,2,2,
	2,2,2,1,0,1,1,1,1,1,0,1,2,2,2,2,
	2,2,2,2,1,0,0,0,0,0,1,2,2,2,2,2,
	2,2,2,2,1,0,0,0,0,0,1,2,2,2,2,2,
	2,2,2,2,1,0,0,0,0,0,1,2,2,2,2,2,
	2,2,2,2,1,0,0,0,0,0,1,2,2,2,2,2
};
static byte * app_wait_data8 [] = {
	&app_wait_pixels[16*0],
	&app_wait_pixels[16*1],
	&app_wait_pixels[16*2],
	&app_wait_pixels[16*3],
	&app_wait_pixels[16*4],
	&app_wait_pixels[16*5],
	&app_wait_pixels[16*6],
	&app_wait_pixels[16*7],
	&app_wait_pixels[16*8],
	&app_wait_pixels[16*9],
	&app_wait_pixels[16*10],
	&app_wait_pixels[16*11],
	&app_wait_pixels[16*12],
	&app_wait_pixels[16*13],
	&app_wait_pixels[16*14],
	&app_wait_pixels[16*15]
};
static Image app_wait_imagedata = {
	8,	/* depth */
	16,	/* width */
	16,	/* height */
	3,	/* cmap_size */
	app_wait_cmap,
	app_wait_data8,
	(Colour **) 0
};
Image * app_wait_image = & app_wait_imagedata;
Point app_wait_hotspot = {7,8};

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 16 */
/* height = 16 */
/* cmap_size = 3 */
static Colour app_sizeLR_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_sizeLR_pixels [] = {
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,0,2,2,2,2,0,2,2,2,2,2,
	2,2,2,2,0,0,2,2,2,2,0,0,2,2,2,2,
	2,2,2,0,1,0,2,2,2,2,0,1,0,2,2,2,
	2,2,0,1,1,0,2,2,2,2,0,1,1,0,2,2,
	2,0,1,1,1,0,0,0,0,0,0,1,1,1,0,2,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
	2,0,1,1,1,0,0,0,0,0,0,1,1,1,0,2,
	2,2,0,1,1,0,2,2,2,2,0,1,1,0,2,2,
	2,2,2,0,1,0,2,2,2,2,0,1,0,2,2,2,
	2,2,2,2,0,0,2,2,2,2,0,0,2,2,2,2,
	2,2,2,2,2,0,2,2,2,2,0,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
};
static byte * app_sizeLR_data8 [] = {
	&app_sizeLR_pixels[16*0],
	&app_sizeLR_pixels[16*1],
	&app_sizeLR_pixels[16*2],
	&app_sizeLR_pixels[16*3],
	&app_sizeLR_pixels[16*4],
	&app_sizeLR_pixels[16*5],
	&app_sizeLR_pixels[16*6],
	&app_sizeLR_pixels[16*7],
	&app_sizeLR_pixels[16*8],
	&app_sizeLR_pixels[16*9],
	&app_sizeLR_pixels[16*10],
	&app_sizeLR_pixels[16*11],
	&app_sizeLR_pixels[16*12],
	&app_sizeLR_pixels[16*13],
	&app_sizeLR_pixels[16*14],
	&app_sizeLR_pixels[16*15]
};
static Image app_sizeLR_imagedata = {
	8,	/* depth */
	16,	/* width */
	16,	/* height */
	3,	/* cmap_size */
	app_sizeLR_cmap,
	app_sizeLR_data8,
	(Colour **) 0
};
Image * app_sizeLR_image = & app_sizeLR_imagedata;
Point app_sizeLR_hotspot = {7,7};

/* GraphApp image type 2 */
/* depth  = 8 */
/* width  = 16 */
/* height = 16 */
/* cmap_size = 3 */
static Colour app_sizeTB_cmap [] = {
	{ 0x00, 0x00, 0x00, 0x00},
	{ 0x00, 0xFF, 0xFF, 0xFF},
	{ 0xFF, 0xFF, 0xFF, 0xFF}
};
static byte app_sizeTB_pixels [] = {
	2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,0,1,0,2,2,2,2,2,2,2,
	2,2,2,2,2,0,1,1,1,0,2,2,2,2,2,2,
	2,2,2,2,0,1,1,1,1,1,0,2,2,2,2,2,
	2,2,2,0,1,1,1,1,1,1,1,0,2,2,2,2,
	2,2,0,0,0,0,0,1,0,0,0,0,0,2,2,2,
	2,2,2,2,2,2,0,1,0,2,2,2,2,2,2,2,
	2,2,2,2,2,2,0,1,0,2,2,2,2,2,2,2,
	2,2,2,2,2,2,0,1,0,2,2,2,2,2,2,2,
	2,2,2,2,2,2,0,1,0,2,2,2,2,2,2,2,
	2,2,0,0,0,0,0,1,0,0,0,0,0,2,2,2,
	2,2,2,0,1,1,1,1,1,1,1,0,2,2,2,2,
	2,2,2,2,0,1,1,1,1,1,0,2,2,2,2,2,
	2,2,2,2,2,0,1,1,1,0,2,2,2,2,2,2,
	2,2,2,2,2,2,0,1,0,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,0,2,2,2,2,2,2,2,2
};
static byte * app_sizeTB_data8 [] = {
	&app_sizeTB_pixels[16*0],
	&app_sizeTB_pixels[16*1],
	&app_sizeTB_pixels[16*2],
	&app_sizeTB_pixels[16*3],
	&app_sizeTB_pixels[16*4],
	&app_sizeTB_pixels[16*5],
	&app_sizeTB_pixels[16*6],
	&app_sizeTB_pixels[16*7],
	&app_sizeTB_pixels[16*8],
	&app_sizeTB_pixels[16*9],
	&app_sizeTB_pixels[16*10],
	&app_sizeTB_pixels[16*11],
	&app_sizeTB_pixels[16*12],
	&app_sizeTB_pixels[16*13],
	&app_sizeTB_pixels[16*14],
	&app_sizeTB_pixels[16*15]
};
static Image app_sizeTB_imagedata = {
	8,	/* depth */
	16,	/* width */
	16,	/* height */
	3,	/* cmap_size */
	app_sizeTB_cmap,
	app_sizeTB_data8,
	(Colour **) 0
};
Image * app_sizeTB_image = & app_sizeTB_imagedata;
Point app_sizeTB_hotspot = {7,7};

