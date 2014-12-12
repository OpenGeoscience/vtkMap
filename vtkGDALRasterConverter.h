/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGDALRasterConverter -
// .SECTION Description
// Converts between VTK image representations and GDAL datasets.
// This class is intended to be used internally

#ifndef __vtkGDALRasterConverter_h
#define __vtkGDALRasterConverter_h

#include "vtkObject.h"
#include "vtkmap_export.h"
class vtkImageData;
class GDALDataset;

class VTKMAP_EXPORT vtkGDALRasterConverter : public vtkObject
{
public:
  static vtkGDALRasterConverter* New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkGDALRasterConverter, vtkObject);

  // Description
  // No-data value for pixels in the source image
  // Default is NaN (not used).
  vtkSetMacro(NoDataValue, double);
  vtkGetMacro(NoDataValue, double);

  // Description
  // Builds GDALDataset to match vtkImageData
  // GDALDataset must be initialized to same dimensions as vtk image.
  bool ConvertToGDAL(vtkImageData *input, GDALDataset *output);

  // Description
  // Builds vtkImageData to match GDALDataset
  //bool ConvertToVTK(GDALDataset *input, vtkImageData *output);

protected:
  vtkGDALRasterConverter();
  ~vtkGDALRasterConverter();

  vtkImageData *ImageData;
  GDALDataset *GDALData;
  double NoDataValue;

private:
   // Not implemented:
  vtkGDALRasterConverter(const vtkGDALRasterConverter&);
  vtkGDALRasterConverter& operator=(const vtkGDALRasterConverter&);
};

#endif // __vtkGDALRasterConverter_h
