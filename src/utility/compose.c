/*
 *  A table for composing some Unicode characters using
 *  a standard US keyboard. Handles ISO Latin 1, some other
 *  accented characters, some mathematical symbols, and
 *  the Greek and Cyrillic alphabets.
 *
 *  The principle is this:
 *  The user wants to input an accented European character.
 *  We allow the user to use two key sequences to do that.
 *
 *  Alt ~ n produces the Spanish small letter n with tilde.
 *  Alt / o produces the Danish/Norwegian small letter o with a slash.
 *  Alt ` E produces capital E with a grave accent.
 *  Alt A E produces capital AE ligature.
 *
 *  The Alt key is held down to produce the accent, then released,
 *  followed by the letter. The Shift key may also be needed to
 *  type the appropriate accent or the following character.
 *
 *  The general rules are:
 *  Alt ' produces an acute (rising line) above a letter.
 *  Alt ` produces a grave (falling line) above a letter.
 *  Alt ~ produces a tilde above a letter.
 *  Alt ^ produces a circumflex above a letter.
 *  Alt " produces a diaeresis (double dots) above a letter.
 *  Alt . produces a dot above a letter.
 *  Alt - produces a macron (line) above a letter, or a stroke through it.
 *  Alt _ produces a horizontal line joined to the bottom of a letter.
 *  Alt | produces a vertical line joined to the left of a letter.
 *  Alt / produces an angled stroke through a letter.
 *  Alt , produces a cedilla for consonants or an ogonek for vowels.
 *  Alt u produces a breve (u shape) above a letter.
 *  Alt n produces an inverted breve above a letter.
 *  Alt v produces a caron (v shape) above a letter.
 *  Alt o produces a ring above a letter.
 *  Alt O produces a ring around a letter, e.g. copyright or registered symbol.
 *
 *  Ligatures are produced by typing the two letters, so
 *  Alt o e produces lowercase oe ligature (not e with a ring above!)
 *  Alt D z produces Dz (not a ligature, nevertheless a single symbol).
 *
 *  Alt 1 2 produces the fraction one half. Some other fractions work the same.
 * 
 *  If the second letter typed is a space, it can modify the first.
 *  Alt ! (space) produces upside-down !
 *  Alt ? (space) produces upside-down ?
 *  Alt x (space) produces a multiplication symbol.
 *  Alt ' (space) produces an acute accent (similar for other accents listed).
 *
 *  Greek letters are produced phonetically by preceding with Alt \
 *  Alt \ a produces lowercase alpha
 *  Alt \ p produces lowercase pi
 *  Alt \ f produces lowercase phi
 *  Alt \ u produces lowercase upsilon
 *  Alt \ U produces uppercase upsilon
 *  In some cases there is no obvious equivalent in English:
 *  Alt \ Y produces uppercase psi
 *  Alt \ H produces uppercase eta
 *  Alt \ C produces uppercase xi
 *  Alt \ X produces uppercase chi
 *  Alt \ v produces lowercase non-final sigma
 *
 *  Cyrillic letters are produced phonetically by preceding with Alt ]
 *  Some letters require Alt [ as the prefix instead.
 *  Alt ] A produces uppercase Cyrillic A.
 *  Alt ] T produces uppercase Cyrillic TE.
 *  Alt [ T produces uppercase Cyrillic TSE.
 */

#include <stdio.h>
#include <stdlib.h>

typedef struct Composition {
	short  accent;
	short  letter;
	long   unicode;
} Composition;

/*
 *  The table only supports a linear search, but this is
 *  fast enough, since the user won't be able to type many
 *  of these combinations quickly anyway.
 */

