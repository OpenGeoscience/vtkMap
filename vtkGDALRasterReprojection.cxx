/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGDALRasterReprojection.h"

// VTK Includes
#include <vtkObjectFactory.h>

// GDAL Includes
#include <gdal_priv.h>
#include <gdalwarper.h>
#include <ogr_spatialref.h>

vtkStandardNewMacro(vtkGDALRasterReprojection)

//----------------------------------------------------------------------------
vtkGDALRasterReprojection::vtkGDALRasterReprojection()
{
  this->InputDataset = NULL;
  this->MaxError = 0.0;
  this->ResamplingAlgorithm = 0;
}

//----------------------------------------------------------------------------
vtkGDALRasterReprojection::~vtkGDALRasterReprojection()
{
}

//----------------------------------------------------------------------------
void vtkGDALRasterReprojection::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkGDALRasterReprojection" << std::endl
     << "InputDataset" << this->InputDataset << "\n"
     << "MaxError: " << this->MaxError << "\n"
     << "ResamplingAlgorithm" << this->ResamplingAlgorithm << "\n"
     << std::endl;
}

//----------------------------------------------------------------------------
bool vtkGDALRasterReprojection::
SuggestOutputDimensions(GDALDataset *dataset,const char *projection,
                        double geoTransform[6], int *nPixels, int *nLines,
                        double maxError)
{
  // Create OGRSpatialReference for projection
  OGRSpatialReference ref;
  ref.SetFromUserInput(projection);
  if (ref.Validate() != OGRERR_NONE)
    {
    vtkErrorMacro(<< "Projection is not valid or not supported: "
                  << projection);
    //return false;
    }
  char *outputWKT = NULL;
  ref.exportToWkt(&outputWKT);
  std::cout << "outputWKT: \n" << outputWKT << std::endl;

  // Create transformer
  const char *inputWKT = dataset->GetProjectionRef();
  bool useGCPs = false;
  int order = 0;  // only applies to GCP transforms
  void *transformer =
    GDALCreateGenImgProjTransformer(dataset, inputWKT, NULL, outputWKT,
                                    useGCPs, maxError, order);
  CPLFree(outputWKT);

  // Estimate transform coefficients and output image dimensions
  CPLErr err =
    GDALSuggestedWarpOutput(dataset, GDALGenImgProjTransform,
                            transformer, geoTransform, nPixels, nLines);
  GDALDestroyGenImgProjTransformer(transformer);
  std::cout << "Output image: " << *nPixels << " by " << *nLines << std::endl;

  return true;
}

//----------------------------------------------------------------------------
bool vtkGDALRasterReprojection::Reproject(GDALDataset *output)
{
  if (!this->InputDataset)
    {
    vtkErrorMacro(<< "InputDataset is not set");
    return false;
    }

  return false;
}
