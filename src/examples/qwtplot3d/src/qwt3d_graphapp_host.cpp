// Everything the library asks of GraphApp: the native window, text and image files.
// The only file that sees graphapp.h. Its macros and global names (Window, Font, Color)
// must stay out of the library's headers and sources.

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "qwt3d_glwidget.h"

extern "C" {
#include "graphapp.h"
}

namespace
{
  App* host = 0;
  Window* hostwin = 0;

  // GDI's "Courier" is a bitmap font with three sizes
  const char* faceFor(std::string const& family)
  {
    return (family == "Courier") ? "Courier New" : family.c_str();
  }

  // the size Qt gave a point size on a 96 dpi screen
  int pixelHeight(int pointSize)
  {
    return (pointSize * 96 + 36) / 72;
  }

  // UTF-8 lead bytes from 0xC4 up start characters above U+00FF
  bool beyondLatin1(std::string const& text)
  {
    for (size_t i = 0; i != text.size(); ++i)
      if ((unsigned char)text[i] >= 0xC4)
        return true;
    return false;
  }
}

void* Qwt3D::GraphAppHost::nativeWindow(::Window* w)
{
  host = w->app;
  if (!hostwin)
    hostwin = w; // any window will do to make bitmaps with
  // no public accessor: the HWND is the first member of the private WindowExtra
  return *(void**)w->extra;
}

std::string Qwt3D::GraphAppHost::askFile(std::string const& title, std::string const& filter,
                                         std::string const& name, bool save)
{
  // the system's file dialog, which GraphApp gives no filter: the title says what to pick
  std::string heading = title;
  size_t bar = filter.find('|');
  if (bar != std::string::npos)
  {
    std::string patterns = filter.substr(bar + 1);
    for (size_t semi; (semi = patterns.find(';')) != std::string::npos;)
      patterns.replace(semi, 1, ", ");
    heading += " (" + patterns + ")";
  }
  char* picked = save ? ask_file_save(host, heading.c_str(), "Save", name.c_str())
                      : ask_file_open(host, heading.c_str(), "Open", name.c_str());
  std::string result = picked ? picked : "";
  if (picked)
    del_string(picked);
  return result;
}

void Qwt3D::GraphAppHost::tell(std::string const& title, std::string const& message)
{
  ask_ok(host, title.c_str(), message.c_str());
}

std::string Qwt3D::GraphAppHost::askString(std::string const& title, std::string const& question,
                                           std::string const& answer)
{
  char* typed = ask_string(host, title.c_str(), question.c_str(), answer.c_str());
  std::string result = typed ? typed : "";
  if (typed)
    del_string(typed);
  return result;
}

bool Qwt3D::GraphAppHost::inMenuBar(::Window* w, int x, int y)
{
  if (!w->menubar)
    return false;
  Rect r = w->menubar->ctrl->area;
  return x >= r.x && y >= r.y && x < r.x + r.width && y < r.y + r.height;
}

bool Qwt3D::GraphAppHost::renderText(std::string const& text, Qwt3D::Font const& font,
                                     double r, double g, double b,
                                     std::vector<unsigned char>& rgba, int& width, int& height)
{
  rgba.clear();
  width = height = 0;
  if (!host)
    return false;

  bool bold = font.weight() >= Qwt3D::Font::Bold;
  int pixels = pixelHeight(font.pointSize());
#ifdef __APPLE__
  // the plot is drawn in backing pixels, twice the points on a Retina display
  pixels = (int)(pixels * Qwt3D::GraphAppHost::backingScale() + 0.5);
#endif
  // (if that fails, as on an X server without Unicode fonts, wrong letters beat no label)
  if (beyondLatin1(text) && renderUnicodeText(text, faceFor(font.family()), pixels,
                                              bold, font.italic(), r, g, b, rgba, width, height))
    return true;

  int style = NATIVE_FONT;
  if (bold)
    style |= BOLD;
  if (font.italic())
    style |= ITALIC;

  ::Font* f = new_font(host, faceFor(font.family()), style, pixels);
  if (!f)
    return false;

  const char* s = text.empty() ? " " : text.c_str();
  int n = (int)strlen(s);
  width = font_width(f, s, n);
  height = font_height(f);
  if (width < 1 || height < 1)
  {
    del_font(f);
    return false;
  }

  Bitmap* bmp = new_bitmap(hostwin, width, height);
  Graphics* gr = get_bitmap_graphics(bmp);
  set_rgb(gr, rgb(0, 0, 0));
  fill_rect(gr, rect(0, 0, width, height));
  set_rgb(gr, rgb(255, 255, 255));
  set_font(gr, f);
  draw_text(gr, rect(0, 0, width, height), ALIGN_LEFT | VALIGN_TOP, s, n);
  del_graphics(gr);
  Image* img = bitmap_to_image(bmp);
  del_bitmap(bmp);
  del_font(f);

  rgba.resize(width * height * 4);
  unsigned char red = (unsigned char)(r * 255 + 0.5);
  unsigned char green = (unsigned char)(g * 255 + 0.5);
  unsigned char blue = (unsigned char)(b * 255 + 0.5);
  for (int y = 0; y != height; ++y)
  {
    unsigned char* p = &rgba[(height - 1 - y) * width * 4];
    for (int x = 0; x != width; ++x, p += 4)
    {
      Colour c = img->data32[y][x];
      p[0] = red;
      p[1] = green;
      p[2] = blue;
      p[3] = (unsigned char)((c.red + c.green + c.blue) / 3);
    }
  }
  del_image(img);
  return true;
}

bool Qwt3D::GraphAppHost::writeImage(std::vector<unsigned char> const& rgba, int width, int height,
                                     std::string const& fname, std::string const& fmt)
{
  // GraphApp picks the format from the file name, which here may say something else
  std::string ext = ".";
  for (size_t i = 0; i != fmt.size(); ++i)
    ext += (char)tolower((unsigned char)fmt[i]);
  std::string tmp = fname + ".tmp" + ext;

  Image* img = new_image(width, height, 32);
  if (!img)
    return false;
  for (int y = 0; y != height; ++y)
  {
    unsigned char const* p = &rgba[(height - 1 - y) * width * 4];
    for (int x = 0; x != width; ++x, p += 4)
      img->data32[y][x] = rgb(p[0], p[1], p[2]);
  }
  bool ok = write_image(img, tmp.c_str()) != 0;
  del_image(img);
  if (ok)
  {
    remove(fname.c_str());
    ok = rename(tmp.c_str(), fname.c_str()) == 0;
  }
  return ok;
}

std::vector<std::string> Qwt3D::GraphAppHost::imageFormats()
{
  std::vector<std::string> list;
  // app_save_jpeg is a stub in GraphApp: it always fails
  list.push_back("PNG");
  list.push_back("GIF");
  return list;
}
