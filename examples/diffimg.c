/*
 *  Determine if two images are identical.
 *
 *  Returns zero if they are identical, non-zero if they differ.
 *  Compares the two images with a 32-bit pixel depth.
 *  If the widths or heights are different, they are not identical.
 */

#include <stdio.h>
#include <graphapp.h>

static char *usage = "diffimg file1.gif file2.png";

int diff_images(Image *img1, Image *img2)
{
	int result = 0;
	int x, y;
	Colour c1, c2;

	if (img1->width != img2->width) {
		printf("Image widths differ (%d vs %d)\n",
					img1->width, img2->width);
		return 1;
	}
	if (img1->height != img2->height) {
		printf("Image heights differ (%d vs %d)\n",
					img1->height, img2->height);
		return 2;
	}
	for (y=0; y < img1->height; y++) {
		for (x=0; x < img1->width; x++) {
			c1 = img1->data32[y][x];
			c2 = img2->data32[y][x];
			if (c1.alpha != c2.alpha)
				result = 3;
			if (c1.red != c2.red)
				result = 4;
			if (c1.green != c2.green)
				result = 5;
			if (c1.blue != c2.blue)
				result = 6;
			if (result) {
				printf("Images differ at point (%d,%d)\n",
					x, y);
				return result;
			}
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	App *app;
	Image *img1, *img2;
	int diff;

	if (argc < 3) {
		fprintf(stderr, usage);
		return -2;
	}

	app = new_app(argc, argv);

	img1 = read_image(argv[1], 32);
	img2 = read_image(argv[2], 32);

	if (img1 == NULL) {
		fprintf(stderr, "The image file %s could not be read.\n",
				argv[1]);
		fprintf(stderr, usage);
		return -3;
	}
	if (img2 == NULL) {
		fprintf(stderr, "The image file %s could not be read.\n",
				argv[2]);
		fprintf(stderr, usage);
		return -4;
	}

	diff = diff_images(img1, img2);

	del_app(app);
	return diff;
}

