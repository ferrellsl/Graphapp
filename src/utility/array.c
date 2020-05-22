/*
 *  Arrays (Null-terminated array of void *).
 *
 *  Platform: Neutral
 *
 *  Version: 3.57  2005/08/16  First released version.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "apputils.h"

int app_get_array_length(void **array)
{
	int i;

	if (! array)
		return 0;
	for (i=0; array[i]; i++)
		continue;
	return i;
}

void ** app_add_array_element(void **array, void *insertion)
{
	int length = app_get_array_length(array);

	array = app_realloc(array, (length + 2) * sizeof(void *));
	if (array) {
		array[length] = insertion;
		array[length+1] = NULL;
	}
	return array;
}

void ** app_del_array_element(void **array, void *deletion)
{
	int length;
	int i;

	/* If array is empty, stop now. */
	if (array == NULL)
		return array;

	/* Find index of 'deletion' within the array. */
	for (i = -1, length = 0; array[length] != NULL; length++)
	{
		if (array[length] == deletion)
			i = length;
	}

	/* If not found, stop now. */
	if (i == -1)
		return array;

	/* Found, and it must be the first and only element. */
	if (length == 1) {
		app_free(array);
		return NULL;
	}

	memmove(&array[i], &array[i+1], (length - i) * sizeof(void *));
	return app_realloc(array, length * sizeof(void *));
}
