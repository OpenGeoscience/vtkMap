/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMap

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGeoMapFeatureSelector.h"
#include "vtkFeature.h"
#include "vtkGeoMapSelection.h"
#include "vtkMapMarkerSet.h"
#include "vtkRasterFeature.h"

#include <vtkAbstractArray.h>
#include <vtkActor.h>
#include <vtkAreaPicker.h>
#include <vtkCellData.h>
#include <vtkExtractSelectedFrustum.h>
#include <vtkHardwareSelector.h>
#include <vtkIdList.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkIntArray.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPlanes.h>
#include <vtkPolyData.h>
#include <vtkProp.h>
#include <vtkProp3DCollection.h>
#include <vtkRenderWindow.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkUnstructuredGrid.h>

#include <set>
#include <string>


vtkStandardNewMacro(vtkGeoMapFeatureSelector);

//-----------------------------------------------------------------------------
class vtkGeoMapFeatureSelector::vtkGeoMapFeatureSelectorInternal
{
public:
  enum Selection : unsigned char {
    POLYGON,
    RUBBER_BAND
  };

  vtkGeoMapFeatureSelectorInternal ()
  : Selector(vtkSmartPointer<vtkHardwareSelector>::New())
  , Mode(Selection::RUBBER_BAND) {
  }

  /**
   * Pick rendered items either doing a rubber-band or polygon selection.
   */
  vtkSelection* DoSelect(vtkRenderer* ren)
  {
    if (!this->PrepareSelect(ren))
    {
      return nullptr;
    }

    vtkSelection* sel;
    switch (this->Mode)
    {
      case Selection::POLYGON:
        sel = this->Selector->GeneratePolygonSelection(
          this->PolygonPoints, this->PolygonPointsCount);
        break;
      case Selection::RUBBER_BAND:
        sel = this->Selector->GenerateSelection(
          this->PolygonPoints[0], this->PolygonPoints[1],
          this->PolygonPoints[2], this->PolygonPoints[3]);
        break;
    }

    return sel;
  }

  /**
   * Setup vtkHardwareSelector and capture rendered buffers.
   */
  bool PrepareSelect(vtkRenderer* ren)
  {
    int* size = ren->GetSize();
    int* origin = ren->GetOrigin();

    auto sel = this->Selector;
    sel->SetArea(origin[0], origin[1], origin[0] + size[0] - 1,
      origin[1] + size[1] - 1);
    sel->SetRenderer(ren);

    if (!sel->CaptureBuffers())
    {
      return false;
    }

    return true;
  }

  /**
   * Transform a std::vector into a vtkDataArray
   */
  bool VectorToArray(const std::vector<vtkVector2i>& vec, vtkIntArray* arr)
  {
    if (vec.size() >= 3)
    {
      arr->SetNumberOfComponents(2);
      arr->SetNumberOfTuples(vec.size());

      for (unsigned int j = 0; j < vec.size(); j++)
      {
        const vtkVector2i& v = vec[j];
        int pos[2] = { v[0], v[1] };
        arr->SetTypedTuple(j, pos);
      }
      return true;
    }

    return false;
  }

  /**
   * Bounding box of a polygon. Implemented in vtkHardwareSelector.
   */
  void ComputePolygonBounds(int* polygonPoints, vtkIdType count, int* bounds)
  {
    bounds[0] = VTK_INT_MAX; bounds[2] = VTK_INT_MIN; // x_min, x_max
    bounds[1] = VTK_INT_MAX, bounds[3] =VTK_INT_MIN;  // y_min, y_max
    for(vtkIdType i = 0; i < count; i += 2)
    {
      bounds[0] = std::min(polygonPoints[i], bounds[0]);     // x_min
      bounds[2] = std::max(polygonPoints[i], bounds[2]);     // x_max
      bounds[1] = std::min(polygonPoints[i + 1], bounds[1]); // y_min
      bounds[3] = std::max(polygonPoints[i + 1], bounds[3]); // y_max
    }
  }

