/*
 *  Pizza
 *  -----
 *  This program is a pizza ordering program.
 *
 *  It creates a window, several fields for typing in your name,
 *  and several checkboxes and radio buttons for choosing what
 *  you want on the pizza. Having filled in all the information,
 *  a "place order" button will print out the order and exit the
 *  the program. A "reset form" button empties the fields and
 *  sets the program back to its inital state.
 *
 *  This example adds tab-focus cycling to the UI.
 */

#include <stdio.h>
#include <stdlib.h>
#include <graphapp.h>

Control *name, *phone;
Control *address;
Control *ham, *mushrooms, *olives, *capsicum;
Control *tomato, *barbeque;
Control *order, *reset;

Control **tab_controls[] = {
	&name, &phone, &address,
	&tomato, &barbeque,
	&ham, &mushrooms, &olives, &capsicum,
	&order, &reset,
	NULL
  };

void tab_focus(Window *w, unsigned long key)
{
	int i = 0;

	/* printf( "Tab focus here.\n  key is %d\n", key ); */

	if (key != (unsigned long)'\t')
		return; /* ignore non-tab keys */
				
	while (tab_controls[i] != NULL) {
		if (has_focus(*tab_controls[i]))
			break;
		i++;
	}
	if (tab_controls[i] == NULL) {
		/* odd, none of them has focus */
		set_focus(*tab_controls[0]);  /* pick the first one */
	} else {
		i++;  /* advance to the next one */
		if (tab_controls[i] == NULL) {
			/* past end, back to first */
			i = 0;
		}
		set_focus(*tab_controls[i]);
	}
}

void place_order(Control *btn)
{
	printf("Name = %s\n", get_control_text(name));
	printf("Phone = %s\n", get_control_text(phone));
	printf("Address = %s\n", get_control_text(address));

	printf("Sauce:\n");
	if (is_checked(tomato))     printf("  Tomato\n");
	if (is_checked(barbeque))   printf("  Barbeque\n");

	printf("Toppings:\n");
	if (is_checked(ham))        printf("  Ham\n");
	if (is_checked(mushrooms))  printf("  Mushrooms\n");
	if (is_checked(olives))     printf("  Olives\n");
	if (is_checked(capsicum))   printf("  Capsicum\n");

	exit(0);
}

void reset_form(Control *btn)
{
	set_control_text(name, "");
	set_control_text(phone, "");
	set_control_text(address, "");

	check(tomato);
	uncheck(barbeque);
	uncheck(ham);
	uncheck(mushrooms);
	uncheck(olives);
	uncheck(capsicum);

	set_focus(name);
}

int main(int argc, char *argv[])
{
	App *app;
	Window *w;
	Rect r;

	app = new_app(argc, argv);
	w = new_window(app, rect(0,0,400,450),
		"Order a Pizza", STANDARD_WINDOW);
	on_window_key_down(w, tab_focus);

	r = rect(10,10,120,30);

	new_label(w, r, "Name:", ALIGN_RIGHT);              r.x += 130;
	name = new_field(w, r, "");                         r.y += 35;
	set_field_disallowed_chars(name, "\t");
	r.x = 10;

	new_label(w, r, "Phone:", ALIGN_RIGHT);             r.x += 130;
	phone = new_field(w, r, "");                        r.y += 35;
	set_field_disallowed_chars(phone, "\t");
	r.x = 10;

	new_label(w, r, "Address:", ALIGN_RIGHT);           r.x += 130;
	r.height = 75;
	address = new_text_box(w, r, "");                   r.y += 80;
	set_field_disallowed_chars(address, "\t");
	r.height = 25;
	r.x = 10;

	r.y += 10;
	new_label(w, r, "Sauce:", ALIGN_RIGHT);             r.x += 130;
	tomato   = new_radio_button(w, r, "Tomato", NULL);  r.y += 35;
	barbeque = new_radio_button(w, r, "BBQ", NULL);     r.y += 35;
	check(tomato);
	r.x = 10;

	r.y += 10;
	new_label(w, r, "Toppings:", ALIGN_RIGHT);          r.x += 130;
	ham       = new_check_box(w, r, "Ham", NULL);       r.y += 35;
	mushrooms = new_check_box(w, r, "Mushrooms", NULL); r.y += 35;
	olives    = new_check_box(w, r, "Olives", NULL);    r.y += 35;
	capsicum  = new_check_box(w, r, "Capsicum", NULL);  r.y += 35;
	r.x = 50;

	r.y += 10;
	order = new_button(w, r, "Order Pizza", place_order); r.x += 130;
	reset = new_button(w, r, "Reset Form", reset_form);

	set_focus(name);
	show_window(w);
	main_loop(app);
	return 0;
}
