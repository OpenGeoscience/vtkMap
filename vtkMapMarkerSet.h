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

#include "vtkPolydataFeature.h"
#include "vtkmap_export.h"
#include <set>

class vtkActor;
class vtkIdList;
class vtkLookupTable;
class vtkMapper;
class vtkPolyDataMapper;
class vtkPolyData;
class vtkRenderer;

class VTKMAP_EXPORT vtkMapMarkerSet : public vtkPolydataFeature
{
public:
  static vtkMapMarkerSet *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkMapMarkerSet, vtkPolydataFeature);

  // Description:
  // Set/get the Z value, in world coordinates, to assign each marker.
  // This value is intended to be set once, before adding markers.
  // The default is 0.1
  vtkSetMacro(ZCoord, double);
  vtkGetMacro(ZCoord, double);

  // Description:
  // Set/get whether to apply hierarchical clustering to map markers.
  // The default is off, and once turned on, behavior is undefined if
  // clustering is turned off.
  vtkSetMacro(Clustering, bool);
  vtkGetMacro(Clustering, bool);
  vtkBooleanMacro(Clustering, bool);

  // Description:
  // Threshold distance to use when creating clusters.
  // The value is in display units (pixels).
  // Default value is 80 pixels.
  vtkSetMacro(ClusterDistance, double);
  vtkGetMacro(ClusterDistance, double);

  // Description:
  // Max scale factor to apply to cluster markers, default is 2.0
  // The scale function is 2nd order model: y = k*x^2 / (x^2 + b).
  // Coefficient k sets the max scale factor, i.e., y(inf) = k
  // Coefficient b is computed to set min to 1, i.e., y(2) = 1.0
  vtkSetClampMacro(MaxClusterScaleFactor, double, 1.0, 100.0);
  vtkGetMacro(MaxClusterScaleFactor, double);

  // Description:
  // Get number of markers
  int GetNumberOfMarkers();

  // Description:
  // Add marker to map, returns id
  vtkIdType AddMarker(double latitude, double longitude);

  // Description:
  // Set marker visibility
  // Note that you MUST REDRAW after changing visibility
  bool SetMarkerVisibility(int markerId, bool visible);
  bool GetMarkerVisibility(int markerId) const;

  // Description:
  // Select or unselect marker
  // Note that you MUST REDRAW after changing selection
  bool SetMarkerSelection(int markerId, bool selected);

  // Description:
  // Return descendent ids for given cluster id.
  // This is inteneded for traversing selected clusters.
  void GetClusterChildren(vtkIdType clusterId, vtkIdList *childMarkerIds,
                          vtkIdList *childClusterIds);

  // Description:
  // Return all marker ids descending from given cluster id
  void GetAllMarkerIds(vtkIdType clusterId, vtkIdList *markerIds);

  // Description:
  // Override
  virtual void Init();

  // Description:
  // Update the marker geometry to draw the map
  //void Update(int zoomLevel);
  virtual void Update();

  // Description:
  // Override
  virtual void Cleanup();

  // Description:
  // Return cluster id for display id at current zoom level.
  // The cluster id is a unique, persistent id assigned
  // to each display element, whether it represents a single
  // marker or a cluster. As such, cluster ids are different
  // for each zoom level.
  // Returns -1 for invalid display id
  vtkIdType GetClusterId(vtkIdType displayId);

  // Description:
  // Return marker id for display id at current zoom level.
  // Marker ids are independent of zoom level.
  // If the display id represents a cluster, returns -1.
  // Also returns -1 for invalid display id.
  vtkIdType GetMarkerId(vtkIdType displayId);

 protected:
  vtkMapMarkerSet();
  ~vtkMapMarkerSet();

  // Description:
  // Z Coordinate of this map feature
  double ZCoord;

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
  // Geometry representation; gets updated each zoom-level change
  vtkPolyData *PolyData;

  // Description:
  // Threshold distance when combining markers/clusters
  double ClusterDistance;

  // Description:
  // Stores colors for standard display and selection
  vtkLookupTable *ColorTable;

  class ClusteringNode;

  // Computes clustering distance in gcs coordinates
  double ComputeDistanceThreshold2(double latitude, double longitude,
                                   double clusteringDistance) const;

  // Find closest node within distance threshold squared
  ClusteringNode *FindClosestNode(ClusteringNode *node, int zoomLevel,
                                  double distanceThreshold2);
  void MergeNodes(ClusteringNode *node, ClusteringNode *mergingNode,
                  std::set<ClusteringNode*>& parentsToMerge, int level);

  void GetMarkerIdsRecursive(vtkIdType clusterId, vtkIdList *markerIds);

  // Description:
  // Generates next color for actor (which can be overridden, of course)
  void ComputeNextColor(double color[3]);
  static unsigned int NextMarkerHue;
  double SelectionHue;

 private:
  class MapMarkerSetInternals;
  MapMarkerSetInternals* Internals;

  vtkMapMarkerSet(const vtkMapMarkerSet&);  // not implemented
  vtkMapMarkerSet& operator=(const vtkMapMarkerSet&);  // not implemented
};

#endif // __vtkMapMarkerSet_h
