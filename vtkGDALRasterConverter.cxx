/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGDALRasterConverter.h"

// VTK Includes
#include <vtkDataArray.h>
#include <vtkDataArrayIteratorMacro.h>
#include <vtkImageData.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>

// GDAL Includes
#include <gdal_priv.h>
#include <ogr_spatialref.h>

#include <exception>
#include <iostream>

vtkStandardNewMacro(vtkGDALRasterConverter)

//----------------------------------------------------------------------------
template<class Iterator, typename VTK_TYPE>
void copyToGDAL(Iterator begin, Iterator end, VTK_TYPE typeVar,
                vtkDataArray *vtkData, GDALDataset *gdalData)
{
  // Create local buffer
  int stride = vtkData->GetNumberOfComponents();
  int numElements = vtkData->GetNumberOfTuples() / stride;
  VTK_TYPE *buffer = new VTK_TYPE[numElements];
  int xSize = gdalData->GetRasterXSize();
  int ySize = gdalData->GetRasterYSize();

  // Copy each vtk component to separate gdal band
  for (int i=0; i<stride; i++)
    {
    GDALRasterBand *band = gdalData->GetRasterBand(i+1);

    // Copy from iterator to buffer
    Iterator iter = begin;

    // Offset iterator to next element in the tuple
    for (int j=0; j<i; j++)
      {
      iter++;
      }

    // Copy data from vtk iterator to buffer
    // Traverse by gdal row & column to make inversion easier
    for (int row=0; row < ySize; row++)
      {
      // GDAL data starts at top-left, vtk at bottom-left
      // So need to invert in the y direction
      int invertedRow = ySize - row - 1;
      int offset = invertedRow * xSize;
      for (int col=0; col < xSize; col++)
        {
        buffer[offset + col] = *iter;

        // Advance to next tuple
        for (int k=0; k<stride; k++)
          {
          iter++;
          }
        }
      }

    // Copy from buffer to GDAL band
    GDALDataType gdalDataType = band->GetRasterDataType();
    CPLErr err = band->RasterIO(GF_Write, 0, 0, xSize, ySize, buffer, xSize, ySize, gdalDataType, 0, 0);
    }

  delete [] buffer;
}

//----------------------------------------------------------------------------
vtkGDALRasterConverter::vtkGDALRasterConverter()
{
  this->ImageData = NULL;
  this->GDALData = NULL;
  this->NoDataValue = vtkMath::Nan();
}

//----------------------------------------------------------------------------
vtkGDALRasterConverter::~vtkGDALRasterConverter()
{
}

//----------------------------------------------------------------------------
void vtkGDALRasterConverter::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkGDALRasterConverter" << std::endl;
}

//----------------------------------------------------------------------------
bool vtkGDALRasterConverter::
ConvertToGDAL(vtkImageData *input, GDALDataset *output)
{
  // Check that both images have the same dimensions
  int *inputDimensions = input->GetDimensions();
  if (output->GetRasterXSize() != inputDimensions[0] ||
      output->GetRasterYSize() != inputDimensions[1])
    {
    vtkErrorMacro(<< "Image dimensions do not match.");
    return false;
    }

  this->ImageData = input;
  this->GDALData = output;

  // Set spatial reference
  OGRSpatialReference ref;
  ref.SetFromUserInput("EPSG:4326");  // presume lat-lon for now
  char *outputWKT = NULL;
  ref.exportToWkt(&outputWKT);
  output->SetProjection(outputWKT);

  // Initialize geo transform
  double *origin = input->GetOrigin();
  double *spacing = input->GetSpacing();
  double geoTransform[6];
  geoTransform[0] = origin[0];
  geoTransform[1] = spacing[0];
  geoTransform[2] = 0.0;
  geoTransform[3] = origin[1];
  geoTransform[4] = 0.0;
  geoTransform[5] = -spacing[1];
  output->SetGeoTransform(geoTransform);

  if (!vtkMath::IsNan(this->NoDataValue))
    {
    for (int i=0; i<output->GetRasterCount(); i++)
      {
      GDALRasterBand *band = output->GetRasterBand(i+1);
      band->SetNoDataValue(this->NoDataValue);
      }
    }

  // Copy scalars to gdal bands
  vtkDataArray *array = this->ImageData->GetPointData()->GetScalars();
  switch (array->GetDataType())
    {
    vtkDataArrayIteratorMacro(array,
      copyToGDAL(vtkDABegin, vtkDAEnd, vtkDAValueType(), array, output));
    }

  // Finis
  return true;
}
