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
#include <set>

#include "vtkPolydataFeature.h"
#include "vtkmapcore_export.h"

class vtkActor;
class vtkIdList;
class vtkLookupTable;
class vtkMapper;
class vtkPolyDataMapper;
class vtkPolyData;
class vtkRenderer;
class vtkTextProperty;

class VTKMAPCORE_EXPORT vtkMapMarkerSet : public vtkPolydataFeature
{
public:
  static vtkMapMarkerSet* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkMapMarkerSet, vtkPolydataFeature);

  // Description:
  // Set/get the Z value, in world coordinates, to assign each marker.
  // This value is intended to be set once, before adding markers.
  // The default is 0.1
  vtkSetMacro(ZCoord, double);
  vtkGetMacro(ZCoord, double);

  // Description:
  // Set/get the RGBA color assigned to the markers
  void SetColor(double rgba[4]);

  // Description:
  // Set/get the size to display point markers, in image pixels.
  // The default is true
  vtkSetMacro(EnablePointMarkerShadow, bool);
  vtkGetMacro(EnablePointMarkerShadow, bool);
  vtkBooleanMacro(EnablePointMarkerShadow, bool);

  // Description:
  // Set/get the size to display point markers, in image pixels.
  // The default is 50
  vtkSetMacro(PointMarkerSize, unsigned int);
  vtkGetMacro(PointMarkerSize, unsigned int);
  vtkSetMacro(ClusterMarkerSize, unsigned int);
  vtkGetMacro(ClusterMarkerSize, unsigned int);

  enum ClusterSize {
    POINTS_CONTAINED = 0,
    USER_DEFINED
  };
  vtkSetMacro(ClusterMarkerSizeMode, int);
  vtkGetMacro(ClusterMarkerSizeMode, int);

  // Description:
  // Set/get the Z offset value, in world coordinates, assigned
  // to each marker in the current selected set.
  // This can be used to prevent selected markers from being
  // obscured by other (non-selected) markers.
  // The default is 0.0
  vtkSetMacro(SelectedZOffset, double);
  vtkGetMacro(SelectedZOffset, double);

  // Description:
  // Set/get whether to apply hierarchical clustering to map markers.
  // The default is off, and once turned on, behavior is undefined if
  // clustering is turned off.
  vtkSetMacro(Clustering, bool);
  vtkGetMacro(Clustering, bool);
  vtkBooleanMacro(Clustering, bool);

  // Description:
  // Set/get the (maximum) depth of the point-clustering tree.
  // This function should only be called *before* adding any markers.
  // Valid values are between 2 and 20 inclusive; other values are ignored.
  // The default is 14.
  vtkSetClampMacro(ClusteringTreeDepth, unsigned int, 2, 20);
  vtkGetMacro(ClusteringTreeDepth, unsigned int);

  // Description:
  // Threshold distance to use when creating clusters.
  // The value is in display units (pixels).
  // Default value is 80 pixels.
  vtkSetMacro(ClusterDistance, int);
  vtkGetMacro(ClusterDistance, int);

  // Description:
  // Rebuild the internal clustering tree, to reflect
  // changes to settings and deleted markers
  void RecomputeClusters();

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
  // Remove marker from map, returns boolean indicating success
  bool DeleteMarker(vtkIdType markerId);

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
  void GetClusterChildren(
    vtkIdType clusterId, vtkIdList* childMarkerIds, vtkIdList* childClusterIds);

  // Description:
  // Return all marker ids descending from given cluster id
  void GetAllMarkerIds(vtkIdType clusterId, vtkIdList* markerIds);

  // Description:
  // Override
  void Init() override;

  // Description:
  // Update the marker geometry to draw the map
  //void Update(int zoomLevel);
  void Update() override;

  // Description:
  // Override
  void CleanUp() override;

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

  // Description:
  // For debug, writes out the set of cluster nodes
  // ascending from given marker
  void PrintClusterPath(ostream& os, int markerId);

  void SetLabelProperties(vtkTextProperty* property);
  vtkTextProperty* GetLabelProperties() const;

  void SetLabelOffset(std::array<double, 3>& offset);
  std::array<double, 3> GetLabelOffset() const;

protected:
  vtkMapMarkerSet();
  ~vtkMapMarkerSet();

  class ClusteringNode;

  // Used when rebuilding clustering tree
  void InsertIntoNodeTable(ClusteringNode* node);

  // Computes clustering distance in gcs coordinates
  double ComputeDistanceThreshold2(
    double latitude, double longitude, int clusteringDistance) const;

  // Find closest node within distance threshold squared
  ClusteringNode* FindClosestNode(
    ClusteringNode* node, int zoomLevel, double distanceThreshold2);
  void MergeNodes(ClusteringNode* node, ClusteringNode* mergingNode,
    std::set<ClusteringNode*>& parentsToMerge, int level);

  void GetMarkerIdsRecursive(vtkIdType clusterId, vtkIdList* markerIds);

  // Description:
  // Generates next color for actor (which can be overridden, of course)
  void ComputeNextColor(double color[3]);

  void InitializeLabels(vtkRenderer* rend);

  void OnRenderStart();

  ////////////////////////////////////////////////////////////////////////////////

  // Description:
  // Z Coordinate of this map feature
  double ZCoord;

  // Description:
  // Sets whether to include shadow with map marker display.
  // Default value is true.
  bool EnablePointMarkerShadow;

  // Description:
  // Size to display point (single) or cluster (multiple) markers, in pixels.
  const unsigned int BaseMarkerSize = 50;
  unsigned int PointMarkerSize = 50;
  unsigned int ClusterMarkerSize = 50;
  int ClusterMarkerSizeMode = POINTS_CONTAINED;

  // Description:
  // Offset to apply to Z coordinate of markers in the selected state
  double SelectedZOffset;

  // Description:
  // Indicates that internal logic & pipeline have been initialized
  bool Initialized;

  // Description:
  // Flag to enable/disable marker clustering logic
  bool Clustering;

  // Description:
  // Depth of clustering tree to use
  unsigned int ClusteringTreeDepth;

  // Description:
  // Sets the max size to render cluster glyphs (based on marker count)
  double MaxClusterScaleFactor;

  // Description:
  // Geometry representation; gets updated each zoom-level change
  vtkPolyData* PolyData;

  // Description:
  // Threshold distance when combining markers/clusters
  int ClusterDistance;

  // Description:
  // Stores colors for standard display and selection
  vtkLookupTable* ColorTable;

  static unsigned int NextMarkerHue;
  double SelectionHue;

private:
  class MapMarkerSetInternals;
  MapMarkerSetInternals* Internals;

  vtkMapMarkerSet(const vtkMapMarkerSet&) = delete;
  vtkMapMarkerSet& operator=(const vtkMapMarkerSet&) = delete;
};

#endif // __vtkMapMarkerSet_h
