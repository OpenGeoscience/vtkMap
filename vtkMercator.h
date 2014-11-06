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
    return (int)(floor((1.0 - log( tan(lat * M_PI/180.0) + 1.0 /
      cos(lat * M_PI/180.0)) / M_PI) / 2.0 * std::pow(2.0, z)));
  }

  //----------------------------------------------------------------------------
  static double tilex2long(int x, int z)
  {
    return x / std::pow(2.0, z) * 360.0 - 180;
  }

  //----------------------------------------------------------------------------
  static double tiley2lat(int y, int z)
  {
    double n = M_PI - 2.0 * M_PI * y / std::pow(2.0, z);
    return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
  }

  //----------------------------------------------------------------------------
  static double y2lat(double a)
  {
    return 180 / M_PI * (2 * atan(exp(a * M_PI / 180.0)) - M_PI / 2.0);
  }

  //----------------------------------------------------------------------------
  static double lat2y(double a)
  {
    return 180.0 / M_PI * log(tan(M_PI / 4.0 + a * (M_PI / 180.0) / 2.0));
  }

protected:
  vtkMercator() {;}
  virtual ~vtkMercator() {;}
};

#endif // __vtkMercator_h
