/*
 *  Char
 *  ----
 *
 *  A program to draw pages from the Unicode standard.
 *
 *  Select a page by clicking on the left hand side of
 *  the window, then the right hand side will display the
 *  glyphs from that page. If a glyph is absent from the
 *  font, a small rectangle will appear instead.
 *
 *  Clicking on the right hand side will display that
 *  glyph's Unicode value in decimal and hex.
 *  It will also copy that Unicode value as a UTF-8
 *  byte sequence into the clipboard, ready to be
 *  pasted into a GraphApp program, or indeed into
 *  any program which understands UTF-8.
 *
 *  This program is based loosely on the 'char' utility
 *  described in the Plan 9 Programmer's Manual Volume 1.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graphapp.h>

Window *w;
Font *f;
long page = 0L;
long code = 0L;

/*
 *  Unicode values to display in the summary panel:
 */
long summary_char[] = {
	0x00000061,	/* subfont 0000 */
	0x00000101,	/* subfont 0001 */
	0x00000250,	/* subfont 0002 */
	0x000003A6,	/* subfont 0003 */
	0x0000042F,	/* subfont 0004 */
	0x000005D0,	/* subfont 0005 */
	0x00000634,	/* subfont 0006 */
	0x00000700,	/* subfont 0007 */
	0x00000800,	/* subfont 0008 */
	0x00000905,	/* subfont 0009 */
	0x00000A00,	/* subfont 000A */
	0x00000B00,	/* subfont 000B */
	0x00000C00,	/* subfont 000C */
	0x00000D00,	/* subfont 000D */
	0x00000E0D,	/* subfont 000E */
	0x00000F40,	/* subfont 000F */
	0x000010D2,	/* subfont 0010 */
	0x00001100,	/* subfont 0011 */
	0x00001200,	/* subfont 0012 */
	0x00001300,	/* subfont 0013 */
	0x00001401,	/* subfont 0014 */
	0x00001500,	/* subfont 0015 */
	0x000016A0,	/* subfont 0016 */
	0x00001700,	/* subfont 0017 */
	0x00001800,	/* subfont 0018 */
	0x00001900,	/* subfont 0019 */
	0x00001A00,	/* subfont 001A */
	0x00001B00,	/* subfont 001B */
	0x00001C00,	/* subfont 001C */
	0x00001D00,	/* subfont 001D */
	0x00001E00,	/* subfont 001E */
	0x00001F00,	/* subfont 001F */
	0x0000201C,	/* subfont 0020 */
	0x00002122,	/* subfont 0021 */
	0x00002234,	/* subfont 0022 */
	0x00002318,	/* subfont 0023 */
	0x00002460,	/* subfont 0024 */
	0x00002569,	/* subfont 0025 */
	0x00002640,	/* subfont 0026 */
	0x00002702,	/* subfont 0027 */
	0x00002836,	/* subfont 0028 */
	0x00002900,	/* subfont 0029 */
	0x00002A00,	/* subfont 002A */
	0x00002B00,	/* subfont 002B */
	0x00002C00,	/* subfont 002C */
	0x00002D00,	/* subfont 002D */
	0x00002E00,	/* subfont 002E */
	0x00002FFB,	/* subfont 002F */
	0x00003042,	/* subfont 0030 */
	0x00003106,	/* subfont 0031 */
	0x00003276,	/* subfont 0032 */
	0x0000338F,	/* subfont 0033 */
	0x00003400,	/* subfont 0034 */
	0x00003500,	/* subfont 0035 */
	0x00003600,	/* subfont 0036 */
	0x00003700,	/* subfont 0037 */
	0x00003800,	/* subfont 0038 */
	0x00003900,	/* subfont 0039 */
	0x00003A00,	/* subfont 003A */
	0x00003B00,	/* subfont 003B */
	0x00003C00,	/* subfont 003C */
	0x00003D00,	/* subfont 003D */
	0x00003E00,	/* subfont 003E */
	0x00003F00,	/* subfont 003F */
	0x00004000,	/* subfont 0040 */
	0x00004100,	/* subfont 0041 */
	0x00004200,	/* subfont 0042 */
	0x00004300,	/* subfont 0043 */
	0x00004400,	/* subfont 0044 */
	0x00004500,	/* subfont 0045 */
	0x00004600,	/* subfont 0046 */
	0x00004700,	/* subfont 0047 */
	0x00004800,	/* subfont 0048 */
	0x00004900,	/* subfont 0049 */
	0x00004A00,	/* subfont 004A */
	0x00004B00,	/* subfont 004B */
	0x00004C00,	/* subfont 004C */
	0x00004D00,	/* subfont 004D */
	0x00004E23,	/* subfont 004E */
	0x00004F00,	/* subfont 004F */
	0x00005000,	/* subfont 0050 */
	0x00005100,	/* subfont 0051 */
	0x00005200,	/* subfont 0052 */
	0x00005300,	/* subfont 0053 */
	0x00005401,	/* subfont 0054 */
	0x00005500,	/* subfont 0055 */
	0x00005600,	/* subfont 0056 */
	0x00005700,	/* subfont 0057 */
	0x00005801,	/* subfont 0058 */
	0x00005902,	/* subfont 0059 */
	0x00005A00,	/* subfont 005A */
	0x00005B01,	/* subfont 005B */
	0x00005C01,	/* subfont 005C */
	0x00005D00,	/* subfont 005D */
	0x00005E00,	/* subfont 005E */
	0x00005F00,	/* subfont 005F */
	0x00006000,	/* subfont 0060 */
	0x00006100,	/* subfont 0061 */
	0x00006200,	/* subfont 0062 */
	0x00006300,	/* subfont 0063 */
	0x00006400,	/* subfont 0064 */
	0x00006500,	/* subfont 0065 */
	0x00006600,	/* subfont 0066 */
	0x00006700,	/* subfont 0067 */
	0x00006800,	/* subfont 0068 */
	0x00006900,	/* subfont 0069 */
	0x00006A00,	/* subfont 006A */
	0x00006B00,	/* subfont 006B */
	0x00006C00,	/* subfont 006C */
	0x00006D00,	/* subfont 006D */
	0x00006E00,	/* subfont 006E */
	0x00006F00,	/* subfont 006F */
	0x00007000,	/* subfont 0070 */
	0x00007100,	/* subfont 0071 */
	0x00007200,	/* subfont 0072 */
	0x00007300,	/* subfont 0073 */
	0x00007400,	/* subfont 0074 */
	0x00007500,	/* subfont 0075 */
	0x00007600,	/* subfont 0076 */
	0x00007700,	/* subfont 0077 */
	0x00007800,	/* subfont 0078 */
	0x00007900,	/* subfont 0079 */
	0x00007A00,	/* subfont 007A */
	0x00007B00,	/* subfont 007B */
	0x00007C00,	/* subfont 007C */
	0x00007D00,	/* subfont 007D */
	0x00007E00,	/* subfont 007E */
	0x00007F00,	/* subfont 007F */
	0x00008000,	/* subfont 0080 */
	0x00008100,	/* subfont 0081 */
	0x00008200,	/* subfont 0082 */
	0x00008301,	/* subfont 0083 */
	0x00008401,	/* subfont 0084 */
	0x00008500,	/* subfont 0085 */
	0x00008600,	/* subfont 0086 */
	0x00008700,	/* subfont 0087 */
	0x00008800,	/* subfont 0088 */
	0x00008901,	/* subfont 0089 */
	0x00008A00,	/* subfont 008A */
	0x00008B00,	/* subfont 008B */
	0x00008C00,	/* subfont 008C */
	0x00008D00,	/* subfont 008D */
	0x00008E00,	/* subfont 008E */
	0x00008F00,	/* subfont 008F */
	0x00009000,	/* subfont 0090 */
	0x00009100,	/* subfont 0091 */
	0x00009200,	/* subfont 0092 */
	0x00009300,	/* subfont 0093 */
	0x00009400,	/* subfont 0094 */
	0x00009500,	/* subfont 0095 */
	0x00009600,	/* subfont 0096 */
	0x00009700,	/* subfont 0097 */
	0x00009800,	/* subfont 0098 */
	0x00009902,	/* subfont 0099 */
	0x00009A01,	/* subfont 009A */
	0x00009B00,	/* subfont 009B */
	0x00009C00,	/* subfont 009C */
	0x00009D00,	/* subfont 009D */
	0x00009E00,	/* subfont 009E */
	0x00009F00,	/* subfont 009F */
	0x0000A000,	/* subfont 00A0 */
	0x0000A100,	/* subfont 00A1 */
	0x0000A200,	/* subfont 00A2 */
	0x0000A300,	/* subfont 00A3 */
	0x0000A400,	/* subfont 00A4 */
	0x0000A500,	/* subfont 00A5 */
	0x0000A600,	/* subfont 00A6 */
	0x0000A700,	/* subfont 00A7 */
	0x0000A800,	/* subfont 00A8 */
	0x0000A900,	/* subfont 00A9 */
	0x0000AA00,	/* subfont 00AA */
	0x0000AB00,	/* subfont 00AB */
	0x0000AC00,	/* subfont 00AC */
	0x0000AD00,	/* subfont 00AD */
	0x0000AE00,	/* subfont 00AE */
	0x0000AF00,	/* subfont 00AF */
	0x0000B000,	/* subfont 00B0 */
	0x0000B100,	/* subfont 00B1 */
	0x0000B200,	/* subfont 00B2 */
	0x0000B300,	/* subfont 00B3 */
	0x0000B400,	/* subfont 00B4 */
	0x0000B500,	/* subfont 00B5 */
	0x0000B600,	/* subfont 00B6 */
	0x0000B700,	/* subfont 00B7 */
	0x0000B800,	/* subfont 00B8 */
	0x0000B900,	/* subfont 00B9 */
	0x0000BA00,	/* subfont 00BA */
	0x0000BB00,	/* subfont 00BB */
	0x0000BC00,	/* subfont 00BC */
	0x0000BD00,	/* subfont 00BD */
	0x0000BE00,	/* subfont 00BE */
	0x0000BF00,	/* subfont 00BF */
	0x0000C000,	/* subfont 00C0 */
	0x0000C100,	/* subfont 00C1 */
	0x0000C200,	/* subfont 00C2 */
	0x0000C300,	/* subfont 00C3 */
	0x0000C400,	/* subfont 00C4 */
	0x0000C500,	/* subfont 00C5 */
	0x0000C600,	/* subfont 00C6 */
	0x0000C700,	/* subfont 00C7 */
	0x0000C800,	/* subfont 00C8 */
	0x0000C900,	/* subfont 00C9 */
	0x0000CA00,	/* subfont 00CA */
	0x0000CB00,	/* subfont 00CB */
	0x0000CC00,	/* subfont 00CC */
	0x0000CD00,	/* subfont 00CD */
	0x0000CE00,	/* subfont 00CE */
	0x0000CF00,	/* subfont 00CF */
	0x0000D000,	/* subfont 00D0 */
	0x0000D100,	/* subfont 00D1 */
	0x0000D200,	/* subfont 00D2 */
	0x0000D300,	/* subfont 00D3 */
	0x0000D400,	/* subfont 00D4 */
	0x0000D500,	/* subfont 00D5 */
	0x0000D600,	/* subfont 00D6 */
	0x0000D700,	/* subfont 00D7 */
	0x0000D800,	/* subfont 00D8 */
	0x0000D900,	/* subfont 00D9 */
	0x0000DA00,	/* subfont 00DA */
	0x0000DB00,	/* subfont 00DB */
	0x0000DC00,	/* subfont 00DC */
	0x0000DD00,	/* subfont 00DD */
	0x0000DE00,	/* subfont 00DE */
	0x0000DF00,	/* subfont 00DF */
	0x0000E000,	/* subfont 00E0 */
	0x0000E100,	/* subfont 00E1 */
	0x0000E200,	/* subfont 00E2 */
	0x0000E300,	/* subfont 00E3 */
	0x0000E400,	/* subfont 00E4 */
	0x0000E500,	/* subfont 00E5 */
	0x0000E600,	/* subfont 00E6 */
	0x0000E700,	/* subfont 00E7 */
	0x0000E800,	/* subfont 00E8 */
	0x0000E900,	/* subfont 00E9 */
	0x0000EA00,	/* subfont 00EA */
	0x0000EB00,	/* subfont 00EB */
	0x0000EC00,	/* subfont 00EC */
	0x0000ED00,	/* subfont 00ED */
	0x0000EE00,	/* subfont 00EE */
	0x0000EF00,	/* subfont 00EF */
	0x0000F000,	/* subfont 00F0 */
	0x0000F100,	/* subfont 00F1 */
	0x0000F200,	/* subfont 00F2 */
	0x0000F300,	/* subfont 00F3 */
	0x0000F400,	/* subfont 00F4 */
	0x0000F500,	/* subfont 00F5 */
	0x0000F600,	/* subfont 00F6 */
	0x0000F700,	/* subfont 00F7 */
	0x0000F800,	/* subfont 00F8 */
	0x0000F900,	/* subfont 00F9 */
	0x0000FA00,	/* subfont 00FA */
	0x0000FB01,	/* subfont 00FB */
	0x0000FC0E,	/* subfont 00FC */
	0x0000FD88,	/* subfont 00FD */
	0x0000FE4C,	/* subfont 00FE */
	0x0000FFEB,	/* subfont 00FF */
	0x00010000,	/* subfont 0100 */
	0x00010100,	/* subfont 0101 */
	0x00010200,	/* subfont 0102 */
	0x00010300,	/* subfont 0103 */
	0x00010400,	/* subfont 0104 */
	0x00010500,	/* subfont 0105 */
	0x00010600,	/* subfont 0106 */
	0x00010700,	/* subfont 0107 */
	0x00010800,	/* subfont 0108 */
	0x00010900,	/* subfont 0109 */
	0x00010A00,	/* subfont 010A */
	0x00010B00,	/* subfont 010B */
	0x00010C00,	/* subfont 010C */
	0x00010D00,	/* subfont 010D */
	0x00010E00,	/* subfont 010E */
	0x00010F00,	/* subfont 010F */
	0x00011000,	/* subfont 0110 */
	0x00011100,	/* subfont 0111 */
	0x00011200,	/* subfont 0112 */
	0x00011300,	/* subfont 0113 */
	0x00011400,	/* subfont 0114 */
	0x00011500,	/* subfont 0115 */
	0x00011600,	/* subfont 0116 */
	0x00011700,	/* subfont 0117 */
	0x00011800,	/* subfont 0118 */
	0x00011900,	/* subfont 0119 */
	0x00011A00,	/* subfont 011A */
	0x00011B00,	/* subfont 011B */
	0x00011C00,	/* subfont 011C */
	0x00011D00,	/* subfont 011D */
	0x00011E00,	/* subfont 011E */
	0x00011F00,	/* subfont 011F */
	0x00012000,	/* subfont 0120 */
	0x00012100,	/* subfont 0121 */
	0x00012200,	/* subfont 0122 */
	0x00012300,	/* subfont 0123 */
	0x00012400,	/* subfont 0124 */
	0x00012500,	/* subfont 0125 */
	0x00012600,	/* subfont 0126 */
	0x00012700,	/* subfont 0127 */
	0x00012800,	/* subfont 0128 */
	0x00012900,	/* subfont 0129 */
	0x00012A00,	/* subfont 012A */
	0x00012B00,	/* subfont 012B */
	0x00012C00,	/* subfont 012C */
	0x00012D00,	/* subfont 012D */
	0x00012E00,	/* subfont 012E */
	0x00012F00,	/* subfont 012F */
	0x00013000,	/* subfont 0130 */
	0x00013100,	/* subfont 0131 */
	0x00013200,	/* subfont 0132 */
	0x00013300,	/* subfont 0133 */
	0x00013400,	/* subfont 0134 */
	0x00013500,	/* subfont 0135 */
	0x00013600,	/* subfont 0136 */
	0x00013700,	/* subfont 0137 */
	0x00013800,	/* subfont 0138 */
	0x00013900,	/* subfont 0139 */
	0x00013A00,	/* subfont 013A */
	0x00013B00,	/* subfont 013B */
	0x00013C00,	/* subfont 013C */
	0x00013D00,	/* subfont 013D */
	0x00013E00,	/* subfont 013E */
	0x00013F00,	/* subfont 013F */
};

