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

#include <vtkObjectFactory.h>
#include <vtkSetGet.h>

#include <algorithm>
#include <iterator>
#include <vector>

vtkStandardNewMacro(vtkFeatureLayer);

//----------------------------------------------------------------------------
class vtkFeatureLayer::vtkInternal
{
public:
  std::vector<vtkFeature*> Features;
};

//----------------------------------------------------------------------------
vtkFeatureLayer::vtkFeatureLayer():
  Impl(new vtkInternal())
{
}

//----------------------------------------------------------------------------
vtkFeatureLayer::~vtkFeatureLayer()
{
  const std::size_t size = this->Impl->Features.size();
  for(std::size_t i=0; i < size; ++i)
    {
    //invoke delete on each vtk class in the vector
    vtkFeature* f = this->Impl->Features[i];
    if(f)
      {
      f->Delete();
      }
    }
  delete this->Impl;
}

//----------------------------------------------------------------------------
void vtkFeatureLayer::PrintSelf(std::ostream&, vtkIndent)
{
  // TODO
}

//----------------------------------------------------------------------------
void vtkFeatureLayer::AddFeature(vtkFeature* feature)
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

  std::vector<vtkFeature*>::iterator itr = std::find(
    this->Impl->Features.begin(), this->Impl->Features.end(), feature);
  if (itr == this->Impl->Features.end())
    {
    feature->SetLayer(this);
    this->Impl->Features.push_back(feature);
    }

  feature->Init();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkFeatureLayer::RemoveFeature(vtkFeature* feature)
{
  if (!feature)
    {
    return;
    }

  feature->CleanUp();
  typedef std::vector< vtkFeature* >::iterator iter;
  iter found_iter =  std::find(this->Impl->Features.begin(),
                               this->Impl->Features.end(), feature);

  //now that we have found the feature delete it, leaving a dangling pointer
  (*found_iter)->Delete();

  //now resize the array to not hold the empty feature
  this->Impl->Features.erase( std::remove(this->Impl->Features.begin(),
                              this->Impl->Features.end(), feature ));
}

//----------------------------------------------------------------------------
void vtkFeatureLayer::Update()
{
  for (size_t i = 0; i < this->Impl->Features.size(); i += 1)
    {
    this->Impl->Features[i]->Update();
    }
}
