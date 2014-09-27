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
  vtkSetMacro(Renderer, vtkRenderer *);

  // Description:
  // Set/get whether to apply hierarchical clustering to map markers.
  // The default is off, and once turned on, behavior is undefined if
  // clustering is turned off.
  vtkSetMacro(Clustering, bool);
  vtkGetMacro(Clustering, bool);
  vtkBooleanMacro(Clustering, bool);

  // Description:
  // Add marker to map, returns id
  vtkIdType AddMarker(double latitude, double longitude);

  // Description:
  // Removes all map markers
  void RemoveMarkers();

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

  void InitializeRenderingPipeline();

  // Description:
  // Indicates that internal logic & pipeline have been initialized
  bool Initialized;

  // Description:
  // Flag to enable/disable marker clustering logic
  bool Clustering;

  // Description:
  // The renderer used to draw maps
  vtkRenderer* Renderer;

  vtkPolyData *PolyData;
  vtkPolyDataMapper *Mapper;
  vtkActor *Actor;

  class ClusteringNode;
  ClusteringNode *FindClosestNode(double gcsCoords[2], int zoomLevel,
                                  double distanceThreshold);
  void MergeNodes(ClusteringNode *src, ClusteringNode *dest, int level);

 private:
  class MapMarkerSetInternals;
  MapMarkerSetInternals* Internals;

  vtkMapMarkerSet(const vtkMapMarkerSet&);  // not implemented
  void operator=(const vtkMapMarkerSet);  // not implemented
};

#endif // __vtkMapMarkerSet_h
