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

class VTKMAP_EXPORT vtkPolydataFeature : public vtkFeature
{
public:
  static vtkPolydataFeature* New();
  void PrintSelf(ostream &os, vtkIndent indent) override;
  vtkTypeMacro(vtkPolydataFeature, vtkFeature);

  // Description
  // Get actor for the polydata
  vtkGetObjectMacro(Actor, vtkActor);

  // Description
  // Get mapper for the polydata
  vtkGetObjectMacro(Mapper, vtkPolyDataMapper);

  void Init() override;

  void CleanUp() override;

  void Update() override;

  vtkProp* PickProp() override;

  /**
   * Maps a cellId to a locally defined Id.
   */
  virtual vtkIdType CellIdToLocalId(vtkIdType cellId);

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
