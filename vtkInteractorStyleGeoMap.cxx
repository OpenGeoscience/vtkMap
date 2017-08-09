/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMap

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "Timer.h"
#include "vtkInteractorStyleGeoMap.h"
#include "vtkGeoMapSelection.h"
#include "vtkMap.h"

#include <vtkActor2D.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCommand.h>
#include <vtkDoubleArray.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkPoints.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <algorithm>

vtkStandardNewMacro(vtkInteractorStyleGeoMap);

//-----------------------------------------------------------------------------
vtkInteractorStyleGeoMap::vtkInteractorStyleGeoMap() :
   vtkInteractorStyleRubberBand2D()
,  Timer(std::unique_ptr<vtkMapType::Timer>(new vtkMapType::Timer))
{
  this->Map = NULL;
  this->RubberBandMode = DisabledMode;
}

//-----------------------------------------------------------------------------
vtkInteractorStyleGeoMap::~vtkInteractorStyleGeoMap() = default;

//-----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::OnLeftButtonDown()
{
  if (this->IsDoubleClick())
  {
    this->ZoomIn(2);
    return;
  }

  if (this->RubberBandMode == vtkInteractorStyleGeoMap::DisabledMode)
    {
    // Default map interaction == select feature & start pan
    int *pos = this->Interactor->GetEventPosition();

    vtkDebugMacro("StartPan()");
    this->Interaction = PANNING;
    this->StartPan();
    }

  this->StartPosition[0] = this->Interactor->GetEventPosition()[0];
  this->StartPosition[1] = this->Interactor->GetEventPosition()[1];
  this->EndPosition[0] = this->StartPosition[0];
  this->EndPosition[1] = this->StartPosition[1];

  this->Superclass::OnLeftButtonDown();
}

//-----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::OnLeftButtonUp()
{
  vtkDebugMacro("EndPan()");
  this->EndPan();

  if (this->RubberBandMode == vtkInteractorStyleGeoMap::DisabledMode)
    {
    this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_DEFAULT);
    }

  // Get corner points of interaction, sorted by min/max
  int boundCoords[4];
  boundCoords[0] = std::min(this->StartPosition[0], this->EndPosition[0]);
  boundCoords[1] = std::min(this->StartPosition[1], this->EndPosition[1]);
  boundCoords[2] = std::max(this->StartPosition[0], this->EndPosition[0]);
  boundCoords[3] = std::max(this->StartPosition[1], this->EndPosition[1]);
  int area = (boundCoords[2] - boundCoords[0]) *
    (boundCoords[3] - boundCoords[1]);
  // Use threshold to distinguish between click and movement
  bool moved = area > 25;
  vtkDebugMacro("RubberBand complete with points:"
                << " " << boundCoords[0] << ", " << boundCoords[1]
                << "  " << boundCoords[2] << ", " << boundCoords[3]
                << ", area  " << area << ", moved " << moved);

  // Compute lat-lon coordinates
  double latLonCoords[4];
  double displayCoords[2];
  double computedCoords[3];
  displayCoords[0] = boundCoords[0];
  displayCoords[1] = boundCoords[1];
  this->Map->ComputeLatLngCoords(displayCoords, 0.0, computedCoords);
  latLonCoords[0] = computedCoords[0];
  latLonCoords[1] = computedCoords[1];

  displayCoords[0] = boundCoords[2];
  displayCoords[1] = boundCoords[3];
  this->Map->ComputeLatLngCoords(displayCoords, 0.0, computedCoords);
  latLonCoords[2] = computedCoords[0];
  latLonCoords[3] = computedCoords[1];

  // Default mode
  if (this->RubberBandMode == vtkInteractorStyleGeoMap::DisabledMode)
    {
    if (!moved)  // treat as mouse click
      {
      vtkNew<vtkGeoMapSelection> pickResult;
      pickResult->SetLatLngBounds(latLonCoords);

      // Expanding the pick area reduces the number of misses
      // by vtkHardwareSelector, which is used in
      // vtkGeoMapFeatureSelector::PickArea(). April 2017
      int x = (this->StartPosition[0] + this->EndPosition[0]) / 2;
      int y = (this->StartPosition[1] + this->EndPosition[1]) / 2;
      boundCoords[0] = x - 10;
      boundCoords[1] = y - 10;
      boundCoords[2] = x + 10;
      boundCoords[3] = y + 10;

      this->Map->PickArea(boundCoords, pickResult.GetPointer());
      this->InvokeEvent(SelectionCompleteEvent, pickResult.GetPointer());
      }
    }

  // Display-only mode
  else if (this->RubberBandMode == vtkInteractorStyleGeoMap::DisplayOnlyMode)
    {
    int command = moved ? DisplayDrawCompleteEvent : DisplayClickCompleteEvent;
    this->InvokeEvent(command, latLonCoords);
    }

  // Selection mode
  else if (this->RubberBandMode == vtkInteractorStyleGeoMap::SelectionMode)
    {
    vtkNew<vtkGeoMapSelection> pickResult;
    pickResult->SetLatLngBounds(latLonCoords);
    this->Map->PickArea(boundCoords, pickResult.GetPointer());
    this->InvokeEvent(SelectionCompleteEvent, pickResult.GetPointer());
    }

  // Zoom mode
  else if (this->RubberBandMode == vtkInteractorStyleGeoMap::ZoomMode)
    {
    if (moved)
      {
      // Change visible bounds and send event
      this->Map->SetVisibleBounds(latLonCoords);
      this->InvokeEvent(vtkInteractorStyleGeoMap::ZoomCompleteEvent,
                        latLonCoords);
      } 
    }

  this->Map->Draw();
  this->Interaction = NONE;
  this->Superclass::OnLeftButtonUp();
}

