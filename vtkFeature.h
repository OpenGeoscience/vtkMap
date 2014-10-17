/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFeature -
// .SECTION Description
//

#ifndef __vtkFeature_h
#define __vtkFeature_h

#include <vtkObject.h>

#include "vtkFeatureLayer.h"

#include <string>

class vtkFeature : public vtkObject
{
public:
  enum Bins
    {
    Hidden = 99,
    Visible = 100,
    };

  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkFeature, vtkObject);

  // Expecting it to EPSG codes (EPSG4326)
  vtkGetStringMacro(Gcs);
  vtkSetStringMacro(Gcs);

  vtkGetMacro(Visibility, int);
  vtkSetMacro(Visibility, int);
  vtkBooleanMacro(Visibility, int);

  vtkGetMacro(Bin, int);
  vtkSetMacro(Bin, int);

  vtkGetObjectMacro(Layer, vtkFeatureLayer);
  vtkSetObjectMacro(Layer, vtkFeatureLayer);

  // Description:
  // Empty implementation
  virtual void Init() { }

  // Description:
  virtual void CleanUp() = 0;

  // Description:
  // Update feature and prepare it for rendering
  virtual void Update() = 0;

protected:
  vtkFeature();
  ~vtkFeature();

  unsigned int Id;
  int Visibility;
  int Bin;

  char* Gcs;

  vtkFeatureLayer* Layer;

private:
  vtkFeature(const vtkFeature&);  // Not implemented
  void operator=(const vtkFeature&); // Not implemented
};


#endif // __vtkFeature_h
