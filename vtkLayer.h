/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLayer -
// .SECTION Description
//

#ifndef __vtkLayer_h
#define __vtkLayer_h

#include "vtkMap.h"

// VTK Includes
#include <vtkObject.h>
#include <vtkRenderer.h>

class vtkLayer : public vtkObject
{
public:
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkLayer, vtkObject)

  // Description:
  // Get the renderer for the layer
  vtkGetMacro(Renderer, vtkRenderer*)

  // Description:
  std::string GetName();
  void SetName(const std::string& Name);

  // Description:
  unsigned int GetId();
  void SetId(const unsigned int& id);

  // Description:
  vtkGetMacro(Opacity, double)
  vtkSetMacro(Opacity, double)

  // Description:
  vtkBooleanMacro(Visibility, int)
  vtkGetMacro(Visibility, int)
  vtkSetMacro(Visibility, int)

  // Description:
  vtkBooleanMacro(Base, int)
  vtkGetMacro(Base, int)
  vtkSetMacro(Base, int)

  // Description:
  vtkGetObjectMacro(Map, vtkMap)
  void SetMap(vtkMap* map);

  // Description:
  virtual void Update() = 0;

protected:

  vtkLayer();
  virtual ~vtkLayer();

  double Opacity;
  int Visibility;
  int Base;

  std::string Name;
  unsigned int Id;

  vtkMap* Map;
  vtkRenderer* Renderer;

  static unsigned int GlobalId;

private:
  vtkLayer(const vtkLayer&);  // Not implemented
  void operator=(const vtkLayer&); // Not implemented
};

#endif // __vtkLayer_h

