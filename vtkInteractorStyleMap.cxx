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

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
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
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  switch (this->State)
    {
    case VTKIS_PAN:
      this->FindPokedRenderer(x, y);
      this->Pan();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;
    }

  this->Superclass::OnMouseMove();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMap::OnLeftButtonDown()
{
  vtkDebugMacro("StartPan()");
  this->StartPan();
  this->Superclass::OnLeftButtonDown();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleMap::OnLeftButtonUp()
{
  vtkDebugMacro("EndPan()");
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
      vtkCamera *camera = this->Map->GetRenderer()->GetActiveCamera();
      camera->Dolly(2.0);  // move toward focal point
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
      vtkCamera *camera = this->Map->GetRenderer()->GetActiveCamera();
      camera->Dolly(0.5);  // move away from focal point
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
