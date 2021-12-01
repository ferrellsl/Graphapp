/*
 * Monty - a simple project editor.
 *
 * File: str-util.c -- string utility functions.
 * Platform: Neutral  Version: 2.00  Date: 2001/08/08
 *
 * Version: 1.00  Changes: Original version by L. Patrick.
 * Version: 1.10  Changes: Added first_word_ends_with function.
 * Version: 2.00  Changes: Added add_strings function.
 */

/* Copyright (C) 1998-2000 L. Patrick

   This file is part of Monty, a simple project editor.
*/


/*
 *  Return 1 if string "name" ends with "ending".
 */
int string_ends_with(char *name, char *ending)
{
	int i, j, result;

	if (name == NULL)
		return (ending == NULL);

	result = 1;
	for (i=0; name[i]; )
		i = i + 1;
	for (j=0; ending[j]; )
		j = j + 1;
	while (result && (i >= 0) && (j >= 0)) {
		if (tolower(name[i]) != tolower(ending[j]))
			result = 0;
		i = i - 1;
		j = j - 1;
	}
	if ((i == 0) && (j > 0))
		result = 0;
	return result;
}

/*
 *  Return 1 if first word in string "name.ending blah" ends with "ending".
 */
int first_word_ends_with(char *name, char *ending)
{
	int i, j, result;

	if (name == NULL)
		return (ending == NULL);

	result = 1;
	for (i=0; (name[i] != '\0') && (name[i] != ' ') && (name[i] != '\t'); )
		i++;
	for (j=0; ending[j]; )
		j++;
	while (result && (--i >= 0) && (--j >= 0)) {
		if (tolower(name[i]) != tolower(ending[j]))
			result = 0;
	}
	if ((i == 0) && (j > 0))
		result = 0;
	return result;
}

/*
 *  Return 1 if string "name" starts with "start".
 */
int string_starts_with(char *name, char *start)
{
	int i, max, result;

	if (name == NULL)
		return (start == NULL);

	result = 1;
	for (max=0; name[max]; )
		max = max + 1;
	for (i=0; start[i]; )
		i = i + 1;
	if (i > max)
		result = 0;
	i = i - 1;
	while (result && (i >= 0)) {
		if (tolower(name[i]) != tolower(start[i]))
			result = 0;
		i = i - 1;
	}
	return result;
}

/*
 *  Concatenate two strings, realloc'ing memory as needed.
 *  The first string is assumed to be an alloc'd string,
 *  which is being modified. The second string is not changed,
 *  and can thus be a fixed array.
 */

char * add_strings(char *s, char *t)
{
	int len1, len2;
	char *result;

	if (s == NULL)
		return app_copy_string(t);

	len1 = strlen(s);
	len2 = strlen(t);
	result = app_realloc(s, len1+len2+1);
	if (result == NULL)
		return s;
	strcpy(result+len1, t);
	return result;
}