static Composition compositions [] = {
	/* ISO Latin 1 0A0-0FF */

	{ ' ',  ' ',  0x00A0 },	/* no-break space */
	{ '!',  ' ',  0x00A1 },	/* inverted exclamation mark */
	{ '/',  'c',  0x00A2 },	/* cent sign */
	{ '_',  'f',  0x00A3 },	/* pound sign */
	{ '-',  'L',  0x00A3 },	/* pound sign */
	{ 'o',  'X',  0x00A4 },	/* currency sign */
	{ '=',  'Y',  0x00A5 },	/* yen sign */
	{ '|',  ' ',  0x00A6 },	/* broken vertical bar */
	{ 'S',  'S',  0x00A7 },	/* section sign */
	{ '\"', ' ',  0x00A8 },	/* diaeresis = German umlaut */
	{ 'O',  'C',  0x00A9 },	/* copyright sign */
	{ 'O',  'c',  0x00A9 },	/* copyright sign */
	{ 'a',  '-',  0x00AA },	/* feminine ordinal indicator = a underbar */
	{ '<',  '<',  0x00AB },	/* left pointing guillemet */
	{ '-',  '|',  0x00AC },	/* not sign */
	{ '-',  '-',  0x00AD },	/* soft hyphen */
	{ 'O',  'R',  0x00AE },	/* registered trade mark sign */
	{ '_',  '_',  0x00AF },	/* macron = overline */
	{ 'o',  ' ',  0x00B0 },	/* degree sign */
	{ '+',  '-',  0x00B1 },	/* plus-minus sign */
	{ '2',  ' ',  0x00B2 },	/* superscript two */
	{ '3',  ' ',  0x00B3 },	/* superscript three */
	{ '\'', ' ',  0x00B4 },	/* acute accent */
	{ '|',  'u',  0x00B5 },	/* micro sign */
	{ 'P',  'P',  0x00B6 },	/* pilcrow sign = paragraph sign */
	{ '.',  '.',  0x00B7 },	/* middle dot */
	{ ',',  ' ',  0x00B8 },	/* cedilla */
	{ '1',  ' ',  0x00B9 },	/* superscript one */
	{ 'o',  '-',  0x00BA },	/* masculine ordinal indicator = o underbar */
	{ '>',  '>',  0x00BB },	/* right pointing guillemet */
	{ '1',  '4',  0x00BC },	/* one quarter */
	{ '1',  '2',  0x00BD },	/* one half */
	{ '3',  '4',  0x00BE },	/* three quarters */
	{ '?',  ' ',  0x00BF },	/* inverted question mark */
	{ '`',  'A',  0x00C0 },	/* capital A with grave */
	{ '\'', 'A',  0x00C1 },	/* capital A with acute */
	{ '^',  'A',  0x00C2 },	/* capital A with circumflex */
	{ '~',  'A',  0x00C3 },	/* capital A with tilde */
	{ '\"', 'A',  0x00C4 },	/* capital A with diaeresis */
	{ 'o',  'A',  0x00C5 },	/* capital A with ring above */
	{ 'A',  'E',  0x00C6 },	/* AE (ash) */
	{ ',',  'C',  0x00C7 },	/* capital C with cedilla */
	{ '`',  'E',  0x00C8 },	/* capital E with grave */
	{ '\'', 'E',  0x00C9 },	/* capital E with acute */
	{ '^',  'E',  0x00CA },	/* capital E with circumflex */
	{ '\"', 'E',  0x00CB },	/* capital E with diaeresis */
	{ '`',  'I',  0x00CC },	/* capital I with grave */
	{ '\'', 'I',  0x00CD },	/* capital I with acute */
	{ '^',  'I',  0x00CE },	/* capital I with circumflex */
	{ '\"', 'I',  0x00CF },	/* capital I with diaeresis */
	{ '-',  'D',  0x00D0 },	/* capital ETH */
	{ '~',  'N',  0x00D1 },	/* capital N with tilde */
	{ '`',  'O',  0x00D2 },	/* capital O with grave */
	{ '\'', 'O',  0x00D3 },	/* capital O with acute */
	{ '^',  'O',  0x00D4 },	/* capital O with circumflex */
	{ '~',  'O',  0x00D5 },	/* capital O with tilde */
	{ '\"', 'O',  0x00D6 },	/* capital O with diaeresis */
	{ 'x',  ' ',  0x00D7 },	/* multiplication sign */
	{ '/',  'O',  0x00D8 },	/* capital O slash */
	{ '`',  'U',  0x00D9 },	/* capital U with grave */
	{ '\'', 'U',  0x00DA },	/* capital U with acute */
	{ '^',  'U',  0x00DB },	/* capital U with circumflex */
	{ '\"', 'U',  0x00DC },	/* capital U with diaeresis */
	{ '\'', 'Y',  0x00DD },	/* capital Y with acute */
	{ '|',  'P',  0x00DE },	/* capital THORN */
	{ 'T',  'H',  0x00DE },	/* capital THORN */
	{ 's',  's',  0x00DF },	/* small sharp S (German) */
	{ '`',  'a',  0x00E0 },	/* small A with grave */
	{ '\'', 'a',  0x00E1 },	/* small A with acute */
	{ '^',  'a',  0x00E2 },	/* small A with circumflex */
	{ '~',  'a',  0x00E3 },	/* small A with tilde */
	{ '\"', 'a',  0x00E4 },	/* small A with diaeresis */
	{ 'o',  'a',  0x00E5 },	/* small A with ring above */
	{ 'a',  'e',  0x00E6 },	/* small AE (ash) */
	{ ',',  'c',  0x00E7 },	/* small C with cedilla */
	{ '`',  'e',  0x00E8 },	/* small E with grave */
	{ '\'', 'e',  0x00E9 },	/* small E with acute */
	{ '^',  'e',  0x00EA },	/* small E with circumflex */
	{ '\"', 'e',  0x00EB },	/* small E with diaeresis */
	{ '`',  'i',  0x00EC },	/* small I with grave */
	{ '\'', 'i',  0x00ED },	/* small I with acute */
	{ '^',  'i',  0x00EE },	/* small I with circumflex */
	{ '\"', 'i',  0x00EF },	/* small I with diaeresis */
	{ '/',  'd',  0x00F0 },	/* small ETH */
	{ '~',  'n',  0x00F1 },	/* small N with tilde */
	{ '`',  'o',  0x00F2 },	/* small O with grave */
	{ '\'', 'o',  0x00F3 },	/* small O with acute */
	{ '^',  'o',  0x00F4 },	/* small O with circumflex */
	{ '~',  'o',  0x00F5 },	/* small O with tilde */
	{ '\"', 'o',  0x00F6 },	/* small O with diaeresis */
	{ '-',  ':',  0x00F7 },	/* division sign */
	{ '/',  'o',  0x00F8 },	/* small O slash */
	{ '`',  'u',  0x00F9 },	/* small U with grave */
	{ '\'', 'u',  0x00FA },	/* small U with acute */
	{ '^',  'u',  0x00FB },	/* small U with circumflex */
	{ '\"', 'u',  0x00FC },	/* small U with diaeresis */
	{ '\'', 'y',  0x00FD },	/* small Y with acute */
	{ '|',  'p',  0x00FE },	/* small THORN */
	{ 't',  'h',  0x00FE },	/* small THORN */
	{ '\"', 'y',  0x00FF },	/* small Y with diaeresis */

	/* Accented chars 0100-01FF */

	{ '-',  'A',  0x0100 },	/* capital A with macron */
	{ '-',  'a',  0x0101 },	/* small A with macron */
	{ 'u',  'A',  0x0102 },	/* capital A with breve */
	{ 'u',  'a',  0x0103 },	/* small A with breve */
	{ ',',  'A',  0x0104 },	/* capital A with ogonek */
	{ ',',  'a',  0x0105 },	/* small A with ogonek */
	{ '\'', 'C',  0x0106 },	/* capital C with acute */
	{ '\'', 'c',  0x0107 },	/* small C with acute */
	{ '^',  'C',  0x0108 },	/* capital C with circumflex */
	{ '^',  'c',  0x0109 },	/* small C with circumflex */
	{ '.',  'C',  0x010A },	/* capital C with dot above */
	{ '.',  'c',  0x010B },	/* small C with dot above */
	{ 'v',  'C',  0x010C },	/* capital C with caron */
	{ 'v',  'c',  0x010D },	/* small C with caron */
	{ 'v',  'D',  0x010E },	/* capital D with caron */
	{ 'v',  'd',  0x010F },	/* small D with caron */
	{ '-',  'D',  0x0110 },	/* capital D with stroke (conflict with ETH) */
	{ '-',  'd',  0x0111 },	/* small D with stroke */
	{ '-',  'E',  0x0112 },	/* capital E with macron */
	{ '-',  'e',  0x0113 },	/* small E with macron */
	{ 'u',  'E',  0x0114 },	/* capital E with breve */
	{ 'u',  'e',  0x0115 },	/* small E with breve */
	{ '.',  'E',  0x0116 },	/* capital E with dot above */
	{ '.',  'e',  0x0117 },	/* small E with dot above */
	{ ',',  'E',  0x0118 },	/* capital E with ogonek */
	{ ',',  'e',  0x0119 },	/* small E with ogonek */
	{ 'v',  'E',  0x011A },	/* capital E with caron */
	{ 'v',  'e',  0x011B },	/* small E with caron */
	{ '^',  'G',  0x011C },	/* capital G with circumflex */
	{ '^',  'g',  0x011D },	/* small G with circumflex */
	{ 'u',  'G',  0x011E },	/* capital G with breve */
	{ 'u',  'g',  0x011F },	/* small G with breve */
	{ '.',  'G',  0x0120 },	/* capital G with dot above */
	{ '.',  'g',  0x0121 },	/* small G with dot above */
	{ ',',  'G',  0x0122 },	/* capital G with cedilla */
	{ ',',  'g',  0x0123 },	/* small G with cedilla */
	{ '^',  'H',  0x0124 },	/* capital H with circumflex */
	{ '^',  'h',  0x0125 },	/* small H with circumflex */
	{ '-',  'H',  0x0126 },	/* capital H with stroke */
	{ '-',  'h',  0x0127 },	/* small H with stroke */
	{ '~',  'I',  0x0128 },	/* capital I with tilde */
	{ '~',  'i',  0x0129 },	/* small I with tilde */
	{ '-',  'I',  0x012A },	/* capital I with macron */
	{ '-',  'i',  0x012B },	/* small I with macron */
	{ 'u',  'I',  0x012C },	/* capital I with breve */
	{ 'u',  'i',  0x012D },	/* small I with breve */
	{ ',',  'I',  0x012E },	/* capital I with ogonek */
	{ ',',  'i',  0x012F },	/* small I with ogonek */
	{ '.',  'I',  0x0130 },	/* capital I with dot above */
	{ '.',  'i',  0x0131 },	/* small dotless I */
	{ 'I',  'J',  0x0132 },	/* capital ligature IJ */
	{ 'i',  'j',  0x0133 },	/* small ligature IJ */
	{ '^',  'J',  0x0134 },	/* capital J with circumflex */
	{ '^',  'j',  0x0135 },	/* small J with circumflex */
	{ ',',  'K',  0x0136 },	/* capital K with cedilla */
	{ ',',  'k',  0x0137 },	/* small K with cedilla */
	{ 'K',  'K',  0x0138 },	/* small KRA (greenlandic) */
	{ '\'', 'L',  0x0139 },	/* capital L with acute */
	{ '\'', 'l',  0x013A },	/* small L with acute */
	{ ',',  'L',  0x013B },	/* capital L with cedilla */
	{ ',',  'l',  0x013C },	/* small L with cedilla */
	{ 'v',  'L',  0x013D },	/* capital L with caron */
	{ 'v',  'l',  0x013E },	/* small L with caron */
	{ '.',  'L',  0x013F },	/* capital L with middle dot */
	{ '.',  'l',  0x0140 },	/* small L with middle dot */
	{ '/',  'L',  0x0141 },	/* capital L with stroke */
	{ '/',  'l',  0x0142 },	/* small L with stroke */
	{ '\'', 'N',  0x0143 },	/* capital N with acute */
	{ '\'', 'n',  0x0144 },	/* small N with acute */
	{ ',',  'N',  0x0145 },	/* capital N with cedilla */
	{ ',',  'n',  0x0146 },	/* small N with cedilla */
	{ 'v',  'N',  0x0147 },	/* capital N with caron */
	{ 'v',  'n',  0x0148 },	/* small N with caron */

	{ '-',  'O',  0x014C },	/* capital O with macron */
	{ '-',  'o',  0x014D },	/* small O with macron */
	{ 'u',  'O',  0x014E },	/* capital O with breve */
	{ 'u',  'o',  0x014F },	/* small O with breve */

	{ 'O',  'E',  0x0152 },	/* capital ligature OE */
	{ 'o',  'e',  0x0153 },	/* small ligature OE */
	{ '\'', 'R',  0x0154 },	/* capital R with acute */
	{ '\'', 'r',  0x0155 },	/* small R with acute */
	{ ',',  'R',  0x0156 },	/* capital R with cedilla */
	{ ',',  'r',  0x0157 },	/* small R with cedilla */
	{ 'v',  'R',  0x0158 },	/* capital R with caron */
	{ 'v',  'r',  0x0159 },	/* small R with caron */
	{ '\'', 'S',  0x015A },	/* capital S with acute */
	{ '\'', 's',  0x015B },	/* small S with acute */
	{ '^',  'S',  0x015C },	/* capital S with circumflex */
	{ '^',  's',  0x015D },	/* small S with circumflex */
	{ ',',  'S',  0x015E },	/* capital S with cedilla */
	{ ',',  's',  0x015F },	/* small S with cedilla */
	{ 'v',  'S',  0x0160 },	/* capital S with caron */
	{ 'v',  's',  0x0161 },	/* small S with caron */
	{ ',',  'T',  0x0162 },	/* capital T with cedilla */
	{ ',',  't',  0x0163 },	/* small T with cedilla */
	{ 'v',  'T',  0x0164 },	/* capital T with caron */
	{ 'v',  't',  0x0165 },	/* small T with caron */
	{ '/',  'T',  0x0166 },	/* capital T with stroke */
	{ '/',  't',  0x0167 },	/* small T with stroke */
	{ '~',  'U',  0x0168 },	/* capital U with tilde */
	{ '~',  'u',  0x0169 },	/* small U with tilde */
	{ '-',  'U',  0x016A },	/* capital U with macron */
	{ '-',  'u',  0x016B },	/* small U with macron */
	{ 'u',  'U',  0x016C },	/* capital U with breve */
	{ 'u',  'u',  0x016D },	/* small U with breve */
	{ 'o',  'U',  0x016E },	/* capital U with ring above */
	{ 'o',  'u',  0x016F },	/* small U with ring above */

	{ ',',  'U',  0x0172 },	/* capital U with ogonek */
	{ ',',  'u',  0x0173 },	/* small U with ogonek */
	{ '^',  'W',  0x0174 },	/* capital W with circumflex */
	{ '^',  'w',  0x0175 },	/* small W with circumflex */
	{ '^',  'Y',  0x0176 },	/* capital Y with circumflex */
	{ '^',  'y',  0x0177 },	/* small Y with circumflex */
	{ '\"', 'Y',  0x0178 },	/* capital Y with diaeresis */
	{ '\'', 'Z',  0x0179 },	/* capital Z with acute */
	{ '\'', 'z',  0x017A },	/* small Z with acute */
	{ '.',  'Z',  0x017B },	/* capital Z with dot above */
	{ '.',  'z',  0x017C },	/* small Z with dot above */
	{ 'v',  'Z',  0x017D },	/* capital Z with caron */
	{ 'v',  'z',  0x017E },	/* small Z with caron */

	{ '-',  'b',  0x0180 },	/* small B with stroke */

	{ 'h',  'v',  0x0195 },	/* small HV (HWAIR) */

	{ '-',  'I',  0x0197 },	/* capital I with stroke (conflict I macron) */

	{ 'O',  'I',  0x01A2 },	/* capital OI (GHA) */
	{ 'o',  'i',  0x01A3 },	/* small OI (GHA) */

	{ '-',  'Z',  0x01B5 },	/* capital Z with stroke */
	{ '-',  'z',  0x01B6 },	/* small Z with stroke */

	{ '-',  '2',  0x01BB },	/* two with stroke */

	{ 'L',  'J',  0x01C7 },	/* capital LJ */
	{ 'L',  'j',  0x01C8 },	/* capital L with small letter J */
	{ 'l',  'j',  0x01C9 },	/* small LJ */
	{ 'N',  'J',  0x01CA },	/* capital NJ */
	{ 'N',  'j',  0x01CB },	/* capital N with small J */
	{ 'n',  'j',  0x01CC },	/* small NJ */
	{ 'v',  'A',  0x01CD },	/* capital A with caron */
	{ 'v',  'a',  0x01CE },	/* small A with caron */
	{ 'v',  'I',  0x01CF },	/* capital I with caron */
	{ 'v',  'i',  0x01D0 },	/* small I with caron */
	{ 'v',  'O',  0x01D1 },	/* capital O with caron */
	{ 'v',  'o',  0x01D2 },	/* small O with caron */
	{ 'v',  'U',  0x01D3 },	/* capital U with caron */
	{ 'v',  'u',  0x01D4 },	/* small U with caron */
	{ 'e',  'e',  0x01DD },	/* small turned E */

	{ '-',  'G',  0x01E4 },	/* capital G with stroke */
	{ '-',  'g',  0x01E5 },	/* small G with stroke */
	{ 'v',  'G',  0x01E6 },	/* capital G with caron */
	{ 'v',  'g',  0x01E7 },	/* small G with caron */
	{ 'v',  'K',  0x01E8 },	/* capital K with caron */
	{ 'v',  'k',  0x01E9 },	/* small K with caron */
	{ ',',  'O',  0x01EA },	/* capital O with ogonek */
	{ ',',  'o',  0x01EB },	/* small O with ogonek */

	{ 'v',  'j',  0x01F0 },	/* small J with caron */
	{ 'D',  'Z',  0x01F1 },	/* capital DZ */
	{ 'D',  'z',  0x01F2 },	/* capital D with small Z */
	{ 'd',  'z',  0x01F3 },	/* small DZ */
	{ '\'', 'G',  0x01F4 },	/* capital G with acute */
	{ '\'', 'g',  0x01F5 },	/* small G with acute */

	{ '`',  'N',  0x01F8 },	/* capital N with grave */
	{ '`',  'n',  0x01F9 },	/* small N with grave */

	/* Accented chars 0200-02FF */

	{ 'n',  'A',  0x0202 },	/* capital A with inverted breve */
	{ 'n',  'a',  0x0203 },	/* small A with inverted breve */

	{ 'n',  'E',  0x0206 },	/* capital E with inverted breve */
	{ 'n',  'e',  0x0207 },	/* small E with inverted breve */

	{ 'n',  'I',  0x020A },	/* capital I with inverted breve */
	{ 'n',  'i',  0x020B },	/* small I with inverted breve */

	{ 'n',  'O',  0x020E },	/* capital O with inverted breve */
	{ 'n',  'o',  0x020F },	/* small O with inverted breve */

	{ 'n',  'R',  0x0212 },	/* capital R with inverted breve */
	{ 'n',  'r',  0x0213 },	/* small R with inverted breve */

	{ 'n',  'U',  0x0216 },	/* capital U with inverted breve */
	{ 'n',  'u',  0x0217 },	/* small U with inverted breve */

	{ 'v',  'H',  0x021E },	/* capital H with caron */
	{ 'v',  'h',  0x021F },	/* small H with caron */

	{ '.',  'A',  0x0226 },	/* capital A with dot above */
	{ '.',  'a',  0x0227 },	/* small A with dot above */

	{ '.',  'O',  0x022E },	/* capital O with dot above */
	{ '.',  'o',  0x022F },	/* small O with dot above */

	{ '-',  'Y',  0x0232 },	/* capital Y with macron */
	{ '-',  'y',  0x0233 },	/* small Y with macron */

	{ 'v',  ' ',  0x02C7 },	/* caron (mandarin chinese third tone) */

	{ 'u',  ' ',  0x02D8 },	/* breve */
	{ '.',  ' ',  0x02D9 },	/* dot above (mandarin chinese light tone) */
	{ 'o',  ' ',  0x02DA },	/* ring above (conflict with degree sign) */

	/* Greek letters */

	{ '\\', 'A',  0x0391 },	/* Greek capital ALPHA */
	{ '\\', 'B',  0x0392 },	/* Greek capital BETA */
	{ '\\', 'G',  0x0393 },	/* Greek capital GAMMA */
	{ '\\', 'D',  0x0394 },	/* Greek capital DELTA */
	{ '\\', 'E',  0x0395 },	/* Greek capital EPSILON */
	{ '\\', 'Z',  0x0396 },	/* Greek capital ZETA */
	{ '\\', 'H',  0x0397 },	/* Greek capital ETA */
	{ '\\', 'Q',  0x0398 },	/* Greek capital THETA */
	{ '\\', 'I',  0x0399 },	/* Greek capital IOTA */
	{ '\\', 'K',  0x039A },	/* Greek capital KAPPA */
	{ '\\', 'L',  0x039B },	/* Greek capital LAMDA */
	{ '\\', 'M',  0x039C },	/* Greek capital MU */
	{ '\\', 'N',  0x039D },	/* Greek capital NU */
	{ '\\', 'C',  0x039E },	/* Greek capital XI */
	{ '\\', 'O',  0x039F },	/* Greek capital OMICRON */
	{ '\\', 'P',  0x03A0 },	/* Greek capital PI */
	{ '\\', 'R',  0x03A1 },	/* Greek capital RHO */
	{ '\\', 'S',  0x03A3 },	/* Greek capital SIGMA */
	{ '\\', 'T',  0x03A4 },	/* Greek capital TAU */
	{ '\\', 'U',  0x03A5 },	/* Greek capital UPSILON */
	{ '\\', 'F',  0x03A6 },	/* Greek capital PHI */
	{ '\\', 'X',  0x03A7 },	/* Greek capital CHI */
	{ '\\', 'Y',  0x03A8 },	/* Greek capital PSI */
	{ '\\', 'W',  0x03A9 },	/* Greek capital OMEGA */

	{ '\\', 'a',  0x03B1 },	/* Greek small ALPHA */
	{ '\\', 'b',  0x03B2 },	/* Greek small BETA */
	{ '\\', 'g',  0x03B3 },	/* Greek small GAMMA */
	{ '\\', 'd',  0x03B4 },	/* Greek small DELTA */
	{ '\\', 'e',  0x03B5 },	/* Greek small EPSILON */
	{ '\\', 'z',  0x03B6 },	/* Greek small ZETA */
	{ '\\', 'h',  0x03B7 },	/* Greek small ETA */
	{ '\\', 'q',  0x03B8 },	/* Greek small THETA */
	{ '\\', 'i',  0x03B9 },	/* Greek small IOTA */
	{ '\\', 'k',  0x03BA },	/* Greek small KAPPA */
	{ '\\', 'l',  0x03BB },	/* Greek small LAMDA */
	{ '\\', 'm',  0x03BC },	/* Greek small MU */
	{ '\\', 'n',  0x03BD },	/* Greek small NU */
	{ '\\', 'c',  0x03BE },	/* Greek small XI */
	{ '\\', 'o',  0x03BF },	/* Greek small OMICRON */
	{ '\\', 'p',  0x03C0 },	/* Greek small PI */
	{ '\\', 'r',  0x03C1 },	/* Greek small RHO */
	{ '\\', 's',  0x03C2 },	/* Greek small final SIGMA */
	{ '\\', 'v',  0x03C3 },	/* Greek small SIGMA */
	{ '\\', 't',  0x03C4 },	/* Greek small TAU */
	{ '\\', 'u',  0x03C5 },	/* Greek small UPSILON */
	{ '\\', 'f',  0x03C6 },	/* Greek small PHI */
	{ '\\', 'x',  0x03C7 },	/* Greek small CHI */
	{ '\\', 'y',  0x03C8 },	/* Greek small PSI */
	{ '\\', 'w',  0x03C9 },	/* Greek small OMEGA */

 	/* Cyrillic letters */
 
 	{ ']',  'A',  0x0410 },	/* Cyrillic capital A */
 	{ ']',  'a',  0x0430 },	/* Cyrillic small A */
 
 	{ ']',  'B',  0x0411 },	/* Cyrillic capital BE */
 	{ ']',  'b',  0x0431 },	/* Cyrillic small BE */
 
 	{ ']',  'V',  0x0412 },	/* Cyrillic capital VE */
 	{ ']',  'v',  0x0432 },	/* Cyrillic small VE */
 
 	{ ']',  'G',  0x0413 },	/* Cyrillic capital GHE */
 	{ ']',  'g',  0x0433 },	/* Cyrillic small GHE */
 
 	{ ']',  'D',  0x0414 },	/* Cyrillic capital DE */
 	{ ']',  'd',  0x0434 },	/* Cyrillic small DE */
 
 	{ ']',  'E',  0x0415 },	/* Cyrillic capital IE */
 	{ ']',  'e',  0x0435 },	/* Cyrillic small IE */
 
 	{ '[',  'E',  0x0401 },	/* Cyrillic capital IO */
 	{ '[',  'e',  0x0451 },	/* Cyrillic small IO */
 
 	{ '[',  'Z',  0x0416 },	/* Cyrillic capital ZHE */
 	{ '[',  'z',  0x0436 },	/* Cyrillic small ZHE */
 
 	{ ']',  'Z',  0x0417 },	/* Cyrillic capital ZE */
 	{ ']',  'z',  0x0437 },	/* Cyrillic small ZE */
 
 	{ ']',  'I',  0x0418 },	/* Cyrillic capital I */
 	{ ']',  'i',  0x0438 },	/* Cyrillic small I */
 
 	{ ']',  'Y',  0x0419 }, /* Cyrillic capital short I (I with breve) */
 	{ ']',  'y',  0x0439 }, /* Cyrillic small short I (I with breve) */
 
 	{ ']',  'K',  0x041a },	/* Cyrillic capital KA */
 	{ ']',  'k',  0x043a },	/* Cyrillic small KA */
 
 	{ ']',  'L',  0x041b },	/* Cyrillic capital EL */
 	{ ']',  'l',  0x043b },	/* Cyrillic small EL */
 
 	{ ']',  'M',  0x041c },	/* Cyrillic capital EM */
 	{ ']',  'm',  0x043c },	/* Cyrillic small EM */
 
 	{ ']',  'N',  0x041d },	/* Cyrillic capital EN */
 	{ ']',  'n',  0x043d },	/* Cyrillic small EN */
 
 	{ ']',  'O',  0x041e },	/* Cyrillic capital O */
 	{ ']',  'o',  0x043e },	/* Cyrillic small O */
 
 	{ ']',  'P',  0x041f },	/* Cyrillic capital PE */
 	{ ']',  'p',  0x043f },	/* Cyrillic small PE */
 
 	{ ']',  'R',  0x0420 },	/* Cyrillic capital ER */
 	{ ']',  'r',  0x0440 },	/* Cyrillic small ER */
 
 	{ ']',  'S',  0x0421 },	/* Cyrillic capital ES */
 	{ ']',  's',  0x0441 },	/* Cyrillic small ES */
 
 	{ ']',  'T',  0x0422 },	/* Cyrillic capital TE */
 	{ ']',  't',  0x0442 },	/* Cyrillic small TE */
 
 	{ ']',  'U',  0x0423 },	/* Cyrillic capital U */
 	{ ']',  'u',  0x0443 },	/* Cyrillic small U */
 
 	{ ']',  'F',  0x0424 },	/* Cyrillic capital F */
 	{ ']',  'f',  0x0444 },	/* Cyrillic small F */
 
 	{ '[',  'K',  0x0425 },	/* Cyrillic capital HA (soft KH) */
 	{ '[',  'k',  0x0445 },	/* Cyrillic small HA */
 
 	{ '[',  'T',  0x0426 },	/* Cyrillic capital TSE */
 	{ '[',  't',  0x0446 },	/* Cyrillic small TSE */
 
 	{ ']',  'C',  0x0427 },	/* Cyrillic capital CHE */
 	{ ']',  'c',  0x0447 },	/* Cyrillic small CHE */
 
 	{ '[',  'S',  0x0428 },	/* Cyrillic capital SHA */
 	{ '[',  's',  0x0448 },	/* Cyrillic small SHA */
 
 	{ '[',  'C',  0x0429 },	/* Cyrillic capital SHCHA (softer SH) */
 	{ '[',  'c',  0x0449 },	/* Cyrillic small SHCHA */
 
 	{ '[',  '\"', 0x042a },	/* Cyrillic capital hard sign */
 	{ '[',  '\'', 0x044a },	/* Cyrillic small hard sign */
 
 	{ '[',  'I',  0x042b },	/* Cyrillic capital YERU (short I) */
 	{ '[',  'i',  0x044b },	/* Cyrillic small YERU */
 
 	{ ']',  '\"', 0x042c },	/* Cyrillic capital soft sign */
 	{ ']',  '\'', 0x044c },	/* Cyrillic small soft sign */
 
 	{ '[',  'Y',  0x042d },	/* Cyrillic capital E */
 	{ '[',  'y',  0x044d },	/* Cyrillic small E */
 
 	{ '[',  'U',  0x042e },	/* Cyrillic capital YU */
 	{ '[',  'u',  0x044e },	/* Cyrillic small YU */
 
 	{ '[',  'A',  0x042f },	/* Cyrillic capital YA */
 	{ '[',  'a',  0x044f },	/* Cyrillic small YA */
 
	/* accented characters 1E00-1EFF */

	{ '.',  'B',  0x1E02 },	/* capital B with dot above */
	{ '.',  'b',  0x1E03 },	/* small B with dot above */

	{ '_',  'B',  0x1E06 },	/* capital B with line below */
	{ '_',  'b',  0x1E07 },	/* small B with line below */

	{ '.',  'D',  0x1E0A },	/* capital D with dot above */
	{ '.',  'd',  0x1E0B },	/* small D with dot above */

	{ '_',  'D',  0x1E0E },	/* capital D with line below */
	{ '_',  'd',  0x1E0F },	/* small D with line below */

	{ ',',  'D',  0x1E10 },	/* capital D with cedilla */
	{ ',',  'd',  0x1E11 },	/* small D with cedilla */

	{ '.',  'F',  0x1E1E },	/* capital F with dot above */
	{ '.',  'f',  0x1E1F },	/* small F with dot above */

	{ '-',  'G',  0x1E20 },	/* capital G with macron */
	{ '-',  'g',  0x1E21 },	/* small G with macron */
	{ '.',  'H',  0x1E22 },	/* capital H with dot above */
	{ '.',  'h',  0x1E23 },	/* small H with dot above */

	{ '\"', 'H',  0x1E26 },	/* capital H with diaeresis */
	{ '\"', 'h',  0x1E27 },	/* small H with diaeresis */
	{ ',',  'H',  0x1E28 },	/* capital H with cedilla */
	{ ',',  'h',  0x1E29 },	/* small H with cedilla */

	{ '\'', 'K',  0x1E30 },	/* capital K with acute */
	{ '\'', 'k',  0x1E31 },	/* small K with acute */

	{ '_',  'K',  0x1E34 },	/* capital K with line below */
	{ '_',  'k',  0x1E35 },	/* small K with line below */

	{ '_',  'L',  0x1E3A },	/* capital L with line below */
	{ '_',  'l',  0x1E3B },	/* small L with line below */

	{ '\'', 'M',  0x1E3E },	/* capital M with acute */
	{ '\'', 'm',  0x1E3F },	/* small M with acute */
	{ '.',  'M',  0x1E40 },	/* capital M with dot above */
	{ '.',  'm',  0x1E41 },	/* small M with dot above */

	{ '.',  'N',  0x1E44 },	/* capital N with dot above */
	{ '.',  'n',  0x1E45 },	/* small N with dot above */

	{ '_',  'N',  0x1E48 },	/* capital N with line below */
	{ '_',  'n',  0x1E49 },	/* small N with line below */

	{ '\'', 'P',  0x1E54 },	/* capital P with acute */
	{ '\'', 'p',  0x1E55 },	/* small P with acute */
	{ '.',  'P',  0x1E56 },	/* capital P with dot above */
	{ '.',  'p',  0x1E57 },	/* small P with dot above */
	{ '.',  'R',  0x1E58 },	/* capital R with dot above */
	{ '.',  'r',  0x1E59 },	/* small R with dot above */

	{ '_',  'R',  0x1E5E },	/* capital R with line below */
	{ '_',  'r',  0x1E5F },	/* small R with line below */
	{ '.',  'S',  0x1E60 },	/* capital S with dot above */
	{ '.',  's',  0x1E61 },	/* small S with dot above */

	{ '.',  'T',  0x1E6A },	/* capital T with dot above */
	{ '.',  't',  0x1E6B },	/* small T with dot above */

	{ '_',  'T',  0x1E6E },	/* capital T with line below */
	{ '_',  't',  0x1E6F },	/* small T with line below */

	{ '~',  'V',  0x1E7C },	/* capital V with tilde */
	{ '~',  'v',  0x1E7D },	/* small V with tilde */

	{ '`',  'W',  0x1E80 },	/* capital W with grave */
	{ '`',  'w',  0x1E81 },	/* small W with grave */
	{ '\'', 'W',  0x1E82 },	/* capital W with acute */
	{ '\'', 'w',  0x1E83 },	/* small W with acute */
	{ '\"', 'W',  0x1E84 },	/* capital W with diaeresis */
	{ '\"', 'w',  0x1E85 },	/* small W with diaeresis */
	{ '.',  'W',  0x1E86 },	/* capital W with dot above */
	{ '.',  'w',  0x1E87 },	/* small W with dot above */

	{ '.',  'X',  0x1E8A },	/* capital X with dot above */
	{ '.',  'x',  0x1E8B },	/* small X with dot above */
	{ '\"', 'X',  0x1E8C },	/* capital X with diaeresis */
	{ '\"', 'x',  0x1E8D },	/* small X with diaeresis */
	{ '.',  'Y',  0x1E8E },	/* capital Y with dot above */
	{ '.',  'y',  0x1E8F },	/* small Y with dot above */
	{ '^',  'Z',  0x1E90 },	/* capital Z with circumflex */
	{ '^',  'z',  0x1E91 },	/* small Z with circumflex */

	{ '_',  'Z',  0x1E94 },	/* capital Z with line below */
	{ '_',  'z',  0x1E95 },	/* small Z with line below */
	{ '_',  'h',  0x1E96 },	/* small H with line below */
	{ '\"', 't',  0x1E97 },	/* small T with diaeresis */
	{ 'o',  'w',  0x1E98 },	/* small W with ring above */
	{ 'o',  'y',  0x1E99 },	/* small Y with ring above */

	{ '~',  'E',  0x1EBC },	/* capital E with tilde */
	{ '~',  'e',  0x1EBD },	/* small E with tilde */

	{ '`',  'Y',  0x1EF2 },	/* capital Y with grave */
	{ '`',  'y',  0x1EF3 },	/* small Y with grave */

	{ '~',  'Y',  0x1EF8 },	/* capital Y with tilde */
	{ '~',  'y',  0x1EF9 },	/* small Y with tilde */

	/* proper quotation marks */

	{ '`',  '`',  0x2018 },	/* left single quotation mark */
	{ '\'', '\'', 0x2019 },	/* right single quotation mark */
	{ '`',  '\"', 0x201C },	/* left double quotation mark */
	{ '\'', '\"', 0x201D },	/* right double quotation mark */

	/* miscellany */

	{ '|',  '-',  0x2020 }, /* dagger */
	{ '|',  '=',  0x2021 }, /* double dagger */
	{ 'N',  'o',  0x2116 }, /* numero sign (No) */
	{ 'O',  'P',  0x2117 }, /* sound recording copyright (circled P) */
	{ 'R',  'x',  0x211E }, /* prescription take (Rx) */
	{ 'S',  'M',  0x2120 }, /* service mark sign (superscript SM) */
	{ 'T',  'M',  0x2122 }, /* trade mark sign (superscript TM) */

	/* fractions */

	{ '1',  '3',  0x2153 },	/* fraction one third */
	{ '2',  '3',  0x2154 },	/* fraction two thirds */
	{ '1',  '5',  0x2155 },	/* fraction one fifth */
	{ '2',  '5',  0x2156 },	/* fraction two fifths */
	{ '3',  '5',  0x2157 },	/* fraction three fifths */
	{ '4',  '5',  0x2158 },	/* fraction four fifths */
	{ '1',  '6',  0x2159 },	/* fraction one sixth */
	{ '5',  '6',  0x215A },	/* fraction five sixths */
	{ '1',  '8',  0x215B },	/* fraction one eighth */
	{ '3',  '8',  0x215C },	/* fraction three eighths */
	{ '5',  '8',  0x215D },	/* fraction five eighths */
	{ '7',  '8',  0x215E },	/* fraction seven eighths */

	/* mathematical symbols */

	{ '<',  '-',  0x2190 },	/* leftwards arrow */
	{ '-',  '>',  0x2192 },	/* rightwards arrow */
	{ '=',  '>',  0x21D2 },	/* rightwards double arrow (implies) */

	{ '-',  '+',  0x2213 },	/* minus-or-plus sign */
	{ '.',  '+',  0x2214 },	/* dot plus */
	{ 'o',  'o',  0x221E },	/* infinity */
	{ '.',  '\"', 0x2234 },	/* therefore */
	{ '\"', '.',  0x2235 },	/* because */
	{ '~',  '-',  0x2243 },	/* asymptotically equal to */
	{ '~',  '=',  0x2245 },	/* approximately equal to */
	{ '~',  '~',  0x2248 },	/* almost equal to */
	{ '/',  '=',  0x2260 },	/* not equal to */
	{ '!',  '=',  0x2260 },	/* not equal to */
	{ '<',  '=',  0x2264 },	/* less-than or equal to */
	{ '>',  '=',  0x2265 },	/* greater-than or equal to */
};

