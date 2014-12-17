/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRasterReprojectionFilter.h"

// VTK includes
#include "vtkGDALRasterConverter.h"
#include "vtkGDALRasterReprojection.h"

#include <vtkDataArray.h>
#include <vtkDataObject.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkUniformGrid.h>
#include <vtkXMLImageDataWriter.h>
// GDAL includes
#include <gdal_priv.h>

// std includes
#include <algorithm>

vtkStandardNewMacro(vtkRasterReprojectionFilter)

//----------------------------------------------------------------------------
class vtkRasterReprojectionFilter::vtkRasterReprojectionFilterInternal
{
public:
  vtkRasterReprojectionFilterInternal();
  ~vtkRasterReprojectionFilterInternal();

  GDALDataset *CreateFromVTK(vtkImageData *data, const char *projection);

  vtkGDALRasterConverter *GDALConverter;
  vtkGDALRasterReprojection *GDALReprojection;
  // GDALDataset *InputGDALDataset;
  GDALDataset *OutputGDALDataset;

  // Data saved during RequestInformation
  int InputImageExtent[6];
  double InputImageOrigin[3];
  double InputImageSpacing[3];

  double OutputImageGeoTransform[6];
};

//----------------------------------------------------------------------------
vtkRasterReprojectionFilter::
vtkRasterReprojectionFilterInternal::vtkRasterReprojectionFilterInternal()
{
  this->GDALConverter = vtkGDALRasterConverter::New();
  this->GDALReprojection = vtkGDALRasterReprojection::New();
  this->OutputGDALDataset = NULL;
  std::fill(this->InputImageExtent, this->InputImageExtent+6, 0);
  std::fill(this->InputImageOrigin, this->InputImageOrigin+3, 0.0);
  std::fill(this->InputImageSpacing, this->InputImageSpacing+3, 0.0);
  std::fill(this->OutputImageGeoTransform,
            this->OutputImageGeoTransform+6, 0.0);
}

//----------------------------------------------------------------------------
vtkRasterReprojectionFilter::
vtkRasterReprojectionFilterInternal::~vtkRasterReprojectionFilterInternal()
{
  this->GDALConverter->Delete();
  this->GDALReprojection->Delete();
  // if (this->InputGDALDataset)
  //   {
  //   GDALClose(this->InputGDALDataset);
  //   }
  if (this->OutputGDALDataset)
    {
    GDALClose(this->OutputGDALDataset);
    }
}

//----------------------------------------------------------------------------
// Uses data saved in previous RequestInformation() call
GDALDataset *vtkRasterReprojectionFilter::
vtkRasterReprojectionFilterInternal::CreateFromVTK(vtkImageData *imageData,
                                                   const char *projection)
{
  int *dimensions = imageData->GetDimensions();
  vtkDataArray *array = imageData->GetPointData()->GetScalars();
  int vtkDataType = array->GetDataType();
  int rasterCount = array->GetNumberOfComponents();
  GDALDataset *dataset =
    this->GDALConverter->CreateGDALDataset(dimensions[0], dimensions[1],
                                           vtkDataType, rasterCount);
  this->GDALConverter->ConvertToGDAL(imageData, dataset);
  this->GDALConverter->SetGDALProjection(dataset, projection);
  this->GDALConverter->SetGDALGeoTransform(dataset, this->InputImageOrigin,
                                           this->InputImageSpacing);
  return dataset;
}

//----------------------------------------------------------------------------
vtkRasterReprojectionFilter::vtkRasterReprojectionFilter()
{
  this->Internal = new vtkRasterReprojectionFilterInternal;
  this->InputProjection = NULL;
  this->OutputProjection = NULL;
  this->OutputDimensions[0] = this->OutputDimensions[1] = 0;
  this->NoDataValue = vtkMath::Nan();
  this->MaxError = 0.0;
  this->ResamplingAlgorithm = 0;
}

//----------------------------------------------------------------------------
vtkRasterReprojectionFilter::~vtkRasterReprojectionFilter()
{
  if (this->InputProjection)
    {
    delete [] this->InputProjection;
    }
  if (this->OutputProjection)
    {
    delete [] this->OutputProjection;
    }
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkRasterReprojectionFilter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InputProjection: ";
  if (this->InputProjection)
    {
    os << this->InputProjection;
    }
  else
    {
    os << "(not specified)";
    }
  os << "\n";
  os << indent << "InputProjection: ";

  if (this->OutputProjection)
    {
    os << this->OutputProjection;
    }
  else
    {
    os << "(not specified)";
    }
  os << "\n";

  os << indent << "OutputDimensions: " << OutputDimensions[0]
     << ", " << OutputDimensions[1] << "\n"
     << indent << "NoDataValue: " << this->NoDataValue << "\n"
     << indent << "MaxError: " << this->MaxError << "\n"
     << indent << "ResamplingAlgorithm: " << this->ResamplingAlgorithm<< "\n"
     << std::endl;
}

