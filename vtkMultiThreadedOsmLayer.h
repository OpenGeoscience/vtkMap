/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiThreadedOsmLayer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiThreadedOsmLayer -
// .SECTION Description
//

#ifndef __vtkMultiThreadedOsmLayer_h
#define __vtkMultiThreadedOsmLayer_h

#include "vtkOsmLayer.h"

class vtkMultiThreadedOsmLayer : public vtkOsmLayer
{
public:
  static vtkMultiThreadedOsmLayer *New();
  vtkTypeMacro(vtkMultiThreadedOsmLayer, vtkOsmLayer)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

protected:
  vtkMultiThreadedOsmLayer();
  ~vtkMultiThreadedOsmLayer();

  // Description:
  // Update needed tiles to draw current map display
  virtual void AddTiles();

private:
  vtkMultiThreadedOsmLayer(const vtkMultiThreadedOsmLayer&);  // Not implemented
  void operator=(const vtkMultiThreadedOsmLayer&); // Not implemented
};

#endif // __vtkMultiThreadedOsmLayer_h
