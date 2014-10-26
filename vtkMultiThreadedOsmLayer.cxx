/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiThreadedOsmLayer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMultiThreadedOsmLayer.h"

#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkMultiThreadedOsmLayer)

//----------------------------------------------------------------------------
vtkMultiThreadedOsmLayer::vtkMultiThreadedOsmLayer()
{
}

//----------------------------------------------------------------------------
vtkMultiThreadedOsmLayer::~vtkMultiThreadedOsmLayer()
{
}

//----------------------------------------------------------------------------
void vtkMultiThreadedOsmLayer::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkMultiThreadedOsmLayer"
     << std::endl;
}

//----------------------------------------------------------------------------
void vtkMultiThreadedOsmLayer::AddTiles()
{
  this->Superclass::AddTiles();
}
