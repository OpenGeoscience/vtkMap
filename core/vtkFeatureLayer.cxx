/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFeatureLayer.h"
#include "vtkFeature.h"

#include <vtkCollection.h>
#include <vtkObjectFactory.h>
#include <vtkSetGet.h>
#include <vtkSmartPointer.h>

#include <algorithm>
#include <iterator>
#include <vector>

vtkStandardNewMacro(vtkFeatureLayer);

//----------------------------------------------------------------------------
class vtkFeatureLayer::vtkInternal
{
public:
  std::vector<vtkSmartPointer<vtkFeature>> Features;
  vtkSmartPointer<vtkCollection> FeatureCollection;

  vtkInternal()
  {
    this->FeatureCollection = vtkSmartPointer<vtkCollection>::New();
  }
};

//----------------------------------------------------------------------------
vtkFeatureLayer::vtkFeatureLayer()
  : Impl(new vtkInternal())
{
}

//----------------------------------------------------------------------------
vtkFeatureLayer::~vtkFeatureLayer() {}

//----------------------------------------------------------------------------
void vtkFeatureLayer::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "vtkFeatureLayer"
     << "\n"
     << indent << "Number Of Features: " << this->Impl->Features.size()
     << std::endl;
}

//----------------------------------------------------------------------------
void vtkFeatureLayer::UnRegister(vtkObjectBase* o)
{
  if (this->GetReferenceCount() > 1)
  {
    this->Superclass::UnRegister(o);
    return;
  }

  // (else) Delete features before calling superclass Delete() method
  this->Impl->FeatureCollection->RemoveAllItems();
  const std::size_t size = this->Impl->Features.size();
  for (std::size_t i = 0; i < size; ++i)
  {
    //invoke delete on each vtk class in the vector
    if (this->Impl->Features[i])
    {
      this->Impl->Features[i]->CleanUp();
    }
  }
  delete this->Impl;

  this->Superclass::UnRegister(o);
}

//----------------------------------------------------------------------------
void vtkFeatureLayer::AddFeature(vtkSmartPointer<vtkFeature> feature)
{
  if (!feature)
  {
    return;
  }

  if (!this->Renderer)
  {
    vtkWarningMacro("Cannot add vtkFeature to vtkFeatureLayer"
      << " because vtkFeatureLayer has not been initialized correctly."
      << " Make sure this layer has been added to vtkMap"
      << " *before* adding features."
      << " Also make sure renderer has been set on the vtkMap.");
    return;
  }

  auto itr = std::find(
    this->Impl->Features.begin(), this->Impl->Features.end(), feature);
  if (itr == this->Impl->Features.end())
  {
    feature->SetLayer(this);
    this->Impl->Features.push_back(feature);
  }

  feature->Init();

  // Notify the map
  this->Map->FeatureAdded(feature.GetPointer());

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkFeatureLayer::RemoveFeature(vtkSmartPointer<vtkFeature> feature)
{
  if (!feature)
  {
    return;
  }

  auto found_iter = std::find(
    this->Impl->Features.begin(), this->Impl->Features.end(), feature);
  if (found_iter != this->Impl->Features.end())
  {
    // Notify the map first
    this->Map->ReleaseFeature(feature.GetPointer());

    feature->CleanUp();

    this->Impl->Features.erase(found_iter);
  }
}

//----------------------------------------------------------------------------
vtkCollection* vtkFeatureLayer::GetFeatures()
{
  // The internal feature collection could be cached, but for,
  // we'll rebuild it every time.
  this->Impl->FeatureCollection->RemoveAllItems();
  auto iter = this->Impl->Features.begin();
  for (; iter != this->Impl->Features.end(); iter++)
  {
    vtkFeature* const feature = iter->GetPointer();
    this->Impl->FeatureCollection->AddItem(feature);
  }
  return this->Impl->FeatureCollection;
}

//----------------------------------------------------------------------------
void vtkFeatureLayer::Update()
{
  for (size_t i = 0; i < this->Impl->Features.size(); i += 1)
  {
    this->Impl->Features[i]->Update();
  }
}
