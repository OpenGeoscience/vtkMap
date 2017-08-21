/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGDALRasterFeature -
// .SECTION Description
// Displays an image at a rectangular map area.

#ifndef __vtkGDALRasterFeature_h
#define __vtkGDALRasterFeature_h

#include "vtkRasterFeature.h"
#include "vtkmapgdal_export.h"

class VTKMAPGDAL_EXPORT vtkGDALRasterFeature : public vtkRasterFeature
{
public:
  static vtkGDALRasterFeature* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkGDALRasterFeature, vtkRasterFeature);

  // Description
  // Set the GDAL_DATA folder, which is generally needed
  // to display raster features. If/when the raster-preojection
  // code is migrated to VTK, this function should
  // be moved with it.
  static void SetGDALDataDirectory(char* path);

protected:
  vtkGDALRasterFeature() = default;
  ~vtkGDALRasterFeature() = default;

  void Reproject() override;

private:
  vtkGDALRasterFeature(const vtkGDALRasterFeature&) = delete;
  vtkGDALRasterFeature& operator=(const vtkGDALRasterFeature&) = delete;
};

#endif // __vtkGDALRasterFeature_h
