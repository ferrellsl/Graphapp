/*
 *  Surface plot demo: QwtPlot3D in a GraphApp window, with the controls of its Qt demo, mesh2.
 *
 *  surface_demo [file.mes] [--map file.map] [--shot base] [--log file]
 *
 *  Drag to rotate (Shift: around y; Ctrl: move; Alt: scale), the wheel zooms, Shift+wheel
 *  exaggerates Z, the arrow keys rotate.  --shot and --log are for testing: keys save frames
 *  as base_N.ext and reach what the menus do (see testKey), the buttons are listed, and
 *  every change is logged.
 */

#include <ctype.h>
#include <float.h>
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

#include "icons.h"

// mesh2's functions and surfaces

class Rosenbrock : public Qwt3D::Function
{
public:
	Rosenbrock(Qwt3D::SurfacePlot& pw) : Qwt3D::Function(pw) {}
	double operator()(double x, double y)
	{
		return 0.7 * log10((1-x)*(1-x) + 10 * (y - x*x)*(y - x*x));
	}
};

class Hat : public Qwt3D::Function
{
public:
	Hat(Qwt3D::SurfacePlot& pw) : Qwt3D::Function(pw) {}
	double operator()(double x, double y)
	{
		return 1.0 / (x*x+y*y+0.5);
	}
};

class Ripple : public Qwt3D::Function
{
public:
	Ripple(Qwt3D::SurfacePlot& pw) : Qwt3D::Function(pw)
	{
		setDomain(-12, 12, -12, 12);
	}
	double operator()(double x, double y)
	{
		const double k = .913 * 2 * Qwt3D::PI;
		return cos(sqrt(x*x+y*y) + cos(sqrt((x+k)*(x+k)+y*y)) + cos(sqrt((x-k)*(x-k)+y*y))) * 4;
	}
};

class Saddle : public Qwt3D::Function
{
public:
	Saddle(Qwt3D::SurfacePlot& pw) : Qwt3D::Function(pw) {}
	double operator()(double x, double y)
	{
		return x*x - y*y;
	}
};

class Mex : public Qwt3D::Function
{
public:
	Mex(Qwt3D::SurfacePlot& pw) : Qwt3D::Function(pw) {}
	double operator()(double x, double y)
	{
		double n = sqrt(x*x+y*y);
		return (n < DBL_MIN) ? 20 : 20 * sin(n) / n;
	}
};

class Torus : public Qwt3D::ParametricSurface
{
public:
	Torus(Qwt3D::SurfacePlot& pw) : Qwt3D::ParametricSurface(pw)
	{
		setMesh(41, 31);
		setDomain(-2*Qwt3D::PI, 0, -2*Qwt3D::PI, 0);
		setPeriodic(true, true);
	}
	Qwt3D::Triple operator()(double u, double v)
	{
		double c = 1.9;
		return Qwt3D::Triple((c + cos(v)) * cos(u), (c + cos(v)) * sin(u), sin(v) + cos(v));
	}
};

class Seashell : public Qwt3D::ParametricSurface
{
public:
	Seashell(Qwt3D::SurfacePlot& pw) : Qwt3D::ParametricSurface(pw)
	{
		setMesh(41, 131);
		setDomain(0, 2*Qwt3D::PI, 0, 2*Qwt3D::PI);
		setPeriodic(true, true);
	}
	Qwt3D::Triple operator()(double u, double v)
	{
		double a = 1, b = 6, c = 0.5;
		int n = 3;
		double f = v / (2*Qwt3D::PI);
		return Qwt3D::Triple(a*(1-f)*cos(n*v)*(1+cos(u)) + c*cos(n*v),
			a*(1-f)*sin(n*v)*(1+cos(u)) + c*sin(n*v), b*f + a*(1-f)*sin(u));
	}
};

