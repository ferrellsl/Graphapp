#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "graphapp.h"
#include "progressbar.h"

long percent;
char percentage_str[3];

Window *w; // Our main window
Window *progress_window; //Our progress window
App *app;  

void about(MenuItem *m)  { printf("About\n"); }
void about_this_program(MenuItem *me)
{
	ask_ok(app, me->text, "Demo by: Steve Ferrell");
}

Control  *order;  // Logic for the button

void compute(void);

void place_order(Control *btn)
{
    compute();
    app_ask_ok(app,"Process Complete ","Press OK to continue.");	
}

int main(int argc, char *argv[])
{
	Rect r;
    MenuBar *mb;
	Menu *m;
	MenuItem *mi;
	app = new_app(argc,argv);  // our application
	//debug_memory(1);
	w = new_window(app, rect(50,30,270,100),"Progress Bar Demo", STANDARD_WINDOW);
	mb = new_menu_bar(w);
	m = new_menu(mb, "About");
	mi = new_menu_item(m, "About",  'A', about_this_program);
    r = rect(10,30,120,30);
	order = new_button(w, r, "Do It!", place_order);    r.x += 130;
	show_window(w);
	
	main_loop(app);
      return;
}
// Function to call and update our progress window 
void compute()
{
	int dummy=0;
	int size = 0;
	percent = 1000000;
	progress_window = new_progress_window(app, "Progress Window", "Progress Meter");
	for (dummy = 1; dummy < 1000000; dummy++){
	sprintf(percentage_str,"%.lf",(float)dummy/(float)percent*99.0);
	size = atoi(percentage_str);
	update_progress_window(progress_window, size);
   }
	hide_window(progress_window);
	return;
}   
