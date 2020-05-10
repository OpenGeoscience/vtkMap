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

#include <vtkFieldData.h>
#include <vtkImageProperty.h>
#include <vtkStringArray.h>

//----------------------------------------------------------------------------
vtkRasterFeature::vtkRasterFeature()
  : vtkFeature()
{
  this->ZCoord = 0.1;
  this->ImageData = nullptr;
  this->InputProjection = nullptr;
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
  delete[] this->InputProjection;
  this->Actor->Delete();
}

//----------------------------------------------------------------------------
void vtkRasterFeature::PrintSelf(std::ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkRasterFeature::Init()
{
  if (!this->ImageData)
  {
    return;
  }

  // Get input map projection
  if (!this->InputProjection)
  {
    vtkAbstractArray* array =
      this->ImageData->GetFieldData()->GetAbstractArray("MAP_PROJECTION");
    if (!array)
    {
      vtkErrorMacro("No map projection data for input image");
      return;
    }
    vtkStringArray* projectionArray = vtkStringArray::SafeDownCast(array);
    this->SetInputProjection(projectionArray->GetValue(0));
  }

  this->Reproject();

  this->Actor->GetProperty()->UseLookupTableScalarRangeOn();
  this->Actor->Update();
  this->Layer->AddActor(this->Actor);
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
  this->Layer->RemoveActor(this->Actor);
  this->SetLayer(nullptr);
}

//----------------------------------------------------------------------------
vtkProp* vtkRasterFeature::PickProp()
{
  return this->Actor;
}
