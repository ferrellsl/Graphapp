#ifndef qwt3d_glwidget_h__2026_11_01_begin_guarded_code
#define qwt3d_glwidget_h__2026_11_01_begin_guarded_code

#include <vector>
#include "qwt3d_global.h"
#include "qwt3d_portability.h"

struct Window; // GraphApp's

namespace Qwt3D
{

//! An OpenGL drawing area inside a GraphApp window (replaces QGLWidget)
/*!
  GraphApp has no OpenGL support and paints over its whole window, so the area is a
  child window of its own with an OpenGL context. Like a QGLWidget it starts hidden and
  draws nothing until show() is called. The parent window does not resize it: call
  setGeometry() from the on_window_resize handler.
*/
class QWT3D_EXPORT GLWidget
{
public:
  GLWidget(Window* parent, int x, int y, int width, int height);
  virtual ~GLWidget();

  void show();
  void hide();
  bool isVisible() const {return visible_;}
  void setGeometry(int x, int y, int width, int height);
  int width() const {return width_;}
  int height() const {return height_;}
  void setFocus();

  //! The system's file dialogs and GraphApp's messages, through GraphApp
  /*! \param filter is "Description|*.ext;*.ext2"; the patterns go in the dialog's title, as
      GraphApp's dialogs have no filter. An empty result means Cancel. */
  std::string askOpenFile(std::string const& title, std::string const& filter);
  std::string askSaveFile(std::string const& title, std::string const& filter, std::string const& name);
  void tellUser(std::string const& title, std::string const& message);
  bool askColor(std::string const& title, unsigned char rgb[3]);
  bool askFont(Font& font);

  void makeCurrent(); //!< Makes this area's OpenGL context the current one
  void updateGL(); //!< Redraws at once (no effect while hidden)

  //! Draws a frame and returns it as RGBA rows, bottom row first
  bool grabFrameBuffer(std::vector<unsigned char>& rgba, int& width, int& height);

protected:
  virtual void initializeGL() {}
  virtual void paintGL() {}
  virtual void resizeGL(int, int) {}

  virtual void mousePressEvent(PixelPoint) {}
  virtual void mouseReleaseEvent(PixelPoint) {}
  virtual void mouseMoveEvent(PixelPoint, MouseState) {}
  virtual void wheelEvent(int, int) {} //!< delta (120 per notch), modifiers
  virtual void keyPressEvent(KeyboardState) {}

private:
  friend struct GLWidgetEvents;

  GLWidget(GLWidget const&);
  void operator=(GLWidget const&);
  void prepare();

  void* hwnd_;
  void* hdc_;
  void* hglrc_;
  int width_, height_;
  bool visible_, initialized_, resized_;
};

//! What the library needs from GraphApp besides the window
namespace GraphAppHost
{
  void* nativeWindow(Window* w); //!< Registers the application too
  bool inMenuBar(Window* w, int x, int y); //!< Is this point of the window in its menu bar?
  std::string askFile(std::string const& title, std::string const& filter,
                      std::string const& name, bool save); //!< An empty result means Cancel
  void tell(std::string const& title, std::string const& message);
  std::string askString(std::string const& title, std::string const& question,
                        std::string const& answer); //!< An empty result means Cancel
  //! Renders one line of UTF-8 text as RGBA rows, bottom row first, coverage in alpha
  bool renderText(std::string const& text, Font const& font, double r, double g, double b,
                  std::vector<unsigned char>& rgba, int& width, int& height);
  //! The same for text beyond Latin-1, which GraphApp's native fonts draw as wrong letters
  bool renderUnicodeText(std::string const& text, std::string const& family, int pixelHeight,
                         bool bold, bool italic, double r, double g, double b,
                         std::vector<unsigned char>& rgba, int& width, int& height);
  //! Writes RGBA rows, bottom row first, in the format named by fmt ("PNG" or "GIF")
  bool writeImage(std::vector<unsigned char> const& rgba, int width, int height,
                  std::string const& fname, std::string const& fmt);
  std::vector<std::string> imageFormats();
}

} // ns

#endif
