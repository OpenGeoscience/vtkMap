/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGeoMapFeatureSelector - Handler for feature-selection operations
// .SECTION Description
// This class offloads
//
// Note: this file is NOT exported, since it should only be used by
// the vtkMap class.
//
// This class uses a vtkHardwareSelector instance to pick features visible
// in vtkMap. It currently selects all of the markers/features lying within
// an area (either a rectangle or an irregular polygon) regardless of whether
// they are hidden behind of other features (see IncrementalSelect).
//
// \note
// Currently, rubber-band selection (PickArea) and polygon selection
// (PickPolygon) do not fully share the same code path. The plan is to unify
// them such that they both fully rely on vtkHardwareSelector (PickPolygon
// already does).
//

#ifndef __vtkGeoMapFeatureSelector_h
#define __vtkGeoMapFeatureSelector_h
#include <unordered_map>
#include <vector>

#include <vtkObject.h>
#include <vtkVector.h>

#include "vtkmap_export.h"


class vtkFeature;
class vtkGeoMapSelection;
class vtkIdList;
class vtkPlanes;
class vtkProp;
class vtkRenderer;
class vtkSelection;
class vtkSelectionNode;

class VTKMAP_NO_EXPORT vtkGeoMapFeatureSelector : public vtkObject
{
 public:
  static vtkGeoMapFeatureSelector* New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkGeoMapFeatureSelector, vtkObject);

  // As features get added/removed, they are passed on to this class
  void AddFeature(vtkFeature *feature);
  void RemoveFeature(vtkFeature *feature);

  void PickPoint(vtkRenderer *renderer, int displayCoords[2],
                 vtkGeoMapSelection *selection);
  void PickArea(vtkRenderer *renderer, int displayCoords[4],
                vtkGeoMapSelection *selection);
  void PickPolygon(vtkRenderer* ren,
    const std::vector<vtkVector2i>& polygonPoints, vtkGeoMapSelection* result);

 protected:
  vtkGeoMapFeatureSelector();
  ~vtkGeoMapFeatureSelector();

  void PickPolyDataCells(
    vtkProp *prop,
    vtkPlanes *frustum,
    vtkIdList *idList);

  void PickMarkers(
    vtkRenderer *renderer,
    int displayCoords[4],
    vtkGeoMapSelection *selection);

  bool PrepareSelect(vtkRenderer* ren);

 private:
  vtkGeoMapFeatureSelector(const vtkGeoMapFeatureSelector&) VTK_DELETE_FUNCTION;
  vtkGeoMapFeatureSelector& operator=(const vtkGeoMapFeatureSelector&) VTK_DELETE_FUNCTION;

  using FeatureMap = std::unordered_map<vtkProp*, vtkFeature*>;
  /**
   * Runs mulitiple selection passes in order to capture markers hidden
   * behind other markers.
   */
  void IncrementalSelect(vtkGeoMapSelection* selection, vtkRenderer* ren,
    FeatureMap& featMap);
  bool SelectMarkerSet(vtkGeoMapSelection* selection, vtkSelectionNode* node,
    FeatureMap& map, FeatureMap::const_iterator& it);
  bool SelectPolyData(vtkGeoMapSelection* selection, vtkSelectionNode* node,
    FeatureMap& map, FeatureMap::const_iterator& it);

  class vtkGeoMapFeatureSelectorInternal;
  vtkGeoMapFeatureSelectorInternal *Internal;
};

#endif   // __vtkGeoMapFeatureSelector_h
