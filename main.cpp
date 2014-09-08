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
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRendererCollection.h>
#include <vtkSphereSource.h>

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

  vtkNew<vtkSphereSource> ss;
  vtkNew<vtkPolyDataMapper> sm;
  vtkNew<vtkActor> sa;
  ss->SetRadius(10.0);
  sa->SetMapper(sm.GetPointer());
  sm->SetInputConnection(ss->GetOutputPort());
  rend->AddActor(sa.GetPointer());

  vtkNew<vtkRenderWindow> wind;
  wind->AddRenderer(rend.GetPointer());;
  wind->SetSize(700, 700);

  vtkNew<vtkRenderWindowInteractor> intr;
  intr->SetRenderWindow(wind.GetPointer());
  intr->SetInteractorStyle(map->GetInteractorStyle());

  intr->Initialize();
  map->Draw();

  vtkPoints* testPoints = vtkPoints::New();
  testPoints->InsertNextPoint(-70.0, 40.0, 0.0);
  vtkPoints* newPoints = map->gcsToDisplay(testPoints);
  std::cerr << newPoints->GetPoint(0)[1] << " " << newPoints->GetPoint(0)[0] << std::endl;
  ss->SetCenter(newPoints->GetPoint(0)[1], newPoints->GetPoint(0)[0], 0.0);

  double center[2];
  map->GetCenter(center);

  std::cerr << "center is " << center[0] << " " << center[1] << std::endl;

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
