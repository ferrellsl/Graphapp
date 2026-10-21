/*
 *  Busy spinner
 *  ------------
 *
 *  A small window with a spinning ring and a message ("Busy..."), so that
 *  the user can see the program is working, not frozen.  The ring is a
 *  thick arc with round ends that turns, growing and shrinking as it goes,
 *  like the spinners of Windows and Android.
 *
 *      Window *w = new_spinner_window(app, "Please wait", NULL);
 *      while (working) {
 *          ...a bit of the work...
 *          spinner_step(w);        <- as often as you like
 *      }
 *      del_spinner_window(w);
 *
 *  spinner_step() costs almost nothing when it is called too soon: it only
 *  draws when the next frame is due (60 a second), and it lets GraphApp
 *  handle its events, so the window is repainted and does not "stop
 *  responding".  While the program is waiting in its main loop instead
 *  (for a reply, a download), the window's own timer keeps it turning.
 *
 *  Each frame is drawn by the program itself, antialiased, into an image of
 *  the ring alone, which is then copied to the window in one go: nothing is
 *  erased first, so nothing flickers.
 *
 *  Steve Ferrell, 2016 (the idea); finished 2026.
 */

#include <math.h>
#include <string.h>
#ifdef _WIN32
/* (the clock, declared here: windows.h and graphapp.h use some of the same names) */
__declspec(dllimport) int __stdcall QueryPerformanceCounter(long long *count);
__declspec(dllimport) int __stdcall QueryPerformanceFrequency(long long *frequency);
#else
#include <time.h>
#endif

Window *new_spinner_window(App *app, const char *title, const char *message);
void    spinner_step(Window *w);
void    set_spinner_message(Window *w, const char *message);
void    del_spinner_window(Window *w);

#define SPINNER_SIZE       56          /* the ring's box, in pixels */
#define SPINNER_THICKNESS  6.5         /* the ring's line */
#define SPINNER_FRAME_MS   16          /* about 60 frames a second */

typedef struct SpinnerData SpinnerData;

struct SpinnerData {
	Image *     image;                 /* the ring, drawn here each frame */
	Rect        ring;                  /* where it goes in the window */
	Rect        words;                 /* where the message goes */
	char        message[200];
	Colour      fg, bg;                /* the ring, and the window behind it */
	double      started, drawn;        /* seconds */
	Timer *     timer;
};

/* seconds, from a clock finer than GraphApp's own (which ticks every 16 ms
   on Windows: the frames would come unevenly) */
static double spinner_seconds(void)
{
#ifdef _WIN32
	static long long freq;
	long long now;
	if (freq == 0)
		QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&now);
	return (double) now / (double) freq;
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
#endif
}

static double spinner_ease(double u)       /* slow, fast, slow */
{
	return u < 0.5 ? 4 * u * u * u : 1 - pow(-2 * u + 2, 3) / 2;
}

/* Where the arc is at a time: from "tail" round to "head", in degrees
   clockwise from the top.  It grows (the head runs ahead), then shrinks
   (the tail catches up), 270 degrees further on each time, while the
   whole of it turns. */
static void spinner_arc(double seconds, double *tail, double *head)
{
	const double cycle = 1.3, smallest = 18, largest = 280;
	double c = floor(seconds / cycle), u = seconds / cycle - c;
	/* (it turns 188 degrees a cycle: with the 262 the arc moves on, 90 degrees
	   in all, so the whole motion repeats every four cycles, 5.2 seconds) */
	double turn = seconds * 188 / cycle;
	double base = fmod(c * (largest - smallest), 360.0);

	if (u < 0.5) {
		*tail = base;
		*head = base + smallest + (largest - smallest) * spinner_ease(u * 2);
	} else {
		*head = base + largest;
		*tail = base + (largest - smallest) * spinner_ease(u * 2 - 1);
	}
	*tail += turn;
	*head += turn;
}

static byte spinner_mix(byte a, byte b, double k)
{
	return (byte) (a + (b - a) * k + 0.5);
}

/* The ring at a time, into the image: every pixel covered as much as the
   arc covers it (so its edges are smooth), over the background. */
