/*
 *  Internal declarations shared by the Objective-C files of the Cocoa
 *  backend.  Not for use by the C side (which sees only bridge.h).
 */

#import <Cocoa/Cocoa.h>
#include "bridge.h"

extern GABCallbacks gab_callbacks;

/* Mouse buttons currently held, as GAB pressed bits (1 L, 2 R, 4 other). */
extern int gab_pressed_buttons;

/* An App window.  'user' is the C side's Window *. */
@interface GAWindow : NSWindow <NSWindowDelegate>
{
@public
	void *user;
	int popup;
}
@end

/* The content view: shows the window's pixel surface, receives input. */
@interface GAView : NSView <NSTextInputClient>
{
@public
	void *user;
	int ready;		/* callbacks allowed once the C side has a handle */
	NSCursor *cursor;
	CGContextRef bitmap;	/* wraps the C side's pixels */
	int pw, ph;
}
- (void)setSurface:(const uint32_t *)pixels width:(int)w height:(int)h;
@end

/* Translate a mouse NSEvent for an App window. Returns NO if it isn't one. */
BOOL gab_translate_mouse(NSEvent *e, GABMouse *out);

/* Screen height in points, for flipping between Cocoa and App y. */
CGFloat gab_screen_height(void);
