/*
 *  Regular expression parsing in UTF-8 strings.
 *
 *  Platform: Neutral
 *
 *  Version: 3.10  2001/12/01  First release.
 */

/*
 *  This file implements a regular expression parsing engine,
 *  which uses a simple syntax:
 *
 *  "abc"	matches the exact string "abc"
 *  "a*bc"	matches "a" followed by zero or more chars, then "bc"
 *  "abc*"	matches any string which starts with "abc"
 *  "*abc"	matches any string which ends with "abc"
 *  "*abc*"	matches any string which contains "abc"
 *  "a?bc"	matches "a" followed by any one char, then "bc"
 *  "*.jpg"	matches any string which ends in ".jpg"
 *
 *  The only special characters are * and ?
 *  The dot character . has no special meaning - it matches only a dot.
 *  The ^ and $ characters have no special meaning - they match ^ and $
 *  There is currently no escape mechanism for * and ?
 *  The \ character has no special meaning.
 *
 *  Strings must match at the start and end, unless there
 *  is a * character at the start or end of the regular expression.
 *  So this scheme is like a shell matching scheme, not like grep.
 *
 *  Note that this regex engine assumes UTF-8 strings throughout.
 *  The only difference from ordinary ISO Latin-1 regex engines
 *  is that ? matches one UTF-8 char, not one byte, while * still
 *  matches any number of bytes (this is equivalent to matching
 *  any number of UTF-8 chars since a UTF-8 char sequence is meant
 *  to be in a canonical minimal representation).
 *
 *  The implementation is a straightforward recursive technique.
 *  This is functional, although not the most efficient method.
 *  This code was adapted from The Practice of Programming
 *  by Kernighan and Pike.
 *
 *  The regex_match function returns 1 if there's a match, else 0.
 */

static int app_regex_matchstar(char *regexp, char *text);

int app_regex_match(char *regexp, char *text)
{
	/* regex_match: search for regexp at start of text */

	while (1) {
		if (*regexp == '\0')
			return (*text == '\0');
		if (*regexp == '*')
			return app_regex_matchstar(regexp+1, text);
		if (*text != '\0') {
			if (*regexp == '?') {
				regexp++;
				if (*text & 0x80) { /* skip UTF-8 char */
					text++;
					while ((*text & 0xC0) == 0x80)
						text++;
				}
				else
					text++;
				continue;
			}
			else if (*regexp == *text) {
				regexp++;
				text++;
				continue;
			}
		}
		return 0;
	}
}

static int app_regex_matchstar(char *regexp, char *text)
{
	/* regex_matchstar: fast search for *regexp at start of text */

	while (1) {
		/* a '*' matches zero or more chars */
		if (app_regex_match(regexp, text))
			return 1;
		if (*text == '\0')
			break;
		text++;
	}
	return 0;
}

