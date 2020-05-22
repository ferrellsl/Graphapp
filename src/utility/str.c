/*
 *  String functions.
 *
 *  Platform: Neutral
 *
 *  Version: 3.00  2001/05/05  First release.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.57  2005/08/16  Using memcpy not strcpy for speed.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include <stdio.h>
#include "apputils.h"

char * app_copy_string(const char *str)
{
	char *ptr;
	long len;

	if (str == NULL)
		return NULL;
	len = (long) strlen(str) + 1;
	ptr = app_alloc(len * sizeof(char));
	if (ptr == NULL)
		return NULL;
	memcpy(ptr, str, len);
	return ptr;
}

void app_del_string(char *str)
{
	app_free(str);
}
