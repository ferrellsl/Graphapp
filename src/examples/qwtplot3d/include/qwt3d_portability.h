#ifndef qwt3d_portability_h__2005_07_02_11_55_begin_guarded_code
#define qwt3d_portability_h__2005_07_02_11_55_begin_guarded_code

//! The small value types that stand in for Qt's

#include <stdio.h>
#include <string>
#include <vector>
#include "qwt3d_global.h"

namespace Qwt3D
{

//! Mouse buttons, combined with |
enum MouseButton
{
  NoButton    = 0,
  LeftButton  = 1,
  RightButton = 2,
  MidButton   = 4
};

//! Keyboard modifiers, combined with |
enum KeyboardModifier
{
  NoModifier      = 0,
  ShiftModifier   = 1,
  ControlModifier = 2,
  AltModifier     = 4
};

//! The keys the default keyboard bindings use
enum Key
{
  Key_unknown = 0,
  Key_Left = 0x1000,
  Key_Right,
  Key_Up,
  Key_Down
};

//! This class creates a (mouse-button,modifier) pair
class MouseState
{
public:
  MouseState(int mb = NoButton, int km = NoModifier)
    : mb_(mb), km_(km)
  {
  }

  bool operator==(const MouseState& ms) const
  {
    return mb_ == ms.mb_ && km_ == ms.km_;
  }

  bool operator!=(const MouseState& ms) const
  {
    return !operator==(ms);
  }

private:
  int mb_;
  int km_;
};

//! This class creates a (key-button,modifier) pair
class KeyboardState
{
public:
  KeyboardState(int key = Key_unknown, int km = NoModifier)
    : key_(key), km_(km)
  {
  }

  bool operator==(const KeyboardState& ms) const
  {
    return key_ == ms.key_ && km_ == ms.km_;
  }

  bool operator!=(const KeyboardState& ms) const
  {
    return !operator==(ms);
  }

private:
  int key_;
  int km_;
};

//! A font request (replaces QFont; weights have Qt's values)
struct Font
{
  enum Weight { Normal = 50, Bold = 75 };

  Font() : family_("Courier"), pointSize_(10), weight_(Normal), italic_(false) {}
  Font(std::string const& family, int pointSize, int weight = Normal, bool italic = false)
    : family_(family), pointSize_(pointSize), weight_(weight), italic_(italic) {}

  std::string const& family() const {return family_;}
  int pointSize() const {return pointSize_;}
  int weight() const {return weight_;}
  bool italic() const {return italic_;}

  bool operator==(Font const& f) const
  {
    return pointSize_ == f.pointSize_ && weight_ == f.weight_
      && italic_ == f.italic_ && family_ == f.family_;
  }
  bool operator!=(Font const& f) const {return !operator==(f);}

private:
  std::string family_;
  int pointSize_;
  int weight_;
  bool italic_;
};

//! A position in the plot window, in pixels
struct PixelPoint
{
  PixelPoint(int xv = 0, int yv = 0) : x(xv), y(yv) {}
  int x, y;
};

//! Number to string the way QString::number(double) did it
inline std::string toString(double val)
{
  char buf[64];
  sprintf(buf, "%g", val);
  return buf;
}

} // ns

#endif
