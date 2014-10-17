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

unsigned int vtkLayer::GlobalId = 0;

//----------------------------------------------------------------------------
vtkLayer::vtkLayer() : vtkObject()
{
  this->Visible = 1;
  this->Opacity = 1.0;
  this->Renderer = NULL;
  this->Base = 0;
  this->Map = NULL;
  this->Id = this->GlobalId + 1;
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
void vtkLayer::SetName(const unsigned int& name)
{
  if (name == this->Name)
    {
    return;
    }

  this->Name = name;
  this->Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkLayer::GetId()
{
  return this->Id;
}

//----------------------------------------------------------------------------
void vtkLayer::SetId(const unsigned int& id)
{
  if (id == this->Id)
    {
    return;
    }

  this->Id = id;
  this->Modified();
}
