/*
 *  Clipboard data-transfer functions.
 *
 *  Platform: X-Windows.
 *
 *  Version: 3.02  2001/10/10  First release.
 *  Version: 3.18  2002/01/15  Now owns CLIPBOARD and PRIMARY selection.
 *  Version: 3.32  2002/09/04  Sends/gets ISO Latin 1 strings if possible.
 *  Version: 3.35  2002/12/23  Renamed app->winlist to app->windows.
 *  Version: 3.50  2004/01/11  Uses const keyword for some param strings.
 */

/* Copyright (c) L. Patrick

   This file is part of the App cross-platform programming package.
   You may redistribute it and/or modify it under the terms of the
   App Software License. See the file LICENSE.TXT for details.
*/

#include "appint.h"

/*
 *  Ensure we have the interned strings we need.
 */
static void app_ensure_atoms(App *app)
{
	if (app_extra(app)->xsel == 0) {
		app_extra(app)->xsel =
			XInternAtom(app_extra(app)->display,
					"APP_SELECTION", False);
	}
	if (app_extra(app)->xclip == 0) {
		app_extra(app)->xclip =
			XInternAtom(app_extra(app)->display,
					"CLIPBOARD", False);
	}
}

/*
 *  To store text into the clipboard, we just use the
 *  XStoreBytes function, which stores the data onto
 *  XA_CUT_BUFFER0 in the root window of screen 0 of
 *  the X server. To retrieve it, we use XFetchBytes.
 *  This is an old method of storing data, used by
 *  xterm and some other older applications.
 *
 *  More modern X programs use selections, which are
 *  inter-process communications mediated by the
 *  SelectionRequest, SelectionNotify and SelectionClear
 *  X events. We too handle these events in the event
 *  loop, but we cheat. If someone asks our application
 *  what text we currently have selected, we just grab
 *  whatever text is in XA_CUT_BUFFER0 and send that
 *  (even if some other program has since overwritten it).
 *  This produces the desired effect of always pasting
 *  whatever text is in the system clipboard. It also
 *  gives interoperability with xterm.
 *
 *  Other applications usually only send their data
 *  when they see a SelectionRequest. Storing it instead
 *  into a system buffer, at the instant the user selects
 *  "Copy", removes some complications, because none of
 *  our controls need to handle SelectionRequests. Only
 *  the event loop need handle these data transfer requests.
 */

