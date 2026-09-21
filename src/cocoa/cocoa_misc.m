/*
 *  Clipboard and cursors.
 *
 *  Platform: macOS (Objective-C, no GraphApp headers).
 */

#import <Cocoa/Cocoa.h>
#include "bridge.h"

/*
 *  Clipboard.
 */

char *gab_clipboard_get(void)
{
	char *result = NULL;

	@autoreleasepool {
		NSString *s = [[NSPasteboard generalPasteboard]
				stringForType:NSPasteboardTypeString];
		if (s != nil) {
			const char *u = [s UTF8String];
			if (u)
				result = strdup(u);
		}
	}
	return result;
}

int gab_clipboard_set(const char *utf8)
{
	int ok = 0;

	@autoreleasepool {
		NSString *s = [NSString stringWithUTF8String:utf8];
		NSPasteboard *pb = [NSPasteboard generalPasteboard];

		if (s != nil) {
			[pb clearContents];
			ok = [pb setString:s forType:NSPasteboardTypeString] ? 1 : 0;
		}
	}
	return ok;
}

/*
 *  Cursors.  Handles are retained NSCursor objects.
 */

/* GraphApp's StandardCursors enum values (see app.h). */
enum {
	C_BLANK, C_ARROW, C_WAIT, C_CARET, C_CROSS, C_HAND, C_GRAB, C_POINTING,
	C_PENCIL, C_LASSO, C_DROPPER, C_MAGNIFY, C_MAGPLUS, C_MAGMINUS,
	C_SIZELR, C_SIZETB
};

void *gab_cursor_standard(int shape)
{
	NSCursor *c;

	@autoreleasepool {
		switch (shape) {
		  case C_BLANK: {
			NSImage *img = [[NSImage alloc] initWithSize:NSMakeSize(1, 1)];
			c = [[NSCursor alloc] initWithImage:img hotSpot:NSZeroPoint];
			break;
		  }
		  case C_CARET:    c = [NSCursor IBeamCursor]; break;
		  case C_CROSS:
		  case C_PENCIL:
		  case C_LASSO:
		  case C_DROPPER:
		  case C_MAGNIFY:
		  case C_MAGPLUS:
		  case C_MAGMINUS: c = [NSCursor crosshairCursor]; break;
		  case C_HAND:     c = [NSCursor openHandCursor]; break;
		  case C_GRAB:     c = [NSCursor closedHandCursor]; break;
		  case C_POINTING: c = [NSCursor pointingHandCursor]; break;
		  case C_SIZELR:   c = [NSCursor resizeLeftRightCursor]; break;
		  case C_SIZETB:   c = [NSCursor resizeUpDownCursor]; break;
		  case C_WAIT:	/* macOS has no public wait cursor */
		  case C_ARROW:
		  default:         c = [NSCursor arrowCursor]; break;
		}
	}
	return (void *) CFBridgingRetain(c);
}

void *gab_cursor_from_argb(const uint32_t *argb, int w, int h, int hx, int hy)
{
	NSCursor *c = nil;

	@autoreleasepool {
		NSBitmapImageRep *rep = [[NSBitmapImageRep alloc]
			initWithBitmapDataPlanes:NULL
			pixelsWide:w pixelsHigh:h bitsPerSample:8 samplesPerPixel:4
			hasAlpha:YES isPlanar:NO
			colorSpaceName:NSDeviceRGBColorSpace
			bytesPerRow:0 bitsPerPixel:32];
		unsigned char *data = [rep bitmapData];
		NSInteger stride = [rep bytesPerRow];
		NSImage *img;
		int x, y;

		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				uint32_t p = argb[y * w + x];
				unsigned char *d = data + y * stride + x * 4;
				unsigned a = p >> 24;
				d[0] = (unsigned char) ((((p >> 16) & 0xFF) * a) / 255);
				d[1] = (unsigned char) ((((p >> 8) & 0xFF) * a) / 255);
				d[2] = (unsigned char) (((p & 0xFF) * a) / 255);
				d[3] = (unsigned char) a;
			}
		}
		img = [[NSImage alloc] initWithSize:NSMakeSize(w, h)];
		[img addRepresentation:rep];
		c = [[NSCursor alloc] initWithImage:img hotSpot:NSMakePoint(hx, hy)];
	}
	return (void *) CFBridgingRetain(c);
}

void gab_cursor_release(void *cursor)
{
	if (cursor)
		CFRelease(cursor);
}

void gab_cursor_get_position(int *x, int *y)
{
	CGEventRef e = CGEventCreate(NULL);
	CGPoint p = CGEventGetLocation(e);

	CFRelease(e);
	*x = (int) p.x;
	*y = (int) p.y;
}

void gab_cursor_set_position(int x, int y)
{
	CGWarpMouseCursorPosition(CGPointMake(x, y));
}
