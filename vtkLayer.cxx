/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkLayer.h"

//----------------------------------------------------------------------------
vtkLayer::vtkLayer() : vtkObject()
{
  this->Renderer = NULL;
}

//----------------------------------------------------------------------------
vtkLayer::~vtkLayer()
{
}

//----------------------------------------------------------------------------
void vtkLayer::PrintSelf(ostream& os, vtkIndent indent)
{
  // TODO
}

//----------------------------------------------------------------------------
std::string vtkLayer::GetName()
{
  return this->Name;
}

//----------------------------------------------------------------------------
void vtkLayer::SetName(const std::string& name)
{
  if (name == this->Name)
    {
    return;
    }

  this->Name = name;
  this->Modified();
}

//----------------------------------------------------------------------------
std::string vtkLayer::GetId()
{
  return this->Id;
}

//----------------------------------------------------------------------------
void vtkLayer::SetId(const std::string& id)
{
  if (id == this->Id)
    {
    return;
    }

  this->Id = id;
  this->Modified();
}
