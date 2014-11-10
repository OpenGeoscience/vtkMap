#include "vtkMap.h"
#include "vtkFeatureLayer.h"
#include "vtkMapMarkerSet.h"
#include "vtkMapPickResult.h"
#include "vtkMercator.h"
#include "vtkOsmLayer.h"
#include "vtkPolydataFeature.h"

#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkDistanceToCamera.h>
#include <vtkGlyph3D.h>
#include <vtkIdTypeArray.h>
#include <vtkInteractorStyle.h>
#include <vtkTransform.h>
#include <vtkTransformCoordinateSystems.h>
#include <vtkPointSet.h>
#include <vtkPlaneSource.h>
#include <vtkPlane.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkPointPicker.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRendererCollection.h>
#include <vtkRegularPolygonSource.h>

#include <iostream>

using namespace std;

class PickCallback : public vtkCommand
{
public:
  static PickCallback *New()
    { return new PickCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
      vtkRenderWindowInteractor *interactor =
        vtkRenderWindowInteractor::SafeDownCast(caller);
      int *pos = interactor->GetEventPosition();
      //std::cout << "Event position: " << pos[0] << ", " << pos[1] << std::endl;
      //std::cout << interactor->GetPicker()->GetClassName() << std::endl;

      vtkNew<vtkMapPickResult> pickResult;

      this->Map->PickPoint(pos, pickResult.GetPointer());
      switch (pickResult->GetMapFeatureType())
        {
        case VTK_MAP_FEATURE_NONE:
          break;

        case VTK_MAP_FEATURE_MARKER:
          std::cout << "Picked marker " << pickResult->GetMapFeatureId()
                    << std::endl;
          break;

        case VTK_MAP_FEATURE_CLUSTER:
          std::cout << "Picked cluster containing "
                    << pickResult->GetNumberOfMarkers() << " markers"
                    << std::endl;
        break;

        default:
          //vtkWarningMacro("Unrecognized map feature type "
          //                << pickResult->GetMapFeatureType());
          std::cerr << "Unrecognized map feature type "
                    << pickResult->GetMapFeatureType() << std::endl;
          break;
        }
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
  map->SetCenter(kwLatitude, kwLongitude);
  map->SetZoom(5);

  vtkOsmLayer* osmLayer = vtkOsmLayer::New();
  map->AddLayer(osmLayer);

  vtkNew<vtkRenderWindow> wind;
  wind->AddRenderer(rend.GetPointer());;
  //wind->SetSize(1920, 1080);
  wind->SetSize(800, 600);

  vtkNew<vtkRenderWindowInteractor> intr;
  intr->SetRenderWindow(wind.GetPointer());
  intr->SetInteractorStyle(map->GetInteractorStyle());
  intr->SetPicker(map->GetPicker());

  intr->Initialize();
  map->Draw();

  double center[2];
  map->GetCenter(center);

  // Initialize test polygon
  vtkFeatureLayer* featureLayer = vtkFeatureLayer::New();
  featureLayer->SetName("test-polygon");
  // Note: Always add feature layer to the map *before* adding features
  map->AddLayer(featureLayer);
  vtkNew<vtkRegularPolygonSource> testPolygon;
  testPolygon->SetNumberOfSides(50);
  testPolygon->SetRadius(2.0);
  vtkPolydataFeature* feature = vtkPolydataFeature::New();
  feature->GetMapper()->SetInputConnection(testPolygon->GetOutputPort());
  feature->GetActor()->GetProperty()->SetColor(1.0, 0.1, 0.1);
  feature->GetActor()->GetProperty()->SetOpacity(0.5);

  double x = kwLongitude;
  double y = vtkMercator::lat2y(kwLatitude);
  feature->GetActor()->SetPosition(x, y, 0.0);
  featureLayer->AddFeature(feature);
  map->Draw();

  // Instantiate markers for array of lat-lon coordinates
  double latlonCoords[][2] = {
    0.0, 0.0,
    42.849604, -73.758345,  // KHQ
    35.911373, -79.072205,  // KRS
    32.301393, -90.871495   // ERDC
  };
  vtkMapMarkerSet *markerSet = map->GetMapMarkerSet();
  markerSet->ClusteringOn();
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
  intr->AddObserver(vtkCommand::LeftButtonPressEvent, pickCallback.GetPointer());

  intr->Start();

  //map->Print(std::cout);
  return 0;
}
