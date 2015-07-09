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
#include "vtkOsmLayer.h"
#include "vtkRasterFeature.h"

#include <vtkColorTransferFunction.h>
#include <vtkGDALRasterReader.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageProperty.h>
#include <vtkInteractorStyle.h>
#include <vtkLookupTable.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtksys/CommandLineArguments.hxx>

#include <iomanip>
#include <iostream>

//----------------------------------------------------------------------------
int TestGDALRaster(int argc, char *argv[])
{
  // Setup command line arguments
  std::string inputFile;
  bool showHelp = false;
  int zoomLevel = 1;
  std::vector<double> centerLatLon;
  bool useBobColormap = false;

  vtksys::CommandLineArguments arg;
  arg.Initialize(argc, argv);
  arg.StoreUnusedArguments(true);
  arg.AddArgument("-h", vtksys::CommandLineArguments::NO_ARGUMENT,
                  &showHelp, "show help message");
  arg.AddArgument("--help", vtksys::CommandLineArguments::NO_ARGUMENT,
                  &showHelp, "show help message");
  arg.AddArgument("-b", vtksys::CommandLineArguments::NO_ARGUMENT,
                  &useBobColormap, "use \"Bob\'s\" color map");
  arg.AddArgument("-c", vtksys::CommandLineArguments::MULTI_ARGUMENT,
                  &centerLatLon, "initial center (latitude longitude)");
  arg.AddArgument("-z", vtksys::CommandLineArguments::SPACE_ARGUMENT,
                  &zoomLevel, "initial zoom level (1-20)");

  if (argc < 2 || !arg.Parse() || showHelp)
    {
    std::cout << "\n"
              << "Input GDAL raster file and display on map." << "\n"
              << "Usage: TestGDALRaster  inputfile  [options]" << "\n"
              << "Note that:" << "\n"
              << "  1. Inputfile must contain corner points"
              << " specified in latitude/longitude" << "\n"
              << "  2. Input image is NOT warped or resampled,"
              << " therefore, assigned colors are NOT precise." << "\n"
              << "\n" << "Optional arguments:"
              << arg.GetHelp()
              << std::endl;
    return -1;
    }

  // Instantiate vtkMap
  vtkNew<vtkMap> map;
  // Always set map's renderer *before* adding layers
  vtkNew<vtkRenderer> renderer;
  map->SetRenderer(renderer.GetPointer());

  // Set initial center and zoom
  double latitude = centerLatLon.size() > 0 ? centerLatLon[0] : 0.0;
  double longitude = centerLatLon.size() > 1 ? centerLatLon[1] : 0.0;
  std::cout << "Setting map center to latitude " << latitude
            << ", longitude " << longitude << std::endl;
  map->SetCenter(latitude, longitude);
  std::cout << "Setting zoom level to " << zoomLevel << std::endl;
  map->SetZoom(zoomLevel);

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

  vtkImageData *image = reader->GetOutput();
  vtkNew<vtkRasterFeature> feature;
  feature->SetImageData(image);
  feature->GetActor()->GetProperty()->SetOpacity(0.5);
  featureLayer->AddFeature(feature.GetPointer());

  // Setup color mapping
  vtkImageProperty *prop = feature->GetActor()->GetProperty();
  double window = range[1] - range[0];
  double level = 0.5 * (range[0] + range[1]);
  prop->SetColorWindow(window);
  prop->SetColorLevel(level);
  if (useBobColormap)
    {
    std::cout << "Using Bob\'s color mapping function" << std::endl;
    vtkNew<vtkColorTransferFunction> colorFunction;
    colorFunction->AddRGBPoint(-1000.0, 0.0, 0.0, 0.0);
    colorFunction->AddRGBPoint(1000.0, 0.0, 0.0, 0.498);
    colorFunction->AddRGBPoint(2000.0, 0.0, 0.0, 1.0);
    colorFunction->AddRGBPoint(2200.0, 0.0, 0.0, 1.0);
    colorFunction->AddRGBPoint(2400.0, 0.333, 1.0, 0.0);
    colorFunction->AddRGBPoint(2600.0, 1.0, 1.0, 0.0);
    colorFunction->AddRGBPoint(3000.0, 1.0, 0.333, 0.0);
    colorFunction->Build();
    //colorFunction->Print(std::cout);
    prop->SetLookupTable(colorFunction.GetPointer());
    }
  else
    {
    std::cout << "Using default color lookup table" << std::endl;
    vtkNew<vtkLookupTable> colorTable;
    colorTable->SetTableRange(range[0], range[1]);
    colorTable->SetValueRange(0.5, 0.5);
    colorTable->Build();

    colorTable->SetBelowRangeColor(0.0, 0.0, 0.0, 0.0);
    colorTable->UseBelowRangeColorOn();

    //std::cout << "Table " << colorTable->GetNumberOfTableValues()
    //          << " colors" << std::endl;
    prop->SetLookupTable(colorTable.GetPointer());
    }

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
  reader->Delete();
  return EXIT_SUCCESS;
}


//----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  TestGDALRaster(argc, argv);
}
