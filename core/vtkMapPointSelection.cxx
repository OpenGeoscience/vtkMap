/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapPointSelection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMapPointSelection.h"
#include "vtkObjectFactory.h"


vtkStandardNewMacro(vtkMapPointSelection)

void vtkMapPointSelection::SetMaskArray(const std::string& name)
{
  if (this->MaskArrayName.compare(name) == 0)
  {
    return;
  }

  this->MaskArrayName = name;
  this->Modified(); 
}

vtkMTimeType vtkMapPointSelection::GetMTime()
{
  /// TODO Consider mask array, flag and name.
  return Superclass::GetMTime();
}

int vtkMapPointSelection::RequestData(vtkInformation* request,
  vtkInformationVector** input, vtkInformationVector* output)
{
  /// TODO
  return Superclass::RequestData(request, input, output);
}

int vtkMapPointSelection::FillInputPortInformation(int port,
  vtkInformation *info)
{
  /// TODO
  return Superclass::FillInputPortInformation(port, info);
}

void vtkMapPointSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