  // Map vtkProp to vtkFeature
  FeatureMap FeaturePickMap;
  Selection Mode;
  int* PolygonPoints = nullptr;
  int PolygonBounds[4];
  vtkIdType PolygonPointsCount = 0;
  vtkSmartPointer<vtkHardwareSelector> Selector;
};

//-----------------------------------------------------------------------------
vtkGeoMapFeatureSelector::vtkGeoMapFeatureSelector()
{
  this->Internal = new vtkGeoMapFeatureSelectorInternal;
}


//-----------------------------------------------------------------------------
vtkGeoMapFeatureSelector::~vtkGeoMapFeatureSelector()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkGeoMapFeatureSelector::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkGeoMapFeatureSelector::AddFeature(vtkFeature *feature)
{
  vtkProp *prop = feature->PickProp();
  if (prop)
    {
    this->Internal->FeaturePickMap[prop] = feature;
    }
}

//----------------------------------------------------------------------------
void vtkGeoMapFeatureSelector::RemoveFeature(vtkFeature *feature)
{
  vtkProp *prop = feature->PickProp();
  if (prop)
    {
    this->Internal->FeaturePickMap.erase(prop);
    }
}

//-----------------------------------------------------------------------------
void vtkGeoMapFeatureSelector::
PickPoint(vtkRenderer *renderer, int displayCoords[2],
          vtkGeoMapSelection *selection)
{
  // Expand area pickers frustum size to increase reliability
  const double offset = 4.0;

  int boundCoords[4];
  boundCoords[0] = displayCoords[0] - offset;
  boundCoords[1] = displayCoords[1] - offset;
  boundCoords[2] = displayCoords[0] + offset;
  boundCoords[3] = displayCoords[1] + offset;
  this->PickArea(renderer, boundCoords, selection);
}

//-----------------------------------------------------------------------------
void vtkGeoMapFeatureSelector::PickPolygon(vtkRenderer* ren,
  const std::vector<vtkVector2i>& polygonPoints, vtkGeoMapSelection* result)
{
  vtkNew<vtkIntArray> array;
  if (!this->Internal->VectorToArray(polygonPoints, array.GetPointer()))
  {
    vtkErrorMacro("Vector to array failed!");
    return;
  }

  this->Internal->PolygonPoints = static_cast<int*>(array->GetVoidPointer(0));
  this->Internal->PolygonPointsCount = array->GetNumberOfValues();
  this->Internal->Mode = vtkGeoMapFeatureSelectorInternal::Selection::POLYGON;
  this->Internal->ComputePolygonBounds(this->Internal->PolygonPoints,
    this->Internal->PolygonPointsCount, this->Internal->PolygonBounds);

  // Temporary feature map to remove elements that have been selected in
  // each IncrementalSelect call.  TODO Caching could be removed if features
  // selected as CELLS or POINTS are stored in separate containers.
  FeatureMap featMap = this->Internal->FeaturePickMap;

  // For marker features
  this->Internal->Selector->SetFieldAssociation(
    vtkDataObject::FIELD_ASSOCIATION_POINTS);
  this->IncrementalSelect(result, ren, featMap);

  // For polygon features
  this->Internal->Selector->SetFieldAssociation(
    vtkDataObject::FIELD_ASSOCIATION_CELLS);

  this->IncrementalSelect(result, ren, featMap);
}

