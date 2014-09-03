/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapTile.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMapMarker.h"

// VTK Includes
#include <vtkActor2D.h>
#include <vtkImageData.h>
#include <vtkImageMapper.h>
#include <vtkObjectFactory.h>
#include <vtkPNGReader.h>
#include <vtkNew.h>


//----------------------------------------------------------------------------
vtkImageData *vtkMapMarker::ImageData = NULL;
vtkStandardNewMacro(vtkMapMarker)

//----------------------------------------------------------------------------
vtkMapMarker::vtkMapMarker()
{
  this->Mapper = vtkImageMapper::New();
  this->Mapper->SetColorWindow(255.0);
  this->Mapper->SetColorLevel(127.5);
  this->Mapper->SetInputData(this->ImageData);

  this->Actor = vtkActor2D::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetPosition(0.0, 0.0);
}

//----------------------------------------------------------------------------
vtkMapMarker::~vtkMapMarker()
{
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
void vtkMapMarker::LoadMarkerImage(const char* PngFilename)
{
  vtkNew<vtkPNGReader> pngReader;
  pngReader->SetFileName(PngFilename);
  pngReader->Update();

  int dims[3];
  pngReader->GetOutput()->GetDimensions(dims);
  std::cout << "Marker dimensions " << dims[0]
            << " x " << dims[1]
            << " x " << dims[2] << std::endl;

  vtkMapMarker::ImageData = pngReader->GetOutput();
}

//----------------------------------------------------------------------------
void vtkMapMarker::SetCoordinates(double Latitude, double Longitude)
{
  // Note that longitude goes first, since it is the X coordinate
  this->Actor->SetPosition(Longitude, Latitude);
}
