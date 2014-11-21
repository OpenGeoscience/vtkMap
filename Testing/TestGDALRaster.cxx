/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGDALRaster.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMap.h"
#include "vtkFeatureLayer.h"
#include "vtkRasterFeature.h"
#include "vtkOsmLayer.h"

#include <vtkGDALRasterReader.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageMapper3D.h>
#include <vtkImageMapToColors.h>
#include <vtkInteractorStyle.h>
#include <vtkLookupTable.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <iomanip>
#include <iostream>

int TestGDALRaster(int argc, char *argv[])
{
  if (argc < 2)
    {
    std::cout << "\n"
              << "Input GDAL file and display on map." << "\n"
              << "Usage: TestGDALImage  inputfile"
              <<"  [ZoomLevel  [CenterLat  CenterLon] ]  " << "\n"
              << std::endl;
    return -1;
    }


  // Instantiate vtkMap
  vtkNew<vtkMap> map;
  // Always set map's renderer *before* adding layers
  vtkNew<vtkRenderer> renderer;
  map->SetRenderer(renderer.GetPointer());
  map->SetCenter(31.5, 50.8);
  map->SetZoom(4);

   // Add OSM layer
  vtkNew<vtkOsmLayer> osmLayer;
  map->AddLayer(osmLayer.GetPointer());

  // Add feature layer w/GDALRaster feature
  vtkNew<vtkFeatureLayer> featureLayer;
  map->AddLayer(featureLayer.GetPointer());

  // Load GDAL raster image
  vtkGDALRasterReader *reader = vtkGDALRasterReader::New();
  reader->SetFileName(argv[1]);
  reader->Update();

  std::cout << "Projection string: " << reader->GetProjectionString() << "\n";
  std::cout << "Corner points:" << "\n";
  const double *corners = reader->GetGeoCornerPoints();
  for (int i=0, index=0; i<4; i++, index += 2)
    {
    std::cout << "  " << std::setprecision(12) << corners[index]
              << ", " << std::setprecision(12) << corners[index+1] << "\n";
    }
  std::cout << "Delta longitude: " << std::setprecision(12)
            << (corners[4] - corners[0]) << "\n";
  std::cout << "Delta latitude:  " << std::setprecision(12)
            << (corners[5] - corners[1]) << "\n";

  int dim[2];
  reader->GetRasterDimensions(dim);
  std::cout << "Raster dimensions: " << dim[0] << ", " << dim[1] << "\n";
  std::cout << "Driver: " << reader->GetDriverLongName() << "\n";

  vtkImageData *rasterData = reader->GetOutput();
  std::cout << "Scalar type: " << rasterData->GetScalarType() << " = "
            << rasterData->GetScalarTypeAsString() << "\n";
  std::cout << "Scalar size: " << rasterData->GetScalarSize()
            << " bytes" << "\n";
  int *rasterDim = rasterData->GetDimensions();
  std::cout << "Raster dimensions: " << rasterDim[0]
            << ", " << rasterDim[1] << "\n";
  double *range = rasterData->GetScalarRange();
  std::cout << "Scalar range: " << range[0]
            << ", " << range[1] << "\n";

  std::cout << std::endl;

  // Setup color lookup table
  vtkNew<vtkLookupTable> colorTable;
  colorTable->SetTableRange(range[0], range[1]);
  colorTable->SetValueRange(0.5, 0.5);
  colorTable->SetAlphaRange(0.5, 0.5);
  colorTable->Build();
  std::cout << "Table " << colorTable->GetNumberOfTableValues()
            << " colors" << std::endl;
  //colorTable->Print(std::cout);

  // Apply color map to image data
  vtkImageMapToColors *colorFilter = vtkImageMapToColors::New();
  colorFilter->SetLookupTable(colorTable.GetPointer());
  colorFilter->PassAlphaToOutputOn();
  colorFilter->SetInputData(reader->GetOutput());
  colorFilter->Update();

  // Initialize vtkRasterFeature
  vtkImageData *image = colorFilter->GetOutput();
  vtkNew<vtkRasterFeature> feature;
  feature->SetImageData(image);
  featureLayer->AddFeature(feature.GetPointer());

  colorFilter->Delete();
  reader->Delete();

 // Process optional args and set zoom, center
  int zoom = 1;  // default
  if (argc > 2)
    {
    zoom = atoi(argv[2]);
    }
  std::cout << "Setting zoom level to " << zoom << std::endl;
  map->SetZoom(zoom);

  double latitude = 0.0;
  double longitude = 0.0;
  if (argc > 4)
    {
    latitude = atof(argv[3]);
    longitude = atof(argv[4]);
    }
  std::cout << "Setting map center to latitude " << latitude
            << ", longitude " << longitude << std::endl;
  map->SetCenter(latitude, longitude);

  // Set up display
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.GetPointer());
  renderWindow->SetSize(500, 500);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow.GetPointer());
  interactor->SetInteractorStyle(map->GetInteractorStyle());
  interactor->Initialize();
  map->Draw();

  // Start interactor
  interactor->Start();

  // Finis
  return EXIT_SUCCESS;
}


int main(int argc, char *argv[])
{
  TestGDALRaster(argc, argv);
}
