/*
 *  Surface plot demo: QwtPlot3D in a GraphApp window.
 *
 *  surface_demo [file.mes] [--map file.map] [--shot base] [--log file]
 *
 *  Drag to rotate (Shift: around y; Ctrl: move; Alt: scale), the wheel zooms, Shift+wheel
 *  exaggerates Z, the arrow keys rotate.  --shot and --log are for testing: keys save frames as base_N.ext and
 *  pick menu items (see testKey), and every change of view is logged.
 */

#include <ctype.h>
#include <fstream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// the library first: graphapp.h defines macros such as check, enable, warning and rect
#include <qwt3d_surfaceplot.h>
#include <qwt3d_function.h>
#include <qwt3d_io.h>

extern "C" {
#include "graphapp.h"
}

class Rosenbrock : public Qwt3D::Function
{
public:
	Rosenbrock(Qwt3D::SurfacePlot& pw) : Qwt3D::Function(pw) {}

	double operator()(double x, double y)
	{
		return log((1-x)*(1-x) + 100 * (y - x*x)*(y - x*x)) / 8;
	}
};

class Ripple : public Qwt3D::Function
{
public:
	Ripple(Qwt3D::SurfacePlot& pw) : Qwt3D::Function(pw) {}

	double operator()(double x, double y)
	{
		double r = sqrt(x*x + y*y);
		return (r < 1e-9) ? 1 : sin(r) / r;
	}
};

class Plot : public Qwt3D::SurfacePlot
{
public:
	Plot(Window* w, int x, int y, int width, int height)
		: Qwt3D::SurfacePlot(w, x, y, width, height), legendShown(false)
	{
		setTitleFont("Arial", 14);
		coordinates()->setLineSmooth(true);
		coordinates()->setGridLinesColor(Qwt3D::RGBA(0.35, 0.35, 0.35, 1));
		setCoordinateStyle(Qwt3D::BOX);
		setKeySpeed(15, 20, 20);
		resetView();
	}

	// mesh2's standard view
	void resetView()
	{
		setRotation(30, 0, 15);
		setViewportShift(0.05, 0);
		setScale(1, 1, 1);
		setZoom(0.95);
	}

	void showRosenbrock()
	{
		setTitle("Rosenbrock");
		setResolution(1);
		Rosenbrock f(*this);
		f.setMesh(41, 31);
		f.setDomain(-1.73, 1.5, -1.5, 1.5);
		f.setMinZ(-10);
		f.create();
		decorate(7, 4);
		resetView();
	}

	void showRipple()
	{
		setTitle("Ripple");
		setResolution(1);
		Ripple f(*this);
		f.setMesh(81, 81);
		f.setDomain(-12, 12, -12, 12);
		f.create();
		decorate(7, 4);
		resetView();
		setScale(1, 1, 10); // the function is far lower than it is wide
	}

	bool showFile(const char* name)
	{
		// before loading, so the data is drawn once: mesh2 draws it at full resolution first
		setResolution(3);
		if (!Qwt3D::IO::load(this, name, "mes"))
			return false;
		const char* base = strrchr(name, '\\');
		setTitle(base ? base + 1 : name);
		decorate(4, 5);
		resetView();
		return true;
	}

	// mesh2 reads color maps the same way: "r g b" lines of 0..255
	bool setColorMap(const char* name)
	{
		std::ifstream file(name);
		if (!file)
			return false;
		Qwt3D::ColorVector cv;
		Qwt3D::RGBA rgb;
		while (file)
		{
			file >> rgb.r >> rgb.g >> rgb.b;
			file.ignore(1000, '\n');
			if (!file.good())
				break;
			rgb.a = 1;
			rgb.r /= 255;
			rgb.g /= 255;
			rgb.b /= 255;
			cv.push_back(rgb);
		}
		if (cv.empty())
			return false;
		Qwt3D::StandardColor* col = new Qwt3D::StandardColor(this);
		col->setColorVector(cv);
		setColors(col);
		return true;
	}

	void resetColors()
	{
		setColors(new Qwt3D::StandardColor(this));
	}

	void redrawLegend(bool show)
	{
		legend()->setMajors(4);
		legend()->setMinors(5);
		double start, stop;
		coordinates()->axes[Qwt3D::Z1].limits(start, stop);
		legend()->setLimits(start, stop);
		showColorLegend(show);
	}