//-----------------------------------------------------------------------------
void vtkGeoMapFeatureSelector::
PickArea(vtkRenderer *renderer, int displayCoords[4],
         vtkGeoMapSelection *selection)
{
  // Clear selection
  selection->Clear();

  // Use rendered-area picker to minimize hits on marker features
  vtkNew<vtkAreaPicker> areaPicker;
  int result = areaPicker->AreaPick(
    displayCoords[0], displayCoords[1],
    displayCoords[2], displayCoords[3],
    renderer);
  //std::cout << "Pick result " << result << std::endl;

  int markerCount = 0;
  if (result)
    {
    vtkNew<vtkIdList> cellIdList;

    // Process each object detected by the area picker
    vtkProp3DCollection *props = areaPicker->GetProp3Ds();
    // std::cout << "Number of prop3d objects: " << props->GetNumberOfItems()
    //           << std::endl;
    // Also need to keep track of non-marker props
    std::set<vtkProp*> nonMarkerProps;
    props->InitTraversal();
    vtkProp *prop = props->GetNextProp3D();
    for (; prop; prop = props->GetNextProp3D())
      {
      // Find corresponding map feature
      FeatureMap::iterator findIter =
        this->Internal->FeaturePickMap.find(prop);
      if (findIter == this->Internal->FeaturePickMap.end())
        {
        vtkWarningMacro("vtkProp is *not* in the feature-pick dictionary");
        continue;
        }
      vtkFeature *feature = findIter->second;
      cellIdList->Reset();

      // Handling depends on feature type
      vtkRasterFeature *rasterFeature =
        vtkRasterFeature::SafeDownCast(feature);
      if (rasterFeature)
        {
        selection->AddFeature(feature);
        prop->PickableOff();
        nonMarkerProps.insert(prop);
        continue;
        }

      vtkMapMarkerSet *markerFeature = vtkMapMarkerSet::SafeDownCast(feature);
      if (markerFeature)
        {
        markerCount++;
        continue;
        }

      vtkPolydataFeature *polyFeature = vtkPolydataFeature::SafeDownCast(feature);
      if (polyFeature)
        {
        this->PickPolyDataCells(
          prop, areaPicker->GetFrustum(), cellIdList.GetPointer());
        if (cellIdList->GetNumberOfIds() > 0)
          {
          selection->AddFeature(feature, cellIdList.GetPointer());
          }
        continue;
        }

      // (else)
      // Not a raster, marker, or polydata feature
      // So call the default method
      feature->PickItems(renderer, displayCoords, selection);
      prop->PickableOff();
      nonMarkerProps.insert(prop);
      }  // for (props)

    if (markerCount > 0)
      {
      this->PickMarkers(renderer, displayCoords, selection);
      }

    // Restore pickable state to props
    std::set<vtkProp*>::iterator iter = nonMarkerProps.begin();
    for (; iter != nonMarkerProps.end(); iter++)
      {
      prop = *iter;
      prop->PickableOn();
      }
    }  // if (result)
}