class Boy : public Qwt3D::ParametricSurface
{
public:
	Boy(Qwt3D::SurfacePlot& pw) : Qwt3D::ParametricSurface(pw)
	{
		setMesh(141, 131);
		setDomain(0, Qwt3D::PI, 0, Qwt3D::PI);
		setPeriodic(true, true);
	}
	Qwt3D::Triple operator()(double u, double v)
	{
		double a = 2/3., b = sqrt(2.);
		return Qwt3D::Triple(a*(cos(u)*cos(2*v)+b*sin(u)*cos(v))*cos(u) / (b-sin(2*u)*sin(3*v)),
			a*(cos(u)*sin(2*v)-b*sin(u)*sin(v))*cos(u) / (b-sin(2*u)*sin(3*v)),
			b*cos(u)*cos(u) / (b-sin(2*u)*sin(2*v)));
	}
};

class Dini : public Qwt3D::ParametricSurface
{
public:
	Dini(Qwt3D::SurfacePlot& pw) : Qwt3D::ParametricSurface(pw)
	{
		setMesh(141, 35);
		setDomain(0, 5*Qwt3D::PI, 0.001, 2);
		setPeriodic(true, true);
	}
	Qwt3D::Triple operator()(double u, double v)
	{
		double a = 5, b = 1;
		return Qwt3D::Triple(a*cos(u)*sin(v), a*sin(u)*sin(v), a*(cos(v)+log(tan(v/2)))+b*u);
	}
};

static void createCone(Qwt3D::TripleField& conepos, Qwt3D::CellField& conecell)
{
	using Qwt3D::Triple;
	using Qwt3D::Cell;
	conepos.clear();
	conecell.clear();
	conepos.push_back(Triple(0, 0, 0));

	const unsigned int count = 17;
	double r1 = 0.7, r2 = 0.9 * r1, h1 = 2, h2 = 1.03 * h1;
	unsigned i;
	Cell c1;
	for (i = 0; i < count; ++i)
		conepos.push_back(Triple(r1*cos(2*i*Qwt3D::PI/count), r1*sin(2*i*Qwt3D::PI/count), h1));
	for (i = 0; i < count; ++i)
	{
		conepos.push_back(Triple(r2*cos(2*i*Qwt3D::PI/count), r2*sin(2*i*Qwt3D::PI/count), h2));
		c1.push_back(count + i + 1);
	}
	conecell.push_back(c1);

	Cell c2(3);
	for (i = 1; i < count + 1; ++i)
	{
		c2[0] = 0;
		c2[1] = i % count + 1;
		c2[2] = i;
		conecell.push_back(c2);
	}
	c2 = Cell(4);
	for (i = 1; i < count + 1; ++i)
	{
		c2[0] = i;
		c2[1] = i % count + 1;
		c2[2] = count + i % count + 1;
		c2[3] = count + i;
		conecell.push_back(c2);
	}
}

class Plot : public Qwt3D::SurfacePlot
{
public:
	Plot(Window* w, int x, int y, int width, int height)
		: Qwt3D::SurfacePlot(w, x, y, width, height), legendShown(false), majors_(4), minors_(5)
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

	void showFunction(const char* name)
	{
		makeCurrent();
		linearScales();
		setTitle(name);
		if (strcmp(name, "Rosenbrock") == 0)
		{
			Rosenbrock f(*this);
			f.setMesh(50, 51);
			f.setDomain(-1.73, 1.55, -1.5, 1.95);
			f.setMinZ(-100);
			f.create();
			coordinates()->axes[Qwt3D::Z1].setScale(Qwt3D::LOG10SCALE);
			coordinates()->axes[Qwt3D::Z2].setScale(Qwt3D::LOG10SCALE);
			coordinates()->axes[Qwt3D::Z3].setScale(Qwt3D::LOG10SCALE);
			coordinates()->axes[Qwt3D::Z4].setScale(Qwt3D::LOG10SCALE);
			legend()->setScale(Qwt3D::LOG10SCALE);
		}
		else if (strcmp(name, "Hat") == 0)
		{
			Hat f(*this);
			f.setMesh(51, 72);
			f.setDomain(-1.5, 1.5, -1.5, 1.5);
			f.create();
		}
		else if (strcmp(name, "Ripple") == 0)
		{
			Ripple f(*this);
			f.setMesh(120, 120);
			f.create();
		}
		else if (strcmp(name, "Saddle") == 0)
		{
			Saddle f(*this);
			f.setMesh(71, 71);
			f.setDomain(-2.5, 2.5, -2.5, 2.5);
			f.create();
		}
		else if (strcmp(name, "Sombrero") == 0)
		{
			Mex f(*this);
			f.setMesh(91, 91);
			f.setDomain(-15, 15, -15, 15);
			f.create();
		}
		decorate(7, 5);
	}

