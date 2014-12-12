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
#include "vtkGDALRasterConverter.h"
#include "vtkGDALRasterReprojection.h"

#include <vtkDataObject.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkUniformGrid.h>

#include <gdal_priv.h>

vtkStandardNewMacro(vtkRasterReprojectionFilter)

//----------------------------------------------------------------------------
class vtkRasterReprojectionFilter::vtkRasterReprojectionFilterInternal
{
public:
  vtkRasterReprojectionFilterInternal();
  ~vtkRasterReprojectionFilterInternal();

  vtkGDALRasterConverter *GDALConverter;
  vtkGDALRasterReprojection *GDALReprojection;
  GDALDataset *InputGDALDataset;
  GDALDataset *OutputGDALDataset;
};

//----------------------------------------------------------------------------
vtkRasterReprojectionFilter::
vtkRasterReprojectionFilterInternal::vtkRasterReprojectionFilterInternal()
{
  this->GDALConverter = vtkGDALRasterConverter::New();
  this->GDALReprojection = vtkGDALRasterReprojection::New();
  this->InputGDALDataset = NULL;
  this->OutputGDALDataset = NULL;
}

//----------------------------------------------------------------------------
vtkRasterReprojectionFilter::
vtkRasterReprojectionFilterInternal::~vtkRasterReprojectionFilterInternal()
{
  this->GDALConverter->Delete();
  this->GDALReprojection->Delete();
  if (this->InputGDALDataset)
    {
    GDALClose(this->InputGDALDataset);
    }
  if (this->OutputGDALDataset)
    {
    GDALClose(this->OutputGDALDataset);
    }
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

//----------------------------------------------------------------------------
bool vtkRasterReprojectionFilter::
SuggestOutputDimensions(const char *outputProjection,
                        int *nPixels, int *nLines) const
{
  return false;
}

//-----------------------------------------------------------------------------
int vtkRasterReprojectionFilter::
RequestInformation(vtkInformation * vtkNotUsed(request),
                   vtkInformationVector **vtkNotUsed(inputVector),
                   vtkInformationVector *outputVector)
{
  return VTK_ERROR;
}

//----------------------------------------------------------------------------
int vtkRasterReprojectionFilter::
RequestData(vtkInformation *vtkNotUsed(request),
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
  double *inputSpacing = inInfo->Get(vtkDataObject::SPACING());
  double *inputOrigin = inInfo->Get(vtkDataObject::ORIGIN());

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (!outInfo)
    {
    vtkErrorMacro("Invalid output information object");
    return VTK_ERROR;
    }

  // Validate current settings
  if (!this->OutputProjection)
    {
    vtkErrorMacro("No output projection specific");
    return VTK_ERROR;
    }

  // Compute output dimensions if needed
  if ((this->OutputDimensions[0] < 1) || (this->OutputDimensions[1] < !))
    {
    // Create mock GDALDataset
    }

  // Set output info
  // TEMP for testing only
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               inputDataExtent, 6);
  outInfo->Set(vtkDataObject::SPACING(), inputSpacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), inputOrigin, 3);

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
