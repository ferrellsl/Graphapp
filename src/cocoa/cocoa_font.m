/*
 *  Native fonts, using CoreText.
 *
 *  Platform: macOS (Objective-C, no GraphApp headers).
 *
 *  A font is a CTFont scaled so that ascent + descent equals the pixel
 *  height the App asked for.  Text is rendered into an 8-bit coverage
 *  bitmap that the C side blends onto its pixel surface.
 */

#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>
#include "bridge.h"

typedef struct GABFont {
	CTFontRef	font;
	int		ascent;
	int		descent;
} GABFont;

/* Make a font of the given family and point size; NULL family = system. */
static CTFontRef make_font(NSString *family, CGFloat size, int bold, int italic)
{
	CTFontRef f = NULL, styled;
	CTFontSymbolicTraits traits =
		(bold ? kCTFontTraitBold : 0) | (italic ? kCTFontTraitItalic : 0);

	if (family != nil) {
		f = CTFontCreateWithName((__bridge CFStringRef) family, size, NULL);
		if (f != NULL) {
			/* CoreText substitutes a default for unknown names;
			   prefer the system font to that. */
			NSString *got = CFBridgingRelease(CTFontCopyFamilyName(f));
			NSString *ps = CFBridgingRelease(CTFontCopyPostScriptName(f));
			if ([got caseInsensitiveCompare:family] != NSOrderedSame &&
			    [ps caseInsensitiveCompare:family] != NSOrderedSame) {
				CFRelease(f);
				f = NULL;
			}
		}
	}
	if (f == NULL) {
		/* Unknown family (GraphApp asks for "unifont" by default). Use Arial,
		   which is (most likely - unverified) what Windows' font mapper substitutes: with the size scaling
		   below (line height = ascent+descent = the requested pixels) it then
		   reproduces the Windows metrics that dialog layouts were tuned against.
		   Helvetica reports a tighter line height, so it comes out ~15% larger;
		   San Francisco (the system font) is a little wider. Both wrap labels. */
		f = CTFontCreateWithName(CFSTR("Arial"), size, NULL);
	}
	if (f == NULL)
		f = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, size, NULL);
	if (f == NULL)
		return NULL;

	if (traits != 0) {
		styled = CTFontCreateCopyWithSymbolicTraits(f, size, NULL, traits, traits);
		if (styled != NULL) {
			CFRelease(f);
			f = styled;
		}
	}
	return f;
}

void *gab_font_create(const char *name, int bold, int italic, int pixel_height,
			int *ascent, int *descent, int *max_width)
{
	GABFont *gf;
	CTFontRef f, scaled;
	NSString *family = name ? [NSString stringWithUTF8String:name] : nil;
	CGFloat size, total;
	int a;

	if (pixel_height < 1)
		pixel_height = 1;

	@autoreleasepool {
		f = make_font(family, pixel_height, bold, italic);
		if (f == NULL)
			return NULL;

		/* Rescale so the line height (ascent+descent) is pixel_height. */
		total = CTFontGetAscent(f) + CTFontGetDescent(f);
		if (total > 0) {
			size = pixel_height * pixel_height / total;
			scaled = CTFontCreateCopyWithAttributes(f, size, NULL, NULL);
			if (scaled != NULL) {
				CFRelease(f);
				f = scaled;
			}
		}

		gf = calloc(1, sizeof(GABFont));
		gf->font = f;
		a = (int) lround(CTFontGetAscent(f));
		if (a > pixel_height)
			a = pixel_height;
		gf->ascent = a;
		gf->descent = pixel_height - a;

		*ascent = gf->ascent;
		*descent = gf->descent;
		*max_width = (int) ceil(CTFontGetBoundingBox(f).size.width);
		if (*max_width < 1)
			*max_width = pixel_height;
	}
	return gf;
}

void gab_font_release(void *font)
{
	GABFont *gf = font;

	if (gf) {
		if (gf->font)
			CFRelease(gf->font);
		free(gf);
	}
}

/* A CTLine for a UTF-8 string, or NULL.  Caller releases. */
static CTLineRef make_line(GABFont *gf, const char *utf8, int nbytes)
{
	NSString *s = [[NSString alloc] initWithBytes:utf8 length:nbytes
				encoding:NSUTF8StringEncoding];
	NSDictionary *attrs;
	NSAttributedString *as;

	if (s == nil)
		s = [[NSString alloc] initWithBytes:utf8 length:nbytes
				encoding:NSISOLatin1StringEncoding];
	if (s == nil)
		return NULL;
	attrs = @{
		(__bridge id) kCTFontAttributeName: (__bridge id) gf->font,
		(__bridge id) kCTForegroundColorFromContextAttributeName: @YES
	};
	as = [[NSAttributedString alloc] initWithString:s attributes:attrs];
	return CTLineCreateWithAttributedString((__bridge CFAttributedStringRef) as);
}

int gab_font_string_width(void *font, const char *utf8, int nbytes)
{
	GABFont *gf = font;
	int w = 0;

	if (gf == NULL || nbytes <= 0)
		return 0;
	@autoreleasepool {
		CTLineRef line = make_line(gf, utf8, nbytes);
		if (line) {
			w = (int) lround(CTLineGetTypographicBounds(line, NULL, NULL, NULL));
			CFRelease(line);
		}
	}
	return w;
}

int gab_font_render(void *font, const char *utf8, int nbytes,
			uint8_t **coverage, int *w, int *h)
{
	GABFont *gf = font;
	int ok = 0;

	*coverage = NULL;
	*w = *h = 0;
	if (gf == NULL || nbytes <= 0)
		return 0;

	@autoreleasepool {
		CTLineRef line = make_line(gf, utf8, nbytes);
		if (line) {
			int width = (int) ceil(CTLineGetTypographicBounds(line,
					NULL, NULL, NULL)) + 2;
			int height = gf->ascent + gf->descent;
			uint8_t *buf = calloc((size_t) width * height, 1);
			CGColorSpaceRef gray = CGColorSpaceCreateWithName(kCGColorSpaceGenericGray);
			CGContextRef ctx = CGBitmapContextCreate(buf, width, height, 8,
					width, gray, (CGBitmapInfo) kCGImageAlphaNone);

			CGColorSpaceRelease(gray);
			if (buf && ctx) {
				CGContextSetShouldAntialias(ctx, true);
				CGContextSetShouldSmoothFonts(ctx, false);
				CGContextSetShouldSubpixelPositionFonts(ctx, false);
				CGContextSetGrayFillColor(ctx, 1.0, 1.0);
				/* memory row 0 is the top; baseline sits 'descent' up */
				CGContextSetTextPosition(ctx, 0, gf->descent);
				CTLineDraw(line, ctx);
				*coverage = buf;
				*w = width;
				*h = height;
				ok = 1;
			}
			else
				free(buf);
			if (ctx)
				CGContextRelease(ctx);
			CFRelease(line);
		}
	}
	return ok;
}