/*
 *  Functions:
 */
void copy_to_clipboard(Char ch)
{
	char buf[10];

	unicode_char_to_utf8(ch, buf);
	set_clipboard_text(w->app, buf);
}

void draw_info(Window *w, Graphics *g)
{
	Control *c = get_window_data(w);
	Rect r = c->area;
	char buffer[80];

	set_rgb(g, WHITE);
	fill_rect(g, get_window_area(w));
	set_rgb(g, BLACK);
	set_font(g, f);

	sprintf(buffer, "Unicode page %ld (0x%08lx)", page, page);
	draw_utf8(g, pt(r.x, r.y-32), buffer, strlen(buffer));

	sprintf(buffer, "Unicode value %ld (0x%08lx)", page+code, page+code);
	draw_utf8(g, pt(r.x, r.y+r.height+16), buffer, strlen(buffer));
}

void draw_summary(Control *c, Graphics *g)
{
	long i;
	Rect r;
	char buf[10];
	Char ch;

	r = get_control_area(c);
	set_rgb(g, LIGHT_GREY);
	draw_rect(g, r);
	set_rgb(g, BLACK);
	set_font(g, f);

	for (i=0; i < 16*20; i++) {
		if (i < sizeof(summary_char)/sizeof(summary_char[0]))
			ch = summary_char[i]; /* choose known glyph */
		else
			ch = i * 256; /* choose first position glyph */
		unicode_char_to_utf8(ch, buf);
		draw_utf8(g, pt((i%16)*17+1, (i/16)*17+1), buf, strlen(buf));
	}
}

