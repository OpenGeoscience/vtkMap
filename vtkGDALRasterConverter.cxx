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

// VTK includes
#include <vtkDataArray.h>
#include <vtkDataArrayIteratorMacro.h>
#include <vtkImageData.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkUniformGrid.h>

// GDAL includes
#include <gdal_priv.h>
#include <ogr_spatialref.h>

// STD includes
#include <iostream>
#include <sstream>

vtkStandardNewMacro(vtkGDALRasterConverter)

//----------------------------------------------------------------------------
class vtkGDALRasterConverter::vtkGDALRasterConverterInternal
{
public:
  GDALDataType ToGDALDataType(int vtkDataType)
  {
    GDALDataType gdalType = GDT_Unknown;
    switch (vtkDataType)
      {
      case VTK_TYPE_UINT8: gdalType = GDT_Byte; break;
      case VTK_TYPE_INT16: gdalType = GDT_Int16; break;
      case VTK_TYPE_UINT16: gdalType = GDT_UInt16; break;
      case VTK_TYPE_INT32: gdalType = GDT_Int32; break;
      case VTK_TYPE_UINT32: gdalType = GDT_UInt32; break;
      case VTK_TYPE_FLOAT32: gdalType = GDT_Float32; break;
      case VTK_TYPE_FLOAT64: gdalType = GDT_Float64; break;
      }
    return gdalType;
  }

  template<typename VTK_TYPE> void
  CopyToVTK(GDALDataset *gdalData, vtkDataArray *vtkData);

  template<typename GDAL_TYPE> void
  FindDataRange(GDALRasterBand *band, double *minValue, double *maxValue);
};


//----------------------------------------------------------------------------
template<typename VTK_TYPE>
void vtkGDALRasterConverter::vtkGDALRasterConverterInternal::
CopyToVTK(GDALDataset *dataset, vtkDataArray *array)
{
  // Initialize array storage
  int stride = dataset->GetRasterCount();
  array->SetNumberOfComponents(stride);
  int xSize = dataset->GetRasterXSize();
  int ySize = dataset->GetRasterYSize();
  int numElements = xSize * ySize;
  array->SetNumberOfTuples(numElements);

  // Copy from GDAL to vtkImageData, one band at a time
  VTK_TYPE *buffer = new VTK_TYPE[numElements];
  for (int i=0; i<stride; i++)
    {
    // Copy one band from dataset to buffer
    GDALRasterBand *band = dataset->GetRasterBand(i+1);
    GDALDataType gdalDataType = band->GetRasterDataType();
    CPLErr err = band->RasterIO(GF_Read, 0, 0, xSize, ySize, buffer, xSize, ySize, gdalDataType, 0, 0);

    // Copy data from buffer to vtkDataArray
    // Traverse by gdal row & column to make y-inversion easier
    for (int row=0, index=0; row < ySize; row++)
      {
      // GDAL data starts at top-left, vtk at bottom-left
      // So need to invert in the y direction
      int invertedRow = ySize - row - 1;
      int offset = invertedRow * xSize;
      for (int col=0; col < xSize; col++)
        {
        array->SetComponent(offset + col, i, buffer[index++]);
        }
      }

    }
  delete [] buffer;
}

//----------------------------------------------------------------------------
template<typename VTK_TYPE> void
vtkGDALRasterConverter::vtkGDALRasterConverterInternal::
FindDataRange(GDALRasterBand *band, double *minValue, double *maxValue)
{
  int xSize = band->GetDataset()->GetRasterXSize();
  int ySize = band->GetDataset()->GetRasterYSize();
  VTK_TYPE *buffer = new VTK_TYPE[xSize*ySize];
  GDALDataType gdalDataType = band->GetRasterDataType();
  band->RasterIO(GF_Read, 0, 0, xSize, ySize, buffer, xSize, ySize,
                 gdalDataType, 0, 0);

  *minValue = VTK_DOUBLE_MAX;
  *maxValue = VTK_DOUBLE_MIN;
  *minValue = 999999.9;
  *maxValue = -999999.9;
  for (int i=0; i<xSize*ySize; i++)
    {
    *minValue = *minValue < buffer[i] ? *minValue : buffer[i];
    *maxValue = *maxValue > buffer[i] ? *maxValue : buffer[i];
    }

  delete [] buffer;
}

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
  this->Internal = new vtkGDALRasterConverterInternal();
  this->ImageData = NULL;
  this->GDALData = NULL;
  this->NoDataValue = vtkMath::Nan();
}

