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
#include "vtkmapcore_export.h"
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>

class VTKMAPCORE_EXPORT vtkRasterFeature : public vtkFeature
{
public:
  void PrintSelf(ostream &os, vtkIndent indent) override;
  vtkTypeMacro(vtkRasterFeature, vtkFeature);

  // Description
  // Set input image data
  // Must contain origin & spacing in vtk-mercator units
  // Use vtkMercator to convert from lat-lon or EPSG:3857 units
  vtkSetObjectMacro(ImageData, vtkImageData);

  // Description:
  // Set the map-projection string for the input image data.
  // This should *only* be used for nonstandard image inputs,
  // when the MAP_PROJECTION is not embedded as field data.
  // Can be specified using any string formats supported by GDAL,
  // such as "well known text" (WKT) formats (GEOGS[]),
  // or shorter "user string" formats, such as EPSG:3857.
  vtkSetStringMacro(InputProjection);
  vtkGetStringMacro(InputProjection);

  // Description:
  // Set/get the Z value, in world coordinates, to assign the feature.
  // The default is 0.1
  vtkSetMacro(ZCoord, double);
  vtkGetMacro(ZCoord, double);

  // Description
  // Get actor for the image data
  vtkGetObjectMacro(Actor, vtkImageActor);

  // Description
  // Get mapper for the image data
  vtkGetObjectMacro(Mapper, vtkImageMapper3D);

  // Description:
  // Override
  void Init() override;

  // Description:
  // Override
  void CleanUp() override;

  // Description:
  // Override
  void Update() override;

  // Description:
  // Override
  vtkProp *PickProp() override;

protected:
  vtkRasterFeature();
  virtual ~vtkRasterFeature();

  virtual void Reproject() = 0;

  double ZCoord;
  vtkImageData *ImageData;
  char *InputProjection;
  vtkImageActor *Actor;
  vtkImageMapper3D *Mapper;

private:
  vtkRasterFeature(const vtkRasterFeature&); // Not implemented
  vtkRasterFeature& operator=(const vtkRasterFeature&); // Not implemented
};

#endif // __vtkRasterFeature_h
