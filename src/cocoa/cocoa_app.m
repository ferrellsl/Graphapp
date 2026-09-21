/*
 *  Application start-up, the event pump, and screen queries.
 *
 *  Platform: macOS (Objective-C, no GraphApp headers).
 */

#import "cocoa_internal.h"

GABCallbacks gab_callbacks;
int gab_pressed_buttons = 0;

void gab_set_callbacks(const GABCallbacks *cb)
{
	gab_callbacks = *cb;
}

CGFloat gab_screen_height(void)
{
	NSArray<NSScreen *> *screens = [NSScreen screens];
	if ([screens count] == 0)
		return 0;
	return [[screens objectAtIndex:0] frame].size.height;
}

/*
 *  Application delegate: Quit (menu or Dock) is reported to the C side,
 *  which hides the windows so that the main loop ends normally.
 */
@interface GAAppDelegate : NSObject <NSApplicationDelegate>
- (void)quit:(id)sender;
@end

@implementation GAAppDelegate

- (void)quit:(id)sender
{
	if (gab_callbacks.quit)
		gab_callbacks.quit();
	else
		[NSApp terminate:nil];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	if (gab_callbacks.quit) {
		gab_callbacks.quit();
		return NSTerminateCancel;
	}
	return NSTerminateNow;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
	return NO;
}

@end

static GAAppDelegate *app_delegate;

static void add_menus(void)
{
	NSString *name = [[NSProcessInfo processInfo] processName];
	NSMenu *bar = [[NSMenu alloc] init];
	NSMenuItem *item;
	NSMenu *menu;

	/* Application menu */
	item = [[NSMenuItem alloc] init];
	[bar addItem:item];
	menu = [[NSMenu alloc] init];
	[menu addItemWithTitle:[@"Hide " stringByAppendingString:name]
			action:@selector(hide:) keyEquivalent:@"h"];
	item = [menu addItemWithTitle:@"Hide Others"
			action:@selector(hideOtherApplications:) keyEquivalent:@"h"];
	[item setKeyEquivalentModifierMask:NSEventModifierFlagCommand | NSEventModifierFlagOption];
	[menu addItemWithTitle:@"Show All"
			action:@selector(unhideAllApplications:) keyEquivalent:@""];
	[menu addItem:[NSMenuItem separatorItem]];
	item = [menu addItemWithTitle:[@"Quit " stringByAppendingString:name]
			action:@selector(quit:) keyEquivalent:@"q"];
	[item setTarget:app_delegate];
	[[bar itemAtIndex:0] setSubmenu:menu];

	/* Window menu */
	item = [[NSMenuItem alloc] init];
	[bar addItem:item];
	menu = [[NSMenu alloc] initWithTitle:@"Window"];
	[menu addItemWithTitle:@"Minimize"
			action:@selector(performMiniaturize:) keyEquivalent:@"m"];
	[item setSubmenu:menu];
	[NSApp setWindowsMenu:menu];

	[NSApp setMainMenu:bar];
}