// ------------------------------------------------------------
void vtkGeoMapFeatureSelector::
PickPolyDataCells(vtkProp *prop, vtkPlanes *frustum, vtkIdList *idList)
{
  idList->Reset();
  vtkActor *actor = vtkActor::SafeDownCast(prop);
  if (!actor)
    {
    vtkWarningMacro("vtkProp is *not* a vtkActor");
    return;
    }

  // Get the actor's polydata
  vtkObject *object = actor->GetMapper()->GetInput();
  vtkPolyData *polyData = vtkPolyData::SafeDownCast(object);
  if (!polyData)
    {
    vtkWarningMacro("Picked vtkActor is *not* displaying vtkPolyData");
    return;
    }
  // std::cout << "Number of PolyData cells: " << polyData->GetNumberOfCells()
  //           << std::endl;

  // Check actor for transform
  vtkPoints *originalPoints = NULL;
  if (!actor->GetIsIdentity())
    {
    // Save original frustum geometry
    originalPoints = vtkPoints::New();
    originalPoints->DeepCopy(frustum->GetPoints());

    // If there is a transform, apply it's inverse to the frustum
    vtkMatrix4x4 *actorMatrix = actor->GetMatrix();
    vtkNew<vtkMatrix4x4> frustumMatrix;
    vtkMatrix4x4::Invert(actorMatrix, frustumMatrix.GetPointer());

    // Create new set of frustum points
    vtkNew<vtkPoints> adjustedPoints;
    adjustedPoints->SetNumberOfPoints(originalPoints->GetNumberOfPoints());
    double fromPoint[] = {0.0, 0.0, 0.0, 1.0};
    double toPoint[] = {0.0, 0.0, 0.0, 1.0};

    for (vtkIdType i=0; i<originalPoints->GetNumberOfPoints(); i++)
      {
      originalPoints->GetPoint(i, fromPoint);
      frustumMatrix->MultiplyPoint(fromPoint, toPoint);
      adjustedPoints->SetPoint(i, toPoint);
      }
    frustum->SetPoints(adjustedPoints.GetPointer());
    }

  // Find all picked cells for this polydata
  vtkNew<vtkExtractSelectedFrustum> extractor;
  extractor->SetInputData(polyData);
  extractor->PreserveTopologyOff();
  extractor->SetFrustum(frustum);
  extractor->Update();
  vtkDataObject *output = extractor->GetOutput();
  vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::SafeDownCast(output);
  //std::cout << "Number of UGrid cells: " << ugrid->GetNumberOfCells() << std::endl;

  // Restore frustum to its original geometry
  if (originalPoints)
    {
    frustum->SetPoints(originalPoints);
    originalPoints->Delete();
    }

  // Make sure that the extractor "worked"
  if (ugrid->GetNumberOfCells() < 1)
    {
    vtkWarningMacro("Expected to select one or more cells.");
    return;
    }

  // Loop over the selected cells
  vtkAbstractArray *array =
    ugrid->GetCellData()->GetAbstractArray("vtkOriginalCellIds");
  //std::cout << "array " << array->GetClassName() << std::endl;
  vtkIdTypeArray *idArray = vtkIdTypeArray::SafeDownCast(array);
  for (int i=0; i<ugrid->GetNumberOfCells(); i++)
    {
    // Get the input polydata id
    vtkIdType id = idArray->GetValue(i);
    idList->InsertNextId(id);
    }
}

// ------------------------------------------------------------
void vtkGeoMapFeatureSelector::PickMarkers(
  vtkRenderer *renderer,
  int displayCoords[4],
  vtkGeoMapSelection *selection)
{
  // Finds marker/cluster features at given display coords
  // Appends its results to the input selection (i.e., does NOT reset it).
  // Calling code *should* set all other feature types unpickable.

  // Sanity check: Cannot use vtkHardwareSelector with AAFrames
  if (renderer->GetRenderWindow()->GetAAFrames() > 0)
    {
    vtkWarningMacro("Render window has anti-aliasing frames set (AAFrames)"
                    << ", so selection of markers may not work");
    return;
    }

  this->Internal->Selector->SetFieldAssociation(
    vtkDataObject::FIELD_ASSOCIATION_POINTS);
  this->Internal->Mode =
    vtkGeoMapFeatureSelectorInternal::Selection::RUBBER_BAND;
  this->Internal->PolygonPoints = displayCoords;

  FeatureMap featMap = this->Internal->FeaturePickMap;
  this->IncrementalSelect(selection, renderer, featMap);
}