//-----------------------------------------------------------------------------
int vtkRasterReprojectionFilter::
RequestData(vtkInformation * vtkNotUsed(request),
            vtkInformationVector **inputVector,
            vtkInformationVector *outputVector)
{
  // Get the input image data
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }

  vtkDataObject *inDataObject = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (!inDataObject)
    {
    return 0;
    }

  vtkImageData *inImageData = vtkImageData::SafeDownCast(inDataObject);
  if (!inImageData)
    {
    return 0;
    }
  //std::cout << "RequestData() has image to reproject!" << std::endl;

  // Get the output image
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
    {
    return 0;
    }

  // Construct GDAL dataset from input image
  GDALDataset *inputGDAL =
    this->Internal->CreateFromVTK(inImageData, this->InputProjection);

  if (this->Debug)
    {
    std::string tifFileName = "inputGDAL.tif";
    this->Internal->GDALConverter->WriteTifFile(inputGDAL, tifFileName.c_str());
    std::cout << "Wrote " << tifFileName << std::endl;

    double minValue, maxValue;
    this->Internal->GDALConverter->FindDataRange(inputGDAL, 1,
                                                 &minValue, &maxValue);
    std::cout << "Min: " << minValue << "  Max: " << maxValue << std::endl;
    }

  // Construct GDAL dataset for output image
  vtkDataArray *array = inImageData->GetPointData()->GetScalars();
  int vtkDataType = array->GetDataType();
  int rasterCount = array->GetNumberOfComponents();
  this->Internal->OutputGDALDataset =
    this->Internal->GDALConverter->CreateGDALDataset(this->OutputDimensions[0],
      this->OutputDimensions[1], vtkDataType, rasterCount);
  this->Internal->GDALConverter->SetGDALProjection(
    this->Internal->OutputGDALDataset, this->OutputProjection);
  this->Internal->OutputGDALDataset->SetGeoTransform(
    this->Internal->OutputImageGeoTransform);

  // Apply the reprojection
  this->Internal->GDALReprojection->SetInputDataset(inputGDAL);
  this->Internal->GDALReprojection->SetMaxError(this->MaxError);
  this->Internal->GDALReprojection->SetResamplingAlgorithm(
    this->ResamplingAlgorithm);
  this->Internal->GDALReprojection->Reproject(
    this->Internal->OutputGDALDataset);

  if (this->Debug)
    {
    std::string tifFileName = "reprojectGDAL.tif";
    this->Internal->GDALConverter->WriteTifFile(
      this->Internal->OutputGDALDataset, tifFileName.c_str());
    std::cout << "Wrote " << tifFileName << std::endl;
    double minValue, maxValue;
    this->Internal->GDALConverter->FindDataRange(
      this->Internal->OutputGDALDataset, 1, &minValue, &maxValue);
    std::cout << "Min: " << minValue << "  Max: " << maxValue << std::endl;
    }

  // Done with input dataset
  GDALClose(inputGDAL);

  // Convert output dataset to vtkUniformGrid
  vtkUniformGrid *outputImage =
    this->Internal->GDALConverter->CreateVTKUniformGrid(
      this->Internal->OutputGDALDataset);
  outputImage->SetDimensions(279, 320, 1);


  // Write image to file
  const char *outputFilename = "computed.vti";
  vtkNew<vtkXMLImageDataWriter> writer;
  writer->SetFileName(outputFilename);
  writer->SetInputData(outputImage);
  writer->SetDataModeToAscii();
  writer->Write();
  std::cout << "Wrote " << outputFilename << std::endl;


  //vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkUniformGrid *output = vtkUniformGrid::GetData(outInfo);

  int extent[] = {0, 278, 0, 319, 0, 0};
  outputImage->SetExtent(extent);
  output->SetExtent(outputImage->GetExtent());
  output->SetDimensions(outputImage->GetDimensions());
  output->SetOrigin(outputImage->GetOrigin());
  output->SetSpacing(outputImage->GetSpacing());
  output->ShallowCopy(outputImage);

  outputImage->Delete();
  return VTK_OK;
}

//-----------------------------------------------------------------------------
int vtkRasterReprojectionFilter::
RequestUpdateExtent(vtkInformation * vtkNotUsed(request),
                    vtkInformationVector **inputVector,
                    vtkInformationVector *outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
              this->Internal->InputImageExtent, 6);
  return VTK_OK;
}

