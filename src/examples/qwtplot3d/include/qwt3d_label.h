#ifndef __LABELPIXMAP_H__
#define __LABELPIXMAP_H__

#include "qwt3d_drawable.h"
#include "qwt3d_glwidget.h"

namespace Qwt3D
{

//! A bitmap string or an output device dependent string
class QWT3D_EXPORT Label : public Drawable 
{
		
public:

  Label();
 	//! Construct label and initialize with font 
	Label(const std::string & family, int pointSize, int weight = Font::Normal, bool italic = false);
	
	//! Sets the labels font
	void setFont(std::string const& family, int pointSize, int weight = Font::Normal, bool italic = false);

	void adjust(int gap); //!< Fine tunes label;
	double gap() const {return gap_;} //!< Returns the gap caused by adjust();
	void setPosition(Qwt3D::Triple pos, ANCHOR a = BottomLeft); //!< Sets the labels position
	void setRelPosition(Tuple rpos, ANCHOR a); //!< Sets the labels position relative to screen
	Qwt3D::Triple first() const { return beg_;} //!< Receives bottom left label position
	Qwt3D::Triple second() const { return end_;} //!< Receives top right label position
	ANCHOR anchor() const { return anchor_; } //!< Defines an anchor point for the labels surrounding rectangle
	virtual void setColor(double r, double g, double b, double a = 1);	
	virtual void setColor(Qwt3D::RGBA rgba);	

	/*!
	\brief Sets the labels string
	Strings are UTF-8. For unicode labeling please look at <a href="http://www.unicode.org/charts/">www.unicode.org</a>.
	*/
	void setString(std::string const& s);
	void draw(); //!< Actual drawing

	/**
		\brief Decides about use of PDF standard fonts for PDF output 
		If true, Label can use one of the PDF standard fonts (unprecise positioning for now), 
		otherwise it dumps  pixmaps in the PDF stream (poor quality) 
	*/
	static void useDeviceFonts(bool val); 
	

private:

	Qwt3D::Triple beg_, end_, pos_;
	std::vector<unsigned char> tex_; // RGBA, bottom row first
	int texwidth_, texheight_;
	Font font_;
	std::string text_;

	ANCHOR anchor_;
	
	void init();
  void init(const std::string & family, int pointSize, int weight = Font::Normal, bool italic = false);
	void update(); //!< Enforces an update of the internal pixmap
	void convert2screen();
	double width() const;
	double height() const;

	int gap_;

	bool flagforupdate_;

	static bool devicefonts_;

};

} // ns

#endif
