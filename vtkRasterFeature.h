/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRasterFeature -
// .SECTION Description
// Displays an image at a rectangular map area.

#ifndef __vtkRasterFeature_h
#define __vtkRasterFeature_h

#include "vtkFeature.h"
#include "vtkmap_export.h"
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <string>

class VTKMAP_EXPORT vtkRasterFeature : public vtkFeature
{
public:
  static vtkRasterFeature* New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkRasterFeature, vtkFeature);

  // Description
  // Set input image data
  vtkSetObjectMacro(ImageData, vtkImageData);

  // Description
  // Get actor for the image data
  vtkGetObjectMacro(Actor, vtkImageActor);

  // Description
  // Get mapper for the image data
  vtkGetObjectMacro(Mapper, vtkImageMapper3D);

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
  vtkRasterFeature();
  ~vtkRasterFeature();

  vtkImageData *ImageData;
  vtkImageActor *Actor;
  vtkImageMapper3D *Mapper;

private:
  vtkRasterFeature(const vtkRasterFeature&); // Not implemented
  vtkRasterFeature& operator=(const vtkRasterFeature&); // Not implemented
};

#endif // __vtkRasterFeature_h
