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