void summary_mouse(Control *c, int buttons, Point p)
{
	page = (p.x-1)/17 + (p.y-1)/17*16;
	if (page < 0)
		page = 0;
	if (page >= 16*20)
		page = 16*20-1;
	page *= 256;

	draw_window(w);
	redraw_control(get_control_data(c));
}

void page_mouse(Control *c, int buttons, Point p)
{
	code = (p.x-1)/17 + (p.y-1)/17*16;
	if (code < 0)
		code = 0;
	if (code > 255)
		code = 255;

	draw_window(w);

	copy_to_clipboard(page + code);
}

void draw_page(Control *c, Graphics *g)
{
	long i;
	Rect r;
	char buf[10];
	Char ch;

	r = get_control_area(c);
	set_rgb(g, LIGHT_GREY);
	draw_rect(g, r);
	set_rgb(g, BLACK);
	set_font(g, f);

	for (i=0; i < 16*16; i++) {
		ch = page + i;
		unicode_char_to_utf8(ch, buf);
		draw_utf8(g, pt((i%16)*17+1, (i/16)*17+1), buf, strlen(buf));
	}
}

int main(int argc, char *argv[])
{
	App *app;
	Control *c1, *c2;
	Rect r;

	app = new_app(argc, argv);
	w = new_window(app, rect(10,10,600,366),
			"Unicode Char Display",
			STANDARD_WINDOW);

	f = new_font(app, "cp1252", PLAIN, 16);

	r = rect(10,10,17*16+2,17*20+2);
	c1 = new_control(w, r);
	on_control_redraw(c1, draw_summary);
	on_control_mouse_down(c1, summary_mouse);

	c2 = new_control(w, rect(r.x+r.width+10,r.y+32,17*16+2,17*16+2));
	on_control_redraw(c2, draw_page);
	on_control_mouse_down(c2, page_mouse);

	set_control_data(c1, c2);

	set_window_data(w, c2);
	on_window_redraw(w, draw_info);

	show_window(w);
	main_loop(app);
	del_app(app);
	return 0;
}
