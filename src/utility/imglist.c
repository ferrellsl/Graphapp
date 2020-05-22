/*
 *  Platform-neutral image data structure:
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.35  2002/12/23  Renamed image_count to num_images.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"

ImageList *app_new_image_list(void)
{
	return app_zero_alloc(sizeof(ImageList));
}

void app_del_image_list(ImageList *imglist)
{
	int i;

	for (i=0; i<imglist->num_images; i++)
		app_del_image(imglist->images[i]);
	app_free(imglist->images);
	app_free(imglist);
}

void app_append_to_image_list(ImageList *imglist, Image *img)
{
	int i;

	i = ++imglist->num_images;
	imglist->images = app_realloc(imglist->images, i*sizeof(Image*));
	imglist->images[i-1] = img;
}
