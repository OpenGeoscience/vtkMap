/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGeoJSON.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFeatureLayer.h"
#include "vtkGeoJSONMapFeature.h"
#include "vtkMap.h"
#include "vtkMercator.h"
#include "vtkOsmLayer.h"

#include <vtkCallbackCommand.h>
#include <vtkInteractorStyle.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include <iostream>

int TestGeoJSON(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << "\n"
              << "Input GeoJSON file and display on map."
              << "\n"
              << "Usage: TestGeoJSON inputfile"
              << "  [ZoomLevel  [CenterLat  CenterLon] ]  "
              << "\n"
              << std::endl;
    return -1;
  }

  // Instantiate vtkMap
  vtkNew<vtkMap> map;
  // Always set map's renderer *before* adding layers
  vtkNew<vtkRenderer> renderer;
  map->SetRenderer(renderer.GetPointer());

  // Add OSM layer
  vtkNew<vtkOsmLayer> osmLayer;
  map->AddLayer(osmLayer.GetPointer());

  // Add feature layer w/GeoJSON feature
  vtkNew<vtkFeatureLayer> featureLayer;
  map->AddLayer(featureLayer.GetPointer());
  vtkNew<vtkGeoJSONMapFeature> feature;
  std::ifstream in(argv[1], std::ios::in | std::ios::binary);
  if (in)
  {
    std::string content(std::istreambuf_iterator<char>(in.rdbuf()),
      std::istreambuf_iterator<char>());
    in.close();
    feature->SetInputString(content.c_str());
    featureLayer->AddFeature(feature.GetPointer());
    feature->GetActor()->GetProperty()->SetColor(0.1, 0.1, 1.0);
    feature->GetActor()->GetProperty()->SetOpacity(0.5);
    feature->GetActor()->GetProperty()->SetLineWidth(3.0);
    feature->GetActor()->GetProperty()->SetPointSize(16.0);
  }
  else
  {
    std::cerr << "Unable to open file " << argv[1] << std::endl;
    return -2;
  }

  // Process optional args and set zoom, center
  int zoom = 1; // default
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
  std::cout << "Setting map center to latitude " << latitude << ", longitude "
            << longitude << std::endl;
  map->SetCenter(latitude, longitude);

  // Set up display
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetMultiSamples(0); // MSAA will create interpolated pixels
  renderWindow->AddRenderer(renderer.GetPointer());
  renderWindow->SetSize(500, 500);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow.GetPointer());
  map->SetInteractor(interactor.GetPointer());
  interactor->Initialize();
  map->Draw();

  // Start interactor
  interactor->Start();

  // Finis
  return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
  TestGeoJSON(argc, argv);
}
