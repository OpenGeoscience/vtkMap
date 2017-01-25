/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFeature.h"

//----------------------------------------------------------------------------
vtkFeature::vtkFeature() : vtkObject()
{
  this->Id = 0;
  this->Visibility = 1;
  this->Bin = Visible;
  this->Gcs = 0;
  this->Layer = 0;

  this->SetGcs("EPSG4326");
}

//----------------------------------------------------------------------------
vtkFeature::~vtkFeature()
{
  delete [] this->Gcs;
}

//----------------------------------------------------------------------------
void vtkFeature::SetLayer(vtkFeatureLayer* layer)
{
  //set our weak pointer to look at the layer passed in.
  if(layer)
    { //don't allow null layer
    this->Layer = layer;
    }
}

//----------------------------------------------------------------------------
void vtkFeature::PrintSelf(std::ostream&, vtkIndent)
{
  // TODO
}

//----------------------------------------------------------------------------
bool vtkFeature::IsVisible()
{
  // Visible only if both layer and feature visibility flags are set
  return this->Layer->GetVisibility() && this->Visibility;
}

//----------------------------------------------------------------------------
vtkProp *vtkFeature::PickProp()
{
  return NULL;
}

//----------------------------------------------------------------------------
void vtkFeature::PickItems(
  vtkRenderer *renderer,
  int displayCoords[4],
  vtkGeoMapSelection *selection)
{
  vtkWarningMacro(
    "vtkFeature::PickItems() called -- should be overridden in "
    << this->GetClassName());
}
