/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <cpl_conv.h>

#include <vtkNew.h>
#include <vtkObjectFactory.h>

#include "vtkGDALRasterFeature.h"
#include "vtkMercator.h"
#include "vtkRasterReprojectionFilter.h"


vtkStandardNewMacro(vtkGDALRasterFeature);

void vtkGDALRasterFeature::SetGDALDataDirectory(char *path)
{
  CPLSetConfigOption("GDAL_DATA", path);
}

void vtkGDALRasterFeature::PrintSelf(std::ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

void vtkGDALRasterFeature::Reproject()
{
  // Transform data to web-mercator projection
  vtkNew<vtkRasterReprojectionFilter> reprojector;
  reprojector->SetInputData(this->ImageData);
  reprojector->SetInputProjection(this->InputProjection);
  reprojector->SetOutputProjection("EPSG:3857");  // (web mercator)
  reprojector->Update();
  vtkImageData *displayImage = reprojector->GetOutput();

  // Can release input image now
  this->ImageData->Delete();
  this->ImageData = NULL;

  // Convert image origin from web-mercator to kitware-mercator projection
  double *origin = displayImage->GetOrigin();
  origin[0] = vtkMercator::web2vtk(origin[0]);
  origin[1] = vtkMercator::web2vtk(origin[1]);
  origin[2] = this->ZCoord;
  displayImage->SetOrigin(origin);

  // Convert image spacing from web-mercator to kitware-mercator projection
  double *spacing = displayImage->GetSpacing();
  spacing[0] = vtkMercator::web2vtk(spacing[0]);
  spacing[1] = vtkMercator::web2vtk(spacing[1]);
  displayImage->SetSpacing(spacing);

  this->Mapper->SetInputData(displayImage);
}
