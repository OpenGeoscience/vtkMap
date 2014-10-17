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
}

//----------------------------------------------------------------------------
void vtkFeature::PrintSelf(std::ostream& os, vtkIndent indent)
{
  // TODO
}


