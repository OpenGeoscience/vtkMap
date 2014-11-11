/*=========================================================================

  Program:   Visualization Toolkit

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
#include "vtkmap_export.h"
#include <set>

class vtkActor;
class vtkMapClusteredMarkerSet;
class vtkMapPickResult;
class vtkMapper;
class vtkPicker;
class vtkPolyDataMapper;
class vtkPolyData;
class vtkRenderer;

class VTKMAP_EXPORT vtkMapMarkerSet : public vtkObject
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
  // Max scale factor to apply to cluster markers, default is 2.0
  // The scale function is 2nd order model: y = k*x^2 / (x^2 + b).
  // Coefficient k sets the max scale factor, i.e., y(inf) = k
  // Coefficient b is computed to set min to 1, i.e., y(2) = 1.0
  vtkSetClampMacro(MaxClusterScaleFactor, double, 1.0, 100.0);
  vtkGetMacro(MaxClusterScaleFactor, double);

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
  // Sets the max size to render cluster glyphs (based on marker count)
  double MaxClusterScaleFactor;

  // Description:
  // The renderer used to draw maps
  vtkRenderer* Renderer;

  vtkPolyData *PolyData;
  vtkPolyDataMapper *Mapper;
  vtkActor *Actor;

  class ClusteringNode;
  ClusteringNode *FindClosestNode(ClusteringNode *node, int zoomLevel,
                                  double distanceThreshold);
  void MergeNodes(ClusteringNode *node, ClusteringNode *mergingNode,
                  std::set<ClusteringNode*>& parentsToMerge, int level);

 private:
  class MapMarkerSetInternals;
  MapMarkerSetInternals* Internals;

  vtkMapMarkerSet(const vtkMapMarkerSet&);  // not implemented
  vtkMapMarkerSet& operator=(const vtkMapMarkerSet);  // not implemented
};

#endif // __vtkMapMarkerSet_h
