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
  // Build feature when added to the feature layer.
  // Init should be called by the feature layer and
  // not directly by the application code.
  virtual void Init() { }

  // Description:
  // Perform any clean up operation related to rendering.
  // CleanUp should be called by the feature layer and
  // not directly by the application code.
  virtual void CleanUp() = 0;

  // Description:
  // Update feature and prepare it for rendering.
  // Update should be called by the feature layer and
  // not directly by the application code.
  virtual void Update() = 0;

protected:
  vtkFeature();
  ~vtkFeature();

  unsigned int Id;
  int Visibility;
  int Bin;

  char* Gcs;

  vtkTimeStamp BuildTime;
  vtkTimeStamp UpdateTime;

  vtkFeatureLayer* Layer;

private:
  vtkFeature(const vtkFeature&);  // Not implemented
  void operator=(const vtkFeature&); // Not implemented
};


#endif // __vtkFeature_h
