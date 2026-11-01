#ifndef qwt3d_graphplot_h__2004_03_06_01_57_begin_guarded_code
#define qwt3d_graphplot_h__2004_03_06_01_57_begin_guarded_code

#include "qwt3d_plot.h"

namespace Qwt3D
{

//! TODO
class QWT3D_EXPORT GraphPlot : public Plot3D
{
public:
  GraphPlot( Window* parent, int x, int y, int width, int height );

protected:
	virtual void createData() = 0;
};

} // ns


#endif