// ------------------------------------------------------------
void vtkGeoMapFeatureSelector::IncrementalSelect(vtkGeoMapSelection* selection,
  vtkRenderer* ren, FeatureMap& featMap)
{
  std::set<vtkProp*> propSet;  // remembers props marked unpickable
  bool done = false;

  // Selection passes
  while (!done)
    {
    vtkSelection* hwSelection = this->Internal->DoSelect(ren);

    if (!hwSelection)  // null if grahpics < 24 bit
      {
      vtkWarningMacro("vtkHardwareSelector::Select() returned null.");
      done = true;
      break;
      }

    if (hwSelection->GetNumberOfNodes() < 1)
      {
      hwSelection->Delete();
      done = true;
      break;
      }

    // Process picked props
    for (int i=0; i<hwSelection->GetNumberOfNodes(); i++)
      {
      vtkSelectionNode *node = hwSelection->GetNode(i);
      vtkObjectBase *base = node->GetProperties()->Get(vtkSelectionNode::PROP());
      if (!base)
        {
        // Workaround intermittent problem with output of hardware
        // selector, currently unresolved (April 2017)
        vtkWarningMacro(
          "(.cxx:" << __LINE__ << ")  vtkSelectionNode PROP missing");
#ifndef NDEBUG
        node->Print(std::cout);
#endif
        continue;
        }
      vtkProp* prop = vtkProp::SafeDownCast(base);
      propSet.insert(prop);
      prop->PickableOff();  // don't pick again (in this method)

      FeatureMap::const_iterator iter = featMap.find(prop);
      if (iter == featMap.cend())
        continue;

///TODO Uncomment this once both rubber-band & polygon selection paths unify
//      auto raster = vtkRasterFeature::SafeDownCast(feature);
//      if (raster)
//      {
//        vtkFeature* feature = iter->second;
//        selection->AddFeature(feature);
//        continue;
//      }

      bool selected = false;
      if (this->Internal->Selector->GetFieldAssociation() ==
        vtkDataObject::FIELD_ASSOCIATION_POINTS)
        if (this->SelectMarkerSet(selection, node, featMap, iter))
          continue;

      if (this->Internal->Selector->GetFieldAssociation() ==
        vtkDataObject::FIELD_ASSOCIATION_CELLS)
        if (this->SelectPolyData(selection, node, featMap, iter))
          continue;

///TODO Uncomment this once both rubber-band & polygon selection paths unify
//      feature->PickItems(ren, this->Internal->PolygonBounds, selection);
      }
    hwSelection->Delete();
    }

  // Restore pickable state on all props
  std::set<vtkProp*>::iterator iter = propSet.begin();
  for (; iter != propSet.end(); iter++)
    {
    vtkProp* prop = *iter;
    prop->PickableOn();
    }
}

// ------------------------------------------------------------
bool vtkGeoMapFeatureSelector::SelectPolyData(vtkGeoMapSelection* selection,
  vtkSelectionNode* node, FeatureMap& map, FeatureMap::const_iterator& it)
{
  vtkPolydataFeature* polyFeature =
    dynamic_cast<vtkPolydataFeature*>(it->second);
  if (!polyFeature)
    return false;

  vtkNew<vtkIdList> polyIdList;
  vtkAbstractArray* abs = node->GetSelectionList();
  vtkIdTypeArray* ids = vtkIdTypeArray::SafeDownCast(abs);

  for (vtkIdType id = 0; id < ids->GetNumberOfTuples(); id++)
  {
    const vtkIdType primitiveId = ids->GetValue(id);
    const vtkIdType localId = polyFeature->CellIdToLocalId(primitiveId);
    polyIdList->InsertNextId(localId);
  }

  selection->AddFeature(polyFeature, polyIdList.GetPointer());
  map.erase(it);
  return true;
}

// ------------------------------------------------------------
bool vtkGeoMapFeatureSelector::SelectMarkerSet(vtkGeoMapSelection* selection,
  vtkSelectionNode* node, FeatureMap& map, FeatureMap::const_iterator& it)
{
  vtkMapMarkerSet* markerFeature = vtkMapMarkerSet::SafeDownCast(it->second);
  if (!markerFeature)
    return false;

  vtkNew<vtkIdList> markerIdList;
  vtkNew<vtkIdList> clusterIdList;
  vtkAbstractArray* abs = node->GetSelectionList();
  vtkIdTypeArray* ids = vtkIdTypeArray::SafeDownCast(abs);

  for (vtkIdType id=0; id<ids->GetNumberOfTuples(); id++)
  {
    vtkIdType displayId = ids->GetValue(id);
    vtkIdType markerId = markerFeature->GetMarkerId(displayId);
    if (markerId >= 0)
    {
      markerIdList->InsertNextId(markerId);
    }
    else
    {
      vtkIdType clusterId = markerFeature->GetClusterId(displayId);
      clusterIdList->InsertNextId(clusterId);
    }
  }

  selection->AddFeature(markerFeature, markerIdList.GetPointer(),
                        clusterIdList.GetPointer());
  map.erase(it);
  return true;
}
