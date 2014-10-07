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
// .NAME vtkOsmLayer -
// .SECTION Description
//

#ifndef __vtkOsmLayer_h
#define __vtkOsmLayer_h

#include "vtkLayer.h"

// VTK Includes
#include <vtkObject.h>
#include <vtkRenderer.h>

class vtkOsmLayer : public vtkLayer
{
public:
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkOsmLayer, vtkLayer)

  // Description:
  virtual void Update();

protected:
  vtkOsmLayer();
  virtual ~vtkOsmLayer();

private:
  vtkOsmLayer(const vtkOsmLayer&);  // Not implemented
  void operator=(const vtkOsmLayer&); // Not implemented
};

#endif // __vtkOsmLayer_h

