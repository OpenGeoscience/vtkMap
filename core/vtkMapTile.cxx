/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMapTile.h"
#include "vtkOsmLayer.h"

// VTK Includes
#include <vtkActor.h>
#include <vtkJPEGReader.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPNGReader.h>
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkTexture.h>
#include <vtkTextureMapToPlane.h>
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkMapTile)

  //----------------------------------------------------------------------------
  vtkMapTile::vtkMapTile()
{
  this->Visibility = 0;
  Plane = nullptr;
  TexturePlane = nullptr;
  Actor = nullptr;
  Mapper = nullptr;
  this->Corners[0] = this->Corners[1] = this->Corners[2] = this->Corners[3] =
    0.0;
}

//----------------------------------------------------------------------------
vtkMapTile::~vtkMapTile()
{
  if (Plane)
  {
    Plane->Delete();
  }

  if (TexturePlane)
  {
    TexturePlane->Delete();
  }

  if (Actor)
  {
    Actor->Delete();
  }

  if (Mapper)
  {
    Mapper->Delete();
  }
}

//----------------------------------------------------------------------------
void vtkMapTile::Build()
{
  this->Plane = vtkPlaneSource::New();
  this->Plane->SetPoint1(this->Corners[2], this->Corners[1], 0.0);
  this->Plane->SetPoint2(this->Corners[0], this->Corners[3], 0.0);
  this->Plane->SetOrigin(this->Corners[0], this->Corners[1], 0.0);
  this->Plane->SetNormal(0, 0, 1);

  this->TexturePlane = vtkTextureMapToPlane::New();

  // Read the image which will be the texture
  vtkImageReader2* imageReader = nullptr;
  std::string fileExtension =
    vtksys::SystemTools::GetFilenameLastExtension(this->ImageFile);
  if (fileExtension == ".png")
  {
    vtkPNGReader* pngReader = vtkPNGReader::New();
    imageReader = pngReader;
  }
  else if (fileExtension == ".jpg")
  {
    vtkJPEGReader* jpgReader = vtkJPEGReader::New();
    imageReader = jpgReader;
  }
  else
  {
    vtkErrorMacro("Unsupported map-tile extension " << fileExtension);
    return;
  }
  imageReader->SetFileName(this->ImageFile.c_str());
  imageReader->Update();

  // Apply the texture
  vtkNew<vtkTexture> texture;
  texture->SetInputConnection(imageReader->GetOutputPort());
  texture->SetQualityTo32Bit();
  texture->SetInterpolate(0);
  this->TexturePlane->SetInputConnection(Plane->GetOutputPort());

  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(this->TexturePlane->GetOutputPort());

  this->Actor = vtkActor::New();
  this->Actor->SetMapper(Mapper);
  this->Actor->SetTexture(texture.GetPointer());
  this->Actor->PickableOff();

  this->BuildTime.Modified();
  imageReader->Delete();
}

//----------------------------------------------------------------------------
void vtkMapTile::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkMapTile" << std::endl
     << "ImageSource: " << this->ImageSource << std::endl;
}

//----------------------------------------------------------------------------
void vtkMapTile::Init()
{
  if (this->GetMTime() > this->BuildTime.GetMTime())
  {
    this->Build();
  }
}

//----------------------------------------------------------------------------
void vtkMapTile::CleanUp()
{
  this->Layer->RemoveActor(this->Actor);
  this->SetLayer(nullptr);
}

//----------------------------------------------------------------------------
void vtkMapTile::Update()
{
  this->Actor->SetVisibility(this->IsVisible());
  this->UpdateTime.Modified();
}