int app_set_clipboard_text(App *app, const char *text)
{
	int n, size, result;
	Window *win;
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
	n = size = strlen(text);
	if (app_utf8_is_ascii(text, n)) /* ASCII always works */
		;
	else if (app_utf8_is_latin1(text, n)) { /* valid candidate */
		latin1 = app_utf8_to_latin1(text, &n);
		if (latin1 != NULL) {
			utf8 = app_correct_utf8(latin1, &n);
			if ((n == size) && (!memcmp(text, utf8, n))) {
				/* success, text->latin1->utf8==text */
				text = latin1;	/* swap to new string */
				size = strlen(text);
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

	/* ensure we have the appropriate X Server atom strings */

	app_ensure_atoms(app);

	/* use the old method first */

	XStoreBytes(app_extra(app)->display, text, size);

	/* also use the modern method: owning the selection */

	win = NULL;
	for (n=0; n < app->num_windows; n++) {
		win = app->windows[n];
		if (win_extra(win)->xid)
			break;
	}
	if (! win) {
		if (latin1)
			app_free(latin1);
		return 0; /* no window to use */
	}

	result = 1;

	XSetSelectionOwner(app_extra(app)->display, app_extra(app)->xclip,
		win_extra(win)->xid, app_extra(app)->last_event_time);
	if (XGetSelectionOwner(app_extra(app)->display,
		app_extra(app)->xclip) != win_extra(win)->xid)
		result = 0; /* couldn't obtain selection ownership */

	XSetSelectionOwner(app_extra(app)->display, XA_PRIMARY,
		win_extra(win)->xid, app_extra(app)->last_event_time);
	if (XGetSelectionOwner(app_extra(app)->display,
		XA_PRIMARY) != win_extra(win)->xid)
		result = 0; /* couldn't obtain selection ownership */

	if (latin1)
		app_free(latin1);
	return result;
}

/*
 *  Private event handler, called from the event loop.
 *  To send the clipboard to a process, in response to
 *  a SelectionRequest event, we try to attach the
 *  global clipboard's data (XA_CUT_BUFFER0) to a
 *  property belonging to the requestor's window.
 *  Then we send the requestor a SelectionNotify event
 *  saying "the data is in your property, go get it".
 */
void app_send_clipboard(App *app, XSelectionRequestEvent *e)
{
	XSelectionEvent reply;
	int bytes;
	char *xdata;

	app_ensure_atoms(app);

	reply.type = SelectionNotify;
	reply.serial = app_extra(app)->last_event_number;
	reply.send_event = True;
	reply.display = app_extra(app)->display;
	reply.requestor = e->requestor;
	reply.selection = e->selection;
	reply.target = e->target;
	reply.property = None;
	reply.time = app_extra(app)->last_event_time;

	if (e->target == XA_STRING)
	{
		xdata = XFetchBytes(app_extra(app)->display, &bytes);

		XChangeProperty(e->display, e->requestor,
				e->property, e->target,
				8, PropModeReplace,
				xdata, bytes);
		XFree(xdata);

		reply.property = e->property;
	}

	XSendEvent(e->display, e->requestor, 0, NoEventMask,
			(XEvent *) &reply);
}

/*
 *  Private event handler, called from the event loop,
 *  and also from a custom event loop below.
 *  To receive data from some other process,
 *  we look for the data on a property belonging to
 *  to this process. If we find some text there, we
 *  copy it into the global clipboard XA_CUT_BUFFER0
 *  for later use.
 */
char *app_receive_clipboard(App *app, XSelectionEvent *e, long *lenptr)
{
	Atom type;
	int format;
	unsigned long nitems;
	unsigned long remaining;
	unsigned char *xdata;
	long length;

	if (lenptr)
		*lenptr = 0;

	if (e->property == None)
		return NULL;	/* oops, nothing was sent */

	app_ensure_atoms(app);

	/* First, check to see how large the data is */
	xdata = NULL;
	XGetWindowProperty(e->display, e->requestor,
		app_extra(app)->xsel, 0, 0, False,
		AnyPropertyType, &type, &format,
		&nitems, &remaining, &xdata);

	if (type == None)
		return NULL;
	if (xdata)
		XFree(xdata);

	/* Now retrieve the data, in 4-byte chunks */
	xdata = NULL;
	XGetWindowProperty(e->display, e->requestor,
		app_extra(app)->xsel, 0, (remaining+3)/4, False,
		AnyPropertyType, &type, &format,
		&nitems, &remaining, &xdata);

	if (type == None)
		return NULL;
	if (! xdata)
		return NULL;

	length = nitems * format / 8;

	/* Store the data into the clipboard */
	if (length)
		XStoreBytes(app_extra(app)->display, xdata, length);

	/* We really should check that remaining is zero before deleting */
	XDeleteProperty(e->display, e->requestor, app_extra(app)->xsel);

	if (lenptr)
		*lenptr = length;
	return xdata;
}

/*
 *  Handle clipboard reply events in a dedicated event loop.
 *  This just handles and dispatches SelectionNotify events
 *  which have been sent as a result of a request (by this
 *  process) to obtain the current selection from whoever
 *  owns the selection.
 */
static char *app_handle_clipboard_request(App *app, long *lenptr)
{
	int max_out = 0;
	XEvent e;

	while ((app->visible_windows > 0) && (max_out++ < 100))
	{
		XNextEvent(app_extra(app)->display, &e);

		switch (e.type)
		{
		  case SelectionNotify:
			app_extra(app)->last_event_time = e.xselection.time;
			if (e.xselection.property == None)
				return NULL; /* failed */

			return app_receive_clipboard(app,
				&e.xselection, lenptr);

		  default:
			continue;
		}
	}

	return NULL; /* failed to receive a clipboard reply */
}

/*
 *  Public function to obtain the contents of the clipboard.
 *  We send a SelectionRequest event to obtain the data
 *  from whoever currently owns the selection. If they reply,
 *  the data will be stored in XA_CUT_BUFFER0 by the above
 *  two functions. So we should just be able to fetch the
 *  data from there.
 */
char *app_get_clipboard_text(App *app)
{
	int n, bytes;
	long length;
	char *text, *utf8;
	char *xdata = NULL;
	Window *win;
	XID owner;

	/* try the modern method first */

	app_ensure_atoms(app);

	owner = XGetSelectionOwner(app_extra(app)->display, XA_PRIMARY);
	win = NULL;
	for (n=0; n < app->num_windows; n++) {
		win = app->windows[n];
		if (win_extra(win)->xid == owner) {
			win = NULL;	/* we already own the selection! */
			break;		/* so just get data from clipboard */
		}
	}
	if (win) {
		/* request the data from another process */

		XConvertSelection(app_extra(app)->display,
			XA_PRIMARY,		/* selection = PRIMARY */
			XA_STRING,		/* type = STRING */
			app_extra(app)->xsel,	/* name = APP_SELECTION */
			win_extra(win)->xid,
			app_extra(app)->last_event_time);

		xdata = app_handle_clipboard_request(app, &length);
	}
	if ((win == NULL) || (xdata == NULL))
	{
		/* otherwise just fetch the data from the clipboard */

		xdata = XFetchBytes(app_extra(app)->display, &bytes);
		length = bytes;
	}

	if ((length == 0) || (xdata == NULL))
		return NULL;

	text = app_alloc(length+1);
	memcpy(text, xdata, length);
	text[length] = '\0';
	XFree(xdata);

	/* Correct ISO Latin 1 strings into UTF-8 strings. */

	n = length;
	utf8 = app_correct_utf8(text, &n);
	if ((utf8 != NULL) && (utf8 != text)) {
		app_free(text);
		text = utf8;
	}

	return text;
}