//-----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::OnRightButtonDown()
{
  if (this->IsDoubleClick())
  {
    this->ZoomOut(2);
    return;
  }
}

//-----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::OnRightButtonUp()
{
  this->StartPosition[0] = this->Interactor->GetEventPosition()[0];
  this->StartPosition[1] = this->Interactor->GetEventPosition()[1];
  this->EndPosition[0] = this->StartPosition[0];
  this->EndPosition[1] = this->StartPosition[1];
  this->Interaction = NONE;
  this->InvokeEvent(vtkInteractorStyleGeoMap::RightButtonCompleteEvent, this->EndPosition);
}

//-----------------------------------------------------------------------------
bool vtkInteractorStyleGeoMap::IsDoubleClick()
{
  const size_t elapsed = this->Timer->elapsed<std::chrono::milliseconds>();
  const bool onTime = elapsed < this->DoubleClickDelay;

  bool doubleClicked = false;
  if (!onTime || this->MouseClicks == 0)
  {
    this->MouseClicks = 1;
    this->Timer->reset();
  }
  else
    this->MouseClicks++;

  if (this->MouseClicks == 1)
  {
    this->Timer->reset();
  }
  else if (this->MouseClicks == 2)
  {
    doubleClicked = onTime;
    this->MouseClicks = 0;
  }

  return doubleClicked;
}


