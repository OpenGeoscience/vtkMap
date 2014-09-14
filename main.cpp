#include "vtkMap.h"

#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkDistanceToCamera.h>
#include <vtkGlyph3D.h>
#include <vtkInteractorStyle.h>
#include <vtkTransformCoordinateSystems.h>
#include <vtkPointSet.h>
#include <vtkPlaneSource.h>
#include <vtkPlane.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
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

double lat2y(double);


int main()
{
  vtkNew<vtkMap> map;

  vtkNew<vtkRenderer> rend;
  map->SetRenderer(rend.GetPointer());
  map->SetCenter(40, -70);
  map->SetZoom(0);

  vtkNew<vtkRegularPolygonSource> marker;
  vtkNew<vtkPolyDataMapper> markerMapper;
  vtkNew<vtkActor> markerActor;
  marker->SetNumberOfSides(50);
  marker->SetRadius(2.0);
  markerActor->SetMapper(markerMapper.GetPointer());
  markerMapper->SetInputConnection(marker->GetOutputPort());
  markerActor->GetProperty()->SetColor(1.0, 0.1, 0.1);
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

  // Initialize test marker
  vtkPoints* testPoints = vtkPoints::New(VTK_DOUBLE);
  double kwLatitude = 42.849604;
  double kwLongitude = -73.758345;
  //testPoints->InsertNextPoint(0.0, 0.0, 0.0);
  testPoints->InsertNextPoint(kwLatitude, kwLongitude, 0.0);
  vtkPoints *displayPoints = map->gcsToDisplay(testPoints);
  vtkPoints *gcsPoints = map->displayToGcs(displayPoints);
  double coords[3];
  gcsPoints->GetPoint(0, coords);
  double x = coords[1];         // longitude
  double y = lat2y(coords[0]);  // latitude
  markerActor->SetPosition(x, y, 0.0);
  map->Draw();

  // Initialize glyph-based markers
  vtkNew<vtkPoints> markerPoints;
  markerPoints->SetDataTypeToDouble();
  x = kwLongitude;
  y = lat2y(kwLatitude);
  markerPoints->InsertNextPoint(x, y, 0.0);

  double krsLatitude = 35.911373;
  double krsLongitude = -79.072205;
  x = krsLongitude;
  y = lat2y(krsLatitude);
  markerPoints->InsertNextPoint(x, y, 0.0);

  double erdcLatitude = 32.301393;
  double erdcLongitude = -90.871495;
  x = erdcLongitude;
  y = lat2y(erdcLatitude);
  markerPoints->InsertNextPoint(x, y, 0.0);
  vtkNew<vtkPolyData> markerPolys;
  markerPolys->SetPoints(markerPoints.GetPointer());

  vtkNew<vtkDistanceToCamera> dFilter;
  dFilter->SetScreenSize(50.0);
  dFilter->SetRenderer(rend.GetPointer());
  dFilter->SetInputData(markerPolys.GetPointer());

  vtkNew<vtkArrowSource> arrow;
  arrow->InvertOn();
  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(dFilter->GetOutputPort());
  glyph->SetSourceConnection(arrow->GetOutputPort());
  glyph->SetScaleModeToScaleByScalar();
  // Just gotta know this:
  glyph->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "DistanceToCamera");
  glyph->GeneratePointIdsOn();

  vtkNew<vtkPolyDataMapper> glyphMapper;
  glyphMapper->SetInputConnection(glyph->GetOutputPort());
  vtkNew<vtkActor> glyphActor;
  glyphActor->SetMapper(glyphMapper.GetPointer());
  rend->AddActor(glyphActor.GetPointer());
  map->Draw();


  vtkNew<vtkCallbackCommand> callback;
  callback->SetClientData(map.GetPointer());
  callback->SetCallback(callbackFunction);
  intr->AddObserver(vtkCommand::MouseWheelForwardEvent, callback.GetPointer());
  intr->AddObserver(vtkCommand::MouseWheelBackwardEvent, callback.GetPointer());
  intr->Start();

  map->Print(std::cout);
  return 0;
}

void callbackFunction(vtkObject* caller, long unsigned int vtkNotUsed(eventId),
                      void* clientData, void* vtkNotUsed(callData) )
{
  // Prove that we can access the sphere source
  vtkMap* map = static_cast<vtkMap*>(clientData);
  map->Draw();
}
