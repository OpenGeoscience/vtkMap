/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapClusteredMarkerSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMapClusteredMarkerSource - points source for map markers
// .SECTION Description
// Computes 2 vtkPolyData for (i) individual map markers, and (ii) map
// marker clusters. The computed polydata contains point sets and
// point data (i.e., no cells). Point data arrays are named:
// "MarkerType", which specifies 0 for point marker, 1 for cluster marker
// "Color", which specifies RGB for each marker.


#ifndef __vtkMapClusteredMarkerSource_h
#define __vtkMapClusteredMarkerSource_h

#include <vtkObject.h>
#include <vtkType.h>

class vtkPolyData;
class vtkUnsignedCharArray;

class vtkMapClusteredMarkerSource : public vtkObject
{
public:
  static vtkMapClusteredMarkerSource *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkMapClusteredMarkerSource, vtkObject);
  vtkGetMacro(PolyData, vtkPolyData*);

  // Description:
  // Add marker coordinates
  vtkIdType AddMarker(double latitude, double longitude);

  // Description:
  // Removes all map markers
  void RemoveMapMarkers();

  // Description:
  // Updates polydata based on zoom level
  void Update(int ZoomLevel);

  // Description
  // Returns marker type for specified point id
  int GetMarkerType(int pointId);

  // Description
  // Returns marker id for specified point id (returns -1 for clusters)
  int GetMarkerId(int pointId);

 protected:
  vtkMapClusteredMarkerSource();
  ~vtkMapClusteredMarkerSource();

  vtkPolyData *PolyData;

  vtkUnsignedCharArray *SetupUCharArray(vtkPolyData *polyData, const char *name,
                                        int numberOfComponents=3);

  class MapClusteredMarkerSourceInternals;
  MapClusteredMarkerSourceInternals* Internals;
private:
  vtkMapClusteredMarkerSource(const vtkMapClusteredMarkerSource&);  // not implemented
  void operator=(const vtkMapClusteredMarkerSource);  // not implemented
};

#endif // __vtkMapClusteredMarkerSource_h
