/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkMercator_h
#define __vtkMercator_h

#include <cmath>

class vtkMercator : vtkObject
{
//by default visual studio doesn't provide the M_PI define as it is a
//non standard function, but with the around defines it does. It is far to
//complicated to make sure everything gets included in the right order for
//that to happen for a header only class. Instead we provide our own
//M_PI define if it isn't already defined. The value we have chose is the
//one that math.h uses on compilers that support M_PI.
  static double m_pi()
  {
#ifndef M_PI
  return 3.14159265358979323846;
#else
  return M_PI;
#endif
  };

public:
  virtual void PrintSelf(ostream &os, vtkIndent indent) {;}
  vtkTypeMacro(vtkMercator, vtkObject);

  //----------------------------------------------------------------------------
  static int long2tilex(double lon, int z)
  {
    return (int)(floor((lon + 180.0) / 360.0 * std::pow(2.0, z)));
  }

  //----------------------------------------------------------------------------
  static int lat2tiley(double lat, int z)
  {
    return (int)(floor((1.0 - log( tan(lat * m_pi()/180.0) + 1.0 /
      cos(lat * m_pi()/180.0)) / m_pi()) / 2.0 * std::pow(2.0, z)));
  }

  //----------------------------------------------------------------------------
  static double tilex2long(int x, int z)
  {
    return x / std::pow(2.0, z) * 360.0 - 180;
  }

  //----------------------------------------------------------------------------
  static double tiley2lat(int y, int z)
  {
    double n = m_pi() - 2.0 * m_pi() * y / std::pow(2.0, z);
    return 180.0 / m_pi() * atan(0.5 * (exp(n) - exp(-n)));
  }

  //----------------------------------------------------------------------------
  static double y2lat(double a)
  {
    return 180 / m_pi() * (2 * atan(exp(a * m_pi() / 180.0)) - m_pi() / 2.0);
  }

  //----------------------------------------------------------------------------
  static double lat2y(double a)
  {
    return 180.0 / m_pi() * log(tan(m_pi() / 4.0 + a * (m_pi() / 180.0) / 2.0));
  }

protected:
  vtkMercator() {;}
  virtual ~vtkMercator() {;}
};



#endif // __vtkMercator_h
