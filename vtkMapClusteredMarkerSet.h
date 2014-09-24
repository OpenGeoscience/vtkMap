/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapClusteredMarkerSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMapClusteredMarkerSet - collection of map markers
// .SECTION Description
// Computes 2 vtkPolyData for (i) individual "point" map markers, and (ii)
// "cluster" map markers. The computed polydata contains point sets and
// point data (i.e., no cells). Point data arrays are named:
// "MarkerType", which specifies 0 for point marker, 1 for cluster marker,
// and "Color", which specifies RGB for each marker.


#ifndef __vtkMapClusteredMarkerSet_h
#define __vtkMapClusteredMarkerSet_h

#include <vtkObject.h>
#include <vtkType.h>

class vtkPolyData;
class vtkUnsignedCharArray;

class vtkMapClusteredMarkerSet : public vtkObject
{
public:
  static vtkMapClusteredMarkerSet *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkMapClusteredMarkerSet, vtkObject);
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
  // Returns marker type for specified glyph number (in current display)
  // 0 == point marker, 1 == cluster marker (-1 == invalid glyph number)
  int GetMarkerType(int glyphNumber);

  // Description
  // Returns marker id for specified glyph number (in current display)
  // If glyph is a cluster or glyphNumber is invalid, returns -1
  int GetMarkerId(int glyphNumber);

 protected:
  vtkMapClusteredMarkerSet();
  ~vtkMapClusteredMarkerSet();

  vtkPolyData *PolyData;

  class MapCluster;
  MapCluster *FindClosestCluster(double gcsCoords[2], int zoomLevel,
                                 double distanceThreshold);
  vtkUnsignedCharArray *SetupUCharArray(vtkPolyData *polyData, const char *name,
                                        int numberOfComponents=3);
private:
  class MapClusteredMarkerSetInternals;
  MapClusteredMarkerSetInternals* Internals;
  vtkMapClusteredMarkerSet(const vtkMapClusteredMarkerSet&);  // not implemented
  void operator=(const vtkMapClusteredMarkerSet);  // not implemented
};

#endif // __vtkMapClusteredMarkerSet_h
