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
#undef LT_OBJDIR // fixes compiler warning (collision w/vtkIOStream.h)
#include <gdal_alg.h>
#include <gdal_priv.h>
#include <gdalwarper.h>
#include <ogr_spatialref.h>

vtkStandardNewMacro(vtkGDALRasterReprojection)

//----------------------------------------------------------------------------
vtkGDALRasterReprojection::vtkGDALRasterReprojection()
{
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
     << "MaxError: " << this->MaxError << "\n"
     << "ResamplingAlgorithm" << this->ResamplingAlgorithm << "\n"
     << std::endl;
}

//----------------------------------------------------------------------------
bool vtkGDALRasterReprojection::
SuggestOutputDimensions(GDALDataset *dataset, const char *projection,
                        double geoTransform[6], int *nPixels, int *nLines,
                        double maxError)
{
  // Create OGRSpatialReference for projection
  OGRSpatialReference ref;
  OGRErr errcode = ref.SetFromUserInput(projection);
  if (errcode != OGRERR_NONE)
    {
    vtkWarningMacro(<< "OGRSpatialReference::SetFromUserInput("
                    << projection << ") returned " << errcode
                    << ". You might need to set GDAL_DATA.");
    }
  char *outputWKT = NULL;
  errcode = ref.exportToWkt(&outputWKT);
  if (errcode != OGRERR_NONE)
    {
    vtkWarningMacro(<< "OGRSpatialReference::exportToWKT("
                    << ") returned " << errcode
                    << ". You might need to set GDAL_DATA.");
    std::cout << "Resulting outputWKT: \n" << outputWKT << std::endl;
    }

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
  //std::cout << "Output image: " << *nPixels << " by " << *nLines << std::endl;

  return true;
}

//----------------------------------------------------------------------------
bool vtkGDALRasterReprojection::
Reproject(GDALDataset *input, GDALDataset *output)
{
  // Convert this->ResamplingAlgorithm to GDALResampleAlg
  GDALResampleAlg algorithm = GRA_NearestNeighbour;
  switch (this->ResamplingAlgorithm)
    {
    case 0:  algorithm = GRA_NearestNeighbour; break;
    case 1:  algorithm = GRA_Bilinear;         break;
    case 2:  algorithm = GRA_Cubic;            break;
    case 3:  algorithm = GRA_CubicSpline;      break;
    case 4:  algorithm = GRA_Lanczos;          break;
    // GRA_Average and GRA_Mode available starting GDAL version 1.10
    case 5:  algorithm = GRA_Average;          break;
    case 6:  algorithm = GRA_Mode;             break;
    }

  GDALProgressFunc progressFcn = GDALTermProgress;
  void *progressArg = NULL;
  double memoryLimit = 0.0;  // use default
  GDALWarpOptions *warpOptions = GDALCreateWarpOptions();

  warpOptions->hSrcDS = input;
  warpOptions->hDstDS = output;
  warpOptions->nBandCount = 0;  // all bands
  warpOptions->pfnProgress = GDALTermProgress;

  warpOptions->pTransformerArg = GDALCreateGenImgProjTransformer(
    input, GDALGetProjectionRef(input),
    output, GDALGetProjectionRef(output),
    false, 0.0, 1);
  warpOptions->pfnTransformer = GDALGenImgProjTransform;

  // Set multithreaded option, even though it does not seem to work
  char **stringWarpOptions = NULL;
  stringWarpOptions = CSLSetNameValue(
    stringWarpOptions, "NUM_THREADS", "ALL_CPUS");
  warpOptions->papszWarpOptions = stringWarpOptions;

  CPLErr err = GDALReprojectImage(
    input, input->GetProjectionRef(),
    output, output->GetProjectionRef(),
    algorithm, memoryLimit, this->MaxError, progressFcn, progressArg, warpOptions);
  //std::cout << "warp returned: " << err << std::endl;

  return true;
}