	bool legendShown;

private:
	void setColors(Qwt3D::StandardColor* col)
	{
		setDataColor(col);
		updateData();
		redrawLegend(legendShown);
	}

	// new data resets the axes
	void decorate(int majors, int minors)
	{
		for (unsigned i=0; i!=coordinates()->axes.size(); ++i)
		{
			coordinates()->axes[i].setMajors(majors);
			coordinates()->axes[i].setMinors(minors);
		}
		// the box shows whichever of the four parallel axes faces the viewer
		static const Qwt3D::AXIS xs[4] = {Qwt3D::X1, Qwt3D::X2, Qwt3D::X3, Qwt3D::X4};
		static const Qwt3D::AXIS ys[4] = {Qwt3D::Y1, Qwt3D::Y2, Qwt3D::Y3, Qwt3D::Y4};
		static const Qwt3D::AXIS zs[4] = {Qwt3D::Z1, Qwt3D::Z2, Qwt3D::Z3, Qwt3D::Z4};
		for (int i = 0; i != 4; ++i)
		{
			coordinates()->axes[xs[i]].setLabelString("x");
			coordinates()->axes[ys[i]].setLabelString("y");
			coordinates()->axes[zs[i]].setLabelString("z");
		}
		redrawLegend(legendShown);
		updateData();
		updateGL();
	}
};

static App* app;
static Window* win;
static MenuBar* bar;
static Plot* plot;
static MenuItem* styleitem[5];
static MenuItem* resitem[5];
static MenuItem* legenditem;
static MenuItem* lightitem;
static MenuItem* perspectiveitem;
static const char* shotbase;
static const char* mapname;
static FILE* logfile;
static int shots;

static void updateTitle();
static void setStyle(MenuItem* mi);

static int barHeight()
{
	return bar->ctrl->area.height;
}

static void resized(Window* w)
{
	Rect r = get_window_area(w);
	plot->setGeometry(0, barHeight(), r.width, r.height - barHeight());
}

static void quit(Window*)
{
	delete plot;
	exit(0);
}

static void menuQuit(MenuItem*)
{
	quit(win);
}

static void saveAs(MenuItem*)
{
	std::string file = plot->askSaveFile("Save plot", "PNG, GIF, EPS, PS or PDF|*.png;*.gif;*.eps;*.ps;*.pdf", "plot.png");
	if (file.empty())
		return;
	const char* name = file.c_str();
	const char* dot = strrchr(name, '.');
	std::string ext = dot ? dot + 1 : "";
	for (size_t i = 0; i != ext.size(); ++i)
		ext[i] = (char)toupper((unsigned char)ext[i]);
	if (!plot->save(name, ext))
		plot->tellUser("Save plot", "That could not be saved. Is the extension one of those listed?");
}

static void openFile(MenuItem*)
{
	std::string name = plot->askOpenFile("Open surface", "QwtPlot3D grids|*.mes");
	if (name.empty())
		return;
	setStyle(styleitem[1]);
	if (!plot->showFile(name.c_str()))
		plot->tellUser("Open surface", "That is not a complete .mes file.");
	updateTitle();
}

static void showRosenbrock(MenuItem*)
{
	plot->showRosenbrock();
}

static void showRipple(MenuItem*)
{
	plot->showRipple();
}

static void setStyle(MenuItem* mi)
{
	static const Qwt3D::PLOTSTYLE styles[5] =
		{Qwt3D::FILLEDMESH, Qwt3D::FILLED, Qwt3D::WIREFRAME, Qwt3D::HIDDENLINE, Qwt3D::POINTS};
	for (int i = 0; i != 5; ++i)
	{
		if (styleitem[i] == mi)
		{
			check_menu_item(mi);
			plot->setPlotStyle(styles[i]);
		}
		else
			uncheck_menu_item(styleitem[i]);
	}
	plot->updateData();
	plot->updateGL();
}

static void openColorMap(MenuItem*)
{
	std::string name = plot->askOpenFile("Open color map", "Color maps|*.map");
	if (!name.empty() && !plot->setColorMap(name.c_str()))
		plot->tellUser("Open color map", "That is not a color map file.");
}

static void resetColors(MenuItem*)
{
	plot->resetColors();
}

static void setResolutionItem(MenuItem* mi)
{
	static const int steps[5] = {1, 2, 3, 5, 10};
	for (int i = 0; i != 5; ++i)
	{
		if (resitem[i] == mi)
		{
			check_menu_item(mi);
			plot->setResolution(steps[i]);
		}
		else
			uncheck_menu_item(resitem[i]);
	}
}

