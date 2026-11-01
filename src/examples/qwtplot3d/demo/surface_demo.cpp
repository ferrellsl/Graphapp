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
#include <qwt3d_parametricsurface.h>
#include <qwt3d_enrichment.h>
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

// The controls around the plot, as in mesh2

class LightPointer : public Qwt3D::VertexEnrichment
{
public:
	Qwt3D::Enrichment* clone() const {return new LightPointer(*this);}
	void configure() {plot = 0;}
	void drawBegin()
	{
		GLint mode;
		glGetIntegerv(GL_MATRIX_MODE, &mode);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glColor3d(1, 0, 0);
		glBegin(GL_LINES);
			glVertex3d(pos.x, pos.y, pos.z);
			glVertex3d(0, 0, 0);
		glEnd();
		glPopMatrix();
		glMatrixMode(mode);
	}
	void draw(Qwt3D::Triple const&) {}

	Qwt3D::Triple pos;
};

class Sphere : public Qwt3D::ParametricSurface
{
public:
	Sphere(Qwt3D::SurfacePlot& pw) : Qwt3D::ParametricSurface(pw)
	{
		setMesh(41, 31);
		setDomain(0, 2 * Qwt3D::PI, 0, Qwt3D::PI);
		setPeriodic(false, false);
	}

	Qwt3D::Triple operator()(double u, double v)
	{
		return Qwt3D::Triple(cos(u) * sin(v), sin(u) * sin(v), cos(v));
	}
};

// The sphere in the lighting dialog: turn it to turn the light.
class LightPlot : public Qwt3D::SurfacePlot
{
public:
	LightPlot(Window* w, int x, int y, int width, int height)
		: Qwt3D::SurfacePlot(w, x, y, width, height)
	{
		Sphere sphere(*this);
		sphere.create();
		reset();
		assignMouse(Qwt3D::LeftButton, Qwt3D::RightButton, Qwt3D::LeftButton,
			Qwt3D::NoButton, Qwt3D::NoButton, Qwt3D::NoButton,
			Qwt3D::NoButton, Qwt3D::NoButton, Qwt3D::NoButton);
		stick = (LightPointer*)addEnrichment(LightPointer());
		stick->pos = Qwt3D::Triple(0, 0, 1);
		updateData();
	}

	void reset()
	{
		makeCurrent();
		setRotation(0, 0, 0);
		setTitle("Use your mouse buttons and keyboard");
		setTitleFont("Arial", 8, Qwt3D::Font::Bold);
		setTitleColor(Qwt3D::RGBA(0.9, 0.9, 0.9));
		setSmoothMesh(true);
		setZoom(0.9);
		setCoordinateStyle(Qwt3D::NOCOORD);
		setMeshColor(Qwt3D::RGBA(0.6, 0.6, 0.6, 0.3));
		setPlotStyle(Qwt3D::FILLEDMESH);
		setBackgroundColor(Qwt3D::RGBA(0, 0, 0));
		updateData();
	}

	LightPointer* stick;
};

static Control* orthobox;
static Control* legendbox;
static Control* autoscalebox;
static Control* mousebox;
static Control* shadingbox;
static Control* normalsbox;
static Control* normalslabel;
static Control* qualitybar;
static Control* lengthbar;
static Control* stdbutton;
static Control* lightbutton;
static Control* lightbox;
static Control* offsetlabel;
static Control* offsetbar;
static Control* reslabel;
static Control* resbar;

static Window* lightwin;
static LightPlot* lightplot;
static Control* distbar;
static Control* emissbar;
static Control* diffbar;
static Control* specbar;
static Control* shinbar;

enum { SIDEWIDTH = 120, BOTTOMHEIGHT = 58, MAXRES = 70 };

static void placeControls(Rect r)
{
	int top = barHeight();
	int plotheight = r.height - top - BOTTOMHEIGHT;
	int side = r.width - SIDEWIDTH + 8;
	plot->setGeometry(0, top, r.width - SIDEWIDTH, plotheight);

	Control* boxes[6] = {orthobox, legendbox, autoscalebox, mousebox, shadingbox, normalsbox};
	for (int i = 0; i != 6; ++i)
		set_control_area(boxes[i], rect(side, top + 8 + 24 * i, SIDEWIDTH - 12, 22));
	int barstop = top + 8 + 24 * 6 + 4;
	set_control_area(qualitybar, rect(side + 16, barstop, 18, plotheight - barstop + top - 8));
	set_control_area(lengthbar, rect(side + 64, barstop, 18, plotheight - barstop + top - 8));

	int y = r.height - BOTTOMHEIGHT + 8;
	int slider = (r.width - 240) / 2 - 16;
	set_control_area(stdbutton, rect(8, y + 8, 60, 28));
	set_control_area(lightbutton, rect(74, y + 8, 80, 28));
	set_control_area(lightbox, rect(160, y + 12, 22, 22));
	set_control_area(offsetlabel, rect(196, y - 2, slider, 20));
	set_control_area(offsetbar, rect(196, y + 20, slider, 18));
	set_control_area(reslabel, rect(196 + slider + 16, y - 2, slider, 20));
	set_control_area(resbar, rect(196 + slider + 16, y + 20, slider, 18));
}

