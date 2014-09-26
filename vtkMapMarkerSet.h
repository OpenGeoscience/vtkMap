/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapMarkerSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMapMarkerSet - collection of map markers
// .SECTION Description
//

#ifndef __vtkMapMarkerSet_h
#define __vtkMapMarkerSet_h

#include <vtkObject.h>

class vtkActor;
class vtkMapClusteredMarkerSet;
class vtkMapPickResult;
class vtkMapper;
class vtkPicker;
class vtkPolyDataMapper;
class vtkPolyData;
class vtkRenderer;

class vtkMapMarkerSet : public vtkObject
{
public:
  static vtkMapMarkerSet *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkMapMarkerSet, vtkObject);

  // Description:
  // Set the renderer in which map markers will be added
  void SetRenderer(vtkRenderer *renderer);

  // Description:
  // Add marker to map, returns id
  vtkIdType AddMarker(double Latitude, double Longitude);

  // Description:
  // Removes all map markers
  void RemoveMapMarkers();

  // Description:
  // Update the marker geometry to draw the map
  void Update(int zoomLevel);

  // Description:
  // Returns id of marker at specified display coordinates
  void PickPoint(vtkRenderer *renderer, vtkPicker *picker,
            int displayCoords[2], vtkMapPickResult *result);

 protected:
  vtkMapMarkerSet();
  ~vtkMapMarkerSet();

  // Description:
  // The renderer used to draw maps
  vtkRenderer* Renderer;

  vtkPolyDataMapper *Mapper;
  vtkActor *Actor;

  vtkMapClusteredMarkerSet *MarkerSet;
};

#endif // __vtkMapMarkerSet_h
