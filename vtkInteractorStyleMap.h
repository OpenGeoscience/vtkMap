/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleMap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInteractorStyleMap -
// .SECTION Description
//

#ifndef __vtkInteractorStyleMap_h
#define __vtkInteractorStyleMap_h

// VTK Includes
#include <vtkInteractorStyle.h>
#include "vtkmap_export.h"

class vtkMap;

class VTKMAP_DEPRECATED vtkInteractorStyleMap : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleMap *New();
  vtkTypeMacro(vtkInteractorStyleMap, vtkInteractorStyle)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // vtkMap object we are interacting with
  vtkSetMacro(Map, vtkMap*);
  vtkGetMacro(Map, vtkMap*);

  // Description:
  // Event bindings used for map interaction
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();
  virtual void OnMouseWheelForward();
  virtual void OnMouseWheelBackward();

  // Description:
  // Motion methods
  virtual void Pan();

protected:
  vtkInteractorStyleMap();
  ~vtkInteractorStyleMap();

protected:
  vtkMap *Map;

private:
  vtkInteractorStyleMap(const vtkInteractorStyleMap&);  // Not implemented
  vtkInteractorStyleMap& operator=(const vtkInteractorStyleMap&); // Not implemented
};

#endif // __vtkInteractorStyleMap_h
