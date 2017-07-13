/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkInformation.h>
#include <vtkInformationIntegerKey.h>

#include "vtkGeoMapLayerPass.h"
#include "vtkLayer.h"


vtkInformationKeyMacro(vtkLayer, ID, Integer);
unsigned int vtkLayer::GlobalId = 0;

//----------------------------------------------------------------------------
vtkLayer::vtkLayer() : vtkObject()
, RenderPass(vtkSmartPointer<vtkGeoMapLayerPass>::New())
{
  this->Visibility = 1;
  this->Opacity = 1.0;
  this->Renderer = NULL;
  this->Base = 0;
  this->Map = NULL;
  this->AsyncMode = false;
  this->Id = this->GlobalId + 1;
  this->RenderPass->SetLayerId(this->Id);
  this->GlobalId++;
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

//----------------------------------------------------------------------------
vtkRenderPass* vtkLayer::GetRenderPass()
{
  return this->RenderPass.GetPointer(); 
}

//----------------------------------------------------------------------------
void vtkLayer::RemoveActor(vtkProp* prop)
{
  if (!this->Renderer || !prop)
  {
    vtkErrorMacro(<< "Could not remove vtkProp.");
    return;
  }

  this->Renderer->RemoveActor(prop);
}

//----------------------------------------------------------------------------
void vtkLayer::AddActor(vtkProp* prop)
{
  if (!this->Renderer || !prop)
  {
    vtkErrorMacro(<< "Could not register vtkProp.");
    return;
  }

  this->Renderer->AddActor(prop);

  vtkInformation* keys = vtkInformation::New();
  keys->Set(vtkLayer::ID(), this->Id);
  prop->SetPropertyKeys(keys);
  keys->Delete();
}

//----------------------------------------------------------------------------
void vtkLayer::AddActor2D(vtkProp* prop)
{
  if (!this->Renderer || !prop)
  {
    vtkErrorMacro(<< "Could not register vtkProp.");
    return;
  }

  this->Renderer->AddActor2D(prop);

  vtkInformation* keys = vtkInformation::New();
  keys->Set(vtkLayer::ID(), this->Id);
  prop->SetPropertyKeys(keys);
  keys->Delete();
}