static void spinner_render(SpinnerData *sd, double seconds)
{
	const double pi = 3.14159265358979;
	Image *img = sd->image;
	int n = img->width, x, y;
	double c = (n - 1) / 2.0;
	double half = SPINNER_THICKNESS / 2;
	double radius = n / 2.0 - half - 1;
	double tail, head, span, a0, hx, hy, tx, ty;

	spinner_arc(seconds, &tail, &head);
	span = head - tail;
	a0 = fmod(tail, 360.0);
	if (a0 < 0) a0 += 360;
	/* the round ends: the middle of the line at each end of the arc */
	tx = c + radius * sin(a0 * pi / 180);
	ty = c - radius * cos(a0 * pi / 180);
	hx = c + radius * sin((a0 + span) * pi / 180);
	hy = c - radius * cos((a0 + span) * pi / 180);

	for (y = 0; y < n; y++) {
		Colour *row = img->data32[y];
		for (x = 0; x < n; x++) {
			double dx = x - c, dy = y - c;
			double r = sqrt(dx * dx + dy * dy);
			double angle = atan2(dx, -dy) * 180 / pi;   /* clockwise from the top */
			double along = angle - a0, d, k;
			while (along < 0) along += 360;
			if (along <= span)
				d = fabs(r - radius);                    /* beside the arc */
			else {                                           /* by one of its ends */
				double d1 = sqrt((x - tx) * (x - tx) + (y - ty) * (y - ty));
				double d2 = sqrt((x - hx) * (x - hx) + (y - hy) * (y - hy));
				d = d1 < d2 ? d1 : d2;
			}
			k = half + 0.5 - d;                              /* coverage, 0 to 1 */
			if (k < 0) k = 0;
			if (k > 1) k = 1;
			row[x].alpha = 0;
			row[x].red   = spinner_mix(sd->bg.red,   sd->fg.red,   k);
			row[x].green = spinner_mix(sd->bg.green, sd->fg.green, k);
			row[x].blue  = spinner_mix(sd->bg.blue,  sd->fg.blue,  k);
		}
	}
}

static void spinner_draw_ring(Window *w, Graphics *g)
{
	SpinnerData *sd = (SpinnerData *) get_window_data(w);
	double now = spinner_seconds();
	spinner_render(sd, now - sd->started);
	draw_image(g, sd->ring, sd->image, rect(0, 0, sd->image->width, sd->image->height));
	sd->drawn = now;
}

static void draw_spinner_window(Window *w, Graphics *g)
{
	SpinnerData *sd = (SpinnerData *) get_window_data(w);
	set_colour(g, sd->bg);
	fill_rect(g, get_window_area(w));
	set_colour(g, BLACK);
	set_font(g, find_default_font(w->app));
	draw_text(g, sd->words, ALIGN_CENTER | VALIGN_CENTER, sd->message, strlen(sd->message));
	spinner_draw_ring(w, g);
}

/* Draws the next frame if it is due, and lets GraphApp handle what has
   happened meanwhile.  Call it often from a loop that does the work. */
void spinner_step(Window *w)
{
	SpinnerData *sd;
	Graphics *g;

	if (w == NULL) return;
	sd = (SpinnerData *) get_window_data(w);
	if ((spinner_seconds() - sd->drawn) * 1000 < SPINNER_FRAME_MS)
		return;
	g = get_window_graphics(w);
	spinner_draw_ring(w, g);
	del_graphics(g);
	while (peek_event(w->app))
		do_event(w->app);
}

/* (while the program waits in its main loop: the window's timer turns it) */
static void spinner_tick(Timer *t)
{
	spinner_step((Window *) t->data);
}

void set_spinner_message(Window *w, const char *message)
{
	SpinnerData *sd = (SpinnerData *) get_window_data(w);
	if (message == NULL) message = "Busy...";
	if (strncmp(sd->message, message, sizeof(sd->message) - 1) == 0)
		return;                        /* (the same: nothing to repaint, nothing flashes) */
	strncpy(sd->message, message, sizeof(sd->message) - 1);
	redraw_rect(w, sd->words);
}

/* A spinner in a window of its own, in the middle of the screen; its
   message "Busy..." unless another is given. */
Window *new_spinner_window(App *app, const char *title, const char *message)
{
	Window *w;
	SpinnerData *sd;
	Font *f = find_default_font(app);
	int h = font_height(f);
	int width = 240, height = 40 + SPINNER_SIZE + 16 + h + 28;

	w = new_window(app, rect(0, 0, width, height), title ? title : "Busy", MODAL + TITLEBAR + CENTERED);
	sd = (SpinnerData *) zero_alloc(sizeof(SpinnerData));
	set_window_data(w, sd);
	strncpy(sd->message, message ? message : "Busy...", sizeof(sd->message) - 1);
	sd->fg = rgb(0x4F, 0x3C, 0xD6);    /* the purple of Windows' own */
	sd->bg = WHITE;
	sd->image = new_image(SPINNER_SIZE, SPINNER_SIZE, 32);
	sd->ring = rect((width - SPINNER_SIZE) / 2, 40, SPINNER_SIZE, SPINNER_SIZE);
	sd->words = rect(0, 40 + SPINNER_SIZE + 16, width, h + 4);
	sd->started = spinner_seconds();
	on_window_redraw(w, draw_spinner_window);
	show_window(w);
	sd->timer = new_timer(app, spinner_tick, SPINNER_FRAME_MS);
	sd->timer->data = w;
	spinner_step(w);
	return w;
}

void del_spinner_window(Window *w)
{
	SpinnerData *sd;
	if (w == NULL) return;
	sd = (SpinnerData *) get_window_data(w);
	del_timer(sd->timer);
	hide_window(w);
	del_image(sd->image);
	set_window_data(w, NULL);
	free(sd);
	del_window(w);
}