static void resized(Window* w)
{
	placeControls(get_window_area(w));
}

static void orthoChanged(Control* c)
{
	plot->setOrtho(is_checked(c) != 0);
}

static void legendChanged(Control* c)
{
	plot->legendShown = is_checked(c) != 0;
	plot->redrawLegend(plot->legendShown);
}

static void autoscaleChanged(Control* c)
{
	plot->coordinates()->setAutoScale(is_checked(c) != 0);
	plot->updateGL();
}

static void mouseChanged(Control* c)
{
	plot->enableMouse(is_checked(c) != 0);
}

static void shadingChanged(Control* c)
{
	plot->setShading(is_checked(c) ? Qwt3D::GOURAUD : Qwt3D::FLAT);
	plot->updateData();
	plot->updateGL();
}

static void normalsChanged(Control* c)
{
	plot->showNormals(is_checked(c) != 0);
	plot->updateNormals();
	plot->updateGL();
}

static void normalQualityChanged(Control* c)
{
	plot->setNormalQuality(3 + (32 - 3) - get_control_value(c)); // up is more
	plot->updateNormals();
	plot->updateGL();
}

static void normalLengthChanged(Control* c)
{
	plot->setNormalLength((1 + (100 - 1) - get_control_value(c)) / 400.);
	plot->updateNormals();
	plot->updateGL();
}

static void standardView(Control*)
{
	plot->resetView();
}

static void lightingChanged(Control* c)
{
	bool on = is_checked(c) != 0;
	plot->enableLighting(on);
	plot->illuminate(0);
	plot->updateGL();
	if (on)
		enable(lightbutton);
	else
		disable(lightbutton);
}

static void offsetChanged(Control* c)
{
	plot->setPolygonOffset(get_control_value(c) / 10.0);
	plot->updateData();
	plot->updateGL();
}

static void resolutionChanged(Control* c)
{
	plot->setResolution(1 + get_control_value(c));
}

static void resolutionMoved(int res)
{
	change_scroll_bar(resbar, res - 1, MAXRES - 1, 1);
	updateTitle();
}

static void lightEmission(Control* c)
{
	plot->setMaterialComponent(GL_EMISSION, get_control_value(c) / 100.);
	plot->updateGL();
}

static void lightDiffuse(Control* c)
{
	plot->setLightComponent(GL_DIFFUSE, get_control_value(c) / 100.);
	plot->updateGL();
}

static void lightSpecular(Control* c)
{
	plot->setMaterialComponent(GL_SPECULAR, get_control_value(c) / 100.);
	plot->updateGL();
}

static void lightShininess(Control* c)
{
	plot->setShininess(get_control_value(c) / 100.);
	plot->updateGL();
}

static void lightDistance(Control* c)
{
	int val = get_control_value(c);
	lightplot->stick->pos = Qwt3D::Triple(0, 0, val / 100.);
	lightplot->updateData();
	lightplot->updateGL();

	Qwt3D::ParallelEpiped h = plot->hull();
	double drad = (h.maxVertex - h.minVertex).length() * val / 20.;
	plot->setLightShift(drad, drad, drad);
	plot->updateGL();
}

static void lightRotated(double x, double y, double z)
{
	lightDistance(distbar);
	plot->setLightRotation(x, y, z);
	plot->updateGL();
}

static void lightStandard(Control*)
{
	lightplot->reset();
	plot->updateGL();
}

static void lightOk(Control*)
{
	hide_window(lightwin);
}

static void lightClosed(Window*)
{
	hide_window(lightwin);
}

static Control* lightSlider(const char* name, int y, int max, int value, ControlFunc fn)
{
	new_label(lightwin, rect(356, y, 200, 20), name, ALIGN_LEFT);
	Control* c = new_scroll_bar(lightwin, rect(356, y + 22, 200, 18), max, 1, fn);
	change_scroll_bar(c, value, max, 1);
	return c;
}

static void pickLighting(Control*)
{
	if (!lightwin)
	{
		lightwin = new_window(app, rect(120, 120, 568, 366), "Lighting", TITLEBAR | CLOSEBOX);
		on_window_close(lightwin, lightClosed);
		set_window_background(lightwin, rgb(240, 240, 240));
		distbar = lightSlider("Distance", 8, 1000, 100, lightDistance);
		emissbar = lightSlider("Emission", 58, 100, 0, lightEmission);
		diffbar = lightSlider("Ambient and Diffuse", 108, 100, 80, lightDiffuse);
		specbar = lightSlider("Specular", 158, 100, 30, lightSpecular);
		shinbar = lightSlider("Shininess", 208, 1000, 500, lightShininess);
		new_button(lightwin, rect(356, 318, 90, 30), "Std", lightStandard);
		new_button(lightwin, rect(466, 318, 90, 30), "OK", lightOk);
		lightplot = new LightPlot(lightwin, 8, 8, 336, 336);
		lightplot->onRotationChanged = lightRotated;
	}
	show_window(lightwin);
	lightplot->show();
}

