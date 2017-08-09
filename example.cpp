#include "vtkMap.h"
#include "vtkFeatureLayer.h"
#include "vtkGeoMapSelection.h"
#include "vtkInteractorStyleGeoMap.h"
#include "vtkMapMarkerSet.h"
#include "vtkMercator.h"
#include "vtkMultiThreadedOsmLayer.h"
#include "vtkOsmLayer.h"
#include "vtkPolydataFeature.h"

#include <vtkCollection.h>
#include <vtkCommand.h>
#include <vtkIdList.h>
#include <vtkInteractorStyle.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkNew.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRegularPolygonSource.h>
#include <vtksys/CommandLineArguments.hxx>

#include <iostream>

// ------------------------------------------------------------
class PickCallback : public vtkCommand
{
public:
  static PickCallback *New()
    { return new PickCallback; }

  void Execute(vtkObject *caller, unsigned long event, void* data) override
    {
      switch (event)
        {
        case vtkInteractorStyleGeoMap::DisplayClickCompleteEvent:
          {
          double *latLonCoords = static_cast<double*>(data);
          std::cout << "Point coordinates: \n"
                    << "  " << latLonCoords[0] << ", " << latLonCoords[1]
                    << std::endl;
          }
          break;

        case vtkInteractorStyleGeoMap::DisplayDrawCompleteEvent:
          {
          double *latLonCoords = static_cast<double*>(data);
          std::cout << "Rectangle coordinates: \n"
                    << "  " << latLonCoords[0] << ", " << latLonCoords[1]
                    << "\n  " << latLonCoords[2] << ", " << latLonCoords[3]
                    << std::endl;
          }
          break;

        case vtkInteractorStyleGeoMap::SelectionCompleteEvent:
          {
          vtkObject *object = static_cast<vtkObject*>(data);
          vtkGeoMapSelection *selection = vtkGeoMapSelection::SafeDownCast(object);

          double *latLonCoords = selection->GetLatLngBounds();
          std::cout << "Selected coordinates: \n"
                    << "  " << latLonCoords[0] << ", " << latLonCoords[1]
                    << "\n  " << latLonCoords[2] << ", " << latLonCoords[3]
                    << std::endl;

          vtkCollection *collection = selection->GetSelectedFeatures();
          std::cout << "Number of features: " << collection->GetNumberOfItems()
                    << std::endl;
          vtkNew<vtkIdList> cellIdList;
          vtkNew<vtkIdList> markerIdList;
          vtkNew<vtkIdList> clusterIdList;
          for (int i=0; i<collection->GetNumberOfItems(); i++)
            {
            vtkObject *object = collection->GetItemAsObject(i);
            std::cout << "  " << object->GetClassName() << "\n";
            vtkFeature *feature = vtkFeature::SafeDownCast(object);

            // Retrieve polydata cells (if relevant)
            if (selection->GetPolyDataCellIds(feature, cellIdList.GetPointer()))
              {
              if (cellIdList->GetNumberOfIds() > 0)
                {
                std::cout << "    Cell ids: ";
                for (int j=0; j<cellIdList->GetNumberOfIds(); j++)
                  {
                  std::cout << " " << cellIdList->GetId(j);
                  }
                std::cout << std::endl;
                }
              }

            // Retrieve marker ids (if relevant)
            if (selection->GetMapMarkerIds(feature, markerIdList.GetPointer(),
                                           clusterIdList.GetPointer()))
              {
                std::cout << "    Marker ids: ";
                for (int j=0; j<markerIdList->GetNumberOfIds(); j++)
                  {
                  std::cout << " " << markerIdList->GetId(j);
                  }
                std::cout << std::endl;

                std::cout << "    Cluster ids: ";
                for (int j=0; j<clusterIdList->GetNumberOfIds(); j++)
                  {
                  std::cout << " " << clusterIdList->GetId(j);
                  }
                std::cout << std::endl;
              }
            }  // for (i)
          }  // case
          break;

        case vtkInteractorStyleGeoMap::ZoomCompleteEvent:
          {
          double *latLonCoords = static_cast<double*>(data);
          std::cout << "Zoom coordinates: \n"
                    << "  " << latLonCoords[0] << ", " << latLonCoords[1]
                    << "\n  " << latLonCoords[2] << ", " << latLonCoords[3]
                    << std::endl;
          }
          break;

        case vtkInteractorStyleGeoMap::RightButtonCompleteEvent:
          {
          int* coords = static_cast<int*>(data);
          std::cout << "Right mouse click at ("
                    << coords[0] << ", " << coords[1] << ")" << std::endl;
          }
          break;
        }  // switch
    }

