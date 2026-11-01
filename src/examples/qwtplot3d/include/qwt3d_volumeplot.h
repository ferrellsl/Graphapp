#ifndef qwt3d_volumeplot_h__2004_03_06_01_52_begin_guarded_code
#define qwt3d_volumeplot_h__2004_03_06_01_52_begin_guarded_code

#include "qwt3d_plot.h"

namespace Qwt3D
{

//! TODO
class QWT3D_EXPORT VolumePlot : public Plot3D
{
public:
  VolumePlot( Window* parent, int x, int y, int width, int height ) : Plot3D(parent, x, y, width, height) {}

protected:
	virtual void createData() = 0;
};

} // ns


#endif
