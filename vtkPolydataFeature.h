/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolydataFeature -
// .SECTION Description
//

#ifndef __vtkPolydataFeature_h
#define __vtkPolydataFeature_h

#include <vtkActor.h>
#include <vtkObject.h>
#include <vtkPolyDataMapper.h>

#include "vtkFeature.h"
#include "vtkmap_export.h"

#include <string>

class VTKMAP_EXPORT vtkPolydataFeature : public vtkFeature
{
public:
  static vtkPolydataFeature* New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkPolydataFeature, vtkObject);

  // Description
  // Get actor for the polydata
  vtkGetObjectMacro(Actor, vtkActor);

  // Description
  // Get mapper for the polydata
  vtkGetObjectMacro(Mapper, vtkPolyDataMapper);

  // Description:
  // Override
  virtual void Init();

  // Description:
  // Override
  virtual void CleanUp();

  // Description:
  // Override
  virtual void Update();

protected:
  vtkPolydataFeature();
  ~vtkPolydataFeature();

  vtkActor* Actor;
  vtkPolyDataMapper* Mapper;

private:
  vtkPolydataFeature(const vtkPolydataFeature&); // Not implemented
  vtkPolydataFeature& operator=(const vtkPolydataFeature&); // Not implemented
};


#endif // __vtkPolydataFeature_h
