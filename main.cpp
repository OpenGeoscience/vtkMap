#include <iostream>
#include <vtkRenderer.h>
#include <vtkTransformCoordinateSystems.h>
#include <vtkPointSet.h>
#include <vtkPlaneSource.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCamera.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>

#include "vtkCommand.h"
#include "vtkMap.h"
#include "vtkCallbackCommand.h"
#include "vtkRendererCollection.h"

using namespace std;

int main()
{
  vtkMap *map = vtkMap::New();

  vtkNew<vtkRenderer> rend;
  map->SetRenderer(rend.GetPointer());
  map->SetCenter(0, 0);
  map->SetZoom(5);

  vtkNew<vtkRenderWindow> wind;
  wind->AddRenderer(rend.GetPointer());
  wind->SetSize(1300, 700);

  vtkNew<vtkRenderWindowInteractor> intr;
  intr->SetRenderWindow(wind.GetPointer());

  rend->GetActiveCamera()->SetPosition(0.0, 0.0, 10.0);
  map->Update();

  wind->Render();
  intr->Start();
}
