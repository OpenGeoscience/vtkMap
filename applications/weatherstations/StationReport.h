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
// .NAME StationReport - simple container for weather station data
// .SECTION Description
//

#ifndef __StationReport_h
#define __StationReport_h

#include <string>
#include <stdlib.h>

struct StationReport
{
int         id;
std::string name;
double      latitude;
double      longitude;
double      temperature;
time_t      datetime;
};

#endif /* __StationReport_h */
