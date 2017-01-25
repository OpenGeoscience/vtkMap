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
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkUniformGrid.h>

// GDAL includes
#undef LT_OBJDIR // fixes compiler warning (collision w/vtkIOStream.h)
#include <gdal_priv.h>
#include <ogr_spatialref.h>

// STD includes
#include <iostream>
#include <sstream>

// Preprocessor directive enable/disable row inversion (y-flip)
// Although vtkImageData and GDALDataset have different origin position,
// reprojection of NLCD imagery only "works" if row inversion is not
// applied when converting between formats.
#define INVERT_ROWS 0

vtkStandardNewMacro(vtkGDALRasterConverter)

//----------------------------------------------------------------------------
class vtkGDALRasterConverter::vtkGDALRasterConverterInternal
{
public:
  GDALDataType ToGDALDataType(int vtkDataType);

  template<typename VTK_TYPE> void
  CopyToVTK(GDALDataset *gdalData,
            vtkDataArray *vtkData,
            vtkUniformGrid *uniformGridData);

  template<typename GDAL_TYPE> void
  FindDataRange(GDALRasterBand *band, double *minValue, double *maxValue);
};

//----------------------------------------------------------------------------
// Translates vtk data type to GDAL data type
GDALDataType
vtkGDALRasterConverter::vtkGDALRasterConverterInternal::
ToGDALDataType(int vtkDataType)
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

//----------------------------------------------------------------------------
// Copies contents of GDALDataset to vtkDataArray
template<typename VTK_TYPE>
void vtkGDALRasterConverter::vtkGDALRasterConverterInternal::
CopyToVTK(GDALDataset *dataset,
          vtkDataArray *array,
          vtkUniformGrid *uniformGridData)
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
    CPLErr err = band->RasterIO(GF_Read, 0, 0, xSize, ySize, buffer,
                                xSize, ySize, gdalDataType, 0, 0);

    int hasNoDataValue = 0;
    double noDataValue = band->GetNoDataValue(&hasNoDataValue);

    // Copy data from buffer to vtkDataArray
    // Traverse by gdal row & column to make y-inversion easier
    for (int row=0, index=0; row < ySize; row++)
      {
#if INVERT_ROWS
      // GDAL data starts at top-left, vtk at bottom-left
      // So need to invert in the y direction
      int targetRow = ySize - row - 1;
#else
      int targetRow = row;
#endif
      int offset = targetRow * xSize;
      for (int col=0; col < xSize; col++)
        {
        array->SetComponent(offset + col, i, buffer[index]);
        if (hasNoDataValue &&
            (static_cast<double>(buffer[index]) == noDataValue))
          {
          //std::cout << "Blank Point at col, row: " << col << ", " << row << std::endl;
          uniformGridData->BlankPoint(col, row, 0);
          }
        index++;
        }
      }

    // Check for color table
    if (band->GetColorInterpretation() == GCI_PaletteIndex)
      {
      GDALColorTable *gdalTable = band->GetColorTable();
      if (gdalTable->GetPaletteInterpretation() != GPI_RGB)
        {
        std::cerr << "Color table palette type not supported "
                  << gdalTable->GetPaletteInterpretation() << std::endl;
        continue;
        }

      vtkNew<vtkLookupTable> colorTable;
      char **categoryNames = band->GetCategoryNames();

      colorTable->IndexedLookupOn();
      int numEntries = gdalTable->GetColorEntryCount();
      colorTable->SetNumberOfTableValues(numEntries);
      std::stringstream ss;
      for (int i=0; i< numEntries; ++i)
        {
        const GDALColorEntry *gdalEntry = gdalTable->GetColorEntry(i);
        double r = static_cast<double>(gdalEntry->c1) / 255.0;
        double g = static_cast<double>(gdalEntry->c2) / 255.0;
        double b = static_cast<double>(gdalEntry->c3) / 255.0;
        double a = static_cast<double>(gdalEntry->c4) / 255.0;
        colorTable->SetTableValue(i, r, g, b, a);

        // Copy category name to lookup table annotation
        if (categoryNames)
          {
          // Only use non-empty names
          if (strlen(categoryNames[i]) > 0)
            {
            colorTable->SetAnnotation(vtkVariant(i), categoryNames[i]);
            }
          }
        else
          {
          // Create default annotation
          ss.str("");
          ss.clear();
          ss << "Category " << i;
          colorTable->SetAnnotation(vtkVariant(i), ss.str());
          }
        }

      //colorTable->Print(std::cout);
      array->SetLookupTable(colorTable.GetPointer());
      }

    }
  delete [] buffer;
}

