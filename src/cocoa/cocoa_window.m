/*
 *  Windows and the view that displays a window's pixel surface.
 *
 *  Platform: macOS (Objective-C, no GraphApp headers).
 */

#import "cocoa_internal.h"

/* Virtual key codes that are never typed text (arrows, F-keys, Return ...). */
static BOOL is_special_keycode(int kc)
{
	switch (kc) {
	  case 36: case 76: case 48: case 51: case 53:		/* Return Enter Tab Backspace Esc */
	  case 114: case 115: case 116: case 117: case 119: case 121:	/* Help Home PgUp Del End PgDn */
	  case 123: case 124: case 125: case 126:		/* arrows */
	  case 122: case 120: case 99: case 118: case 96:	/* F1-F5 */
	  case 97: case 98: case 100: case 101: case 109:	/* F6-F10 */
		return YES;
	}
	return NO;
}

static int gab_mods(NSEvent *e)
{
	NSUInteger f = [e modifierFlags];

	return ((f & NSEventModifierFlagShift) ? GAB_MOD_SHIFT : 0) |
	       ((f & NSEventModifierFlagControl) ? GAB_MOD_CTRL : 0) |
	       ((f & NSEventModifierFlagOption) ? GAB_MOD_ALT : 0) |
	       ((f & NSEventModifierFlagCommand) ? GAB_MOD_CMD : 0);
}

/* The first UTF-16 unit of a string (enough for key codes), or 0. */
static unsigned long first_scalar(NSString *s)
{
	if ([s length] == 0)
		return 0;
	return (unsigned long) [s characterAtIndex:0];
}

@implementation GAView

- (BOOL)isFlipped { return YES; }
- (BOOL)isOpaque { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent *)e { return YES; }

- (void)dealloc
{
	if (bitmap)
		CGContextRelease(bitmap);
}

- (void)setSurface:(const uint32_t *)pixels width:(int)w height:(int)h
{
	CGColorSpaceRef cs = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);

	if (bitmap)
		CGContextRelease(bitmap);
	bitmap = CGBitmapContextCreate((void *) pixels, w, h, 8, (size_t) w * 4, cs,
			kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst);
	CGColorSpaceRelease(cs);
	pw = w;
	ph = h;
	ready = 1;
	[self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirty
{
	CGContextRef ctx;
	CGImageRef img;

	if (bitmap == NULL)
		return;
	ctx = [[NSGraphicsContext currentContext] CGContext];
	img = CGBitmapContextCreateImage(bitmap);
	if (img == NULL)
		return;

	/* Show pixels 1:1, without smoothing, and un-flip for the flipped view. */
	CGContextSaveGState(ctx);
	CGContextSetInterpolationQuality(ctx, kCGInterpolationNone);
	CGContextTranslateCTM(ctx, 0, ph);
	CGContextScaleCTM(ctx, 1, -1);
	CGContextDrawImage(ctx, CGRectMake(0, 0, pw, ph), img);
	CGContextRestoreGState(ctx);
	CGImageRelease(img);
}

- (void)setFrameSize:(NSSize)size
{
	NSSize old = [self frame].size;

	[super setFrameSize:size];
	if (ready && gab_callbacks.resized &&
	    (old.width != size.width || old.height != size.height))
		gab_callbacks.resized(user, (int) size.width, (int) size.height);
}

- (void)resetCursorRects
{
	if (cursor)
		[self addCursorRect:[self bounds] cursor:cursor];
}

/* Mouse */

- (void)deliverMouse:(NSEvent *)e
{
	GABMouse m;

	if (gab_translate_mouse(e, &m) && ready && gab_callbacks.mouse)
		gab_callbacks.mouse(m.win, m.kind, m.pressed, m.mods, m.x, m.y);
}

- (void)mouseDown:(NSEvent *)e         { [self deliverMouse:e]; }
- (void)mouseUp:(NSEvent *)e           { [self deliverMouse:e]; }
- (void)mouseDragged:(NSEvent *)e      { [self deliverMouse:e]; }
- (void)rightMouseDown:(NSEvent *)e    { [self deliverMouse:e]; }
- (void)rightMouseUp:(NSEvent *)e      { [self deliverMouse:e]; }
- (void)rightMouseDragged:(NSEvent *)e { [self deliverMouse:e]; }
- (void)otherMouseDown:(NSEvent *)e    { [self deliverMouse:e]; }
- (void)otherMouseUp:(NSEvent *)e      { [self deliverMouse:e]; }
- (void)otherMouseDragged:(NSEvent *)e { [self deliverMouse:e]; }
- (void)mouseMoved:(NSEvent *)e        { [self deliverMouse:e]; }

/* Keyboard */

- (void)keyDown:(NSEvent *)e
{
	int kc = [e keyCode];
	int mods = gab_mods(e);

	if (! ready || ! gab_callbacks.key)
		return;

	if ((mods & (GAB_MOD_CMD | GAB_MOD_CTRL)) || is_special_keycode(kc)) {
		NSString *base = [e charactersIgnoringModifiers];
		gab_callbacks.key(user, kc, first_scalar(base), mods);
		return;
	}

	/* typed text, including dead keys and input methods */
	[self interpretKeyEvents:[NSArray arrayWithObject:e]];
}

- (BOOL)performKeyEquivalent:(NSEvent *)e
{
	/* Let the main menu have Cmd-Q, Cmd-H, Cmd-M; take other Cmd keys. */
	NSString *c = [e charactersIgnoringModifiers];

	if (([e modifierFlags] & NSEventModifierFlagCommand) && [c length] == 1) {
		unichar ch = [c characterAtIndex:0];
		if (ch == 'q' || ch == 'h' || ch == 'm')
			return NO;
		[self keyDown:e];
		return YES;
	}
	return NO;
}

/* NSTextInputClient: just enough to receive composed text. */

- (void)insertText:(id)string replacementRange:(NSRange)range
{
	NSString *s = [string isKindOfClass:[NSAttributedString class]]
			? [string string] : string;
	NSUInteger i;
	int kc = 0, mods = 0;
	NSEvent *cur = [NSApp currentEvent];

	if (cur && [cur type] == NSEventTypeKeyDown) {
		kc = [cur keyCode];
		mods = gab_mods(cur) & GAB_MOD_SHIFT;
	}
	for (i = 0; i < [s length]; ) {
		unsigned long ch;
		unichar u = [s characterAtIndex:i];
		if (u >= 0xD800 && u < 0xDC00 && i + 1 < [s length]) {
			unichar lo = [s characterAtIndex:i + 1];
			ch = 0x10000 + (((unsigned long) u - 0xD800) << 10) + (lo - 0xDC00);
			i += 2;
		}
		else {
			ch = u;
			i++;
		}
		if (ready && gab_callbacks.key)
			gab_callbacks.key(user, kc, ch, mods);
	}
}

- (void)doCommandBySelector:(SEL)selector { }
- (void)setMarkedText:(id)s selectedRange:(NSRange)sel replacementRange:(NSRange)r { }
- (void)unmarkText { }
- (NSRange)selectedRange { return NSMakeRange(NSNotFound, 0); }
- (NSRange)markedRange { return NSMakeRange(NSNotFound, 0); }
- (BOOL)hasMarkedText { return NO; }
- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)r
		actualRange:(NSRangePointer)actual { return nil; }
- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText { return @[]; }
- (NSRect)firstRectForCharacterRange:(NSRange)r actualRange:(NSRangePointer)actual
{
	return NSMakeRect(0, 0, 0, 0);
}
- (NSUInteger)characterIndexForPoint:(NSPoint)p { return NSNotFound; }

@end

@implementation GAWindow

- (BOOL)canBecomeKeyWindow { return ! popup; }
- (BOOL)canBecomeMainWindow { return ! popup; }

- (BOOL)windowShouldClose:(id)sender
{
	/* The C side decides whether the window closes (it hides it). */
	if (gab_callbacks.close)
		gab_callbacks.close(user);
	return NO;
}

- (void)windowDidMove:(NSNotification *)n
{
	if (gab_callbacks.moved)
		gab_callbacks.moved(user);
}

- (void)windowDidBecomeKey:(NSNotification *)n
{
	if (gab_callbacks.activated)
		gab_callbacks.activated(user);
}

@end

/*
 *  Window functions.
 */

static NSRect content_rect(int x, int y, int w, int h)
{
	return NSMakeRect(x, gab_screen_height() - y - h, w, h);
}

void *gab_window_create(void *user, int x, int y, int w, int h,
			int style, int centered, const char *title)
{
	NSWindowStyleMask mask = NSWindowStyleMaskBorderless;
	GAWindow *win;
	GAView *view;

	@autoreleasepool {
		if (style & GAB_TITLED) {
			mask = NSWindowStyleMaskTitled;
			if (style & GAB_CLOSABLE)
				mask |= NSWindowStyleMaskClosable;
			if (style & GAB_MINIATURE)
				mask |= NSWindowStyleMaskMiniaturizable;
			if (style & GAB_RESIZABLE)
				mask |= NSWindowStyleMaskResizable;
		}

		win = [[GAWindow alloc] initWithContentRect:content_rect(x, y, w, h)
				styleMask:mask
				backing:NSBackingStoreBuffered
				defer:NO];
		if (win == nil)
			return NULL;
		win->user = user;
		win->popup = (style & GAB_POPUP) ? 1 : 0;
		[win setDelegate:win];
		[win setReleasedWhenClosed:NO];
		[win setAcceptsMouseMovedEvents:YES];
		[win setTitle:[NSString stringWithUTF8String:title ? title : ""]];
		[win setOpaque:YES];
		if (style & GAB_POPUP) {
			[win setLevel:NSPopUpMenuWindowLevel];
			[win setHasShadow:YES];
		}
		else if (style & GAB_FLOATING)
			[win setLevel:NSFloatingWindowLevel];

		view = [[GAView alloc] initWithFrame:NSMakeRect(0, 0, w, h)];
		view->user = user;
		[win setContentView:view];
		[win makeFirstResponder:view];

		if (centered)
			[win center];
	}
	return (void *) CFBridgingRetain(win);
}