	void showSurface(const char* name)
	{
		makeCurrent();
		linearScales();
		setTitle(name);
		if (strcmp(name, "Torus") == 0)
		{
			Torus f(*this);
			f.create();
		}
		else if (strcmp(name, "Seashell") == 0)
		{
			Seashell f(*this);
			f.create();
		}
		else if (strcmp(name, "Boy") == 0)
		{
			Boy f(*this);
			f.create();
		}
		else if (strcmp(name, "Dini") == 0)
		{
			Dini f(*this);
			f.create();
		}
		else if (strcmp(name, "Cone") == 0)
		{
			Qwt3D::TripleField pos;
			Qwt3D::CellField cell;
			createCone(pos, cell);
			loadFromData(pos, cell);
		}
		decorate(7, 5);
	}

	bool showFile(const char* name)
	{
		// before loading, so the data is drawn once: mesh2 draws it at full resolution first
		setResolution(3);
		linearScales();
		if (!Qwt3D::IO::load(this, name, "mes"))
			return false;
		const char* base = strrchr(name, '\\');
		setTitle(base ? base + 1 : name);
		decorate(4, 5);
		resetView();
		return true;
	}

	// mesh2's femreader: nodes of six numbers, cells of a number and four corners
	bool showFem(const char* nodes, const char* cells)
	{
		std::ifstream nodefile(nodes), cellfile(cells);
		if (!nodefile || !cellfile)
			return false;
		Qwt3D::TripleField pos;
		Qwt3D::CellField cell;
		for (;;)
		{
			double v[6];
			for (int i = 0; i != 6; ++i)
				nodefile >> v[i];
			if (!nodefile.good())
				break;
			pos.push_back(Qwt3D::Triple(v[1], v[2], v[5] / 1000));
		}
		for (;;)
		{
			Qwt3D::Cell c(4);
			cellfile >> c[0];
			for (unsigned i = 0; i != 4; ++i)
			{
				cellfile >> c[i];
				c[i] -= 1;
			}
			if (!cellfile.good())
				break;
			cell.push_back(c);
		}
		if (pos.empty() || cell.empty())
			return false;
		linearScales();
		loadFromData(pos, cell);
		setTitle("FEM");
		decorate(4, 5);
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
		const Qwt3D::RGBA black(0, 0, 0, 1);
		coordinates()->setAxesColor(black);
		setBackgroundColor(Qwt3D::RGBA(1, 1, 1, 1));
		setMeshColor(black);
		coordinates()->setNumberColor(black);
		coordinates()->setLabelColor(black);
		setTitleColor(black);
		setColors(new Qwt3D::StandardColor(this));
	}

