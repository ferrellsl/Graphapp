/*
 *  Clipboard data-transfer functions.
 *
 *  Platform: Windows.
 *
 *  Version: 3.02  2001/10/01  First release.
 *  Version: 3.32  2002/09/04  Sends/gets ISO Latin 1 strings if possible.
 *  Version: 3.35  2002/12/23  Renamed app->winlist to app->windows.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 *  Version: 3.56  2005/08/09  Silenced some size_t conversion warnings.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

int app_set_clipboard_text(App *app, const char *text)
{
	HGLOBAL handle;
	char *ptr;
	int n, size, ch, extra;
	HWND hwnd;
	char *latin1, *utf8;

	/* Check if we can store an ISO Latin 1 string instead:
	 * This would not be needed if all applications stored
	 * UTF-8 strings into the clipboard, but many still use
	 * ISO Latin 1 strings, so interoperability is important.
	 * Here, we just squash the text into an ISO Latin 1 string
	 * and then unpack it back into a UTF-8 string, and check
	 * if that's the same as the original string. If so, paste
	 * the string as ISO Latin 1 instead.
	 */

	latin1 = NULL;
	n = size = (int) strlen(text);
	if (app_utf8_is_ascii(text, n)) /* ASCII always works */
		;
	else if (app_utf8_is_latin1(text, n)) { /* valid candidate */
		latin1 = app_utf8_to_latin1(text, &n);
		if (latin1 != NULL) {
			utf8 = app_correct_utf8(latin1, &n);
			if ((n == size) && (!memcmp(text, utf8, n))) {
				/* success, text->latin1->utf8==text */
				text = latin1;	/* swap to new string */
				size = (int) strlen(text);
				if (utf8 != latin1)
					app_free(utf8);
			}
			else {
				/* failure, so paste original string */
				if ((utf8 != NULL) && (utf8 != latin1))
					app_free(utf8);
				app_free(latin1);
				latin1 = NULL;
			}
		}
	}

	/* find number of bytes to allocate */
	for (n=extra=0; text[n] != '\0'; n++) {
		if ((text[n] == '\r') && (text[n+1] != '\n'))
			extra++;
		else if (text[n] == '\n')
			extra++;
	}
	size = n;

	/* allocate a global memory block */
	handle = GlobalAlloc(GHND, (DWORD) size +extra +1);
	ptr = (char *) GlobalLock(handle);

	/* copy the string, turning "\n" to "\r\n" */
	while (*text != '\0') {
		ch = *text++;
		if (ch == '\r') {
			*ptr++ = ch;
			if (*text == '\n')	/* "\r\n" -> "\r\n" */
				ch = *text++;
			else			/* "\rX" -> "\r\nX" */
				ch = '\n';
		}
		else if (ch == '\n') {		/* "X\n" -> "X\r\n" */
			*ptr++ = '\r';
		}
		*ptr++ = ch;
	}
	*ptr = '\0';
	GlobalUnlock(handle);

	/* find any HWND to use, if NULL that's okay */
	hwnd = 0;
	for (n=0; n < app->num_windows; n++) {
		hwnd = win_extra(app->windows[n])->hwnd;
		if (hwnd)
			break;
	}
	if (! OpenClipboard(hwnd)) {
		if (latin1 != NULL)
			app_free(latin1);
		return 0; /* someone is using the clipboard */
	}
	EmptyClipboard();
	SetClipboardData(CF_TEXT, handle);
	CloseClipboard();

	if (latin1 != NULL)
		app_free(latin1);
	return 1;
}

char *app_get_clipboard_text(App *app)
{
	HGLOBAL handle;
	char *ptr, *text;
	int n, size, ch;
	HWND hwnd;
	char *utf8;

	/* any text available? */
	if (! IsClipboardFormatAvailable(CF_TEXT))
		return NULL;

	/* find any HWND to use, if NULL that's okay */
	hwnd = 0;
	for (n=0; n < app->num_windows; n++) {
		hwnd = win_extra(app->windows[n])->hwnd;
		if (hwnd)
			break;
	}
	if (! OpenClipboard(hwnd))
		return 0; /* someone is using the clipboard */
	handle = GetClipboardData(CF_TEXT);
	if (! handle) {
		CloseClipboard();
		return NULL; /* no text data after all */
	}
	ptr = (char *) GlobalLock(handle);

	/* strlen, being careful of far pointers */
	for (size=0; ptr[size] != '\0'; size++)
		continue;
	text = app_alloc(size+1);
	if (! text) {
		GlobalUnlock(handle);
		CloseClipboard();
		return NULL; /* no memory remaining */
	}

	/* strcpy, being careful of far pointers */
	/* also turn "\r\n" into "\n", or "\r" to "\n" */
	for (n=0; *ptr != '\0'; n++) {
		ch = *ptr++;
		if (ch == '\r') {
			if (*ptr == '\n')
				ch = *ptr++;
			else
				ch = '\n';
		}
		text[n] = ch;
	}
	text[n] = '\0';

	/* tidy up */
	GlobalUnlock(handle);
	CloseClipboard();

	/* Correct ISO Latin 1 strings into UTF-8 strings. */

	utf8 = app_correct_utf8(text, &n);
	if ((utf8 != NULL) && (utf8 != text)) {
		app_free(text);
		text = utf8;
	}

	return text;
}
