/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMapClustering.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMap.h"
#include "vtkMapMarkerSet.h"
#include "vtkOsmLayer.h"
#include <vtkCallbackCommand.h>
#include <vtkInteractorStyle.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtksys/CommandLineArguments.hxx>

#include <iostream>
#include <vector>
#include <utility>


//----------------------------------------------------------------------------
void scrollCallback(vtkObject* caller, long unsigned int vtkNotUsed(eventId),
                    void* clientData, void* vtkNotUsed(callData) )
{
  vtkMap* map = static_cast<vtkMap*>(clientData);
  map->Draw();
}



//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
 // Setup command line arguments
  std::string inputFile;
  int clusteringOff = false;
  bool showHelp = false;
  int zoomLevel = 10;
  std::vector<double> centerLatLon;

  vtksys::CommandLineArguments arg;
  arg.Initialize(argc, argv);
  arg.StoreUnusedArguments(true);
  arg.AddArgument("-h", vtksys::CommandLineArguments::NO_ARGUMENT,
                  &showHelp, "show help message");
  arg.AddArgument("--help", vtksys::CommandLineArguments::NO_ARGUMENT,
                  &showHelp, "show help message");
  arg.AddArgument("-c", vtksys::CommandLineArguments::MULTI_ARGUMENT,
                  &centerLatLon, "initial center (latitude longitude)");
  arg.AddArgument("-i", vtksys::CommandLineArguments::SPACE_ARGUMENT,
                  &inputFile, "input file with \"latitude, longitude\" pairs");
  arg.AddArgument("-o", vtksys::CommandLineArguments::NO_ARGUMENT,
                  &clusteringOff, "turn clustering off");
  arg.AddArgument("-z", vtksys::CommandLineArguments::SPACE_ARGUMENT,
                  &zoomLevel, "initial zoom level (1-20)");

  if (!arg.Parse() || showHelp)
    {
    std::cout << "\n"
              << arg.GetHelp()
              << std::endl;
    return -1;
    }


  vtkNew<vtkMap> map;
  // Always set map's renderer *before* adding layers
  vtkNew<vtkRenderer> renderer;
  map->SetRenderer(renderer.GetPointer());

  vtkNew<vtkOsmLayer> osmLayer;
  map->AddLayer(osmLayer.GetPointer());

  if (centerLatLon.size() < 2)
    {
    // Use KHQ coordinates
    centerLatLon.push_back(42.849604);
    centerLatLon.push_back(-73.758345);
    }
  map->SetCenter(centerLatLon[0], centerLatLon[1]);
  map->SetZoom(zoomLevel);

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer.GetPointer());
  renderWindow->SetSize(640, 640);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow.GetPointer());
  interactor->SetInteractorStyle(map->GetInteractorStyle());
  interactor->Initialize();
  map->Draw();

  // Add feature layer for markers
  vtkNew<vtkFeatureLayer> featureLayer;
  featureLayer->SetName("markers");
  map->AddLayer(featureLayer.GetPointer());

  // Optional input argument to pass in lat-lon coord pairs
  std::pair<double, double> latLon;
  std::vector<std::pair<double, double> > latLonPairs;
  if (inputFile == "")
    {
    //std::cout << "Test" << std::endl;
    double latLonCoords[][2] = {
      42.915081,  -73.805122,  // Country Knolls
      42.902851,  -73.687340,  // Mechanicville
      42.792580,  -73.681229,  // Waterford
      42.774239,  -73.700119,  // Cohoes
      42.779800,  -73.845680   // Niskayuna
    };
    unsigned numMarkers = sizeof(latLonCoords) / sizeof(double) / 2;
    for (unsigned i=0; i<numMarkers; ++i)
      {
      latLon.first = latLonCoords[i][0];
      latLon.second = latLonCoords[i][1];
      latLonPairs.push_back(latLon);
      }
    }
  else
    {
    // Read input file to populate latLonCoords
    double lat;
    double lon;
    std::ifstream in(inputFile.c_str());
    while(in >> lat >> lon)
      {
      latLon.first = lat;
      latLon.second = lon;
      latLonPairs.push_back(latLon);
      }
    }

  vtkNew<vtkMapMarkerSet> markerSet;
  bool clusteringOn = !clusteringOff;
  markerSet->SetClustering(clusteringOn);
  featureLayer->AddFeature(markerSet.GetPointer());
  std::vector<std::pair<double, double> >::const_iterator iter;
  for (iter=latLonPairs.begin(); iter != latLonPairs.end(); iter++)
    {
    double lat = iter->first;
    double lon = iter->second;
    markerSet->AddMarker(lat, lon);
    }

  map->Draw();

  // Hide the first marker
  markerSet->SetMarkerVisibility(0, false);
  map->Draw();

  interactor->Start();

  return EXIT_SUCCESS;
}