static void toggleLegend(MenuItem* mi)
{
	plot->legendShown = !plot->legendShown;
	if (plot->legendShown)
		check_menu_item(mi);
	else
		uncheck_menu_item(mi);
	plot->redrawLegend(plot->legendShown);
}

static void togglePerspective(MenuItem* mi)
{
	bool ortho = !plot->ortho();
	if (ortho)
		uncheck_menu_item(mi);
	else
		check_menu_item(mi);
	plot->setOrtho(ortho);
}

static void toggleLighting(MenuItem* mi)
{
	static bool on = false;
	on = !on;
	if (on)
		check_menu_item(mi);
	else
		uncheck_menu_item(mi);
	plot->enableLighting(on);
	plot->illuminate(0);
	plot->updateData();
	plot->updateGL();
}

static void resetView(MenuItem*)
{
	plot->resetView();
}

// what mesh2 shows in its status bar
static void updateTitle()
{
	char title[200];
	std::pair<int,int> cells = plot->facets();
	sprintf(title, "QwtPlot3D in GraphApp - %dx%d  Z %.2f  zoom %.2f  angles %g %g %g", cells.first,
		cells.second, plot->zScale(), plot->zoom(), plot->xRotation(), plot->yRotation(), plot->zRotation());
	set_window_title(win, title);
}

static void viewChanged(double, double, double)
{
	updateTitle();
}

static void zoomChanged(double)
{
	updateTitle();
}

static void testSave(const char* ext, const char* format)
{
	char name[300];
	sprintf(name, "%s_%d.%s", shotbase, ++shots, ext);
	bool ok = plot->save(name, format);
	if (logfile)
	{
		fprintf(logfile, "saved %s: %s (rotation %g %g %g, zoom %g, size %dx%d)\n", name,
			ok ? "ok" : "FAILED", plot->xRotation(), plot->yRotation(), plot->zRotation(),
			plot->zoom(), plot->width(), plot->height());
		fflush(logfile);
	}
}

static void testKey(Window*, unsigned long key)
{
	if (!shotbase)
		return;
	switch (key)
	{
		case 's': testSave("png", "PNG"); break;
		case 'g': testSave("gif", "GIF"); break;
		case 'e': testSave("eps", "EPS"); break;
		case 'd': testSave("pdf", "PDF"); break;
		case 'l': toggleLegend(legenditem); break;
		case 'h': toggleLighting(lightitem); break;
		case 'p': togglePerspective(perspectiveitem); break;
		case 'r': plot->showRipple(); break;
		case 'v': plot->resetView(); break;
		case 'o': openFile(NULL); break;
		case 'M': openColorMap(NULL); break;
		case 'a': setResolutionItem(resitem[0]); break;
		case 'c': setResolutionItem(resitem[2]); break;
		case 'm':
			if (mapname)
				plot->setColorMap(mapname);
			break;
		case 'z':
		{
			clock_t start = clock();
			for (int i = 0; i != 100; ++i)
			{
				plot->setScale(1, 1, 1 + i * 0.5);
				glFinish();
			}
			if (logfile)
				fprintf(logfile, "100 Z scales in %ld ms\n", (long)(clock() - start) * 1000 / CLOCKS_PER_SEC);
			break;
		}
		case 't':
		{
			clock_t start = clock();
			for (int i = 0; i != 100; ++i)
			{
				plot->setRotation(30, 0, 15 + 3 * i);
				glFinish(); // the GPU's time too
			}
			if (logfile)
				fprintf(logfile, "100 rotations in %ld ms\n", (long)(clock() - start) * 1000 / CLOCKS_PER_SEC);
			break;
		}
		case '1': case '2': case '3': case '4': case '5':
			setStyle(styleitem[key - '1']);
			break;
	}
}

static void logRotation(double x, double y, double z)
{
	updateTitle();
	fprintf(logfile, "rotation %g %g %g\n", x, y, z);
	fflush(logfile);
}

static void logZoom(double z)
{
	updateTitle();
	fprintf(logfile, "zoom %g\n", z);
	fflush(logfile);
}

static void logScale(double x, double y, double z)
{
	updateTitle();
	fprintf(logfile, "scale %g %g %g\n", x, y, z);
	fflush(logfile);
}

