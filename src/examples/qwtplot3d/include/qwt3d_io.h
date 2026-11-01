#ifndef __qwt3d_io_2003_07_04_23_27__
#define __qwt3d_io_2003_07_04_23_27__

#include <vector>
#include <algorithm>
#include "qwt3d_global.h"

namespace Qwt3D
{

class Plot3D;
/** 
IO provides a generic interface for standard and user written I/O handlers. 
It also provides functionality for the registering of such handlers in the
framework.\n 
The interface mimics roughly the QImageIO functions of Qt for defining  
image input/output functions. 
*/
class QWT3D_EXPORT IO
{

public:
  /*! 
    The function type that can be processed by the define... members.
    An extension is the IO::Functor.
  */
  typedef bool (*Function)(Plot3D*, std::string const& fname);
  
  
  /*! 
    This class gives more flexibility in implementing 
    userdefined IO handlers than the simple IO::Function type. 
  */
  class Functor
  {
  public:
    virtual ~Functor() {}
    /*! Must clone the content of *this for an object of a derived class with 
    \c new and return the pointer. Like operator() the predefined Functors 
    hide this function from the user, still allowing IO access 
    (friend declaration)
    */
    virtual Functor* clone() const = 0;
    /*! The workhorse of the user-defined implementation. Eventually, the 
    framework will call this operator.
    */
    virtual bool operator()(Plot3D* plot, std::string const& fname) = 0;
  };
  
  static bool defineInputHandler( std::string const& format, Function func);
  static bool defineOutputHandler( std::string const& format, Function func);
  static bool defineInputHandler( std::string const& format, Functor const& func);
  static bool defineOutputHandler( std::string const& format, Functor const& func);
  static bool save(Plot3D*, std::string const& fname, std::string const& format);
  static bool load(Plot3D*, std::string const& fname, std::string const& format);
  static std::vector<std::string> inputFormatList();
  static std::vector<std::string> outputFormatList();
  static Functor* outputHandler(std::string const& format);
  static Functor* inputHandler(std::string const& format);
  
private:  
  IO(){}
  
  //! Lightweight Functor encapsulating an IO::Function
  class Wrapper : public Functor
  {
  public:
    //! Performs actual input
    Functor* clone() const { return new Wrapper(*this); }
    //! Creates a Wrapper object from a function pointer
    explicit Wrapper(Function h) : hdl(h) {}
    //! Returns a pointer to the wrapped function
    bool operator()(Plot3D* plot, std::string const& fname)
    {
      return (hdl) ? (*hdl)(plot, fname) : false;
    }
  private: 
    Function hdl;
  };  
  
  struct Entry
  {
    Entry();    
    ~Entry();

    Entry(Entry const& e);
    void operator=(Entry const& e);
    
    Entry(std::string const& s, Functor const& f);
    Entry(std::string const& s, Function f);
    
    std::string fmt;
    Functor* iofunc;
  };

  struct FormatCompare
  {
    explicit FormatCompare(Entry const& e);
    bool operator() (Entry const& e);

    Entry e_;
  };
 
  struct FormatCompare2
  {
    explicit FormatCompare2(std::string s);
    bool operator() (Entry const& e);

    std::string s_;
  };
    
  typedef std::vector<Entry> Container;
  typedef Container::iterator IT;

  static bool add_unique(Container& l, Entry const& e);
  static IT find(Container& l, std::string const& fmt);
  static Container& rlist();
  static Container& wlist();
  static void setupHandler();
};

//! Provides pixmap output through GraphApp (PNG and GIF)
class QWT3D_EXPORT PixmapWriter : public IO::Functor
{
friend class IO;
public:  
  PixmapWriter() : quality_(-1) {}
  void setQuality(int val);
private:
  IO::Functor* clone() const {return new PixmapWriter(*this);}
  bool operator()(Plot3D* plot, std::string const& fname);
  std::string fmt_;
  int quality_;
};

} //ns

#endif
