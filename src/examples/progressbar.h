//#include <graphapp.h>

Window *new_progress_window(App *app, char *name, char *str);
void update_progress_window(Window *p, int percentage);


typedef struct ProgressData  ProgressData;

struct ProgressData {
	Control *display;
	Control *percent;
	long percentage;
};


/*
 *  A progress indicator window:
 */
static void draw_progress_window(Window *w, Graphics *g)
{
	long value;
	Rect r, r1, r2;
	ProgressData *pd;

	//pd = get_window_data(w);
	pd = (ProgressData *) get_window_data(w); //C++ version of the C code above

	r = get_window_area(w);
	r = rect(r.x+10,r.height-26,r.width-20,16);

	value = pd->percentage;

	r1 = rect(r.x,r.y,r.width*value/100,r.height);
	r2 = rect(r1.x+r1.width,r.y,r.width-r1.width,r.height);

	set_colour(g, BLACK);
	draw_rect(g, inset_rect(r,-1));
	set_colour(g, BLUE);
	fill_rect(g, r1);
	set_colour(g, WHITE);
	fill_rect(g, r2);
	set_colour(g, BLACK);
}

void update_progress_window(Window *w, int percentage)
{
	char pbuffer[20];
	Control *percent;
	ProgressData *pd;

	//pd = get_window_data(w);
	pd = (ProgressData *) get_window_data(w); //C++ version of the C code above
	if (percentage == pd->percentage)
		return; 
	//pd = get_window_data(w);
	pd = (ProgressData *) get_window_data(w); //C++ version of the C code above
	percent = pd->percent;
	if (percentage < 0)   percentage = 0;
	pd->percentage = percentage;
	sprintf(pbuffer, "%d%%", percentage);
	set_control_text(percent, pbuffer);
	draw_window(w);
	while (peek_event(w->app))
		do_event(w->app);
}

Window *new_progress_window(App *app, char *name, char *str)
{
	Window *progress;
	ProgressData *pd;
	Font *f;	Rect r1, r2, r3;
	int h, w = 500;

	f = find_default_font(app);
	h = font_height(f) * 3 / 2;
	r1 = rect(10,10,w,h*3);
	r2 = rect(20,20,r1.width+20,r1.height+56+h);
	r3 = rect((w-60)/2,20+h*3,60,h);

	progress = new_window(app, r2, name, MODAL+TITLEBAR+CENTERED);
	//pd = zero_alloc(sizeof(ProgressData));
	pd = (ProgressData *)zero_alloc(sizeof(ProgressData));// C++ version of the C code above
	set_window_data(progress, pd);
	pd->display = new_label(progress, r1, str, ALIGN_LEFT);
	pd->percent = new_label(progress, r3, "0%", ALIGN_CENTER);
	pd->percentage = 0;
	on_window_redraw(progress, draw_progress_window);
	show_window(progress);
	return progress;
}