#define WIN(h) ((__bridge GAWindow *) (h))
#define VIEW(h) ((GAView *) [WIN(h) contentView])

void gab_window_destroy(void *handle)
{
	@autoreleasepool {
		GAWindow *win = WIN(handle);
		[win setDelegate:nil];
		[win orderOut:nil];
		[win close];
	}
	CFRelease(handle);
}

void gab_window_show(void *handle)
{
	@autoreleasepool {
		GAWindow *win = WIN(handle);
		if (win->popup)
			[win orderFront:nil];
		else
			[win makeKeyAndOrderFront:nil];
	}
}

void gab_window_hide(void *handle)
{
	@autoreleasepool {
		[WIN(handle) orderOut:nil];
	}
}

void gab_window_set_title(void *handle, const char *title)
{
	@autoreleasepool {
		[WIN(handle) setTitle:[NSString stringWithUTF8String:title ? title : ""]];
	}
}

void gab_window_get_client(void *handle, int *x, int *y, int *w, int *h)
{
	@autoreleasepool {
		GAWindow *win = WIN(handle);
		NSRect c = [win contentRectForFrameRect:[win frame]];

		*x = (int) c.origin.x;
		*y = (int) (gab_screen_height() - (c.origin.y + c.size.height));
		*w = (int) c.size.width;
		*h = (int) c.size.height;
	}
}

void gab_window_move(void *handle, int x, int y)
{
	@autoreleasepool {
		GAWindow *win = WIN(handle);
		NSSize s = [win contentRectForFrameRect:[win frame]].size;
		NSRect frame = [win frameRectForContentRect:
				content_rect(x, y, (int) s.width, (int) s.height)];

		[win setFrame:frame display:NO];
	}
}

void gab_window_resize(void *handle, int w, int h)
{
	@autoreleasepool {
		GAWindow *win = WIN(handle);
		int x, y, cw, ch;
		NSRect frame;

		gab_window_get_client(handle, &x, &y, &cw, &ch);
		frame = [win frameRectForContentRect:content_rect(x, y, w, h)];
		[win setFrame:frame display:NO];
	}
}

int gab_window_is_minimised(void *handle)
{
	return [WIN(handle) isMiniaturized] ? 1 : 0;
}

void gab_window_set_icon(void *handle, const uint32_t *argb, int w, int h)
{
	@autoreleasepool {
		NSBitmapImageRep *rep = [[NSBitmapImageRep alloc]
			initWithBitmapDataPlanes:NULL
			pixelsWide:w pixelsHigh:h bitsPerSample:8 samplesPerPixel:4
			hasAlpha:YES isPlanar:NO
			colorSpaceName:NSDeviceRGBColorSpace
			bytesPerRow:0 bitsPerPixel:32];
		unsigned char *data = [rep bitmapData];
		NSInteger stride = [rep bytesPerRow];
		int x, y;
		NSImage *img;

		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				uint32_t p = argb[y * w + x];
				unsigned char *d = data + y * stride + x * 4;
				unsigned a = p >> 24;
				/* premultiply, as the rep's default format expects */
				d[0] = (unsigned char) ((((p >> 16) & 0xFF) * a) / 255);
				d[1] = (unsigned char) ((((p >> 8) & 0xFF) * a) / 255);
				d[2] = (unsigned char) (((p & 0xFF) * a) / 255);
				d[3] = (unsigned char) a;
			}
		}
		img = [[NSImage alloc] initWithSize:NSMakeSize(w, h)];
		[img addRepresentation:rep];
		[NSApp setApplicationIconImage:img];
	}
}

void gab_window_set_cursor(void *handle, void *cursor)
{
	@autoreleasepool {
		GAWindow *win = WIN(handle);
		GAView *view = VIEW(handle);

		view->cursor = cursor ? (__bridge NSCursor *) cursor : nil;
		[win invalidateCursorRectsForView:view];
		/* If the pointer is over the window, change it right now. */
		if (view->cursor && [NSWindow windowNumberAtPoint:[NSEvent mouseLocation]
				belowWindowWithWindowNumber:0] == [win windowNumber])
			[view->cursor set];
	}
}

