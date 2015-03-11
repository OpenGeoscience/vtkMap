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

#include "vtkGeoMapSelection.h"
#include "vtkPolydataFeature.h"

#include <vtkCollection.h>
#include <vtkIdList.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

#include <map>

vtkStandardNewMacro(vtkGeoMapSelection);

//-----------------------------------------------------------------------------
class vtkGeoMapSelection::vtkGeoMapSelectionInternal
{
public:
  // Store list of selected cell ids for polydata features
  std::map<vtkFeature*, vtkIdList *> PolyDataCellIdMap;
};

//-----------------------------------------------------------------------------
vtkGeoMapSelection::vtkGeoMapSelection()
{
  this->Internal = new vtkGeoMapSelectionInternal;
  this->SelectedFeatures = vtkCollection::New();
}


//-----------------------------------------------------------------------------
vtkGeoMapSelection::~vtkGeoMapSelection()
{
  this->Clear();
  this->SelectedFeatures->Delete();
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkGeoMapSelection::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
vtkIdList *vtkGeoMapSelection::
GetSelectedCellIds(vtkPolydataFeature *feature) const
{
  vtkSmartPointer<vtkIdList> returnList = vtkSmartPointer<vtkIdList>::New();
  std::map<vtkFeature*, vtkIdList *>::iterator iter =
    this->Internal->PolyDataCellIdMap.find(feature);
  if (iter != this->Internal->PolyDataCellIdMap.end())
    {
    vtkIdList *featureList = iter->second;
    returnList->DeepCopy(featureList);
    }
  return returnList;
}

//-----------------------------------------------------------------------------
bool vtkGeoMapSelection::IsEmpty()
{
  return this->SelectedFeatures->GetNumberOfItems() == 0;
}

//-----------------------------------------------------------------------------
void vtkGeoMapSelection::Clear()
{
  std::map<vtkFeature*, vtkIdList *>::iterator iter =
    this->Internal->PolyDataCellIdMap.begin();
  for (; iter != this->Internal->PolyDataCellIdMap.end(); iter++)
    {
    vtkIdList *idList = iter->second;
    idList->Delete();
    }
  this->Internal->PolyDataCellIdMap.clear();
  this->SelectedFeatures->RemoveAllItems();
}

//-----------------------------------------------------------------------------
void vtkGeoMapSelection::AddFeature(vtkFeature *feature)
{
  this->SelectedFeatures->AddItem(feature);
}

//-----------------------------------------------------------------------------
void vtkGeoMapSelection::AddFeature(vtkFeature *feature, vtkIdList *ids)
{
  this->SelectedFeatures->AddItem(feature);
  vtkIdList *idsCopy = vtkIdList::New();
  idsCopy->DeepCopy(ids);
  this->Internal->PolyDataCellIdMap[feature] = idsCopy;
}
