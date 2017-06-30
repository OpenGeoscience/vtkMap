/*=========================================================================

  Program:   Visualization Toolkit

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
  this->Visibility = 1;
  this->Opacity = 1.0;
  this->Renderer = NULL;
  this->Base = 0;
  this->Map = NULL;
  this->AsyncMode = false;
  this->Id = this->GlobalId + 1;
}

//----------------------------------------------------------------------------
vtkLayer::~vtkLayer()
{
}

//----------------------------------------------------------------------------
void vtkLayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Name: " << this->Name << "\n"
     << indent << "Visibility: " << this->Visibility << "\n"
     << indent << "Opacity: " << this->Opacity << "\n"
     << indent << "AsyncMode: " << this->AsyncMode << "\n"
     << indent << "Id: " << this->Id << std::endl;
}

//----------------------------------------------------------------------------
const std::string& vtkLayer::GetName()
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

//----------------------------------------------------------------------------
void vtkLayer::SetMap(vtkMap* map)
{
  if (this->Map != map)
    {
    this->Map = map;
    this->Renderer = map->GetRenderer();
    this->Modified();
    }
}

//----------------------------------------------------------------------------
bool vtkLayer::IsAsynchronous()
{
  return this->AsyncMode;
}

//----------------------------------------------------------------------------
vtkMap::AsyncState vtkLayer::ResolveAsync()
{
  // Asynchronous layers should always override this method.
  // (Otherwise they don't need to be asynchronous...)
  vtkWarningMacro(<<"vtkLayer::ResolveAsync() should not be called");
  return vtkMap::AsyncOff;
}
