/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMapPickResult.h"
#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkMapPickResult)

//----------------------------------------------------------------------------
vtkMapPickResult::vtkMapPickResult()
{
  this->DisplayCoordinates[0] = 0.0;
  this->DisplayCoordinates[0] = 1.0;
  this->MapLayer = -1;
  this->Latitude = 0.0;
  this->Longitude = 0.0;
  this->MapFeatureType = VTK_MAP_FEATURE_NONE;
  this->NumberOfMarkers = 0;
  this->MapFeatureId = -1;
}

//----------------------------------------------------------------------------
void vtkMapPickResult::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "DisplayCoordinates: " << this->DisplayCoordinates[0]
     << " " << this->DisplayCoordinates[1] << "\n"
     << indent << "MapLayer: " << this->MapLayer << "\n"
     << indent << "Latitude: " << this->Latitude << "\n"
     << indent << "Longitude: " << this->Longitude << "\n"
     << indent << "MapFeatureType: " << this->MapFeatureType << "\n"
     << indent << "NumberOfMarkers: " << this->NumberOfMarkers << "\n"
     << indent << "MapFeatureId: " << this->MapFeatureId << "\n"
     << std::endl;
}