static void buildControls()
{
	orthobox = new_check_box(win, rect(0, 0, 10, 10), "Ortho", orthoChanged);
	legendbox = new_check_box(win, rect(0, 0, 10, 10), "Legend", legendChanged);
	autoscalebox = new_check_box(win, rect(0, 0, 10, 10), "Autoscale", autoscaleChanged);
	mousebox = new_check_box(win, rect(0, 0, 10, 10), "Mouse", mouseChanged);
	shadingbox = new_check_box(win, rect(0, 0, 10, 10), "Shading", shadingChanged);
	normalsbox = new_check_box(win, rect(0, 0, 10, 10), "Normals", normalsChanged);
	check(orthobox);
	check(autoscalebox);
	check(mousebox);
	check(shadingbox);
	qualitybar = new_scroll_bar(win, rect(0, 0, 18, 100), 32 - 3, 1, normalQualityChanged);
	change_scroll_bar(qualitybar, 32 - 3, 32 - 3, 1);
	lengthbar = new_scroll_bar(win, rect(0, 0, 18, 100), 100 - 1, 1, normalLengthChanged);
	change_scroll_bar(lengthbar, 100 - 8, 100 - 1, 1);

	stdbutton = new_button(win, rect(0, 0, 10, 10), "Std", standardView);
	lightbutton = new_button(win, rect(0, 0, 10, 10), "Lighting", pickLighting);
	disable(lightbutton);
	lightbox = new_check_box(win, rect(0, 0, 10, 10), "", lightingChanged);
	offsetlabel = new_label(win, rect(0, 0, 10, 10), "Polygon Offset", ALIGN_LEFT);
	offsetbar = new_scroll_bar(win, rect(0, 0, 100, 18), 30, 1, offsetChanged);
	change_scroll_bar(offsetbar, 5, 30, 1);
	reslabel = new_label(win, rect(0, 0, 10, 10), "Resolution", ALIGN_LEFT);
	resbar = new_scroll_bar(win, rect(0, 0, 100, 18), MAXRES - 1, 1, resolutionChanged);
}

// for testing: what clicking the control does
static void clickBox(Control* c, ControlFunc fn)
{
	if (is_checked(c))
		uncheck(c);
	else
		check(c);
	fn(c);
}

static void moveBar(Control* c, int value, int max, ControlFunc fn)
{
	change_scroll_bar(c, value, max, 1);
	fn(c);
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
		case 'l': clickBox(legendbox, legendChanged); break;
		case 'h': clickBox(lightbox, lightingChanged); break;
		case 'p': clickBox(orthobox, orthoChanged); break;
		case 'x': clickBox(shadingbox, shadingChanged); break;
		case 'n': clickBox(normalsbox, normalsChanged); break;
		case 'b': clickBox(autoscalebox, autoscaleChanged); break;
		case 'u': moveBar(offsetbar, 20, 30, offsetChanged); break;
		case 'L': pickLighting(NULL); break;
		case 'D': moveBar(distbar, 300, 1000, lightDistance); break;
		case 'S': moveBar(specbar, 80, 100, lightSpecular); break;
		case 'E': moveBar(emissbar, 20, 100, lightEmission); break;
		case 'R': lightplot->setRotation(60, 0, 120); break;
		case 'r': plot->showRipple(); break;
		case 'v': plot->resetView(); break;
		case 'o': openFile(NULL); break;
		case 'M': openColorMap(NULL); break;
		case 'a': moveBar(resbar, 0, MAXRES - 1, resolutionChanged); break;
		case 'c': moveBar(resbar, 2, MAXRES - 1, resolutionChanged); break;
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

	m = new_menu(bar, "Style");
	styleitem[0] = new_menu_item(m, "Filled Mesh", 0, setStyle);
	styleitem[1] = new_menu_item(m, "Filled", 0, setStyle);
	styleitem[2] = new_menu_item(m, "Wireframe", 0, setStyle);
	styleitem[3] = new_menu_item(m, "Hidden Line", 0, setStyle);
	styleitem[4] = new_menu_item(m, "Points", 0, setStyle);
	check_menu_item(styleitem[0]);

	m = new_menu(bar, "View");
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

	win = new_window(app, rect(60, 60, 1020, 740), "QwtPlot3D in GraphApp", STANDARD_WINDOW);
	set_window_background(win, rgb(240, 240, 240));
	on_window_close(win, quit);
	on_window_resize(win, resized);
	on_window_key_down(win, testKey);
	buildMenus();
	buildControls();

	Rect r = get_window_area(win);
	plot = new Plot(win, 0, barHeight(), 100, 100);
	placeControls(r);
	plot->onResolutionChanged = resolutionMoved;
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
