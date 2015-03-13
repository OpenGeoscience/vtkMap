/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleMap.cxx

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
#include "vtkPolydataFeature.h"

#include <vtkAbstractArray.h>
#include <vtkActor.h>
#include <vtkCellData.h>
#include <vtkExtractSelectedFrustum.h>
#include <vtkIdList.h>
#include <vtkIdTypeArray.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPlanes.h>
#include <vtkPolyData.h>
#include <vtkProp.h>
#include <vtkProp3DCollection.h>
#include <vtkRenderedAreaPicker.h>
#include <vtkUnstructuredGrid.h>

#include <map>
#include <string>

vtkStandardNewMacro(vtkGeoMapFeatureSelector);

//-----------------------------------------------------------------------------
class vtkGeoMapFeatureSelector::vtkGeoMapFeatureSelectorInternal
{
public:
  // Map vtkProp to vtkFeature
  std::map<vtkProp*, vtkFeature*> FeaturePickMap;

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
void vtkGeoMapFeatureSelector::
PickArea(vtkRenderer *renderer, int displayCoords[4],
         vtkGeoMapSelection *selection)
{
  // Clear selection
  selection->Clear();

  // Use rendered-area picker to minimize hits on marker features
  vtkNew<vtkRenderedAreaPicker> areaPicker;
  int result = areaPicker->AreaPick(
    displayCoords[0], displayCoords[1],
    displayCoords[2], displayCoords[3],
    renderer);
  //std::cout << "Pick result " << result << std::endl;

  if (result)
    {
    vtkNew<vtkIdList> cellIdList;
    vtkNew<vtkIdList> markerIdList;

    // Process each object detected by the area picker
    vtkProp3DCollection *props = areaPicker->GetProp3Ds();
    // std::cout << "Number of prop3d objects: " << props->GetNumberOfItems()
    //           << std::endl;
    props->InitTraversal();
    vtkProp3D *prop = props->GetNextProp3D();
    for (; prop; prop = props->GetNextProp3D())
      {
      // Find corresponding map feature
      std::map<vtkProp*, vtkFeature*>::iterator findIter =
        this->Internal->FeaturePickMap.find(prop);
      if (findIter == this->Internal->FeaturePickMap.end())
        {
        vtkWarningMacro("vtkProp is *not* in the feature-pick dictionary");
        continue;
        }
      vtkFeature *feature = findIter->second;
      cellIdList->Reset();
      markerIdList->Reset();

      // Handling depends on feature type
      vtkMapMarkerSet *markerFeature = vtkMapMarkerSet::SafeDownCast(feature);
      vtkPolydataFeature *polydataFeature =
        vtkPolydataFeature::SafeDownCast(feature);
      if (markerFeature)
        {
        // Process markers first, since they are also polydata features
        this->PickPolyDataCells(
          prop, areaPicker->GetFrustum(), cellIdList.GetPointer());

        if (cellIdList->GetNumberOfIds() > 0)
          {
          // Get list of marker ids for given cell ids
          markerFeature->GetMarkerIds(cellIdList.GetPointer(),
                                      markerIdList.GetPointer());
          selection->AddFeature(feature, markerIdList.GetPointer());
          }
        }
      else if (polydataFeature)
        {
        // Get cell ids
        this->PickPolyDataCells(
          prop, areaPicker->GetFrustum(), cellIdList.GetPointer());
        if (cellIdList->GetNumberOfIds() > 0)
          {
          selection->AddFeature(feature, cellIdList.GetPointer());
          }
        }
      else
        {
        selection->AddFeature(feature);
        }

      }  // while (props)
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
