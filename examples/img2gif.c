/*
 *  Convert an image file to a GIF exact image file.
 *
 *  By 'exact' I mean if the source file has 256 or fewer
 *  pixel colours, the output GIF will have a palette
 *  containing exactly those colours.
 *
 *  If the source file has more than that many colours,
 *  it'll need to use a palette estimation method,
 *  in which case you're no worse off than using
 *  a public domain program such as png2gif.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <app.h>

/*
 *  Convert a 32-bit image into a paletted 8-bit image.
 */
Image * img_32_to_8(Image *src, int *exact)
{
	int i, x, y;
	Colour c1, c2;
	Image *dst;

	dst = app_new_image(src->width, src->height, 8);
	dst->cmap = app_alloc(sizeof(Colour) * 256);
	dst->cmap_size = 0;

	for (y = 0; y < src->height; y++)
	{
		for (x = 0; x < src->width; x++)
		{
			c1 = src->data32[y][x];

			/* Clamp the alpha to only two values. */
			if (c1.alpha > 0x7F)
				c1.alpha = c1.red = c1.green = c1.blue = 255;
			else
				c1.alpha = 0;

			/* Search for that colour in the palette. */
			for (i = 0; i < dst->cmap_size; i++)
			{
				c2 = dst->cmap[i];
				if ((c2.alpha == c1.alpha)
				 && (c2.red   == c1.red)
				 && (c2.green == c1.green)
				 && (c2.blue  == c1.blue))
				{
					/* Found in palette! */
					break;
				}
			}
			/* Was it found in the palette? */
			if (i == dst->cmap_size)
			{
				/* Not found in palette; try to add it. */
				if (i == 255)
				{
					/* Insufficient space to add. */
					/* Estimate colour cube instead. */
					app_del_image(dst);
					if (exact != NULL)
						*exact = 0;
					return app_image_convert_32_to_8(src);
				}
				dst->cmap_size++;
				dst->cmap[i] = c1;
			}
			/* Found the colour in palette. */
			dst->data8[y][x] = i;
		}
	}
	if (exact != NULL)
		*exact = 1;
	return dst;
}

int main(int argc, char *argv[])
{
	int i;
	Image *img;
	Image *gif;
	char *newname;
	int exact;
	int result = 0;

	for (i = 1; i < argc; i++)
	{
		img = app_read_image(argv[i], 32);
		gif = img_32_to_8(img, &exact);
		app_del_image(img);

		newname = app_alloc(strlen(argv[i]) + 5);
		sprintf(newname, "%s.gif", argv[i]);
		printf("Writing %s (%s)\n", newname,
			exact ? "exact palette" : "colour cube estimate");
		if (! app_write_image(gif, newname))
			result = 1;
		app_free(newname);

		app_del_image(gif);
	}

	return result;
}
