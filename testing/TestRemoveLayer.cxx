#include "vtkMap.h"

#include "vtkFeatureLayer.h"
#include "vtkInteractorStyleGeoMap.h"
#include "vtkMapMarkerSet.h"
#include "vtkMercator.h"
#include "vtkMultiThreadedOsmLayer.h"
#include "vtkPolydataFeature.h"

#include <vtkActor.h>
#include <vtkInteractorStyle.h>
#include <vtkNew.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRegularPolygonSource.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include <iostream>

// ------------------------------------------------------------
int main(int argc, char* argv[])
{
  std::cout << "TestRemoveLayer" << std::endl;

  vtkNew<vtkMap> map;

  // Note: Always set map's renderer *before* adding layers
  vtkNew<vtkRenderer> rend;
  map->SetRenderer(rend.GetPointer());

  // Set viewport
  // For reference, Kitware geo coords (Clifton Park, NY)
  double kwLatitude = 42.849604;
  double kwLongitude = -73.758345;
  map->SetCenter(kwLatitude, kwLongitude);
  map->SetZoom(6);

  // Initialize base layer
  vtkNew<vtkMultiThreadedOsmLayer> osmLayer;
  map->AddLayer(osmLayer.GetPointer());

  // Add render window and interactor
  vtkNew<vtkRenderWindow> wind;
  wind->SetMultiSamples(0); // MSAA will create interpolated pixels
  wind->AddRenderer(rend.GetPointer());
  wind->SetSize(800, 600);

  vtkNew<vtkRenderWindowInteractor> intr;
  intr->SetRenderWindow(wind.GetPointer());
  map->SetInteractor(intr.GetPointer());

  intr->Initialize();
  map->Draw();

  // Initialize test polygon
  vtkFeatureLayer* polygonLayer = vtkFeatureLayer::New();
  polygonLayer->SetName("polygon-layer");
  // Note: Always add feature layer to the map *before* adding features
  map->AddLayer(polygonLayer);
  polygonLayer->Delete();
  vtkNew<vtkRegularPolygonSource> testPolygon;
  testPolygon->SetNumberOfSides(50);
  testPolygon->SetRadius(2.0);
  vtkPolydataFeature* polygonFeature = vtkPolydataFeature::New();
  polygonFeature->GetMapper()->SetInputConnection(testPolygon->GetOutputPort());
  polygonFeature->GetActor()->GetProperty()->SetColor(
    0.0, 80.0 / 255, 80.0 / 255); // (teal)
  polygonFeature->GetActor()->GetProperty()->SetOpacity(0.5);

  double x = kwLongitude;
  double y = vtkMercator::lat2y(kwLatitude);
  polygonFeature->GetActor()->SetPosition(x, y, 0.0);
  polygonLayer->AddFeature(polygonFeature);
  polygonFeature->Delete();
  map->Draw();

  // Instantiate markers for array of lat-lon coordinates
  vtkFeatureLayer* markerLayer = vtkFeatureLayer::New();
  markerLayer->SetName("marker-layer");
  map->AddLayer(markerLayer);
  markerLayer->Delete();

  double latlonCoords[][2] = {
    0.0, 0.0, 42.849604, -73.758345, // KHQ
    35.911373, -79.072205,           // KRS
    32.301393, -90.871495            // ERDC
  };
  vtkMapMarkerSet* markerSet = vtkMapMarkerSet::New();
  //markerSet->SetClustering(!clusteringOff);
  markerLayer->AddFeature(markerSet);
  markerSet->Delete();
  unsigned numMarkers = sizeof(latlonCoords) / sizeof(double) / 2;
  for (unsigned i = 0; i < numMarkers; ++i)
  {
    double lat = latlonCoords[i][0];
    double lon = latlonCoords[i][1];
    markerSet->AddMarker(lat, lon);
  }

  map->Draw();

  // Remove polygon layer, which also deletes it
  map->RemoveLayer(polygonLayer);
  map->Draw();

  // Remove marker layer, which also deletes it
  map->RemoveLayer(markerLayer);
  map->Draw();

  intr->Start();

  return 0;
}
