#include <iostream>

#include <vtkCollection.h>
#include <vtkCommand.h>
#include <vtkIdList.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkNew.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRegularPolygonSource.h>
#include <vtksys/CommandLineArguments.hxx>

#include "vtkMap.h"
#include "vtkFeatureLayer.h"
#include "vtkGeoMapSelection.h"
#include "vtkInteractorStyleGeoMap.h"
#include "vtkMapMarkerSet.h"
#include "vtkMercator.h"
#include "vtkMultiThreadedOsmLayer.h"
#include "vtkOsmLayer.h"
#include "vtkPolydataFeature.h"


// ------------------------------------------------------------
class MoveCallback : public vtkCommand
{
public:
  static MoveCallback *New()
    { return new MoveCallback; }

  virtual void Execute(vtkObject *caller, unsigned long event,
    void* data)
  {
    switch (event)
    {
      case vtkCommand::KeyPressEvent:
        {
          auto interactor = vtkRenderWindowInteractor::SafeDownCast(caller);
          const std::string key = interactor->GetKeySym();
          vtkMapType::Move direction;
          if (key == "Left")
          {
            direction = vtkMapType::Move::UP;
          }
          else if (key == "Right")
          {
            direction = vtkMapType::Move::DOWN;
          }
          else if (key == "Up")
          {
            direction = vtkMapType::Move::TOP;
          }
          else if (key == "Down")
          {
            direction = vtkMapType::Move::BOTTOM;
          }

          Map->MoveLayer(Layer, direction);
        }
        break;
    }
  }

  vtkLayer* Layer = nullptr;
  vtkMap* Map = nullptr;
};

// ------------------------------------------------------------
class PickCallback : public vtkCommand
{
public:
  static PickCallback *New()
    { return new PickCallback; }
  virtual void Execute(vtkObject *caller, unsigned long event, void* data)
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
            }
          }
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
                  "set interactor to rubberband selection mode");
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
  // Layer 1 - Map
  map->AddLayer(osmLayer);

  if (tileServer != "")
    {
    osmLayer->SetMapTileServer(
      tileServer.c_str(), tileServerAttribution.c_str(), tileExtension.c_str());
    }
  osmLayer->Delete();

  vtkNew<vtkRenderWindow> wind;
  wind->AddRenderer(rend.GetPointer());;
  wind->SetSize(800, 600);

  vtkNew<vtkRenderWindowInteractor> intr;
  intr->SetRenderWindow(wind.GetPointer());
  map->SetInteractor(intr.GetPointer());

  // Interactor observer
  vtkNew<MoveCallback> moveCallback;
  intr->AddObserver(vtkCommand::KeyPressEvent, moveCallback.GetPointer());

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

  // Picking Callback
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


  intr->Initialize();
  map->Draw();

  double latLon[4];
  map->GetVisibleBounds(latLon);
  std::cout << "lat-lon bounds: "
            << "(" << latLon[0] << ", " << latLon[1] << ")" << "  "
            << "(" << latLon[2] << ", " << latLon[3] << ")"
            << std::endl;

  ///////////////////// Layer 2 - Markers /////////////////////////////////////
  vtkNew<vtkFeatureLayer> markers1;
  markers1->SetName("markers1");
  map->AddLayer(markers1.GetPointer());

  // Instantiate markers for array of lat-lon coordinates
  vtkNew<vtkMapMarkerSet> markerSet;
  markerSet->SetClustering(!clusteringOff);
  markers1->AddFeature(markerSet.GetPointer());

  const double kwLatitude = 42.849604; // Kitware HQ coords
  const double kwLongitude = -73.758345;
  double latlonCoords[][2] = {
    0.0, 0.0,
    kwLatitude, kwLongitude,  // KHQ
    35.911373, -79.072205,    // KRS
    32.301393, -90.871495     // ERDC
  };

  const size_t numMarkers = sizeof(latlonCoords) / sizeof(double) / 2;
  for (size_t i = 0; i < numMarkers; ++i)
  {
    const double lat = latlonCoords[i][0];
    const double lon = latlonCoords[i][1];
    markerSet->AddMarker(lat, lon);
  }
  
  ///////////////////// Layer 3 - Markers2 /////////////////////////////////////
  vtkNew<vtkFeatureLayer> markers2;
  markers2->SetName("markers2");
  map->AddLayer(markers2.GetPointer());

  // Instantiate markers for array of lat-lon coordinates
  vtkNew<vtkMapMarkerSet> markerSet2;
  markerSet2->SetClustering(!clusteringOff);
  markers2->AddFeature(markerSet2.GetPointer());

  const double offset = 0.5;
  double coords[][2] = {
    0.0, 0.0,
    kwLatitude, kwLongitude + offset,  // KHQ
    35.911373, -79.072205 + offset,    // KRS
    32.301393, -90.871495 + offset     // ERDC
  };

  const size_t numMarkers2 = sizeof(coords) / sizeof(double) / 2;
  for (size_t i = 0; i < numMarkers2; ++i)
  {
    const double lat = coords[i][0];
    const double lon = coords[i][1];
    markerSet2->AddMarker(lat, lon);
  }

  ///////////////////// Layer 4 - Circle //////////////////////////////////////
  vtkNew<vtkFeatureLayer> circle;
  circle->SetName("circle");
  map->AddLayer(circle.GetPointer());

  // Note: Always add vtkFeatureLayer to the map before adding features
  vtkNew<vtkRegularPolygonSource> polygon;
  polygon->SetNumberOfSides(50);
  polygon->SetRadius(2.0);
  vtkNew<vtkPolydataFeature> feature;
  feature->GetMapper()->SetInputConnection(polygon->GetOutputPort());
  feature->GetActor()->GetProperty()->SetColor(0.0, 0.0, 1.0);
  feature->GetActor()->GetProperty()->SetOpacity(0.5);

  feature->GetActor()->SetPosition(kwLongitude, vtkMercator::lat2y(kwLatitude), 0.0);
  circle->AddFeature(feature.GetPointer());

  ///////////////////// Layer 5 - Circle2 //////////////////////////////////////
  vtkNew<vtkFeatureLayer> circle2;
  circle2->SetName("circle2");
  map->AddLayer(circle2.GetPointer());

  // Note: Always add vtkFeatureLayer to the map before adding features
  vtkNew<vtkRegularPolygonSource> polygon2;
  polygon2->SetNumberOfSides(50);
  polygon2->SetRadius(5.0);
  vtkNew<vtkPolydataFeature> feature2;
  feature2->GetMapper()->SetInputConnection(polygon2->GetOutputPort());
  feature2->GetActor()->GetProperty()->SetColor(0.0, 1.0, 0.0);
  feature2->GetActor()->GetProperty()->SetOpacity(0.5);

  feature2->GetActor()->SetPosition(-76.072205, vtkMercator::lat2y(38.911373), 0.0);
  circle2->AddFeature(feature2.GetPointer());

  //////////////////////// Manipulate layers ///////////////////////////////////
  map->MoveLayer(circle.GetPointer(), vtkMapType::Move::DOWN);
  map->MoveLayer(circle2.GetPointer(), vtkMapType::Move::BOTTOM);
  map->MoveLayer(circle2.GetPointer(), vtkMapType::Move::UP);

  moveCallback->Map = map.GetPointer();
  moveCallback->Layer = circle.GetPointer();

  intr->Start();
  return 0;
}