//----------------------------------------------------------------------------
// Iterate overall values in raster band to find min & max
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
  for (int i=0; i<xSize*ySize; i++)
    {
    *minValue = *minValue < buffer[i] ? *minValue : buffer[i];
    *maxValue = *maxValue > buffer[i] ? *maxValue : buffer[i];
    }

  delete [] buffer;
}

//----------------------------------------------------------------------------
// Copy vtkDataArray contents to GDAL raster bands
template<class Iterator, typename VTK_TYPE>
void StaticCopyToGDAL(Iterator begin, Iterator end, VTK_TYPE typeVar,
                vtkDataArray *vtkData, GDALDataset *gdalData)
{
  // If data includes a lookup table, copy that
  GDALColorTable *gdalColorTable = NULL;
  vtkLookupTable *inputColorTable = vtkData->GetLookupTable();
  if (inputColorTable)
    {
    gdalColorTable = new GDALColorTable;
    vtkIdType tableSize = inputColorTable->GetNumberOfTableValues();
    double inputColor[4] = {0.0, 0.0, 0.0, 1.0};
    GDALColorEntry gdalColor;
    for (vtkIdType i = 0; i<tableSize; ++i)
      {
      inputColorTable->GetTableValue(i, inputColor);
      gdalColor.c1 = static_cast<short>(inputColor[0] * 255.0);
      gdalColor.c2 = static_cast<short>(inputColor[1] * 255.0);
      gdalColor.c3 = static_cast<short>(inputColor[2] * 255.0);
      gdalColor.c4 = static_cast<short>(inputColor[3] * 255.0);
      gdalColorTable->SetColorEntry(i, &gdalColor);
      }
    }

  // Create local buffer
  int stride = vtkData->GetNumberOfComponents();
  int numElements = vtkData->GetNumberOfTuples();
  VTK_TYPE *buffer = new VTK_TYPE[numElements];
  int xSize = gdalData->GetRasterXSize();
  int ySize = gdalData->GetRasterYSize();

  // Copy each vtk component to separate gdal band
  for (int i=0; i<stride; i++)
    {
    GDALRasterBand *band = gdalData->GetRasterBand(i+1);
    if (gdalColorTable)
      {
      band->SetColorTable(gdalColorTable);
      band->SetColorInterpretation(GCI_PaletteIndex);
      }

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
#if INVERT_ROWS
      // GDAL data starts at top-left, vtk at bottom-left
      // So need to invert in the y direction
      int targetRow = ySize - row - 1;
#else
      int targetRow = row;
#endif
      int offset = targetRow * xSize;
      for (int col=0; col < xSize; col++)
        {
        buffer[offset + col] = *iter;

        // Advance to next tuple
        // Todo is this loop the same as "iter += stride"?
        for (int k=0; k<stride; k++)
          {
          iter++;
          }
        }
      }

    // Copy from buffer to GDAL band
    GDALDataType gdalDataType = band->GetRasterDataType();
    CPLErr err = band->RasterIO(GF_Write, 0, 0, xSize, ySize, buffer,
                                xSize, ySize, gdalDataType, 0, 0);
    }

  delete gdalColorTable;
  delete [] buffer;
}

