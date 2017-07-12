/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkValuePass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <algorithm>

#include "vtkGeoMapLayerPass.h"
#include <vtkObjectFactory.h>
#include <vtkProp.h>
#include <vtkRenderer.h>
#include <vtkRenderState.h>
#include <vtkSetGet.h>


vtkStandardNewMacro(vtkGeoMapLayerPass)

vtkGeoMapLayerPass::vtkGeoMapLayerPass()
: vtkRenderPass()
{
  std::cout << "->>> Constructing vtkGeoMapLayerPass!!\n";
}

vtkGeoMapLayerPass::~vtkGeoMapLayerPass()
{
  for (auto& prop : this->Actors)
  {
    prop->UnRegister(this);
  }
}

void vtkGeoMapLayerPass::Render(const vtkRenderState* state)
{
  this->NumberOfRenderedProps = 0;

  this->RenderOpaqueGeometry(state);
  this->RenderTranslucentGeometry(state);
}

void vtkGeoMapLayerPass::ReleaseGraphicsResources(vtkWindow* win)
{
  Superclass::ReleaseGraphicsResources(win);
}

void vtkGeoMapLayerPass::RenderOpaqueGeometry(const vtkRenderState* state)
{
  for (auto& prop : this->Actors)
  {
    const int count = prop->RenderOpaqueGeometry(state->GetRenderer());
    this->NumberOfRenderedProps += count;
  }
}

void vtkGeoMapLayerPass::RenderTranslucentGeometry(const vtkRenderState* state)
{
  for (auto& prop : this->Actors)
  {
    const int count =
      prop->RenderTranslucentPolygonalGeometry(state->GetRenderer());
    this->NumberOfRenderedProps += count;
  }
}

void vtkGeoMapLayerPass::RenderOverlay(const vtkRenderState* state)
{
  for (auto& prop : this->Actors)
  {
    const int count = prop->RenderOverlay(state->GetRenderer());
    this->NumberOfRenderedProps += count;
  }
}

void vtkGeoMapLayerPass::AddActor(vtkProp* prop)
{
  prop->Register(this);
  this->Actors.push_back(prop);
  this->Modified();
}

void vtkGeoMapLayerPass::RemoveActor(vtkProp* prop)
{
  auto it = std::find(this->Actors.begin(), this->Actors.end(), prop);
  if (it != this->Actors.end())
  {
    (*it)->UnRegister(this);
    this->Actors.erase(it);
    this->Modified();
  }
}

void vtkGeoMapLayerPass::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