//----------------------------------------------------------------------------
vtkGDALRasterConverter::~vtkGDALRasterConverter()
{
  delete this->Internal;
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

  // Set spatial reference - presume lat-lon for now
  this->SetGDALProjection(output, "EPSG:4326");

  // Initialize geo transform
  double *origin = input->GetOrigin();
  double *spacing = input->GetSpacing();
  this->SetGDALGeoTransform(output, origin, spacing);

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

//----------------------------------------------------------------------------
vtkUniformGrid *vtkGDALRasterConverter::
CreateVTKUniformGrid(GDALDataset *dataset)
{
  // Set vtk origin & spacing from GDALGeoTransform
  double geoTransform[6] = {};
  if (dataset->GetGeoTransform(geoTransform) != CE_None)
    {
    vtkErrorMacro(<< "Error calling GetGeoTransform()");
    //return NULL;
    }

  // Initialize image
  vtkUniformGrid *image = vtkUniformGrid::New();
  int imageDimensions[3];
  imageDimensions[0] = dataset->GetRasterXSize();
  imageDimensions[1] = dataset->GetRasterYSize();
  imageDimensions[2] = 0.0;
  image->SetDimensions(imageDimensions);

  double origin[3];
  origin[0] = geoTransform[0];
  origin[1] = geoTransform[3];
  origin[2] = 0.0;
  image->SetOrigin(origin);

  double spacing[3];
  spacing[0] = geoTransform[1];
  spacing[1] = geoTransform[5];
  spacing[2] = 0.0;
  image->SetSpacing(spacing);

  // Copy pixel data
  int rasterCount = dataset->GetRasterCount();
  if (rasterCount < 1)
    {
    return NULL;
    }

  vtkDataArray *array = NULL;
  switch (dataset->GetRasterBand(1)->GetRasterDataType())
    {
    case GDT_Byte:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_UINT8);
      this->Internal->CopyToVTK<vtkTypeUInt8>(dataset, array);
      break;

    case GDT_UInt16:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_UINT16);
      this->Internal->CopyToVTK<vtkTypeUInt16>(dataset, array);
      break;

    case GDT_Int16:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_INT16);
      this->Internal->CopyToVTK<vtkTypeInt16>(dataset, array);
      break;

    case GDT_UInt32:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_UINT32);
      this->Internal->CopyToVTK<vtkTypeUInt32>(dataset, array);
      break;

    case GDT_Int32:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_INT32);
      this->Internal->CopyToVTK<vtkTypeInt32>(dataset, array);
      break;

    case GDT_Float32:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_FLOAT32);
      this->Internal->CopyToVTK<vtkTypeFloat32>(dataset, array);
      break;

    case GDT_Float64:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_FLOAT64);
      this->Internal->CopyToVTK<vtkTypeFloat64>(dataset, array);
      break;
    }

  if (!array)
    {
    return NULL;
    }

  image->GetPointData()->SetScalars(array);
  array->Delete();

  return image;
}

//----------------------------------------------------------------------------
GDALDataset *vtkGDALRasterConverter::
CreateGDALDataset(int xDim, int yDim, int vtkDataType, int numberOfBands)
{
  GDALDriver *driver = static_cast<GDALDriver*>(GDALGetDriverByName("MEM"));
  GDALDataType gdalType = this->Internal->ToGDALDataType(vtkDataType);
  GDALDataset *dataset = driver->Create("", xDim, yDim, numberOfBands,
                                        gdalType, NULL);
  return dataset;
}

//----------------------------------------------------------------------------
void vtkGDALRasterConverter::
SetGDALProjection(GDALDataset *dataset, const char *projectionString)
{
  // Use OGRSpatialReference to convert to WKT
  OGRSpatialReference ref;
  ref.SetFromUserInput(projectionString);
  char *wkt = NULL;
  ref.exportToWkt(&wkt);
  //std::cout << "Projection WKT: " << wkt << std::endl;
  dataset->SetProjection(wkt);
  CPLFree(wkt);
}

//----------------------------------------------------------------------------
void vtkGDALRasterConverter::
SetGDALGeoTransform(GDALDataset *dataset, double origin[2], double spacing[2])
{
  double geoTransform[6];
  geoTransform[0] = origin[0];
  geoTransform[1] = spacing[0];
  geoTransform[2] = 0.0;
  geoTransform[3] = origin[1];
  geoTransform[4] = 0.0;
  geoTransform[5] = spacing[1];
  dataset->SetGeoTransform(geoTransform);
}

//----------------------------------------------------------------------------
void vtkGDALRasterConverter::
WriteTifFile(GDALDataset *dataset, const char *filename)
{
  // Copy dataset to GTiFF driver, which creates file
  GDALDriver *driver = static_cast<GDALDriver*>(GDALGetDriverByName("GTiff"));
  GDALDataset *copy =
    driver->CreateCopy(filename, dataset, false, NULL, NULL, NULL);
  GDALClose(copy);
}

//----------------------------------------------------------------------------
bool vtkGDALRasterConverter::
FindDataRange(GDALDataset *dataset, int bandId,
              double *minValue, double *maxValue)
{
  if ((bandId < 1) || (bandId > dataset->GetRasterCount()))
    {
    return false;
    }
  GDALRasterBand *band = dataset->GetRasterBand(bandId);
  GDALDataType gdalDataType = band->GetRasterDataType();
  switch (gdalDataType)
    {
    case GDT_Byte:
      this->Internal->FindDataRange<vtkTypeUInt8>(band, minValue, maxValue);
      break;

    case GDT_Int16:
      this->Internal->FindDataRange<vtkTypeInt16>(band, minValue, maxValue);
      break;

    case GDT_UInt16:
      this->Internal->FindDataRange<vtkTypeUInt16>(band, minValue, maxValue);
      break;

    case GDT_UInt32:
      this->Internal->FindDataRange<vtkTypeUInt32>(band, minValue, maxValue);
      break;

    case GDT_Int32:
      this->Internal->FindDataRange<vtkTypeInt32>(band, minValue, maxValue);
      break;

    case GDT_Float32:
      this->Internal->FindDataRange<vtkTypeFloat32>(band, minValue, maxValue);
      break;

    case GDT_Float64:
      this->Internal->FindDataRange<vtkTypeFloat64>(band, minValue, maxValue);
      break;
    }

  return true;
}