static void logViewportShift(double x, double y)
{
	fprintf(logfile, "viewport shift %g %g\n", x, y);
	fflush(logfile);
}

static void buildMenus()
{
	Menu* m;

	bar = new_menu_bar(win);

	m = new_menu(bar, "File");
	new_menu_item(m, "Open...", 'O', openFile);
	new_menu_item(m, "Save As...", 0, saveAs);
	new_menu_item(m, "-", 0, NULL);
	new_menu_item(m, "Exit", 'Q', menuQuit);

	m = new_menu(bar, "Data");
	new_menu_item(m, "Rosenbrock", 0, showRosenbrock);
	new_menu_item(m, "Ripple", 0, showRipple);

	m = new_menu(bar, "Color");
	new_menu_item(m, "Open Color Map...", 'M', openColorMap);
	new_menu_item(m, "Reset Colors", 0, resetColors);

	m = new_menu(bar, "Resolution");
	resitem[0] = new_menu_item(m, "Every Point", 0, setResolutionItem);
	resitem[1] = new_menu_item(m, "1 in 2", 0, setResolutionItem);
	resitem[2] = new_menu_item(m, "1 in 3", 0, setResolutionItem);
	resitem[3] = new_menu_item(m, "1 in 5", 0, setResolutionItem);
	resitem[4] = new_menu_item(m, "1 in 10", 0, setResolutionItem);
	check_menu_item(resitem[2]);

	m = new_menu(bar, "Style");
	styleitem[0] = new_menu_item(m, "Filled Mesh", 0, setStyle);
	styleitem[1] = new_menu_item(m, "Filled", 0, setStyle);
	styleitem[2] = new_menu_item(m, "Wireframe", 0, setStyle);
	styleitem[3] = new_menu_item(m, "Hidden Line", 0, setStyle);
	styleitem[4] = new_menu_item(m, "Points", 0, setStyle);
	check_menu_item(styleitem[0]);

	m = new_menu(bar, "View");
	legenditem = new_menu_item(m, "Color Legend", 0, toggleLegend);
	perspectiveitem = new_menu_item(m, "Perspective", 0, togglePerspective);
	lightitem = new_menu_item(m, "Lighting", 0, toggleLighting);
	new_menu_item(m, "-", 0, NULL);
	new_menu_item(m, "Reset View", 'R', resetView);
}

int main(int argc, char* argv[])
{
	const char* file = NULL;

	app = new_app(argc, argv);
	if (!app)
		return 1;

	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "--shot") == 0 && i + 1 < argc)
			shotbase = argv[++i];
		else if (strcmp(argv[i], "--map") == 0 && i + 1 < argc)
			mapname = argv[++i];
		else if (strcmp(argv[i], "--log") == 0 && i + 1 < argc)
			logfile = fopen(argv[++i], "w");
		else
			file = argv[i];
	}

	win = new_window(app, rect(60, 60, 900, 680), "QwtPlot3D in GraphApp", STANDARD_WINDOW);
	on_window_close(win, quit);
	on_window_resize(win, resized);
	on_window_key_down(win, testKey);
	buildMenus();

	Rect r = get_window_area(win);
	plot = new Plot(win, 0, barHeight(), r.width, r.height - barHeight());
	if (logfile)
	{
		plot->onRotationChanged = logRotation;
		plot->onZoomChanged = logZoom;
		plot->onScaleChanged = logScale;
		plot->onViewportShiftChanged = logViewportShift;
	}
	plot->onRotationChanged = plot->onRotationChanged ? plot->onRotationChanged : viewChanged;
	plot->onScaleChanged = plot->onScaleChanged ? plot->onScaleChanged : viewChanged;
	plot->onZoomChanged = plot->onZoomChanged ? plot->onZoomChanged : zoomChanged;

	if (file)
	{
		clock_t start = clock();
		setStyle(styleitem[1]); // the mesh lines of a dense grid would be solid black
		bool ok = plot->showFile(file);
		if (logfile)
		{
			fprintf(logfile, "open %s: %s in %ld ms\n", file, ok ? "ok" : "FAILED",
				(long)(clock() - start) * 1000 / CLOCKS_PER_SEC);
			fflush(logfile);
		}
		if (!ok)
			file = NULL;
	}
	if (!file)
		plot->showRosenbrock();
	if (mapname)
		plot->setColorMap(mapname);
	updateTitle();

	show_window(win);
	plot->show();
	plot->setFocus();
	main_loop(app);
	return 0;
}