  void SetMap(vtkMap *map)
  {
    this->Map = map;
  }

protected:
  vtkMap *Map;
};

// ------------------------------------------------------------
int main(int argc, char *argv[])
{
  // Setup command line arguments
  std::string inputFile;
  int clusteringOff = false;
  bool showHelp = false;
  bool perspective = false;
  bool rubberBandDisplayOnly = false;
  bool rubberBandSelection = false;
  bool drawPolygonSelection = false;
  bool rubberBandZoom = false;
  bool singleThreaded = false;
  int zoomLevel = 2;
  std::vector<double> centerLatLon;
  std::string tileExtension = "png";
  std::string tileServer;
  std::string tileServerAttribution;

  vtksys::CommandLineArguments arg;
  arg.Initialize(argc, argv);
  arg.StoreUnusedArguments(true);
  arg.AddArgument("-h", vtksys::CommandLineArguments::NO_ARGUMENT,
                  &showHelp, "show help message");
  arg.AddArgument("--help", vtksys::CommandLineArguments::NO_ARGUMENT,
                  &showHelp, "show help message");
  arg.AddArgument("-a", vtksys::CommandLineArguments::SPACE_ARGUMENT,
                  &tileServerAttribution, "map-tile server attribution");
  arg.AddArgument("-d", vtksys::CommandLineArguments::NO_ARGUMENT,
                  &rubberBandDisplayOnly,
                  "set interactor to rubberband-draw mode");
  arg.AddArgument("-e", vtksys::CommandLineArguments::SPACE_ARGUMENT,
                  &tileExtension, "map-tile file extension (jpg, png, etc.)");
  arg.AddArgument("-c", vtksys::CommandLineArguments::MULTI_ARGUMENT,
                  &centerLatLon, "initial center (latitude longitude)");
  arg.AddArgument("-m", vtksys::CommandLineArguments::SPACE_ARGUMENT,
                  &tileServer, "map-tile server (tile.openstreetmaps.org)");
  arg.AddArgument("-o", vtksys::CommandLineArguments::NO_ARGUMENT,
                  &clusteringOff, "turn clustering off");
  arg.AddArgument("-p", vtksys::CommandLineArguments::NO_ARGUMENT,
                  &perspective, "use perspective projection");
  arg.AddArgument("-q", vtksys::CommandLineArguments::NO_ARGUMENT,
                  &rubberBandZoom,
                  "set interactor to rubberband zoom mode");
  arg.AddArgument("-r", vtksys::CommandLineArguments::NO_ARGUMENT,
                  &rubberBandSelection,
                  "set interactor to rubberband selection mode");
  arg.AddArgument("-P", vtksys::CommandLineArguments::NO_ARGUMENT,
                  &drawPolygonSelection,
                  "set interactor to polygon selection mode");
  arg.AddArgument("-s", vtksys::CommandLineArguments::NO_ARGUMENT,
                  &singleThreaded, "use single-threaded map I/O");
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

  // For reference, Kitware geo coords (Clifton Park, NY)
  double kwLatitude = 42.849604;
  double kwLongitude = -73.758345;

  // Note: Always set map's renderer *before* adding layers
  vtkNew<vtkRenderer> rend;
  map->SetRenderer(rend.GetPointer());

  // Set viewport
  if (centerLatLon.size() == 2)
    {
    map->SetCenter(centerLatLon[0], centerLatLon[1]);
    }
  else
    {
    // If center not specified, display CONUS
    double latLngCoords[] = {25.0, -115.0, 50.0, -75.0};
    map->SetVisibleBounds(latLngCoords);
    }

  map->SetPerspectiveProjection(perspective);

  // Adjust zoom level to perspective vs orthographic projection
  // Internally, perspective is zoomed in one extra level
  // Do the same here for perspective.
  zoomLevel += perspective ? 0 : 1;
  map->SetZoom(zoomLevel);

  vtkOsmLayer *osmLayer;
  if (singleThreaded)
    {
    osmLayer = vtkOsmLayer::New();
    }
  else
    {
    osmLayer = vtkMultiThreadedOsmLayer::New();
    }
  map->AddLayer(osmLayer);

  if (tileServer != "")
    {
    osmLayer->SetMapTileServer(
      tileServer.c_str(), tileServerAttribution.c_str(), tileExtension.c_str());
    }

  vtkNew<vtkRenderWindow> wind;
  wind->AddRenderer(rend.GetPointer());;
  //wind->SetSize(1920, 1080);
  wind->SetSize(800, 600);

  vtkNew<vtkRenderWindowInteractor> intr;
  intr->SetRenderWindow(wind.GetPointer());
  map->SetInteractor(intr.GetPointer());

  vtkMapType::Interaction mode = vtkMapType::Interaction::Default;
  if (rubberBandDisplayOnly)
  {
    mode = vtkMapType::Interaction::RubberBandDisplayOnly;
  }
  else if (rubberBandSelection)
  {
    mode = vtkMapType::Interaction::RubberBandSelection;
  }
  else if (drawPolygonSelection)
  {
    mode = vtkMapType::Interaction::PolygonSelection;
  }
  else if (rubberBandZoom)
  {
    mode = vtkMapType::Interaction::RubberBandZoom;
  }
  map->SetInteractionMode(mode);

  intr->Initialize();
  map->Draw();

  double latLon[4];
  map->GetVisibleBounds(latLon);
  std::cout << "lat-lon bounds: "
            << "(" << latLon[0] << ", " << latLon[1] << ")" << "  "
            << "(" << latLon[2] << ", " << latLon[3] << ")"
            << std::endl;

  // Initialize test polygon
  vtkNew<vtkFeatureLayer> featureLayer;
  featureLayer->SetName("test-polygon");
  // Note: Always add feature layer to the map *before* adding features
  map->AddLayer(featureLayer.GetPointer());
  vtkNew<vtkRegularPolygonSource> testPolygon;
  testPolygon->SetNumberOfSides(50);
  testPolygon->SetRadius(2.0);
  vtkNew<vtkPolydataFeature> feature;
  feature->GetMapper()->SetInputConnection(testPolygon->GetOutputPort());
  feature->GetActor()->GetProperty()->SetColor(0.0, 80.0/255, 80.0/255);  // (teal)
  feature->GetActor()->GetProperty()->SetOpacity(0.5);

  double x = kwLongitude;
  double y = vtkMercator::lat2y(kwLatitude);
  feature->GetActor()->SetPosition(x, y, 0.0);
  featureLayer->AddFeature(feature.GetPointer());
  map->Draw();

  // Instantiate markers for array of lat-lon coordinates
  double latlonCoords[][2] = {
    0.0, 0.0,
    42.849604, -73.758345,  // KHQ
    35.911373, -79.072205,  // KRS
    32.301393, -90.871495   // ERDC
  };
  vtkNew<vtkMapMarkerSet> markerSet;
  markerSet->SetClustering(!clusteringOff);
  featureLayer->AddFeature(markerSet.GetPointer());
  unsigned numMarkers = sizeof(latlonCoords) / sizeof(double) / 2;
  for (unsigned i=0; i<numMarkers; ++i)
    {
    double lat = latlonCoords[i][0];
    double lon = latlonCoords[i][1];
    markerSet->AddMarker(lat, lon);
    }

  map->Draw();

  // Set callbacks
  vtkNew<PickCallback> pickCallback;
  pickCallback->SetMap(map.GetPointer());
  map->AddObserver(vtkInteractorStyleGeoMap::DisplayClickCompleteEvent,
                    pickCallback.GetPointer());
  map->AddObserver(vtkInteractorStyleGeoMap::DisplayDrawCompleteEvent,
                    pickCallback.GetPointer());
  map->AddObserver(vtkInteractorStyleGeoMap::SelectionCompleteEvent,
                    pickCallback.GetPointer());
  map->AddObserver(vtkInteractorStyleGeoMap::ZoomCompleteEvent,
                    pickCallback.GetPointer());
  map->AddObserver(vtkInteractorStyleGeoMap::RightButtonCompleteEvent,
                    pickCallback.GetPointer());

  intr->Start();

  //map->Print(std::cout);
  osmLayer->Delete();
  return 0;
}
