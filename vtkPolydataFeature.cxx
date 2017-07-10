/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPolydataFeature.h"

#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkPolydataFeature);

//----------------------------------------------------------------------------
vtkPolydataFeature::vtkPolydataFeature() : vtkFeature()
{
  this->Actor = vtkActor::New();
  this->Mapper = vtkPolyDataMapper::New();
}

//----------------------------------------------------------------------------
vtkPolydataFeature::~vtkPolydataFeature()
{
  this->Actor->Delete();
  this->Actor = 0;

  this->Mapper->Delete();
  this->Mapper = 0;
}

//----------------------------------------------------------------------------
void vtkPolydataFeature::PrintSelf(std::ostream&, vtkIndent)
{
  // TODO
}

//----------------------------------------------------------------------------
void vtkPolydataFeature::Init()
{
  if (this->GetMTime() > this->BuildTime.GetMTime())
    {
    this->Mapper->Update();

    if (!this->Actor->GetMapper())
      {
      this->Actor->SetMapper(this->Mapper);
      }
    }

//  vtkInformation* keys = this->Layer->GetPropertyKeys();
//  this->Actor->SetPropertyKeys(keys);

  this->Layer->GetRenderer()->AddActor(this->Actor);
}

//----------------------------------------------------------------------------
void vtkPolydataFeature::Update()
{
  this->Actor->SetVisibility(this->IsVisible());
  this->UpdateTime.Modified();
}

//----------------------------------------------------------------------------
void vtkPolydataFeature::CleanUp()
{
  this->Layer->GetRenderer()->RemoveActor(this->Actor);
  this->SetLayer(0);
}

//----------------------------------------------------------------------------
vtkProp *vtkPolydataFeature::PickProp()
{
  return this->Actor;
}