//-----------------------------------------------------------------------------
int vtkRasterReprojectionFilter::
RequestInformation(vtkInformation * vtkNotUsed(request),
                   vtkInformationVector **inputVector,
                   vtkInformationVector *outputVector)
{
  // Get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()) ||
      !inInfo->Has(vtkDataObject::SPACING()) ||
      !inInfo->Has(vtkDataObject::ORIGIN()))
    {
    vtkErrorMacro("Input information missing");
    return VTK_ERROR;
    }
  int *inputDataExtent =
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  std::copy(inputDataExtent, inputDataExtent+6, this->Internal->InputImageExtent);

  double *inputOrigin = inInfo->Get(vtkDataObject::ORIGIN());
  std::copy(inputOrigin, inputOrigin+3, this->Internal->InputImageOrigin);

  double *inputSpacing = inInfo->Get(vtkDataObject::SPACING());

  std::cout << "Whole extent: " << inputDataExtent[0]
            << ", " << inputDataExtent[1]
            << ", " << inputDataExtent[2]
            << ", " << inputDataExtent[3]
            << ", " << inputDataExtent[4]
            << ", " << inputDataExtent[5]
            << std::endl;
  std::cout << "Input spacing: " << inputSpacing[0]
            << ", " << inputSpacing[1]
            << ", " << inputSpacing[2] << std::endl;
  std::cout << "Input origin: " << inputOrigin[0]
            << ", " << inputOrigin[1]
            << ", " << inputOrigin[2] << std::endl;

   vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
    {
    vtkErrorMacro("Invalid output information object");
    return VTK_ERROR;
    }

  // Validate current settings
  if (!this->InputProjection)
    {
    vtkErrorMacro("No input projection specified");
    return VTK_ERROR;
    }

  if (!this->OutputProjection)
    {
    vtkErrorMacro("No output projection specified");
    return VTK_ERROR;
    }

  // Hack in origin & spacing for sa052483.tif
  this->Internal->InputImageOrigin[0] = 45.999583454395179;
  this->Internal->InputImageOrigin[1] = 29.250416763514124;
  this->Internal->InputImageSpacing[0] = 0.000833333333333;
  this->Internal->InputImageSpacing[1] = -0.000833333333333;  // think I have to invert

  // Create GDALDataset to compute suggested output
  int xDim = inputDataExtent[1] - inputDataExtent[0] + 1;
  int yDim = inputDataExtent[3] - inputDataExtent[2] + 1;
  GDALDataset *gdalDataset =
    this->Internal->GDALConverter->CreateGDALDataset(
      xDim, yDim, VTK_UNSIGNED_CHAR, 1);
  this->Internal->GDALConverter->SetGDALProjection(
    gdalDataset, this->InputProjection);
  this->Internal->GDALConverter->SetGDALGeoTransform(
    gdalDataset, this->Internal->InputImageOrigin,
    this->Internal->InputImageSpacing);

  int nPixels = 0;
  int nLines = 0;
  this->Internal->GDALReprojection->SuggestOutputDimensions(
    gdalDataset, this->OutputProjection,
    this->Internal->OutputImageGeoTransform, &nPixels, &nLines);
  GDALClose(gdalDataset);

  if ((this->OutputDimensions[0] < 1) || (this->OutputDimensions[1] < 1))
    {
    this->OutputDimensions[0] = nPixels;
    this->OutputDimensions[1] = nLines;
    }

  // Set output info
  int outputDataExtent[6] = {};
  outputDataExtent[1] = this->OutputDimensions[0] - 1;
  outputDataExtent[3] = this->OutputDimensions[1] - 1;
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               outputDataExtent, 6);

  double outputImageOrigin[3] = {};
  outputImageOrigin[0] = this->Internal->OutputImageGeoTransform[0];
  outputImageOrigin[1] = this->Internal->OutputImageGeoTransform[3];
  outputImageOrigin[2] = 1.0;
  outInfo->Set(vtkDataObject::SPACING(), outputImageOrigin, 3);

  double outputImageSpacing[3] = {};
  outputImageSpacing[0] =  this->Internal->OutputImageGeoTransform[1];
  outputImageSpacing[1] = -this->Internal->OutputImageGeoTransform[5];
  outputImageSpacing[2] = 1.0;
  outInfo->Set(vtkDataObject::ORIGIN(), outputImageSpacing, 3);

  return VTK_OK;
}

//-----------------------------------------------------------------------------
int vtkRasterReprojectionFilter::
FillInputPortInformation(int port, vtkInformation* info)
{
  this->Superclass::FillInputPortInformation(port, info);
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
    return VTK_OK;
    }
  else
    {
    vtkErrorMacro("Input port: " << port << " is not a valid port");
    return VTK_ERROR;
    }
  return VTK_OK;
}

//-----------------------------------------------------------------------------
int vtkRasterReprojectionFilter::
FillOutputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
    {
    info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUniformGrid");
    return VTK_OK;
    }
  else
    {
    vtkErrorMacro("Output port: " << port << " is not a valid port");
    return VTK_ERROR;
    }
}
