/*
 *  Calculator
 *  ----------
 *  This calculator is a simple demonstration program,
 *  and is not really finished.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graphapp.h>

enum {
	TYPING,
	DISPLAYED
};

App *app;
Control *t;

int mode = DISPLAYED;
int op = '+';
int current = 0;
int result = 0;

void display(int number)
{
	char buffer[100];
	sprintf(buffer, "%d", number);
	set_control_text(t, buffer);
}

void handle_number(int digit)
{
	if (mode == TYPING) {
		current = atoi(get_control_text(t));
		current *= 10;
		current += digit;
	}
	else if (mode == DISPLAYED) {
		current = digit;
		mode = TYPING;
	}
	display(current);
	if (op == '=') {
		result = 0;
		op = '+';
	}
}

void number_button(Control *b)
{
	handle_number(atoi(get_control_text(b)));
}

void handle_op(char *next_op)
{
	switch (op) {
	case '=':
		break;
	case '+':
		result += current;
		break;
	case '-':
		result -= current;
		break;
	case '*':
		result *= current;
		break;
	case '/':
		if (current != 0)
			result /= current;
		else
			result = 0;
		break;
	}
	current = 0;

	display(result);
	mode = DISPLAYED;

	if (strcmp(next_op, "×") == 0)
		op = '*';
	else if (strcmp(next_op, "÷") == 0)
		op = '/';
	else
		op = next_op[0];
}

void do_op(Control *b)
{
	handle_op(get_control_text(b));
}

void clear_last(Control *b)
{
	current = 0;

	display(result);
	mode = DISPLAYED;
}

void clear_all(Control *b)
{
	result = current = 0;
	op = '+';

	display(result);
	mode = DISPLAYED;
}

void handle_key(Window *w, unsigned long key)
{
	char ch[100];

	unicode_char_to_utf8(key, ch);

	if (strstr("0123456789", ch) != NULL)
		handle_number(atoi(ch));
	else if (strstr("+-*/=×÷", ch) != NULL)
		handle_op(ch);
	else if (strstr("\n\r", ch) != NULL)
		handle_op("=");
}

void handle_key_action(Window *w, unsigned long key)
{
	char buffer[100];

	if (key == (CONTROL+'c')) {
		sprintf(buffer, "%d", result);
		app_set_clipboard_text(app, buffer);
	}
}

int main(int argc, char *argv[])
{
	Rect r;
	Window *w;
	Control *b;

	/* Start the app. */
	app = new_app(argc, argv);

	/* Create a window. */
	r = rect(50,50,215,165);
	w = new_window(app, r, "Calculator", TITLEBAR | CLOSEBOX);
	set_window_background(w, rgb(240,240,255));
	on_window_key_down(w, handle_key);
	on_window_key_action(w, handle_key_action);

	/* Create the interface. */
	r = rect(10,10,r.width-20,25);
	t = new_label(w, r, "0", ALIGN_RIGHT);
	set_control_background(t, WHITE);
	r.y += r.height + 5;
	r.width = 35;

	b = new_button(w, r, "7", number_button);
	r.x += r.width + 5;
	b = new_button(w, r, "8", number_button);
	r.x += r.width + 5;
	b = new_button(w, r, "9", number_button);
	r.x += r.width + 5;
	b = new_button(w, r, "C", clear_last);
	set_control_background(b, ORANGE);
	r.x += r.width + 5;
	b = new_button(w, r, "AC", clear_all);
	set_control_background(b, ORANGE);
	r.x = 10; r.y += r.height + 5;
	b = new_button(w, r, "4", number_button);
	r.x += r.width + 5;
	b = new_button(w, r, "5", number_button);
	r.x += r.width + 5;
	b = new_button(w, r, "6", number_button);
	r.x += r.width + 5;
	b = new_button(w, r, "×", do_op);
	r.x += r.width + 5;
	b = new_button(w, r, "÷", do_op);
	r.x = 10; r.y += r.height + 5;
	b = new_button(w, r, "1", number_button);
	r.x += r.width + 5;
	b = new_button(w, r, "2", number_button);
	r.x += r.width + 5;
	b = new_button(w, r, "3", number_button);
	r.x += r.width + 5;
	b = new_button(w, r, "+", do_op);
	r.x += r.width + 5;
	b = new_button(w, r, "-", do_op);
	r.x = 10; r.y += r.height + 5;
	b = new_button(w, r, "0", number_button);
	r.x += r.width + 5;
	b = new_button(w, r, ".", NULL);
	r.x += r.width + 5;
	b = new_button(w, r, "EXP", NULL);
	r.x += r.width + 5;
	b = new_button(w, r, "=", do_op);
	r.x += r.width + 5;
	b = new_button(w, r, "M+", NULL);

	set_focus(t);

	/* Show the window. */
	show_window(w);

	/* Allow GraphApp to handle all events. */
	main_loop(app);

	/* Tidy up. */
	del_app(app);
	return 0;
}