//--------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::OnMouseMove()
{
  if (this->RubberBandMode == DisabledMode)
    {
    // Original map interaction == pan
    int *pos = this->Interactor->GetEventPosition();
    switch (this->State)
      {
      case VTKIS_PAN:
        this->FindPokedRenderer(pos[0], pos[1]);
        this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_SIZEALL);
        this->MouseMoved = true;
        this->Pan();
        break;
      }
    }

  this->EndPosition[0] = this->Interactor->GetEventPosition()[0];
  this->EndPosition[1] = this->Interactor->GetEventPosition()[1];

  Superclass::OnMouseMove();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::OnMouseWheelForward()
{
  this->ZoomIn(1);
  this->Superclass::OnMouseWheelForward();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::OnMouseWheelBackward()
{
  this->ZoomOut(1);
  this->Superclass::OnMouseWheelBackward();
}

//-----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::ZoomIn(int levels)
{
  if (this->Map)
  {
    int zoom = this->Map->GetZoom();
    if (zoom < 19)
    {
      zoom += levels;
      this->Map->SetZoom(zoom);
      this->SetCurrentRenderer(this->Map->GetRenderer());

      vtkCamera *camera = this->Map->GetRenderer()->GetActiveCamera();

      // Get current mouse coordinates (to make that screen position constant)
      int *pos = this->Interactor->GetEventPosition();

      // Get corresponding world coordinates
      double zoomCoords[4];
      this->ComputeDisplayToWorld(pos[0], pos[1], 0.0, zoomCoords);

      // Get camera coordinates before zooming in
      double cameraCoords[3];
      double nextCameraCoords[3];
      camera->GetPosition(cameraCoords);

      if (this->Map->GetPerspectiveProjection())
      {
        // Apply the dolly operation (move closer to focal point)
        camera->Dolly(2.0);

        // Get new camera coordinates
        camera->GetPosition(nextCameraCoords);

        // Adjust xy position to be proportional to change in z
        // That way, the zoom point remains stationary
        const double f = 0.5;   // fraction that camera moved closer to origin
        double losVector[3];  // line-of-sight vector, from camera to zoomCoords
        vtkMath::Subtract(zoomCoords, cameraCoords, losVector);
        vtkMath::Normalize(losVector);
        vtkMath::MultiplyScalar(losVector, f * cameraCoords[2]);
        nextCameraCoords[0] = cameraCoords[0] + losVector[0];
        nextCameraCoords[1] = cameraCoords[1] + losVector[1];
        camera->SetPosition(nextCameraCoords);
      }
      else
      {
        double camZ = cameraCoords[2];
        vtkMath::Add(zoomCoords, cameraCoords, nextCameraCoords);
        vtkMath::MultiplyScalar(nextCameraCoords, 0.5);
        nextCameraCoords[2] = camZ;
        camera->SetPosition(nextCameraCoords);
      }

      // Set same xy coords for the focal point
      nextCameraCoords[2] = 0.0;
      camera->SetFocalPoint(nextCameraCoords);

      // Redraw the map
      this->Map->Draw();
    }
  }
}

//-----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::ZoomOut(int levels)
{
  if (this->Map)
  {
    int zoom = this->Map->GetZoom();
    if (zoom > 0)
    {
      zoom -= levels;
      this->Map->SetZoom(zoom);
      this->SetCurrentRenderer(this->Map->GetRenderer());

      vtkCamera *camera = this->Map->GetRenderer()->GetActiveCamera();

      // Get current mouse coordinates (to make that screen position constant)
      int *pos = this->Interactor->GetEventPosition();

      // Get corresponding world coordinates
      double zoomCoords[4];
      this->ComputeDisplayToWorld(pos[0], pos[1], 0.0, zoomCoords);

      // Get camera coordinates before zooming out
      double cameraCoords[3];
      double nextCameraCoords[3];
      camera->GetPosition(cameraCoords);

      if (this->Map->GetPerspectiveProjection())
      {
        // Apply the dolly operation (move away from focal point)
        camera->Dolly(0.5);

        // Get new camera coordinates
        camera->GetPosition(nextCameraCoords);

        // Adjust xy position to be proportional to change in z
        // That way, the zoom point remains stationary
        double losVector[3];  // line-of-sight vector, from camera to zoomCoords
        vtkMath::Subtract(zoomCoords, cameraCoords, losVector);
        vtkMath::Normalize(losVector);
        vtkMath::MultiplyScalar(losVector, -1.0 * cameraCoords[2]);
        nextCameraCoords[0] = cameraCoords[0] + losVector[0];
        nextCameraCoords[1] = cameraCoords[1] + losVector[1];
        camera->SetPosition(nextCameraCoords);
      }
      else
      {
        double camZ = cameraCoords[2];
        cameraCoords[2] = 0.0;
        zoomCoords[2] = 0.0;

        // Adjust xy position to be proportional to change in z
        // That way, the zoom point remains stationary
        double losVector[3];  // line-of-sight vector, from camera to zoomCoords
        vtkMath::Subtract(cameraCoords, zoomCoords, losVector);
        vtkMath::Add(cameraCoords, losVector, nextCameraCoords);
        nextCameraCoords[2] = camZ;
        camera->SetPosition(nextCameraCoords);
      }

      // Set same xy coords for the focal point
      nextCameraCoords[2] = 0.0;
      camera->SetFocalPoint(nextCameraCoords);

      // Redraw the map
      this->Map->Draw();
    }
  }
}

//-----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::SetMap(vtkMap *map)
{
  this->Map = map;
  this->SetCurrentRenderer(map->GetRenderer());
}

//----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::Pan()
{
  if (this->CurrentRenderer == nullptr || this->Map == nullptr ||
    !this->MouseMoved)
  {
    return;
  }

  // Following logic is copied from vtkInteractorStyleTrackballCamera:

  vtkRenderWindowInteractor *rwi = this->Interactor;

  double viewFocus[4], focalDepth, viewPoint[3];
  double newPickPoint[4], oldPickPoint[4], motionVector[3];

  // Calculate the focal depth since we'll be using it a lot

  vtkCamera *camera = this->CurrentRenderer->GetActiveCamera();
  camera->GetFocalPoint(viewFocus);
  this->ComputeWorldToDisplay(viewFocus[0], viewFocus[1], viewFocus[2],
                              viewFocus);
  focalDepth = viewFocus[2];

  this->ComputeDisplayToWorld(rwi->GetEventPosition()[0],
                              rwi->GetEventPosition()[1],
                              focalDepth,
                              newPickPoint);

  // Has to recalc old mouse point since the viewport has moved,
  // so can't move it outside the loop

  this->ComputeDisplayToWorld(rwi->GetLastEventPosition()[0],
                              rwi->GetLastEventPosition()[1],
                              focalDepth,
                              oldPickPoint);

  // Camera motion is reversed

  motionVector[0] = oldPickPoint[0] - newPickPoint[0];
  motionVector[1] = oldPickPoint[1] - newPickPoint[1];
  motionVector[2] = oldPickPoint[2] - newPickPoint[2];

  camera->GetFocalPoint(viewFocus);
  camera->GetPosition(viewPoint);
  camera->SetFocalPoint(motionVector[0] + viewFocus[0],
                        motionVector[1] + viewFocus[1],
                        motionVector[2] + viewFocus[2]);

  camera->SetPosition(motionVector[0] + viewPoint[0],
                      motionVector[1] + viewPoint[1],
                      motionVector[2] + viewPoint[2]);

  this->Map->Draw();
  this->MouseMoved = false;
}
