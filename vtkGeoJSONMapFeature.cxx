/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGeoJSONMapFeature.h"
#include "vtkMercator.h"

#include <vtkGeoJSONReader.h>

#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>

vtkStandardNewMacro(vtkGeoJSONMapFeature);

//----------------------------------------------------------------------------
vtkGeoJSONMapFeature::vtkGeoJSONMapFeature() : vtkPolydataFeature()
{
  this->InputString = NULL;
  this->PolyData = NULL;
}

//----------------------------------------------------------------------------
vtkGeoJSONMapFeature::~vtkGeoJSONMapFeature()
{
  if (this->PolyData)
    {
    this->PolyData->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkGeoJSONMapFeature::PrintSelf(std::ostream& os, vtkIndent indent)
{
  // TODO
}

//----------------------------------------------------------------------------
void vtkGeoJSONMapFeature::Init()
{
  // Parse InputString to generate vtkPolyData
  vtkNew<vtkGeoJSONReader> reader;
  reader->StringInputModeOn();
  reader->SetStringInput(this->InputString);
  reader->Update();
  this->PolyData = reader->GetOutput();

  // Convert poly data points from <lon, lat> to <x, y>
  vtkPoints *points = this->PolyData->GetPoints();
  for (vtkIdType i=0; i<points->GetNumberOfPoints(); i++)
    {
    double *coords = points->GetPoint(i);
    coords[1] = vtkMercator::lat2y(coords[1]);
    points->SetPoint(i, coords);
    }

  std::cout << "Points:   " << this->PolyData->GetNumberOfPoints() << "\n"
            << "Vertices: " << this->PolyData->GetNumberOfVerts() << "\n"
            << "Lines:    " << this->PolyData->GetNumberOfLines() << "\n"
            << "Polygons: " << this->PolyData->GetNumberOfPolys()
            << std::endl;

  // Set PolyData to superclass and call its init
  this->Superclass::GetMapper()->SetInputData(this->PolyData);
  this->Superclass::Init();
}
