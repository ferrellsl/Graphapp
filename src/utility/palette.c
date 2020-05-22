/*
 *  Palette functions.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"

Palette *app_new_palette(int size, Colour *elem)
{
	Palette *pal;
	int i;

	pal = app_zero_alloc(sizeof(struct Palette));
	pal->size = size;

	/* copy elements over to the new palette structure */

	pal->element = app_zero_alloc(sizeof(Colour) * size);
	if (elem)
		for (i=0; i < size; i++)
			pal->element[i] = elem[i];

	return pal;
}

void app_del_palette(Palette *pal)
{
	app_free(pal->element);
	app_free(pal);
}

/* 
 * Using the equation given in Poynton's ColorFAQ at
 * <http://www.inforamp.net/~poynton/>
 */
byte * app_palette_translation(Palette *target, byte *dest,
	int src_size, Colour *src_elem)
{
	/* Return a translation matrix which maps src colours to the
	 * nearest target colours. Nearest in terms of human perception.
	 */
	int t, s, bestmatch;
	unsigned long min_dist, distance;
	long dr, dg, db;
	Colour src, tgt;

	for (s=0; s < src_size; s++)
	{
		src = src_elem[s];
		min_dist = ~(0UL);
		bestmatch = -1;	/* not-an-index */

		for (t=0; t < target->size; t++)
		{
			tgt = target->element[t];

			/* RGB match in decimal:
			dr = (((long)src.red  -(long)tgt.red)   *30) /100;
			dg = (((long)src.green-(long)tgt.green) *59) /100;
			db = (((long)src.blue -(long)tgt.blue)  *11) /100;
			*/

			/* RGB match in hex: */
			dr = (((long)src.red  -(long)tgt.red)   *77)  >>8;
			dg = (((long)src.green-(long)tgt.green) *151) >>8;
			db = (((long)src.blue -(long)tgt.blue)  *28)  >>8;

			/* RGB to greyscale:
			dr = (((long)src.red  -(long)tgt.red)   *21) /100;
			dg = (((long)src.green-(long)tgt.green) *72) /100;
			db = (((long)src.blue -(long)tgt.blue)  * 7) /100;
			*/

			distance = dr * dr + dg * dg + db * db;

			if (distance == 0) {
				bestmatch = t;
				break;
			}
			else if (bestmatch < 0) {
				bestmatch = t;
				min_dist = distance;
			}
			else if (distance < min_dist) {
				bestmatch = t;
				min_dist = distance;
			}
		}
		dest[s] = bestmatch;
	}

	return dest;
}

