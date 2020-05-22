/*
 *  Convert a 32-bit black and white image file into
 *  a 8-bit black and white paletted image file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <app.h>

/*
 *  Convert a black and white 32-bit image into
 *  a paletted black and white image.
 *  Delete the 32-bit image, too.
 */
Image * to_bw_paletted_image(Image *img)
{
	int x, y;
	Colour c;
	Image *bw;

	bw = app_new_image(img->width, img->height, 8);
	bw->cmap = app_alloc(sizeof(Colour) * 5);
	bw->cmap_size = 5;
	bw->cmap[0] = BLACK;
	bw->cmap[1] = DARK_GREY;
	bw->cmap[2] = GREY;
	bw->cmap[3] = LIGHT_GREY;
	bw->cmap[4] = WHITE;

	for (y = 0; y < img->height; y++)
	{
		for (x = 0; x < img->width; x++)
		{
			c = img->data32[y][x];
			bw->data8[y][x] = (c.red > 0x7F) ? 4 : 0;
		}
	}
	app_del_image(img);
	return bw;
}


int main(int argc, char *argv[])
{
	int i;
	Image *img;

	for (i = 1; i < argc; i++)
	{
		img = app_read_image(argv[i], 32);
		img = to_bw_paletted_image(img);
		printf("Writing %s\n", argv[i]);
		app_write_image(img, argv[i]);
	}

	return 0;
}