	void redrawLegend(bool show)
	{
		legend()->setMajors(majors_);
		legend()->setMinors(minors_);
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

	void linearScales()
	{
		coordinates()->setStandardScale();
		legend()->setScale(Qwt3D::LINEARSCALE);
	}

	// new data resets the axes
	void decorate(int majors, int minors)
	{
		majors_ = majors;
		minors_ = minors;
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

	int majors_, minors_;
};

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

static App* app;
static Window* win;
static MenuBar* bar;
static Plot* plot;
static const char* shotbase;
static const char* mapname;
static FILE* logfile;
static int shots, dumps;

static void updateTitle();

enum { SIDEWIDTH = 120, BOTTOMHEIGHT = 58, TOOLHEIGHT = 34, MAXRES = 70, TOOLSIZE = 27 };

static Control* orthobox;
static Control* legendbox;
static Control* autoscalebox;
static Control* mousebox;
static Control* shadingbox;
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

static Control* formatlist;
static Control* functionlist;
static Control* surfacelist;
static Control* moviebutton;
static Control* coordbuttons[3];
static Control* gridbuttons[6];
static Control* stylebuttons[6];
static Control* floorbuttons[3];
static Control* normalsbutton;
static MenuItem* animationitem;
static Timer* animationtimer;

static Window* lightwin;
static LightPlot* lightplot;
static Control* distbar;
static Control* emissbar;
static Control* diffbar;
static Control* specbar;
static Control* shinbar;

static char* formats[] = {(char*)"png", (char*)"gif", (char*)"EPS", (char*)"PS", (char*)"PDF", NULL};
static char* functions[] = {(char*)"---", (char*)"Hat", (char*)"Rosenbrock", (char*)"Saddle",
	(char*)"Sombrero", (char*)"Ripple", NULL};
static char* surfaces[] = {(char*)"---", (char*)"Torus", (char*)"Seashell", (char*)"Boy",
	(char*)"Dini", (char*)"Cone", NULL};

static void note(const char* what)
{
	if (logfile)
	{
		fprintf(logfile, "did %s\n", what);
		fflush(logfile);
	}
}

static int topHeight()
{
	return bar->ctrl->area.height + TOOLHEIGHT;
}

static void placeControls(Rect r)
{
	int top = topHeight();
	int plotheight = r.height - top - BOTTOMHEIGHT;
	int side = r.width - SIDEWIDTH + 8;
	plot->setGeometry(0, top, r.width - SIDEWIDTH, plotheight);

	Control* boxes[5] = {orthobox, legendbox, autoscalebox, mousebox, shadingbox};
	for (int i = 0; i != 5; ++i)
		set_control_area(boxes[i], rect(side, top + 8 + 24 * i, SIDEWIDTH - 12, 22));
	set_control_area(normalslabel, rect(side, top + 8 + 24 * 5, SIDEWIDTH - 12, 20));
	int barstop = top + 8 + 24 * 6;
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

static void quit(Window*)
{
	delete plot;
	exit(0);
}

static void menuQuit(MenuItem*)
{
	quit(win);
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

// toolbar: exclusive groups

static void pick(Control** group, int n, Control* chosen)
{
	for (int i = 0; i != n; ++i)
	{
		if (group[i] == chosen)
			check(group[i]);
		else
			uncheck(group[i]);
	}
}

static int indexIn(Control** group, int n, Control* c)
{
	for (int i = 0; i != n; ++i)
		if (group[i] == c)
			return i;
	return 0;
}

static void pickCoord(Control* c)
{
	pick(coordbuttons, 3, c);
	int i = indexIn(coordbuttons, 3, c);
	plot->setCoordinateStyle(i == 0 ? Qwt3D::BOX : i == 1 ? Qwt3D::FRAME : Qwt3D::NOCOORD);
	for (int k = 0; k != 6; ++k)
	{
		if (i == 2)
			disable(gridbuttons[k]);
		else
			enable(gridbuttons[k]);
	}
	note("coordinate style");
}

static void pickGrid(Control* c)
{
	static const Qwt3D::SIDE sides[6] = {Qwt3D::FRONT, Qwt3D::BACK, Qwt3D::RIGHT, Qwt3D::LEFT,
		Qwt3D::CEIL, Qwt3D::FLOOR};
	int side = sides[indexIn(gridbuttons, 6, c)];
	int sum = plot->coordinates()->grids();
	if (is_checked(c))
		sum |= side;
	else
		sum &= ~side;
	plot->coordinates()->setGridLines(sum != Qwt3D::NOSIDEGRID, sum != Qwt3D::NOSIDEGRID, sum);
	plot->updateGL();
	note("grid side");
}

static void pickStyle(Control* c)
{
	static const Qwt3D::PLOTSTYLE styles[6] = {Qwt3D::POINTS, Qwt3D::WIREFRAME, Qwt3D::HIDDENLINE,
		Qwt3D::FILLED, Qwt3D::FILLEDMESH, Qwt3D::NOPLOT};
	pick(stylebuttons, 6, c);
	plot->setPlotStyle(styles[indexIn(stylebuttons, 6, c)]);
	plot->updateData();
	plot->updateGL();
	note("plot style");
}

static void pickFloor(Control* c)
{
	static const Qwt3D::FLOORSTYLE styles[3] = {Qwt3D::FLOORDATA, Qwt3D::FLOORISO, Qwt3D::NOFLOOR};
	pick(floorbuttons, 3, c);
	plot->setFloorStyle(styles[indexIn(floorbuttons, 3, c)]);
	plot->updateData();
	plot->updateGL();
	note("floor style");
}

static void toggleNormals(Control* c)
{
	plot->showNormals(is_checked(c) != 0);
	plot->updateNormals();
	plot->updateGL();
	note("normals");
}

static void openFile(Control*)
{
	std::string name = plot->askOpenFile("Open surface", "QwtPlot3D grids|*.mes");
	if (name.empty())
		return;
	pickStyle(stylebuttons[3]); // the mesh lines of a dense grid would be solid black
	if (!plot->showFile(name.c_str()))
		plot->tellUser("Open surface", "That is not a complete .mes file.");
	updateTitle();
}

static void openFem(Control*)
{
	std::string nodes = plot->askOpenFile("FEM nodes", "Nodes|*.nod");
	if (nodes.empty())
		return;
	std::string cells = plot->askOpenFile("FEM connectivities", "Connectivities|*.cel");
	if (cells.empty())
		return;
	if (!plot->showFem(nodes.c_str(), cells.c_str()))
		plot->tellUser("Open FEM file", "Those files could not be read.");
	updateTitle();
}

// the save button: dump_N in the current folder, in the format of the list beside it
static void dumpImage(Control*)
{
	const char* ext = formats[get_control_value(formatlist)];
	std::string format(ext), lower(ext);
	for (size_t i = 0; i != format.size(); ++i)
	{
		format[i] = (char)toupper((unsigned char)format[i]);
		lower[i] = (char)tolower((unsigned char)lower[i]);
	}
	char name[64];
	sprintf(name, "dump_%d.%s", dumps++, lower.c_str());
	plot->save(name, format);
	note("save button");
}

static void animate(Timer*)
{
	plot->setRotation(int(plot->xRotation() + 1) % 360, int(plot->yRotation() + 1) % 360,
		int(plot->zRotation() + 1) % 360);
}

static void setAnimation(bool on)
{
	if (on && !animationtimer)
		animationtimer = new_timer(app, animate, 50);
	else if (!on && animationtimer)
	{
		del_timer(animationtimer);
		animationtimer = NULL;
	}
	if (on)
	{
		check_menu_item(animationitem);
		check(moviebutton);
	}
	else
	{
		uncheck_menu_item(animationitem);
		uncheck(moviebutton);
	}
	note("animation");
}

static void movieClicked(Control* c)
{
	setAnimation(is_checked(c) != 0);
}

static void functionPicked(Control* c)
{
	long i = get_control_value(c);
	if (i > 0)
		plot->showFunction(functions[i]);
	updateTitle();
	note("function");
}

static void surfacePicked(Control* c)
{
	long i = get_control_value(c);
	if (i > 0)
		plot->showSurface(surfaces[i]);
	updateTitle();
	note("surface");
}

// menus

static void menuOpen(MenuItem*)
{
	openFile(NULL);
}

static void menuFem(MenuItem*)
{
	openFem(NULL);
}

static void menuAnimation(MenuItem*)
{
	setAnimation(!animationtimer);
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

static bool askColor(const char* title, Qwt3D::RGBA& col)
{
	unsigned char rgb[3];
	if (!plot->askColor(title, rgb))
		return false;
	col = Qwt3D::RGBA(rgb[0] / 255., rgb[1] / 255., rgb[2] / 255., 1);
	return true;
}

static void pickAxesColor(MenuItem*)
{
	Qwt3D::RGBA c;
	if (!askColor("Axes", c))
		return;
	plot->coordinates()->setAxesColor(c);
	plot->updateGL();
}

static void pickBackground(MenuItem*)
{
	Qwt3D::RGBA c;
	if (!askColor("Background", c))
		return;
	plot->setBackgroundColor(c);
	plot->updateGL();
}

static void pickMeshColor(MenuItem*)
{
	Qwt3D::RGBA c;
	if (!askColor("Mesh", c))
		return;
	plot->setMeshColor(c);
	plot->updateData();
	plot->updateGL();
}

static void pickNumberColor(MenuItem*)
{
	Qwt3D::RGBA c;
	if (!askColor("Numbers", c))
		return;
	plot->coordinates()->setNumberColor(c);
	plot->updateGL();
}

static void pickLabelColor(MenuItem*)
{
	Qwt3D::RGBA c;
	if (!askColor("Label", c))
		return;
	plot->coordinates()->setLabelColor(c);
	plot->updateGL();
}

static void pickCaptionColor(MenuItem*)
{
	Qwt3D::RGBA c;
	if (!askColor("Caption", c))
		return;
	plot->setTitleColor(c);
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

static void pickNumberFont(MenuItem*)
{
	Qwt3D::Font font("Courier", 12);
	if (!plot->askFont(font))
		return;
	plot->coordinates()->setNumberFont(font);
	plot->updateGL();
}

static void pickLabelFont(MenuItem*)
{
	Qwt3D::Font font("Courier", 14, Qwt3D::Font::Bold);
	if (!plot->askFont(font))
		return;
	plot->coordinates()->setLabelFont(font);
	plot->updateGL();
}

static void pickCaptionFont(MenuItem*)
{
	Qwt3D::Font font("Arial", 14);
	if (!plot->askFont(font))
		return;
	plot->setTitleFont(font.family(), font.pointSize(), font.weight(), font.italic());
	plot->updateGL();
}

static void resetFonts(MenuItem*)
{
	plot->coordinates()->setNumberFont(Qwt3D::Font("Courier", 12));
	plot->coordinates()->setLabelFont(Qwt3D::Font("Courier", 14, Qwt3D::Font::Bold));
	plot->setTitleFont("Arial", 14);
	plot->updateGL();
}

// the check boxes and sliders

static void orthoChanged(Control* c)
{
	plot->setOrtho(is_checked(c) != 0);
	note("ortho");
}

static void legendChanged(Control* c)
{
	plot->legendShown = is_checked(c) != 0;
	plot->redrawLegend(plot->legendShown);
	note("legend");
}

static void autoscaleChanged(Control* c)
{
	plot->coordinates()->setAutoScale(is_checked(c) != 0);
	plot->updateGL();
	note("autoscale");
}

static void mouseChanged(Control* c)
{
	plot->enableMouse(is_checked(c) != 0);
	note("mouse");
}

static void shadingChanged(Control* c)
{
	plot->setShading(is_checked(c) ? Qwt3D::GOURAUD : Qwt3D::FLAT);
	plot->updateData();
	plot->updateGL();
	note("shading");
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
	note("std");
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
	note("lighting");
}

static void offsetChanged(Control* c)
{
	plot->setPolygonOffset(get_control_value(c) / 10.0);
	plot->updateData();
	plot->updateGL();
	note("polygon offset");
}

static void resolutionChanged(Control* c)
{
	plot->setResolution(1 + get_control_value(c));
	note("resolution");
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

static Image* iconImage(const unsigned char* rgb, int w, int h)
{
	Image* img = new_image(w, h, 32);
	for (int y = 0; y != h; ++y)
		for (int x = 0; x != w; ++x, rgb += 3)
			img->data32[y][x] = app_new_rgb(rgb[0], rgb[1], rgb[2]);
	return img;
}

static int toolx = 4;

static void listButton(const char* name, Rect r)
{
	if (logfile)
		fprintf(logfile, "button %s %d %d\n", name, r.x + r.width / 2, r.y + r.height / 2);
}

static Rect toolRect(const char* name)
{
	Rect r = rect(toolx, bar->ctrl->area.height + 3, TOOLSIZE, TOOLSIZE);
	toolx += TOOLSIZE;
	listButton(name, r);
	return r;
}

#define ICON(n) iconImage(icon_##n, icon_##n##_w, icon_##n##_h)
#define PUSH(n, fn) new_image_button(win, toolRect(#n), ICON(n), fn)
#define TOGGLE(n, fn) new_image_check_box(win, toolRect(#n), ICON(n), fn)

static Control* dropList(int width, char** lines, ControlFunc fn)
{
	Control* c = new_drop_list(win, rect(toolx, bar->ctrl->area.height + 5, width, 24), lines, fn);
	toolx += width + 4;
	return c;
}

static void buildToolbar()
{
	PUSH(fileopen, openFile);
	PUSH(filecell, openFem);
	PUSH(savecontent, dumpImage);
	toolx += 2;
	formatlist = dropList(66, formats, NULL);
	functionlist = dropList(96, functions, functionPicked);
	surfacelist = dropList(96, surfaces, surfacePicked);
	moviebutton = TOGGLE(movie, movieClicked);
	toolx += 8;
	coordbuttons[0] = TOGGLE(box, pickCoord);
	coordbuttons[1] = TOGGLE(frame, pickCoord);
	coordbuttons[2] = TOGGLE(grid, pickCoord);
	toolx += 6;
	gridbuttons[0] = TOGGLE(gridfr, pickGrid);
	gridbuttons[1] = TOGGLE(gridb, pickGrid);
	gridbuttons[2] = TOGGLE(gridr, pickGrid);
	gridbuttons[3] = TOGGLE(gridl, pickGrid);
	gridbuttons[4] = TOGGLE(gridc, pickGrid);
	gridbuttons[5] = TOGGLE(gridf, pickGrid);
	toolx += 6;
	stylebuttons[0] = TOGGLE(scattered, pickStyle);
	stylebuttons[1] = TOGGLE(wireframe, pickStyle);
	stylebuttons[2] = TOGGLE(hiddenline, pickStyle);
	stylebuttons[3] = TOGGLE(polygon, pickStyle);
	stylebuttons[4] = TOGGLE(filledmesh, pickStyle);
	stylebuttons[5] = TOGGLE(nodata, pickStyle);
	toolx += 6;
	floorbuttons[0] = TOGGLE(floordata, pickFloor);
	floorbuttons[1] = TOGGLE(flooriso, pickFloor);
	floorbuttons[2] = TOGGLE(floorempty, pickFloor);
	toolx += 6;
	normalsbutton = TOGGLE(normals, toggleNormals);

	check(coordbuttons[0]);
	check(stylebuttons[4]);
	check(floorbuttons[2]);
}

static void buildControls()
{
	orthobox = new_check_box(win, rect(0, 0, 10, 10), "Ortho", orthoChanged);
	legendbox = new_check_box(win, rect(0, 0, 10, 10), "Legend", legendChanged);
	autoscalebox = new_check_box(win, rect(0, 0, 10, 10), "Autoscale", autoscaleChanged);
	mousebox = new_check_box(win, rect(0, 0, 10, 10), "Mouse", mouseChanged);
	shadingbox = new_check_box(win, rect(0, 0, 10, 10), "Shading", shadingChanged);
	check(orthobox);
	check(autoscalebox);
	check(mousebox);
	check(shadingbox);
	normalslabel = new_label(win, rect(0, 0, 10, 10), "Normals", ALIGN_LEFT);
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

static void buildMenus()
{
	Menu* m;

	bar = new_menu_bar(win);

	m = new_menu(bar, "File");
	new_menu_item(m, "Open File", 'O', menuOpen);
	new_menu_item(m, "Open FEM File", 0, menuFem);
	new_menu_item(m, "Save As...", 0, saveAs);
	animationitem = new_menu_item(m, "Animation", 0, menuAnimation);
	new_menu_item(m, "-", 0, NULL);
	new_menu_item(m, "Exit", 'Q', menuQuit);

	m = new_menu(bar, "Color");
	new_menu_item(m, "Axes", 0, pickAxesColor);
	new_menu_item(m, "Background", 0, pickBackground);
	new_menu_item(m, "Mesh", 0, pickMeshColor);
	new_menu_item(m, "Numbers", 0, pickNumberColor);
	new_menu_item(m, "Label", 0, pickLabelColor);
	new_menu_item(m, "Caption", 0, pickCaptionColor);
	new_menu_item(m, "Data color", 'M', openColorMap);
	new_menu_item(m, "Reset", 0, resetColors);

	m = new_menu(bar, "Fonts");
	new_menu_item(m, "Scale numbering", 0, pickNumberFont);
	new_menu_item(m, "Axes label", 0, pickLabelFont);
	new_menu_item(m, "Caption", 0, pickCaptionFont);
	new_menu_item(m, "Reset", 0, resetFonts);
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
		case 'n': clickBox(normalsbutton, toggleNormals); break;
		case 'b': clickBox(autoscalebox, autoscaleChanged); break;
		case 'u': moveBar(offsetbar, 20, 30, offsetChanged); break;
		case 'L': pickLighting(NULL); break;
		case 'D': moveBar(distbar, 300, 1000, lightDistance); break;
		case 'S': moveBar(specbar, 80, 100, lightSpecular); break;
		case 'E': moveBar(emissbar, 20, 100, lightEmission); break;
		case 'R': lightplot->setRotation(60, 0, 120); break;
		case 'r': plot->showFunction("Ripple"); break;
		case 'v': plot->resetView(); break;
		case 'o': openFile(NULL); break;
		case 'N': openFem(NULL); break;
		case 'M': openColorMap(NULL); break;
		case 'k': pickBackground(NULL); break;
		case 'K': pickNumberFont(NULL); break;
		case 'q': resetColors(NULL); break;
		case 'Q': resetFonts(NULL); break;
		case 'a': moveBar(resbar, 0, MAXRES - 1, resolutionChanged); break;
		case 'c': moveBar(resbar, 2, MAXRES - 1, resolutionChanged); break;
		case 'm':
			if (mapname)
				plot->setColorMap(mapname);
			break;
		case '1': case '2': case '3': case '4': case '5':
						plot->showFunction(functions[key - '0']);
			break;
		case '6': case '7': case '8': case '9': case '0':
			plot->showSurface(surfaces[key == '0' ? 5 : key - '5']);
			break;
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

	win = new_window(app, rect(60, 60, 1020, 760), "QwtPlot3D in GraphApp", STANDARD_WINDOW);
	set_window_background(win, rgb(240, 240, 240));
	on_window_close(win, quit);
	on_window_resize(win, resized);
	on_window_key_down(win, testKey);
	buildMenus();
	buildToolbar();
	buildControls();

	Rect r = get_window_area(win);
	plot = new Plot(win, 0, topHeight(), 100, 100);
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
		pickStyle(stylebuttons[3]); // the mesh lines of a dense grid would be solid black
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
		plot->showFunction("Rosenbrock");
	if (mapname)
		plot->setColorMap(mapname);
	updateTitle();

	show_window(win);
	plot->show();
	plot->setFocus();
	main_loop(app);
	return 0;
}
