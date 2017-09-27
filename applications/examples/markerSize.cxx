#include <iostream>

#include <vtkCollection.h>
#include <vtkCommand.h>
#include <vtkIdList.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegularPolygonSource.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtksys/CommandLineArguments.hxx>

#include "vtkFeatureLayer.h"
#include "vtkGeoMapSelection.h"
#include "vtkInteractorStyleGeoMap.h"
#include "vtkMap.h"
#include "vtkMapMarkerSet.h"
#include "vtkMercator.h"
#include "vtkMultiThreadedOsmLayer.h"
#include "vtkOsmLayer.h"
#include "vtkPolydataFeature.h"

// ------------------------------------------------------------
class ResizeCallback : public vtkCommand
{
public:
  static ResizeCallback* New() { return new ResizeCallback; }

  void Execute(vtkObject* caller, unsigned long event, void* data) override
  {
    switch (event)
    {
      case vtkCommand::KeyPressEvent:
      {
        auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
        const std::string key = interactor->GetKeySym();

        int deltaPoint = 0;
        int deltaCluster = 0;
        if (key == "Left")
          deltaPoint = -1;
        else if (key == "Right")
          deltaPoint = 1;
        else if (key == "Up")
          deltaCluster = 1;
        else if (key == "Down")
          deltaCluster = -1;
        else if (key == "u")
          MarkerSet->SetClusterMarkerSizeMode(vtkMapMarkerSet::USER_DEFINED);
        else if (key == "p")
          MarkerSet->SetClusterMarkerSizeMode(vtkMapMarkerSet::POINTS_CONTAINED);
        else
          return;

        // Point marker (droplet)
        if (deltaPoint)
        {
          const auto size = MarkerSet->GetPointMarkerSize() +
            deltaPoint * step;
          MarkerSet->SetPointMarkerSize(size);
        }

        // Cluster marker (circle)
        if (deltaCluster)
        {
          const auto size = MarkerSet->GetClusterMarkerSize() +
            deltaCluster * step;
          MarkerSet->SetClusterMarkerSize(size);
        }

        Map->Draw();
      }
      break;
    }
  }

  vtkMapMarkerSet* MarkerSet = nullptr;
  vtkMap* Map = nullptr;
  int step = 5;
};

// ------------------------------------------------------------
int main(int argc, char* argv[])
{
  // Setup command line arguments
  std::string inputFile;
  int clusteringOff = false;
  bool showHelp = false;
  int zoomLevel = 10;

  vtksys::CommandLineArguments arg;
  arg.Initialize(argc, argv);
  arg.StoreUnusedArguments(true);
  arg.AddArgument("-h", vtksys::CommandLineArguments::NO_ARGUMENT, &showHelp,
    "show help message");
  arg.AddArgument("--help", vtksys::CommandLineArguments::NO_ARGUMENT,
    &showHelp, "show help message");
  arg.AddArgument("-o", vtksys::CommandLineArguments::NO_ARGUMENT,
    &clusteringOff, "turn clustering off");
  arg.AddArgument("-z", vtksys::CommandLineArguments::SPACE_ARGUMENT,
    &zoomLevel, "initial zoom level (1-20)");

  if (!arg.Parse() || showHelp)
  {
    std::cout << "\n" << arg.GetHelp() << std::endl;
    return -1;
  }

  vtkNew<vtkMap> map;
  vtkNew<vtkRenderer> rend;
  map->SetRenderer(rend.GetPointer());

  // Kitware HQ coords
  const double kwLatitude = 42.849604;
  const double kwLongitude = -73.758345;
  map->SetCenter(kwLatitude, kwLongitude);
  map->SetZoom(zoomLevel + 1);

  // Layer 1 - Map tiles
  vtkOsmLayer* osmLayer;
  osmLayer = vtkMultiThreadedOsmLayer::New();
  map->AddLayer(osmLayer);
  osmLayer->Delete();

  vtkNew<vtkRenderWindow> wind;
  wind->AddRenderer(rend.GetPointer());
  wind->SetSize(800, 600);

  vtkMapType::Interaction mode = vtkMapType::Interaction::Default;
  map->SetInteractionMode(vtkMapType::Interaction::Default);

  vtkNew<vtkRenderWindowInteractor> intr;
  intr->SetRenderWindow(wind.GetPointer());
  map->SetInteractor(intr.GetPointer());
  intr->Initialize();

  // Layer 2 - Markers
  vtkNew<vtkFeatureLayer> markers1;
  markers1->SetName("markers1");
  map->AddLayer(markers1.GetPointer());

  vtkNew<vtkMapMarkerSet> markerSet;
  markerSet->SetClustering(!clusteringOff);
  markers1->AddFeature(markerSet.GetPointer());

  const double offset = 0.1;
  double latlonCoords[][2] = {
    0.0, 0.0,
    kwLatitude, kwLongitude,
    kwLatitude, kwLongitude + offset,
    kwLatitude + 2 * offset, kwLongitude,
    kwLatitude , kwLongitude - 3 * offset,
    kwLatitude - 4 * offset, kwLongitude
  };

  const size_t numMarkers = sizeof(latlonCoords) / sizeof(double) / 2;
  for (size_t i = 0; i < numMarkers; ++i)
  {
    const double lat = latlonCoords[i][0];
    const double lon = latlonCoords[i][1];
    markerSet->AddMarker(lat, lon);
  }

  // Interactor observer
  vtkNew<ResizeCallback> resizeCallback;
  intr->AddObserver(vtkCommand::KeyPressEvent, resizeCallback.GetPointer());
  resizeCallback->MarkerSet = markerSet.GetPointer();
  resizeCallback->Map = map.GetPointer();

  map->Draw();
  intr->Start();
  return 0;
}
