#include "vtkMap.h"

#include <vtkActor.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkInteractorStyle.h>
#include <vtkTransformCoordinateSystems.h>
#include <vtkPointSet.h>
#include <vtkPlaneSource.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
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

int main()
{
  vtkNew<vtkMap> map;

  vtkNew<vtkRenderer> rend;
  map->SetRenderer(rend.GetPointer());
  map->SetCenter(40, -70);
  map->SetZoom(5);

  vtkNew<vtkRegularPolygonSource> marker;
  vtkNew<vtkPolyDataMapper> markerMapper;
  vtkNew<vtkActor> markerActor;
  marker->SetNumberOfSides(50);
  marker->SetRadius(5);
  markerActor->SetMapper(markerMapper.GetPointer());
  markerMapper->SetInputConnection(marker->GetOutputPort());
  markerActor->GetProperty()->SetOpacity(0.5);
  rend->AddActor(markerActor.GetPointer());

  vtkNew<vtkRenderWindow> wind;
  wind->AddRenderer(rend.GetPointer());;
  wind->SetSize(1920, 1080);

  vtkNew<vtkRenderWindowInteractor> intr;
  intr->SetRenderWindow(wind.GetPointer());
  intr->SetInteractorStyle(map->GetInteractorStyle());

  intr->Initialize();
  map->Draw();

  double center[2];
  map->GetCenter(center);

  vtkPoints* testPoints = vtkPoints::New();
  testPoints->InsertNextPoint(40.0, -70.0, 0.0);
  vtkPoints* newPoints = map->gcsToDisplay(testPoints);
  newPoints = map->displayToGcs(newPoints);
  markerActor->SetPosition(newPoints->GetPoint(0)[1], newPoints->GetPoint(0)[0], 0.0);
  map->Draw();

  vtkNew<vtkCallbackCommand> callback;
  callback->SetClientData(map.GetPointer());
  callback->SetCallback(callbackFunction);
  intr->AddObserver(vtkCommand::MouseWheelForwardEvent, callback.GetPointer());
  intr->AddObserver(vtkCommand::MouseWheelBackwardEvent, callback.GetPointer());
  intr->Start();
}

void callbackFunction(vtkObject* caller, long unsigned int vtkNotUsed(eventId),
                      void* clientData, void* vtkNotUsed(callData) )
{
  // Prove that we can access the sphere source
  vtkMap* map = static_cast<vtkMap*>(clientData);
  map->Draw();
}