//----------------------------------------------------------------------------
vtkGDALRasterConverter::vtkGDALRasterConverter()
{
  this->Internal = new vtkGDALRasterConverterInternal();
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
// Copy image data contents, origin, & spacing to GDALDataset
bool vtkGDALRasterConverter::
CopyToGDAL(vtkImageData *input, GDALDataset *output)
{
  // Check that both images have the same dimensions
  int *inputDimensions = input->GetDimensions();
  if (output->GetRasterXSize() != inputDimensions[0] ||
      output->GetRasterYSize() != inputDimensions[1])
    {
    vtkErrorMacro(<< "Image dimensions do not match.");
    return false;
    }

  // Initialize geo transform
  double *origin = input->GetOrigin();
  double *spacing = input->GetSpacing();
  this->SetGDALGeoTransform(output, origin, spacing);

  // Check for NO_DATA_VALUE array
  int index = -1;
  vtkDataArray *array = input->GetFieldData()->GetArray("NO_DATA_VALUE", index);
  vtkDoubleArray *noDataArray = vtkDoubleArray::SafeDownCast(array);
  if (noDataArray)
    {
    for (int i=0; i<array->GetNumberOfTuples(); i++)
      {
      double value = noDataArray->GetValue(i);
      if (!vtkMath::IsNan(value))
        {
        GDALRasterBand *band = output->GetRasterBand(i+1);
        band->SetNoDataValue(value);
        }
      }
    }  // if (noDataArray)

  // Copy scalars to gdal bands
  array = input->GetPointData()->GetScalars();
  switch (array->GetDataType())
    {
    vtkDataArrayIteratorMacro(array,
      StaticCopyToGDAL(vtkDABegin, vtkDAEnd, vtkDAValueType(), array, output));
    }

  // Finis
  return true;
}

//----------------------------------------------------------------------------
GDALDataset *vtkGDALRasterConverter::
CreateGDALDataset(vtkImageData *imageData, const char *mapProjection)
{
  int *dimensions = imageData->GetDimensions();
  vtkDataArray *array = imageData->GetPointData()->GetScalars();
  int vtkDataType = array->GetDataType();
  int rasterCount = array->GetNumberOfComponents();
  GDALDataset *dataset =
    this->CreateGDALDataset(dimensions[0], dimensions[1],
                            vtkDataType, rasterCount);
  this->CopyToGDAL(imageData, dataset);
  this->SetGDALProjection(dataset, mapProjection);
  this->SetGDALGeoTransform(dataset, imageData->GetOrigin(),
                            imageData->GetSpacing());
  return dataset;
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
    return NULL;
    }

  // Initialize image
  vtkUniformGrid *image = vtkUniformGrid::New();
  int imageDimensions[3];
  imageDimensions[0] = dataset->GetRasterXSize();
  imageDimensions[1] = dataset->GetRasterYSize();
  imageDimensions[2] = 1;
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
      this->Internal->CopyToVTK<vtkTypeUInt8>(dataset, array, image);
      break;

    case GDT_UInt16:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_UINT16);
      this->Internal->CopyToVTK<vtkTypeUInt16>(dataset, array, image);
      break;

    case GDT_Int16:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_INT16);
      this->Internal->CopyToVTK<vtkTypeInt16>(dataset, array, image);
      break;

    case GDT_UInt32:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_UINT32);
      this->Internal->CopyToVTK<vtkTypeUInt32>(dataset, array, image);
      break;

    case GDT_Int32:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_INT32);
      this->Internal->CopyToVTK<vtkTypeInt32>(dataset, array, image);
      break;

    case GDT_Float32:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_FLOAT32);
      this->Internal->CopyToVTK<vtkTypeFloat32>(dataset, array, image);
      break;

    case GDT_Float64:
      array = vtkDataArray::CreateDataArray(VTK_TYPE_FLOAT64);
      this->Internal->CopyToVTK<vtkTypeFloat64>(dataset, array, image);
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
CopyBandInfo(GDALDataset *src, GDALDataset *dest)
{
  // Copy color interpretation and color table info
  int numSrcBands = src->GetRasterCount();
  for (int i=0; i<numSrcBands; i++)
    {
    int index = i+1;
    GDALRasterBand *srcBand = src->GetRasterBand(index);
    GDALRasterBand *destBand = dest->GetRasterBand(index);
    destBand->SetColorInterpretation(srcBand->GetColorInterpretation());

    GDALColorTable *colorTable = srcBand->GetColorTable();
    if (colorTable)
      {
      destBand->SetColorTable(colorTable);
      }
    }

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
CopyNoDataValues(GDALDataset *src, GDALDataset *dst)
{
  // Check that raster count is consistent and > 0
  int numSrcBands = src->GetRasterCount();
  int numDstBands = dst->GetRasterCount();
  if (numSrcBands != numDstBands)
    {
    vtkWarningMacro("raster count different between src & dst datasets");
    return;
    }

  if (numSrcBands == 0)
    {
    return;
    }

  double noDataValue = vtkMath::Nan();
  int success = -1;
  for (int i=0; i<numSrcBands; i++)
    {
    int index = i+1;
    GDALRasterBand *srcBand = src->GetRasterBand(index);
    noDataValue = srcBand->GetNoDataValue(&success);
    if (success)
      {
      GDALRasterBand *dstBand = dst->GetRasterBand(index);
      dstBand->SetNoDataValue(noDataValue);
      }
    }
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
