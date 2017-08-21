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

#include <vtk_glew.h>
#include <vtkInformation.h>
#include <vtkObjectFactory.h>
#include <vtkProp.h>
#include <vtkRenderer.h>
#include <vtkRenderState.h>
#include <vtkSetGet.h>

#include "vtkGeoMapLayerPass.h"
#include "vtkLayer.h"


vtkStandardNewMacro(vtkGeoMapLayerPass)

vtkGeoMapLayerPass::vtkGeoMapLayerPass()
: vtkRenderPass()
{
}

vtkGeoMapLayerPass::~vtkGeoMapLayerPass()
{
}

void vtkGeoMapLayerPass::Render(const vtkRenderState* state)
{
  this->FilterLayerProps(state);

  // Up to this point, the depth test is enabled (probably enabled somewhere within
  // vtkRenderer). It is disabled next in order to overwrite previously rendered
  // layers (since the layering order is defined by vtkMap::LayerCollection anyway).
  glDisable(GL_DEPTH_TEST);

  this->NumberOfRenderedProps = 0;

  // Only certain layers currently use multiple actors (e.g. markers & their shadows,
  // etc.). Because depth-test is disabled above it is necessary to do something else
  // to correctly order actors within a layer. Addressing only markers front (opaque)
  // and shadows behind (translucent), translucent geometry is rendered first and then
  // opaque geometry on top. A more appropriate way to handle this would be to clear
  // the depth buffer for every layer and enable depth testing.
  this->RenderTranslucentGeometry(state);
  this->RenderOpaqueGeometry(state);
  this->RenderOverlay(state);
}

void vtkGeoMapLayerPass::FilterLayerProps(const vtkRenderState* state)
{
  const size_t maxCount = state->GetPropArrayCount();
  this->LayerProps.reserve(maxCount);
  this->LayerProps.clear();

  for (size_t i = 0; i < maxCount; i++)
  {
    vtkProp* prop = state->GetPropArray()[i];
    vtkInformation* keys = prop->GetPropertyKeys();
    if (keys->Has(vtkLayer::ID()))
    {
      const int id = keys->Get(vtkLayer::ID());
      if (id == this->LayerId)
      {
        this->LayerProps.push_back(prop);
      }
    }
  }
}

void vtkGeoMapLayerPass::ReleaseGraphicsResources(vtkWindow* win)
{
  Superclass::ReleaseGraphicsResources(win);
}

void vtkGeoMapLayerPass::RenderOpaqueGeometry(const vtkRenderState* state)
{
  for (auto& prop : this->LayerProps)
  {
    const int count = prop->RenderOpaqueGeometry(state->GetRenderer());
    this->NumberOfRenderedProps += count;
  }
}

void vtkGeoMapLayerPass::RenderTranslucentGeometry(const vtkRenderState* state)
{
  for (auto& prop : this->LayerProps)
  {
    const int count =
      prop->RenderTranslucentPolygonalGeometry(state->GetRenderer());
    this->NumberOfRenderedProps += count;
  }
}

void vtkGeoMapLayerPass::RenderOverlay(const vtkRenderState* state)
{
  for (auto& prop : this->LayerProps)
  {
    const int count = prop->RenderOverlay(state->GetRenderer());
    this->NumberOfRenderedProps += count;
  }
}

void vtkGeoMapLayerPass::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
