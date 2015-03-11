/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRasterFeature.h"
#include "vtkMercator.h"
#include "vtkRasterReprojectionFilter.h"

#include <vtkFieldData.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkStringArray.h>

vtkStandardNewMacro(vtkRasterFeature);

//----------------------------------------------------------------------------
vtkRasterFeature::vtkRasterFeature() : vtkFeature()
{
  this->ImageData = NULL;
  this->InputProjection = NULL;
  this->Actor = vtkImageActor::New();
  this->Mapper = this->Actor->GetMapper();
}

//----------------------------------------------------------------------------
vtkRasterFeature::~vtkRasterFeature()
{
  if (this->ImageData)
    {
    this->ImageData->Delete();
    }
  delete [] this->InputProjection;
  this->Actor->Delete();
}

//----------------------------------------------------------------------------
void vtkRasterFeature::PrintSelf(std::ostream&, vtkIndent)
{
  // TODO
}

//----------------------------------------------------------------------------
void vtkRasterFeature::Init()
{
  if (!this->ImageData)
    {
    return;
    }

  // Get input map projection
  if (!this->InputProjection)
    {
    vtkAbstractArray *array =
      this->ImageData->GetFieldData()->GetAbstractArray("MAP_PROJECTION");
    if (!array)
      {
      vtkErrorMacro("No map projection data for input image");
      return;
      }
    vtkStringArray *projectionArray = vtkStringArray::SafeDownCast(array);
    this->SetInputProjection(projectionArray->GetValue(0));
    }

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
  origin[2] = 0.1;  // in front of map tiles
  displayImage->SetOrigin(origin);

  // Convert image spacing from web-mercator to kitware-mercator projection
  double *spacing = displayImage->GetSpacing();
  spacing[0] = vtkMercator::web2vtk(spacing[0]);
  spacing[1] = vtkMercator::web2vtk(spacing[1]);
  displayImage->SetSpacing(spacing);

  this->Mapper->SetInputData(displayImage);
  this->Actor->Update();
  this->Layer->GetRenderer()->AddActor(this->Actor);
}

//----------------------------------------------------------------------------
void vtkRasterFeature::Update()
{
  this->Actor->SetVisibility(this->IsVisible());
  this->UpdateTime.Modified();
}

//----------------------------------------------------------------------------
void vtkRasterFeature::CleanUp()
{
  this->Layer->GetRenderer()->RemoveActor(this->Actor);
  this->SetLayer(0);
}

//----------------------------------------------------------------------------
vtkProp *vtkRasterFeature::PickProp()
{
  return this->Actor;
}
