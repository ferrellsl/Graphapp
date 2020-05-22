/*
 *  Image writing.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/08/15  First release.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced a size_t conversion warning.
 *  Version: 3.57  2002/08/09  Added saving of PNG file format.
 *  Version: 3.60  2007/06/06  Added app_write_image_at function.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h" 
#include "writegif.h"
#include "writepng.h"
#include "writejpg.h"
#include "writeh.h"

/*
 *  So far this function can only save images as GIF or PNG format,
 *  and only into a named file, not to a pipe or standard
 *  output stream. These problems will be fixed, so that
 *  writing to JPEG format or to any file stream
 *  will be possible.
 */
int app_write_image(Image *img, const char *filename)
{
	return app_write_image_at(img, filename, 0, 0);
}

int app_write_image_at(Image *img, const char *filename, int dpi, int interlace)
{
	int length;

	length = (int) strlen(filename);

	if ((length > 2)
	 && ((strcmp(&filename[length-2], ".h") == 0)
	  || (strcmp(&filename[length-2], ".H") == 0)))
		return app_save_header_image(img, filename);

	if (length < 4)
		return 0;

	if ((strcmp(&filename[length-4], ".gif") == 0)
	 || (strcmp(&filename[length-4], ".GIF") == 0))
		return app_save_gif(img, filename, interlace);

	else if ((strcmp(&filename[length-4], ".png") == 0)
	      || (strcmp(&filename[length-4], ".PNG") == 0))
		return app_save_png(img, filename, dpi, interlace);

	else if ((strcmp(&filename[length-4], ".jpg") == 0)
	      || (strcmp(&filename[length-4], ".JPG") == 0))
		return app_save_jpeg(img, filename, dpi, interlace);

	return 0;
}
