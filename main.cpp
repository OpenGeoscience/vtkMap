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

#include "vtkCommand.h"
#include "vtkMap.h"
#include "vtkCallbackCommand.h"
#include "vtkRendererCollection.h"

using namespace std;

int main()
{
  vtkMap *map = vtkMap::New();

  vtkRenderer *rend = vtkRenderer::New();
  map->SetRenderer(rend);
  map->Setlatitude(25);
  map->Setlongitude(50);
  map->SetZoom(5);
  vtkRenderWindow *wind = vtkRenderWindow::New();
  wind->AddRenderer(rend);
  wind->SetSize(1300, 700);
  wind->Render();

  double *cammeraPosition = rend->GetActiveCamera()->GetPosition();
  rend->GetActiveCamera()->SetPosition(cammeraPosition[0], cammeraPosition[1], cammeraPosition[2] + 10);
  map->Update();

  for (int i = 0; i < 500; i++)
    {
    wind->Render();
    }
}
