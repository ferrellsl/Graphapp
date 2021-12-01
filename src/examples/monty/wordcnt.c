/*
 * Monty - a simple project editor.
 *
 * File: wordcnt.c -- a word count utility.
 * Platform: Neutral  Version: 2.00  Date: 1999/12/12
 *
 * Version: 1.00  Changes: Original version by L. Patrick.
 */

/* Copyright (C) 1998 L. Patrick

   This file is part of Monty, a simple project editor.
*/



#define LF 10
#define CR 13

char *title  = "File                 Para.  Words  Chars  Lines  Sent.  Empty\n";
char *title2 = "-------------------  ------ ------ ------ ------ ------ ------\n";

/*
 *  Counts paragraphs, words and characters and display the totals.
 *  File must be opened in binary mode.
 */

#define OUT 0       /* outside a word */
#define IN 1        /* inside a word  */

static char *wordcount(FILE *f, char *name)
{
	long np, nw, nc; /* number of paras, words and chars */
	long nl, ns; /* number of lines and sentences */
	long ne; /* number of empty lines */
	int cl; /* number of chars on the line so far, maximum 80 */
	int c, state, prev = LF; /* initially not a carriage return */
	char *str; /* result string */
	char buf[80];

	state = OUT;
	np = nw = nc = nl = ns = ne = cl = 0;

	while ((c = getc(f)) != EOF) {
		++nc;
		++cl;
		if (c == '.') /* full-stop increments number of sentences */
			++ns;
		if (c == CR) { /* DOS and MAC end-of-line */
			++np;
			if (prev == LF) ++ne;
			state = LF;
		}
		else if ((c == LF) && (prev != CR)) { /* UNIX end-of-line */
			++np;
			if (prev == LF) ++ne;
			state = LF;
		}
		if ((cl >= 80) || (state == LF)) {
			nl++;
			cl = 0;
		}
		if ((c == ' ') || (c == '\t') || (state == LF))
			state = OUT;
		else if (state == OUT) {
			state = IN;
			++nw;
		}
		prev = c;
	}
	sprintf(buf, "%-20.20s ", name);
	str = add_strings(NULL, buf);
	sprintf(buf, "%-6ld %-6ld %-6ld ", np, nw, nc);
	str = add_strings(str, buf);
	sprintf(buf, "%-6ld %-6ld %-6ld\n", nl, ns, ne);
	str = add_strings(str, buf);

	return str;
}

static char * word_count_file(char *name)
{
	FILE *f;
	char *str;

	if ((f = app_open_file(name, "rb")) == NULL) {
		ask_ok_str(stderr, "%s: Could not open source file %s\n", prog, name);
		return;
	}

	str = wordcount(f, name);

	if (app_close_file(f) != 0) {
		ask_ok_str(stderr, "%s: Could not close source file %s\n", prog, name);
		return;
	}

	return str;
}

void char * word_count_dialog(char *name)
{
	/* show a dialog stating the word count for a file */
}
 
