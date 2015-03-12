/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMapPickResult - data computed from pick operation
// .SECTION Description
//

#ifndef __vtkMapPickResult_h
#define __vtkMapPickResult_h

#include <vtkObject.h>
#include <vtkType.h>
#include "vtkmap_export.h"

//----------------------------------------------------------------------------
// Define a unique integer value for each map feature type
#define VTK_MAP_FEATURE_NONE 0
#define VTK_MAP_FEATURE_MARKER 1
#define VTK_MAP_FEATURE_CLUSTER 2

//----------------------------------------------------------------------------
class VTKMAP_DEPRECATED vtkMapPickResult : public vtkObject
{
 public:
  static vtkMapPickResult *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkMapPickResult, vtkObject);

  // Description:
  // The display coordinate of the point that was picked
  vtkGetVector2Macro(DisplayCoordinates, int);
  vtkSetVector2Macro(DisplayCoordinates, int);

  // Description:
  // The map layer when the feature was picked
  vtkGetMacro(MapLayer, int);
  vtkSetMacro(MapLayer, int);

  // Description:
  // The latitude coordinate of the point that was picked
  vtkGetMacro(Latitude, double);
  vtkSetMacro(Latitude, double);

  // Description:
  // The longitude coordinate of the point that was picked
  vtkGetMacro(Longitude, double);
  vtkSetMacro(Longitude, double);

  // Description:
  // The type of map feature that was picked
  vtkGetMacro(MapFeatureType, int);
  vtkSetMacro(MapFeatureType, int);

  // Description:
  // The number of map markers represented by the feature
  vtkGetMacro(NumberOfMarkers, int);
  vtkSetMacro(NumberOfMarkers, int);

  // Description:
  // The id associated with the picked map feature
  vtkGetMacro(MapFeatureId, int);
  vtkSetMacro(MapFeatureId, int);

 protected:
  int DisplayCoordinates[2];
  int MapLayer;
  double Latitude;
  double Longitude;
  int MapFeatureType;
  int NumberOfMarkers;
  vtkIdType MapFeatureId;

 private:
  vtkMapPickResult();

  vtkMapPickResult(const vtkMapPickResult&);  // Not implemented
  vtkMapPickResult& operator=(const vtkMapPickResult&); // Not implemented
};

#endif // __vtkMapPickResult_h
