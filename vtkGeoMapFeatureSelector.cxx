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
#include "vtkPolydataFeature.h"

#include <vtkAbstractArray.h>
#include <vtkActor.h>
#include <vtkAreaPicker.h>
#include <vtkCellData.h>
#include <vtkExtractSelectedFrustum.h>
#include <vtkIdList.h>
#include <vtkIdTypeArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkProp3D.h>
#include <vtkProp3DCollection.h>
#include <vtkUnstructuredGrid.h>

#include <map>
#include <string>

vtkStandardNewMacro(vtkGeoMapFeatureSelector);

//-----------------------------------------------------------------------------
class vtkGeoMapFeatureSelector::vtkGeoMapFeatureSelectorInternal
{
public:
  // Map feature id to vtkFeature instance
  std::map<std::string, vtkFeature*> FeatureIdMap;

  // Map vtkProp3D to feature id
  std::map<vtkProp3D*, std::string> FeaturePickMap;

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

//-----------------------------------------------------------------------------
void vtkGeoMapFeatureSelector::
PickPoint(vtkRenderer *renderer, int displayCoords[2],
          vtkGeoMapSelection *selection)
{
  // Clear internal selection
  selection->Clear();

  vtkNew<vtkAreaPicker> areaPicker;
  //int result = areaPicker->Pick(pos[0], pos[1], 0.0, this->Renderer);
  // Expand area pickers frustum size to increase reliability
  const double offset = 4.0;
  int result =
    areaPicker->AreaPick(displayCoords[0]-offset, displayCoords[1]-offset,
                         displayCoords[0]+offset, displayCoords[1]+offset,
                         renderer);
  //std::cout << "Pick result " << result << std::endl;
  if (result)
    {
    vtkNew<vtkIdList> idList;

    // Process each object detected by the area picker
    vtkProp3DCollection *props = areaPicker->GetProp3Ds();
    // std::cout << "Number of prop3d objects: " << props->GetNumberOfItems()
    //           << std::endl;
    props->InitTraversal();
    vtkProp3D *prop = props->GetNextProp3D();
    for (; prop; prop = props->GetNextProp3D())
      {
      // If not in the feature map, skip it
      if (this->Internal->FeaturePickMap.count(prop) < 1)
        {
        continue;
        }

      std::string featureId = this->Internal->FeaturePickMap[prop];
      vtkFeature *feature = this->Internal->FeatureIdMap[featureId];

      // For polydata features, find specific cells
      vtkPolydataFeature *polydataFeature =
        vtkPolydataFeature::SafeDownCast(feature);
      if (polydataFeature)
        {
        idList->Reset();
        this->PickPolyDataCells(
          prop, areaPicker->GetFrustum(), idList.GetPointer());
        if (idList->GetNumberOfIds())
          {
          selection->AddFeature(feature, idList.GetPointer());
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
PickPolyDataCells(vtkProp3D *prop, vtkPlanes *frustum, vtkIdList *idList)
{
  idList->Reset();
  vtkActor *actor = vtkActor::SafeDownCast(prop);
  if (!actor)
    {
    vtkWarningMacro("vtkProp3D is *not* a vtkActor");
    return;
    }

  // Find all cell ids for given actor
  std::string featureId = this->Internal->FeaturePickMap[prop];
  vtkFeature *feature = this->Internal->FeatureIdMap[featureId];

  // Get the actor's polydata
  vtkObject *object = actor->GetMapper()->GetInput();
  //std::cout << "Input " << object->GetClassName() << std::endl;
  vtkPolyData *polyData = vtkPolyData::SafeDownCast(object);
  if (!polyData)
    {
    vtkWarningMacro("Picked vtkActor is *not* displaying vtkPolyData");
    return;
    }

  // Find all picked cells for this polydata
  vtkNew<vtkExtractSelectedFrustum> extractor;
  extractor->SetInputData(polyData);
  extractor->PreserveTopologyOff();
  extractor->SetFrustum(frustum);
  extractor->Update();
  vtkDataObject *output = extractor->GetOutput();
  //std::cout << "Output " << output->GetClassName() << std::endl;
  vtkUnstructuredGrid *ugrid = vtkUnstructuredGrid::SafeDownCast(output);
  //std::cout << "#cells: " << ugrid->GetNumberOfCells() << std::endl;

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