long app_compose_unicode(int accent, int letter)
{
	int i;

	for (i=0; i < (sizeof(compositions)/sizeof(Composition)); i++)
	{
		if (compositions[i].accent == accent)
			if (compositions[i].letter == letter)
				return compositions[i].unicode;
	}
	return -1;
}

/*
 *  Here's a function for checking the table of compositions for
 *  conflicting definitions.
 *  The first definition wins if there is a conflict.
 *  There are currently three conflicts.
 */

/*
static void print_conflicts(void)
{
	int i, j;
	char cmd[40];

	for (i=0; i < (sizeof(compositions)/sizeof(Composition)); i++)
	 for (j=0; j < i; j++) {
		if ((compositions[i].accent == compositions[j].accent)
		 && (compositions[i].letter == compositions[j].letter)) {
			printf("Compositions for %04X and %04X conflict\n",
				compositions[j].unicode,
				compositions[i].unicode);
			sprintf(cmd, "grep 0x%04X compose.c",
				compositions[j].unicode);
			system(cmd);
			sprintf(cmd, "grep 0x%04X compose.c",
				compositions[i].unicode);
			system(cmd);
		}
	 }
}
*/

/*
 *  Here's a program for testing compositions using the console.
 *  Uncomment the main function if you want to use it.
 *  Currently it only produces ISO Latin 1 output, not UTF-8, sorry.
 */

/*
int main(int argc, char *argv[])
{
	int ch, accent, letter;
	long unicode;

	printf("Type exactly two characters then hit enter.\n");

	while (! feof(stdin)) {
		accent = getc(stdin);
		letter = getc(stdin);
		unicode = app_compose_unicode(accent, letter);
		if (unicode >= 0)
			printf("%c\n", (int) unicode);
		else
			printf("Not a composition\n");
		ch = getc(stdin);
	}
}
*/