int gab_init(void)
{
	@autoreleasepool {
		[NSApplication sharedApplication];
		[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
		app_delegate = [[GAAppDelegate alloc] init];
		[NSApp setDelegate:app_delegate];
		add_menus();
		[NSApp finishLaunching];
		if (@available(macOS 14.0, *))
			[NSApp activate];
		else
			[NSApp activateIgnoringOtherApps:YES];
	}
	return 1;
}

void gab_screen_info(int *width, int *height, int *mm_width, int *mm_height)
{
	NSScreen *s = [[NSScreen screens] firstObject];
	CGSize mm = CGDisplayScreenSize(CGMainDisplayID());

	*width = s ? (int) [s frame].size.width : 1024;
	*height = s ? (int) [s frame].size.height : 768;
	*mm_width = (int) mm.width;
	*mm_height = (int) mm.height;
}

void gab_beep(void)
{
	NSBeep();
}

/*
 *  Translate a mouse event aimed at an App window.
 *  Also tracks which buttons are down.
 */
BOOL gab_translate_mouse(NSEvent *e, GABMouse *out)
{
	int kind, bit = 0;
	NSUInteger f;
	GAWindow *win;
	NSPoint p;

	switch ([e type]) {
	  case NSEventTypeLeftMouseDown:   kind = GAB_MOUSE_DOWN; bit = 1; break;
	  case NSEventTypeRightMouseDown:  kind = GAB_MOUSE_DOWN; bit = 2; break;
	  case NSEventTypeOtherMouseDown:  kind = GAB_MOUSE_DOWN; bit = 4; break;
	  case NSEventTypeLeftMouseUp:     kind = GAB_MOUSE_UP;   bit = 1; break;
	  case NSEventTypeRightMouseUp:    kind = GAB_MOUSE_UP;   bit = 2; break;
	  case NSEventTypeOtherMouseUp:    kind = GAB_MOUSE_UP;   bit = 4; break;
	  case NSEventTypeLeftMouseDragged:
	  case NSEventTypeRightMouseDragged:
	  case NSEventTypeOtherMouseDragged: kind = GAB_MOUSE_DRAG; break;
	  case NSEventTypeMouseMoved:      kind = GAB_MOUSE_MOVE; break;
	  default:
		return NO;
	}

	if (! [[e window] isKindOfClass:[GAWindow class]])
		return NO;
	win = (GAWindow *) [e window];
	p = [[win contentView] convertPoint:[e locationInWindow] fromView:nil];

	/* Clicks on the title bar or frame belong to the system. */
	if (kind == GAB_MOUSE_DOWN && ! NSPointInRect(p, [[win contentView] bounds]))
		return NO;

	if (kind == GAB_MOUSE_DOWN)
		gab_pressed_buttons |= bit;
	else if (kind == GAB_MOUSE_UP)
		gab_pressed_buttons &= ~bit;

	f = [e modifierFlags];
	out->win = win->user;
	out->kind = kind;
	out->pressed = gab_pressed_buttons;
	out->mods = ((f & NSEventModifierFlagShift) ? GAB_MOD_SHIFT : 0) |
		    ((f & NSEventModifierFlagControl) ? GAB_MOD_CTRL : 0) |
		    ((f & NSEventModifierFlagOption) ? GAB_MOD_ALT : 0) |
		    ((f & NSEventModifierFlagCommand) ? GAB_MOD_CMD : 0);
	out->x = (int) floor(p.x);
	out->y = (int) floor(p.y);
	return YES;
}

/*
 *  Wait up to timeout_ms for one event and handle it.
 *  See bridge.h.
 */
int gab_next_event(int timeout_ms, GABMouse *mouse)
{
	int result = GAB_EV_OTHER;

	@autoreleasepool {
		NSDate *until;
		NSEvent *e;

		if (timeout_ms < 0)
			until = [NSDate distantFuture];
		else if (timeout_ms == 0)
			until = [NSDate distantPast];
		else
			until = [NSDate dateWithTimeIntervalSinceNow:
					timeout_ms / 1000.0];

		e = [NSApp nextEventMatchingMask:NSEventMaskAny
				untilDate:until
				inMode:NSDefaultRunLoopMode
				dequeue:YES];
		if (e == nil)
			return GAB_EV_NONE;

		if (mouse != NULL) {
			if (gab_translate_mouse(e, mouse))
				return GAB_EV_MOUSE;
			if ([e type] == NSEventTypeKeyDown &&
			    [[e window] isKindOfClass:[GAWindow class]]) {
				/* leave it for the caller's next event loop */
				[NSApp postEvent:e atStart:YES];
				return GAB_EV_KEY;
			}
		}

		[NSApp sendEvent:e];
	}
	return result;
}

int gab_events_pending(void)
{
	@autoreleasepool {
		NSEvent *e = [NSApp nextEventMatchingMask:NSEventMaskAny
				untilDate:[NSDate distantPast]
				inMode:NSDefaultRunLoopMode
				dequeue:NO];
		return e != nil;
	}
}
