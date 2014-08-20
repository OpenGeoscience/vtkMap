#include "vtkMap.h"

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
#include <vtkCallbackCommand.h>
#include <vtkRendererCollection.h>
#include <vtkCommand.h>

#include <iostream>

using namespace std;

int main()
{
  vtkNew<vtkMap> map;

  vtkNew<vtkRenderer> rend;
  map->SetRenderer(rend.GetPointer());
  map->SetCenter(0, 0);
  map->SetZoom(5);

  vtkNew<vtkRenderWindow> wind;
  wind->AddRenderer(rend.GetPointer());
  wind->SetSize(1300, 700);

  vtkNew<vtkRenderWindowInteractor> intr;
  intr->SetRenderWindow(wind.GetPointer());

  intr->Initialize();
  map->Draw();
  intr->Start();
}
