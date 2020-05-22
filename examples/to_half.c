/*
 *  Convert a 32-bit black and white image file into
 *  a 8-bit 5-grey greyscale paletted image file,
 *  halving the dimensions of the image at the same time!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <app.h>

/*
 *  Convert a black and white 32-bit image into
 *  a paletted 5-grey greyscale image.
 *  Delete the 32-bit image, too.
 */
Image * to_half_size_greyscale_image(Image *img)
{
	int x, y;
	Colour c1, c2, c3, c4;
	int value;
	Image *bw;

	bw = app_new_image(img->width / 2, img->height / 2, 8);
	bw->cmap = app_alloc(sizeof(Colour) * 5);
	bw->cmap_size = 5;
	bw->cmap[0] = BLACK;
	bw->cmap[1] = DARK_GREY;
	bw->cmap[2] = GREY;
	bw->cmap[3] = LIGHT_GREY;
	bw->cmap[4] = WHITE;

	for (y = 0; y < img->height - 1; y+=2)
	{
		for (x = 0; x < img->width - 1; x+=2)
		{
			c1 = img->data32[y][x];
			c2 = img->data32[y][x+1];
			c3 = img->data32[y+1][x];
			c4 = img->data32[y+1][x+1];
			value = c1.red;
			value += c1.green;
			value += c1.blue;
			value += c2.red;
			value += c2.green;
			value += c2.blue;
			value += c3.red;
			value += c3.green;
			value += c3.blue;
			value += c4.red;
			value += c4.green;
			value += c4.blue;
			value /= 12;
			if (value >= 0x7F)
			{
				if (value >= 0xDF)
					value = 4;
				else if (value >= 0x9F)
					value = 3;
				else
					value = 2;
			}
			else
			{
				if (value >= 0x5F)
					value = 2;
				else if (value >= 0x1F)
					value = 1;
				else
					value = 0;
			}
			bw->data8[y/2][x/2] = value;
		}
	}
	app_del_image(img);
	return bw;
}


int main(int argc, char *argv[])
{
	int i;
	Image *img;
	char newname[512];

	for (i = 1; i < argc; i++)
	{
		img = app_read_image(argv[i], 32);
		img = to_half_size_greyscale_image(img);
		sprintf(newname, "%s_half.png", argv[i]);
		printf("Writing %s\n", newname);
		app_write_image(img, newname);
	}

	return 0;
}
