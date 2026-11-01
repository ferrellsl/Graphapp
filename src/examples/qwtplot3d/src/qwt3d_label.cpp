#include "qwt3d_label.h"

using namespace Qwt3D;

bool Label::devicefonts_ = false;

Label::Label()
{
	init();
}

Label::Label(const std::string & family, int pointSize, int weight, bool italic)
{
	init(family, pointSize, weight, italic);
}


void Label::init(const std::string & family, int pointSize, int weight, bool italic)
{
	init();
	font_ = Font(family, pointSize, weight, italic );
}

void Label::init()
{
	beg_ = Triple(0.0, 0.0, 0.0);
	end_ = beg_;
	pos_ = beg_;
	setColor(0,0,0);
	texwidth_ = texheight_ = 0;
	font_ = Font();
	anchor_ = BottomLeft;
	gap_ = 0;
	flagforupdate_ = true;
}

void Label::useDeviceFonts(bool val)
{
	devicefonts_ = val;
}

void Label::setFont(const std::string & family, int pointSize, int weight, bool italic)
{
	Font font(family, pointSize, weight, italic);
	if (font == font_)
		return;
	font_ = font;
	flagforupdate_ = true;
}

void Label::setString(std::string const& s)
{
	if (s == text_)
		return;
	text_ = s;
	flagforupdate_ = true;
}

void Label::setColor(double r, double g, double b, double a)
{
	if (r == color.r && g == color.g && b == color.b && a == color.a)
		return;
	Drawable::setColor(r,g,b,a);
	flagforupdate_ = true;
}

void Label::setColor(Qwt3D::RGBA rgba)
{
	setColor(rgba.r, rgba.g, rgba.b, rgba.a);
}

/**
example:

\verbatim

   Anchor TopCenter (*)  resp. BottomRight(X) 

   +----*----+
   |  Pixmap |
   +---------X

\endverbatim
*/
void Label::setPosition(Triple pos, ANCHOR a)
{
	anchor_ = a;
	pos_ = pos;
}

void Label::setRelPosition(Tuple rpos, ANCHOR a)
{
	double ot = 0.99;

	getMatrices(modelMatrix, projMatrix, viewport);
	beg_ = relativePosition(Triple(rpos.x, rpos.y, ot));
	setPosition(beg_, a);	
}

void Label::update()
{
	if (!GraphAppHost::renderText(text_, font_, color.r, color.g, color.b, tex_, texwidth_, texheight_))
		texwidth_ = texheight_ = 0;
}

/**
Adds an additional shift to the anchor point. This happens in a more or less intelligent manner
depending on the nature of the anchor:
\verbatim
anchor type         shift

left aligned         -->
right aligned        <--
top aligned          top-down            
bottom aligned       bottom-up
\endverbatim
The unit is user space dependend (one pixel on screen - play around to get satisfying results)
*/
void Label::adjust(int gap)
{
	gap_ = gap;
}

void Label::convert2screen()
{
	Triple start = World2ViewPort(pos_);
	
	switch (anchor_)
	{
		case BottomLeft :
			beg_ = pos_;
			break;
		case BottomRight:
			beg_ = ViewPort2World(start - Triple(width() + gap_, 0, 0));
			break;
		case BottomCenter:
			beg_ = ViewPort2World(start - Triple(width() / 2, -gap_, 0));
			break;
		case TopRight:
			beg_ = ViewPort2World(start - Triple(width() + gap_, height(), 0));
			break;
		case TopLeft:
			beg_ = ViewPort2World(start - Triple(-gap_, height(), 0));
			break;
		case TopCenter:
			beg_ = ViewPort2World(start - Triple(width() / 2, height() + gap_, 0));
			break;
		case CenterLeft:
			beg_ = ViewPort2World(start - Triple(-gap_, height() / 2, 0));
			break;
		case CenterRight:
			beg_ = ViewPort2World(start - Triple(width() + gap_, height() / 2, 0));
			break;
		case Center:
			beg_ = ViewPort2World(start - Triple(width() / 2, height() / 2, 0));
			break;
		default:
			break;
	}
	start = World2ViewPort(beg_);
	end_ = ViewPort2World(start + Triple(width(), height(), 0));	
}

void Label::draw()
{
	if (flagforupdate_)
	{
		update();
		flagforupdate_ = false;
	}

	if (tex_.empty())
		return;
		
	GLboolean b;
	GLint func;
	GLdouble v;
	glGetBooleanv(GL_ALPHA_TEST, &b);
	glGetIntegerv(GL_ALPHA_TEST_FUNC, &func);
	glGetDoublev(GL_ALPHA_TEST_REF, &v);
	
	GLint blendsrc, blenddst;
	glGetIntegerv(GL_BLEND_SRC, &blendsrc);
	glGetIntegerv(GL_BLEND_DST, &blenddst);
	GLStateBewarer blend(GL_BLEND, true);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // the edges of the text are partly covered

	glEnable (GL_ALPHA_TEST);
  glAlphaFunc (GL_NOTEQUAL, 0.0);
	
	convert2screen();
	glRasterPos3d(beg_.x, beg_.y, beg_.z);
 
	
 
	if (devicefonts_)
	{		
		drawDeviceText(text_.c_str(), "Courier", font_.pointSize(), pos_, color, anchor_, gap_);
	}
	else
	{
		drawDevicePixels(texwidth_, texheight_, GL_RGBA, GL_UNSIGNED_BYTE, &tex_[0]);
	}


	glBlendFunc(blendsrc, blenddst);
	glAlphaFunc(func,v);
	Enable(GL_ALPHA_TEST, b);
}


double Label::width() const 
{ 
	return texwidth_; 
}

double Label::height() const 
{ 
	return texheight_; 
}	
