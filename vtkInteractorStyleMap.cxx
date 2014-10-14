/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleMap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkInteractorStyleMap.h"
#include "vtkMap.h"
#include "vtkMapPickResult.h"

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

vtkStandardNewMacro(vtkInteractorStyleMap)

//----------------------------------------------------------------------------
vtkInteractorStyleMap::vtkInteractorStyleMap()
{
  this->Map = NULL;
}

//----------------------------------------------------------------------------
vtkInteractorStyleMap::~vtkInteractorStyleMap()
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMap::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkInteractorStyleMap" << std::endl
     << std::endl;
}


//----------------------------------------------------------------------------
void vtkInteractorStyleMap::OnMouseMove()
{
  int *pos = this->Interactor->GetEventPosition();
  switch (this->State)
    {
    case VTKIS_PAN:
      this->FindPokedRenderer(pos[0], pos[1]);
      this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_SIZEALL);
      this->Pan();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;
    }

  this->Superclass::OnMouseMove();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMap::OnLeftButtonDown()
{
  int *pos = this->Interactor->GetEventPosition();

  // Check if anything was picked
  vtkNew<vtkMapPickResult> pickResult;
  this->Map->PickPoint(pos, pickResult.GetPointer());

  switch (pickResult->GetMapFeatureType())
    {
    case VTK_MAP_FEATURE_NONE:
      vtkDebugMacro("StartPan()");
      this->StartPan();
      break;

    case VTK_MAP_FEATURE_MARKER:
    case VTK_MAP_FEATURE_CLUSTER:
      // Todo highlight marker (?)
      break;
    }

  this->Superclass::OnLeftButtonDown();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMap::OnLeftButtonUp()
{
  vtkDebugMacro("EndPan()");
      this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_DEFAULT);
  this->EndPan();
  this->Superclass::OnLeftButtonUp();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMap::OnMouseWheelForward()
{
  if (this->Map)
    {
    int zoom = this->Map->GetZoom();
    if (zoom < 19)
      {
      zoom++;
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
      camera->GetPosition(cameraCoords);

      // Apply the dolly operation (move closer to focal point)
      camera->Dolly(2.0);

      // Get new camera coordinates
      double nextCameraCoords[3];
      camera->GetPosition(nextCameraCoords);

      // Adjust xy position to be proportional to change in z
      // That way, the zoom point remains stationary
      double f = 0.5;   // fraction that camera moved closer to origin
      double losVector[3];  // line-of-sight vector, from camera to zoomCoords
      vtkMath::Subtract(zoomCoords, cameraCoords, losVector);
      vtkMath::Normalize(losVector);
      vtkMath::MultiplyScalar(losVector, 0.5 * cameraCoords[2]);
      nextCameraCoords[0] = cameraCoords[0] + losVector[0];
      nextCameraCoords[1] = cameraCoords[1] + losVector[1];
      camera->SetPosition(nextCameraCoords);

      // Set same xy coords for the focal point
      nextCameraCoords[2] = 0.0;
      camera->SetFocalPoint(nextCameraCoords);

      // Redraw the map
      this->Map->Draw();
      }
    }
  this->Superclass::OnMouseWheelForward();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMap::OnMouseWheelBackward()
{
  if (this->Map)
    {
    int zoom = this->Map->GetZoom();
    if (zoom > 0)
      {
      zoom--;

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
      camera->GetPosition(cameraCoords);

      // Apply the dolly operation (move away from focal point)
      camera->Dolly(0.5);

      // Get new camera coordinates
      double nextCameraCoords[3];
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

      // Set same xy coords for the focal point
      nextCameraCoords[2] = 0.0;
      camera->SetFocalPoint(nextCameraCoords);

      // Redraw the map
      this->Map->Draw();
      }
    }
  this->Superclass::OnMouseWheelBackward();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMap::Pan()
{
  if (this->CurrentRenderer == NULL)
    {
    return;
    }

  if (this->Map == NULL)
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
}
