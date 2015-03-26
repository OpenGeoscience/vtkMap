#include "vtkMap.h"
#include "vtkFeatureLayer.h"
#include "vtkGeoMapSelection.h"
#include "vtkInteractorStyleGeoMap.h"
#include "vtkMapMarkerSet.h"
#include "vtkMercator.h"
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

#include <iostream>

class PickCallback : public vtkCommand
{
public:
  static PickCallback *New()
    { return new PickCallback; }
  virtual void Execute(vtkObject *caller, unsigned long event, void* data)
    {
      switch (event)
        {
        case vtkInteractorStyleGeoMap::DisplayCompleteEvent:
          {
          double *latLonCoords = static_cast<double*>(data);
          std::cout << "Selected coordinates: \n"
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
        }  // switch
    }

  void SetMap(vtkMap *map)
  {
    this->Map = map;
  }

protected:
  vtkMap *Map;
};

int main()
{
  // Kitware geo coords (Clifton Park, NY)
  double kwLatitude = 42.849604;
  double kwLongitude = -73.758345;

  vtkNew<vtkMap> map;

  // Note: Always set map's renderer *before* adding layers
  vtkNew<vtkRenderer> rend;
  map->SetRenderer(rend.GetPointer());

  // Set viewpoint to display continental US
  double latLngCoords[] = {25.0, -115.0, 50.0, -75.0};
  map->SetVisibleBounds(latLngCoords);

  vtkNew<vtkOsmLayer> osmLayer;
  map->AddLayer(osmLayer.GetPointer());

  vtkNew<vtkRenderWindow> wind;
  wind->AddRenderer(rend.GetPointer());;
  //wind->SetSize(1920, 1080);
  wind->SetSize(800, 600);

  vtkNew<vtkRenderWindowInteractor> intr;
  intr->SetRenderWindow(wind.GetPointer());
  vtkInteractorStyle *style = map->GetInteractorStyle();
  vtkInteractorStyleGeoMap *mapStyle =
    vtkInteractorStyleGeoMap::SafeDownCast(style);
  //mapStyle->SetRubberBandModeToSelection();
  intr->SetInteractorStyle(style);

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
  feature->GetActor()->GetProperty()->SetColor(1.0, 0.1, 0.1);
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
  markerSet->ClusteringOn();
  featureLayer->AddFeature(markerSet.GetPointer());
  unsigned numMarkers = sizeof(latlonCoords) / sizeof(double) / 2;
  for (unsigned i=0; i<numMarkers; ++i)
    {
    double lat = latlonCoords[i][0];
    double lon = latlonCoords[i][1];
    markerSet->AddMarker(lat, lon);
    }

  // Set marker color & opacity
  markerSet->GetActor()->GetProperty()->SetOpacity(0.9);
  map->Draw();

  // Set callbacks
  vtkNew<PickCallback> pickCallback;
  pickCallback->SetMap(map.GetPointer());
  style->AddObserver(vtkInteractorStyleGeoMap::DisplayCompleteEvent,
                    pickCallback.GetPointer());
  style->AddObserver(vtkInteractorStyleGeoMap::SelectionCompleteEvent,
                    pickCallback.GetPointer());
  style->AddObserver(vtkInteractorStyleGeoMap::ZoomCompleteEvent,
                    pickCallback.GetPointer());

  intr->Start();

  //map->Print(std::cout);
  return 0;
}