void gab_window_set_surface(void *handle, const uint32_t *pixels, int w, int h)
{
	@autoreleasepool {
		[VIEW(handle) setSurface:pixels width:w height:h];
	}
}

void gab_window_dirty(void *handle, int x, int y, int w, int h)
{
	[VIEW(handle) setNeedsDisplayInRect:NSMakeRect(x, y, w, h)];
}

void gab_flush_windows(void)
{
	@autoreleasepool {
		NSWindow *win;
		for (win in [NSApp windows])
			if ([win isKindOfClass:[GAWindow class]] && [win isVisible])
				[win displayIfNeeded];
	}
}

void *gab_window_under_cursor(void)
{
	@autoreleasepool {
		NSInteger num = [NSWindow windowNumberAtPoint:[NSEvent mouseLocation]
				belowWindowWithWindowNumber:0];
		NSWindow *w = [NSApp windowWithWindowNumber:num];

		if (w && [w isKindOfClass:[GAWindow class]])
			return ((GAWindow *) w)->user;
	}
	return NULL;
}

/*
 *  Test hooks; see bridge.h.
 */

int gab_test_snapshot(void *handle, const char *png_path)
{
	int ok = 0;

	@autoreleasepool {
		NSView *view = [WIN(handle) contentView];
		NSBitmapImageRep *rep = [view bitmapImageRepForCachingDisplayInRect:[view bounds]];
		NSData *png;

		[view cacheDisplayInRect:[view bounds] toBitmapImageRep:rep];
		png = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
		ok = [png writeToFile:[NSString stringWithUTF8String:png_path] atomically:YES] ? 1 : 0;
	}
	return ok;
}

static NSUInteger cocoa_flags(int mods)
{
	return ((mods & GAB_MOD_SHIFT) ? NSEventModifierFlagShift : 0) |
	       ((mods & GAB_MOD_CTRL) ? NSEventModifierFlagControl : 0) |
	       ((mods & GAB_MOD_ALT) ? NSEventModifierFlagOption : 0) |
	       ((mods & GAB_MOD_CMD) ? NSEventModifierFlagCommand : 0);
}

/* kind: GAB_MOUSE_*; button: 1 left, 2 right, 4 other; (x,y) view coords. */
void gab_test_mouse(void *handle, int kind, int x, int y, int button, int mods)
{
	@autoreleasepool {
		GAWindow *win = WIN(handle);
		NSEventType t;
		NSPoint p = [[win contentView] convertPoint:NSMakePoint(x, y) toView:nil];
		NSEvent *e;

		switch (kind) {
		  case GAB_MOUSE_DOWN:
			t = button == 2 ? NSEventTypeRightMouseDown : NSEventTypeLeftMouseDown; break;
		  case GAB_MOUSE_UP:
			t = button == 2 ? NSEventTypeRightMouseUp : NSEventTypeLeftMouseUp; break;
		  case GAB_MOUSE_DRAG:
			t = button == 2 ? NSEventTypeRightMouseDragged : NSEventTypeLeftMouseDragged; break;
		  default:
			t = NSEventTypeMouseMoved; break;
		}
		e = [NSEvent mouseEventWithType:t location:p
				modifierFlags:cocoa_flags(mods)
				timestamp:[[NSProcessInfo processInfo] systemUptime]
				windowNumber:[win windowNumber] context:nil
				eventNumber:0 clickCount:1 pressure:1.0];
		[NSApp postEvent:e atStart:NO];
	}
}

void gab_test_key(void *handle, int keycode, const char *chars, int mods)
{
	@autoreleasepool {
		GAWindow *win = WIN(handle);
		NSString *s = [NSString stringWithUTF8String:chars ? chars : ""];
		NSEvent *e = [NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint
				modifierFlags:cocoa_flags(mods)
				timestamp:[[NSProcessInfo processInfo] systemUptime]
				windowNumber:[win windowNumber] context:nil
				characters:s charactersIgnoringModifiers:s
				isARepeat:NO keyCode:keycode];
		[NSApp postEvent:e atStart:NO];
	}
}

void gab_test_mouse_front(int kind, int x, int y, int button, int mods)
{
	@autoreleasepool {
		NSWindow *w;
		for (w in [NSApp orderedWindows]) {
			if ([w isKindOfClass:[GAWindow class]] && [w isVisible]) {
				gab_test_mouse((__bridge void *) w, kind, x, y, button, mods);
				return;
			}
		}
	}
}
