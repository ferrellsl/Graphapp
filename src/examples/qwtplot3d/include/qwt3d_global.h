#ifndef QWT3D_GLOBAL_H
#define QWT3D_GLOBAL_H

// Altered from the original QwtPlot3D: Qt removed, hosted by GraphApp (see README.md).

#include <assert.h>

#define QWT3D_MAJOR_VERSION 0
#define QWT3D_MINOR_VERSION 2
#define QWT3D_PATCH_VERSION 6

#if defined(_MSC_VER) /* MSVC Compiler */
  #pragma warning(disable: 4251) // dll interface required for stl templates
  #pragma warning(disable: 4786) // truncating debug info after 255 characters
  #pragma warning(disable: 4660) // template-class specialization 'identifier' is already instantiated
  #pragma warning(disable: 4996) // MS security enhancements
#endif

// The library is built as a static library only.
#ifndef QWT3D_EXPORT
  #define QWT3D_EXPORT
#endif

#endif
