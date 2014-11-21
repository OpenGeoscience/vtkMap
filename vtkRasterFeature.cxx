/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRasterFeature.h"
#include "vtkImageActor.h"
#include "vtkMercator.h"

#include <vtkNew.h>
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkRasterFeature);

//----------------------------------------------------------------------------
vtkRasterFeature::vtkRasterFeature() : vtkFeature()
{
  this->ImageData = NULL;
  this->Actor = vtkImageActor::New();
  this->Mapper = this->Actor->GetMapper();
}

//----------------------------------------------------------------------------
vtkRasterFeature::~vtkRasterFeature()
{
  if (this->ImageData)
    {
    this->ImageData->Delete();
    }
  this->Actor->Delete();
}

//----------------------------------------------------------------------------
void vtkRasterFeature::PrintSelf(std::ostream&, vtkIndent)
{
  // TODO
}

//----------------------------------------------------------------------------
void vtkRasterFeature::Init()
{
  if (!this->ImageData)
    {
    return;
    }

  // Convert image origin from lat-lon to world coordinates
  double *origin = this->ImageData->GetOrigin();
  double lat0 = origin[1];
  double y0 = vtkMercator::lat2y(lat0);
  origin[1] = y0;
  origin[2] = 0.1;  // in front of map tiles
  this->ImageData->SetOrigin(origin);

  // Convert image spacing from lat-lon to world coordinates
  // Note that this only approximates the map projection
  double *spacing = this->ImageData->GetSpacing();
  int *dimensions = this->ImageData->GetDimensions();
  double lat1 = lat0 + spacing[1] * dimensions[1];
  double y1 = vtkMercator::lat2y(lat1);
  spacing[1] = (y1 - y0) / dimensions[1];
  this->ImageData->SetSpacing(spacing);

  this->Mapper->SetInputData(this->ImageData);
  this->Actor->Update();
  this->Layer->GetRenderer()->AddActor(this->Actor);
}

//----------------------------------------------------------------------------
void vtkRasterFeature::Update()
{
  this->Actor->SetVisibility(this->IsVisible());
  this->UpdateTime.Modified();
}

//----------------------------------------------------------------------------
void vtkRasterFeature::CleanUp()
{
  this->Layer->GetRenderer()->RemoveActor(this->Actor);
  this->SetLayer(0);
}
