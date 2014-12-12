/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkRasterReprojectionFilter -
// .SECTION Description
// Applies map reprojection to vtkUniformGrid or vtkImageData.
// Internally uses GDAL/Proj4 for the reprojection calculations.

#ifndef __vtkRasterReprojectionFilter_h
#define __vtkRasterReprojectionFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkmap_export.h"

class VTKMAP_EXPORT vtkRasterReprojectionFilter : public vtkDataObjectAlgorithm
{
public:
  static vtkRasterReprojectionFilter* New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkRasterReprojectionFilter, vtkDataObjectAlgorithm);

  // Description:
  // Set the map-projection string for the input image data.
  // Can be specified either with "well known text" (WKT) formats,
  // (GEOGS[]), or shorter "user string" formats, such as EPSG:3857.
  // The default value is "EPSG:4326" (latitude/longitude).
  vtkSetStringMacro(InputProjection);
  vtkGetStringMacro(InputProjection);

  // Description:
  // Set the map-projection string for the output image data.
  vtkSetStringMacro(OutputProjection);
  vtkGetStringMacro(OutputProjection);

  // Description:
  // Set the width and height of the output image.
  // The default case is to leave this variable unset, in which case,
  // the filter will use the GDAL suggested dimensions to construct the
  // output image. This set method can be used to override this, and
  // impose specific output image dimensions.
  vtkSetVector2Macro(OutputDimensions, int);
  vtkGetVector2Macro(OutputDimensions, int);

  // Description:
  // Set the data value to represent blank points in the output image.
  // Note that this value only used when the input data is a
  // vtkUniformGrid; the NoDataValue is *not* used when the input data
  // is a vtkImageData object, since it does not support black points.
  vtkSetMacro(NoDataValue, double);
  vtkGetMacro(NoDataValue, double);

  // Description:
  // Set the maximum error, measured in input pixels, that is allowed
  // in approximating the GDAL reprojection transformation.
  // The default is 0.0, for exact calculations.
  vtkSetClampMacro(MaxError, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(MaxError, double);

  // Description:
  // Set the pixel resampling algorithm. Choices range between 0 and 6:
  //   0 = Nearest Neighbor (default)
  //   1 = Bilinear
  //   2 = Cubic
  //   3 = CubicSpline
  //   4 = Lanczos
  //   5 = Average
  //   6 = Mode
  vtkSetClampMacro(ResamplingAlgorithm, int, 0, 6);


  // Is this relevant/practical?
  // Description:
  // Compute the suggested image dimensions for a specified projection.
  // The outputProjection parameter can be either the full "well known text"
  // definition, or shorter commonly-used names such as "EPSG:4326" or
  // "WGS84".
  // This method is not required to use the filter, but is provided
  // for convenience, to obtain the output dimensions without performing
  // the reprojection calculations.
  // Returns boolean indicating if computed dimensions are valid.
  bool SuggestOutputDimensions(const char *outputProjection,
                               int *nPixels, int *nLines) const;

protected:
  vtkRasterReprojectionFilter();
  ~vtkRasterReprojectionFilter();

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  char *InputProjection;
  char *OutputProjection;
  int OutputDimensions[2];
  double NoDataValue;
  double MaxError;
  int ResamplingAlgorithm;

  class vtkRasterReprojectionFilterInternal;
  vtkRasterReprojectionFilterInternal *Internal;

private:
   // Not implemented:
  vtkRasterReprojectionFilter(const vtkRasterReprojectionFilter&);
  vtkRasterReprojectionFilter& operator=(const vtkRasterReprojectionFilter&);
};

#endif // __vtkRasterReprojectionFilter_h
