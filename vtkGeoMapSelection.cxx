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
#include "vtkFeature.h"
#include "vtkMapMarkerSet.h"
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
  // Store list of selected component ids for individual features
  // For vtkPolydataFeature, ids are polydata cell ids
  // For vtkMapMarkerFeature, ids are clustering-node ids
  std::map<vtkFeature*, vtkIdList *> ComponentIdMap;

  // Second std:: map for cluster components in map marker sets
  std::map<vtkFeature*, vtkIdList*> ClusterIdMap;
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
bool vtkGeoMapSelection::
GetPolyDataCellIds(vtkFeature *feature, vtkIdList *idList) const
{
  idList->Reset();
  // Only defined for vtkPolydataFeature types that are *not* markers
  vtkPolydataFeature *polyFeature = vtkPolydataFeature::SafeDownCast(feature);
  vtkMapMarkerSet *markerFeature = vtkMapMarkerSet::SafeDownCast(feature);
  if (markerFeature || !polyFeature)
    {
    return false;
    }

  // Copy the list of cell ids
  std::map<vtkFeature*, vtkIdList *>::iterator iter =
    this->Internal->ComponentIdMap.find(feature);
  if (iter != this->Internal->ComponentIdMap.end())
    {
    idList->DeepCopy(iter->second);
    }
  return true;
}

//-----------------------------------------------------------------------------
bool vtkGeoMapSelection::
GetMapMarkerIds(vtkFeature *feature, vtkIdList *markerIdList,
             vtkIdList *clusterIdList) const
{
  markerIdList->Reset();
  clusterIdList->Reset();

  vtkMapMarkerSet *markerFeature = vtkMapMarkerSet::SafeDownCast(feature);
  if (!markerFeature)
    {
    return false;
    }

  std::map<vtkFeature*, vtkIdList *>::iterator iter;

  // Traverse ids and get info from marker feature
  iter = this->Internal->ComponentIdMap.find(feature);
  if (iter != this->Internal->ComponentIdMap.end())
    {
    markerIdList->DeepCopy(iter->second);
    }

  // Repeat for clusters
  iter = this->Internal->ClusterIdMap.find(feature);
  if (iter != this->Internal->ComponentIdMap.end())
    {
    clusterIdList->DeepCopy(iter->second);
    }

  return true;
}

//-----------------------------------------------------------------------------
bool vtkGeoMapSelection::IsEmpty()
{
  return this->SelectedFeatures->GetNumberOfItems() == 0;
}

//-----------------------------------------------------------------------------
void vtkGeoMapSelection::Clear()
{
  std::map<vtkFeature*, vtkIdList *>::iterator iter;
  iter = this->Internal->ComponentIdMap.begin();
  for (; iter != this->Internal->ComponentIdMap.end(); iter++)
    {
    vtkIdList *idList = iter->second;
    idList->Delete();
    }
  this->Internal->ComponentIdMap.clear();

  iter = this->Internal->ClusterIdMap.begin();
  for (; iter != this->Internal->ClusterIdMap.end(); iter++)
    {
    vtkIdList *idList = iter->second;
    idList->Delete();
    }
  this->Internal->ClusterIdMap.clear();

  this->SelectedFeatures->RemoveAllItems();
}

//-----------------------------------------------------------------------------
void vtkGeoMapSelection::AddFeature(vtkFeature *feature)
{
  // For features with no components
  this->SelectedFeatures->AddItem(feature);
}

//-----------------------------------------------------------------------------
void vtkGeoMapSelection::AddFeature(vtkFeature *feature, vtkIdList *cellIds)
{
  // For polydata features (which have cells)
  this->SelectedFeatures->AddItem(feature);
  vtkIdList *idsCopy = vtkIdList::New();
  idsCopy->DeepCopy(cellIds);
  this->Internal->ComponentIdMap[feature] = idsCopy;
}

//-----------------------------------------------------------------------------
void vtkGeoMapSelection::
AddFeature(vtkFeature *feature, vtkIdList *markerIds, vtkIdList *clusterIds)
{
  // For map marker features, which have markers & clusters
  this->SelectedFeatures->AddItem(feature);

  vtkIdList *markerIdsCopy = vtkIdList::New();
  markerIdsCopy->DeepCopy(markerIds);
  this->Internal->ComponentIdMap[feature] = markerIdsCopy;

  vtkIdList *clusterIdsCopy = vtkIdList::New();
  clusterIdsCopy->DeepCopy(clusterIds);
  this->Internal->ClusterIdMap[feature] = clusterIdsCopy;
}
