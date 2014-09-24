#include "vtkMap.h"
#include "vtkMapMarkerSet.h"

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

static void callbackFunction ( vtkObject* caller, long unsigned int eventId,
          void* clientData, void* callData );

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

      vtkIdType markerId = this->Map->PickMarker(pos);
      if (markerId >= 0)
        {
        std::cout << "Picked marker " << markerId << std::endl;
        }
      else
        {
        std::cout << "Picked cluster" << std::endl;
        }
    }

  void SetMap(vtkMap *map)
  {
    this->Map = map;
  }

protected:
  vtkMap *Map;
};


double lat2y(double);


int main()
{
  vtkNew<vtkMap> map;

  vtkNew<vtkRenderer> rend;
  map->SetRenderer(rend.GetPointer());
  map->SetCenter(40, -70);
  map->SetZoom(0);

  vtkNew<vtkRenderWindow> wind;
  wind->AddRenderer(rend.GetPointer());;
  wind->SetSize(1920, 1080);

  vtkNew<vtkRenderWindowInteractor> intr;
  intr->SetRenderWindow(wind.GetPointer());
  intr->SetInteractorStyle(map->GetInteractorStyle());
  intr->SetPicker(map->GetPicker());

  intr->Initialize();
  map->Draw();

  double center[2];
  map->GetCenter(center);

  // Initialize test marker
  vtkNew<vtkPoints> testPoints;
  testPoints->SetDataTypeToDouble();
  double kwLatitude = 42.849604;
  double kwLongitude = -73.758345;
  //testPoints->InsertNextPoint(0.0, 0.0, 0.0);
  testPoints->InsertNextPoint(kwLatitude, kwLongitude, 0.0);
  vtkPoints *displayPoints = map->gcsToDisplay(testPoints.GetPointer());
  vtkPoints *gcsPoints = map->displayToGcs(displayPoints);
  double coords[3];
  gcsPoints->GetPoint(0, coords);
  double x = coords[1];         // longitude
  double y = lat2y(coords[0]);  // latitude
  vtkNew<vtkRegularPolygonSource> testPolygon;
  vtkNew<vtkPolyDataMapper> testMapper;
  vtkNew<vtkActor> testActor;
  testPolygon->SetNumberOfSides(50);
  testPolygon->SetRadius(2.0);
  testActor->SetMapper(testMapper.GetPointer());
  testMapper->SetInputConnection(testPolygon->GetOutputPort());
  testActor->GetProperty()->SetColor(1.0, 0.1, 0.1);
  testActor->GetProperty()->SetOpacity(0.5);
  rend->AddActor(testActor.GetPointer());
  testActor->SetPosition(x, y, 0.0);
  map->Draw();

  // Instantiate markers for array of lat-lon coordinates
  double latlonCoords[][2] = {
    0.0, 0.0,
    42.849604, -73.758345,  // KHQ
    35.911373, -79.072205,  // KRS
    32.301393, -90.871495   // ERDC
  };
  vtkMapMarkerSet *markerSet = map->GetMapMarkerSet();
  unsigned numMarkers = sizeof(latlonCoords) / sizeof(double) / 2;
  for (unsigned i=0; i<numMarkers; ++i)
    {
    double lat = latlonCoords[i][0];
    double lon = latlonCoords[i][1];
    markerSet->AddMarker(lat, lon);
    }
  map->Draw();

  // Set callbacks
  vtkNew<vtkCallbackCommand> callback;
  callback->SetClientData(map.GetPointer());
  callback->SetCallback(callbackFunction);
  intr->AddObserver(vtkCommand::MouseWheelForwardEvent, callback.GetPointer());
  intr->AddObserver(vtkCommand::MouseWheelBackwardEvent, callback.GetPointer());

  //PickCallback *pickCallback = PickCallback::New();
  vtkNew<PickCallback> pickCallback;
  pickCallback->SetMap(map.GetPointer());
  intr->AddObserver(vtkCommand::LeftButtonPressEvent, pickCallback.GetPointer());

  intr->Start();

  //map->Print(std::cout);
  return 0;
}

void callbackFunction(vtkObject* caller, long unsigned int vtkNotUsed(eventId),
                      void* clientData, void* vtkNotUsed(callData) )
{
  // Prove that we can access the sphere source
  vtkMap* map = static_cast<vtkMap*>(clientData);
  map->Draw();
}
