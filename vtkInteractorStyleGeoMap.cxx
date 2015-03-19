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

#include "vtkInteractorStyleGeoMap.h"
#include "vtkGeoMapSelection.h"
#include "vtkMap.h"
//#include "vtkMapPickResult.h"

//#include "vtkVgRendererUtils.h"

// VTK includes.
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

#include <algorithm>  // std::copy, std::max, std::min

vtkStandardNewMacro(vtkInteractorStyleGeoMap);

//-----------------------------------------------------------------------------
vtkInteractorStyleGeoMap::vtkInteractorStyleGeoMap() :
  vtkInteractorStyleRubberBand2D()
{
  this->Map = NULL;
  this->RubberBandMode = DisabledMode;
  this->RubberBandSelectionWithCtrlKey = 0;
  this->RubberBandActor = 0;
  this->RubberBandPoints = 0;

  // Default rubber band colors (RGBA)
  double violet[] = {1.0, 0.6, 1.0, 0.2};
  std::copy(violet, violet+4, this->OverlayColor);

  double magenta[] = {1.0, 0.0, 1.0, 1.0};
  std::copy(magenta, magenta+4, this->EdgeColor);
}

//-----------------------------------------------------------------------------
vtkInteractorStyleGeoMap::~vtkInteractorStyleGeoMap()
{
  if (this->RubberBandActor)
    {
    this->RubberBandActor->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::OnLeftButtonDown()
{
  if (this->RubberBandMode == vtkInteractorStyleGeoMap::DisabledMode)
    {
    // Default map interaction == select feature & start pan
    int *pos = this->Interactor->GetEventPosition();

    // Check if anything was picked
    vtkNew<vtkGeoMapSelection> pickResult;
    this->Map->PickPoint(pos, pickResult.GetPointer());
    this->InvokeEvent(SelectionCompleteEvent, pickResult.GetPointer());
    vtkDebugMacro("StartPan()");
    //std::cout << "Start Pan" << std::endl;
    this->Interaction = PANNING;
    this->StartPan();
    }

  // fall back to built-in rubberband drawing if no renderer was given
  if (!this->Map->GetRenderer())
    {
    this->Superclass::OnLeftButtonDown();
    return;
    }

  this->Interaction = SELECTING;
  vtkRenderer *renderer = this->Map->GetRenderer();

  // initialize the rubberband actor on first use
  if (!this->RubberBandActor)
    {
    this->RubberBandActor = vtkActor2D::New();
    this->RubberBandPoints = vtkPoints::New();
    vtkPolyData* PD  = vtkPolyData::New();
    vtkCellArray* CA = vtkCellArray::New();
    vtkCellArray* CA2 = vtkCellArray::New();
    vtkPolyDataMapper2D* PDM = vtkPolyDataMapper2D::New();
    vtkDoubleArray* colorArray = vtkDoubleArray::New();

    this->RubberBandPoints->SetNumberOfPoints(4);

    vtkIdType ids[] = { 0, 1, 2, 3, 0 };
    CA2->InsertNextCell(5, ids);
    CA->InsertNextCell(4, ids);

    colorArray->SetNumberOfComponents(4);
    colorArray->SetName("Colors");
    colorArray->InsertNextTupleValue(this->EdgeColor);
    colorArray->InsertNextTupleValue(this->OverlayColor);

    PD->GetCellData()->SetScalars(colorArray);
    PD->SetPoints(this->RubberBandPoints);
    PD->SetPolys(CA);
    PD->SetLines(CA2);
    PDM->SetInputData(PD);
    PDM->SetColorModeToDirectScalars();

    this->RubberBandActor->SetMapper(PDM);

    renderer->AddViewProp(this->RubberBandActor);

    CA->FastDelete();
    CA2->FastDelete();
    colorArray->FastDelete();
    PD->FastDelete();
    PDM->FastDelete();
    this->RubberBandPoints->FastDelete();
    }
  else
    {
    this->RubberBandActor->VisibilityOn();

    // Our actor may have been removed since it isn't in the scene graph.
    // Don't bother checking if it has already been added, since the renderer
    // will do that anyways.
    renderer->AddViewProp(this->RubberBandActor);
    }

  this->StartPosition[0] = this->Interactor->GetEventPosition()[0];
  this->StartPosition[1] = this->Interactor->GetEventPosition()[1];
  this->EndPosition[0] = this->StartPosition[0];
  this->EndPosition[1] = this->StartPosition[1];

  double pos[] =
    {
    this->StartPosition[0] + 0.5,
    this->StartPosition[1] + 0.5,
    0.0
    };

  this->RubberBandPoints->SetPoint(0, pos);
  this->RubberBandPoints->SetPoint(1, pos);
  this->RubberBandPoints->SetPoint(2, pos);
  this->RubberBandPoints->SetPoint(3, pos);

  vtkPolyDataMapper2D::SafeDownCast(this->RubberBandActor->GetMapper())
  ->GetInput()->Modified();

  this->SetCurrentRenderer(renderer);
  this->InvokeEvent(vtkCommand::StartInteractionEvent);
  this->GetInteractor()->Render();
}

//-----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::OnLeftButtonUp()
{
  vtkDebugMacro("EndPan()");
  //std::cout << "End Pan" << std::endl;
  this->EndPan();
  this->Interaction = NONE;

  if (this->RubberBandMode == vtkInteractorStyleGeoMap::DisabledMode)
    {
    this->Interactor->GetRenderWindow()->SetCurrentCursor(VTK_CURSOR_DEFAULT);
    }

  if (!this->RubberBandActor)
    {
    this->Superclass::OnLeftButtonUp();
    return;
    }

  this->RubberBandActor->VisibilityOff();

  // Get corner points of interaction, sorted by min/max
  int boundCoords[4];
  boundCoords[0] = std::min(this->StartPosition[0], this->EndPosition[0]);
  boundCoords[1] = std::min(this->StartPosition[1], this->EndPosition[1]);
  boundCoords[2] = std::max(this->StartPosition[0], this->EndPosition[0]);
  boundCoords[3] = std::max(this->StartPosition[1], this->EndPosition[1]);
  vtkDebugMacro("RubberBand complete with points:"
                << " " << boundCoords[0] << ", " << boundCoords[1]
                << "  " << boundCoords[2] << ", " << boundCoords[3]);

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

  // Display-only mode
  if (this->RubberBandMode == vtkInteractorStyleGeoMap::DisplayOnlyMode)
    {
    this->InvokeEvent(vtkInteractorStyleGeoMap::DisplayCompleteEvent,
                      latLonCoords);
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
    // Ignore small rubberband; probably unintentional
    int area = (boundCoords[2] - boundCoords[0]) *
      (boundCoords[3] - boundCoords[1]);
    if (area > 25)
      {
      // Change visible bounds and send event
      this->Map->SetVisibleBounds(latLonCoords);
      this->InvokeEvent(vtkInteractorStyleGeoMap::ZoomCompleteEvent,
                        latLonCoords);
      }  // if (area)
    }  // else if (zoom mode)

  this->Map->Draw();
  this->Interaction = NONE;
  this->Superclass::OnLeftButtonUp();
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
        this->Pan();
        this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
        break;
      }
    // Do NOT call superclass OnMouseMove() here
    return;
    }

  if (this->Interaction != SELECTING || !this->RubberBandActor)
    {
    this->Superclass::OnMouseMove();
    return;
    }

  this->EndPosition[0] = this->Interactor->GetEventPosition()[0];
  this->EndPosition[1] = this->Interactor->GetEventPosition()[1];

  int* size = this->Interactor->GetRenderWindow()->GetSize();
  if (this->EndPosition[0] > (size[0] - 1))
    {
    this->EndPosition[0] = size[0] - 1;
    }
  if (this->EndPosition[0] < 0)
    {
    this->EndPosition[0] = 0;
    }
  if (this->EndPosition[1] > (size[1] - 1))
    {
    this->EndPosition[1] = size[1] - 1;
    }
  if (this->EndPosition[1] < 0)
    {
    this->EndPosition[1] = 0;
    }

  double pos1[] =
    {
    this->EndPosition[0] + 0.5,
    this->StartPosition[1] + 0.5,
    0.0
    };

  double pos2[] =
    {
    this->EndPosition[0] + 0.5,
    this->EndPosition[1] + 0.5,
    0.0
    };

  double pos3[] =
    {
    this->StartPosition[0] + 0.5,
    this->EndPosition[1] + 0.5,
    0.0
    };

  this->RubberBandPoints->SetPoint(1, pos1);
  this->RubberBandPoints->SetPoint(2, pos2);
  this->RubberBandPoints->SetPoint(3, pos3);

  vtkPolyDataMapper2D::SafeDownCast(this->RubberBandActor->GetMapper())
  ->GetInput()->Modified();

  this->InvokeEvent(vtkCommand::InteractionEvent);
  this->GetInteractor()->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::OnMouseWheelForward()
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
      const double f = 0.5;   // fraction that camera moved closer to origin
      double losVector[3];  // line-of-sight vector, from camera to zoomCoords
      vtkMath::Subtract(zoomCoords, cameraCoords, losVector);
      vtkMath::Normalize(losVector);
      vtkMath::MultiplyScalar(losVector, f * cameraCoords[2]);
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
void vtkInteractorStyleGeoMap::OnMouseWheelBackward()
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

//-----------------------------------------------------------------------------
// void vtkInteractorStyleGeoMap::ZoomToExtents(vtkRenderer* ren,
//     double extents[4])
// {
//   // vtkVgRendererUtils::ZoomToExtents2D(ren, extents);
//   vtkWarningMacro("Sorry - ZoomToExtents not implemented");
//   this->InvokeEvent(ZoomCompleteEvent);
// }

//-----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::SetMap(vtkMap *map)
{
  this->Map = map;
  this->SetCurrentRenderer(map->GetRenderer());
}

//----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::Pan()
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

//-----------------------------------------------------------------------------
void vtkInteractorStyleGeoMap::Zoom()
{
  int width, height;
  width = abs(this->EndPosition[0] - this->StartPosition[0]);
  height = abs(this->EndPosition[1] - this->StartPosition[1]);

  // compute world position of lower left corner
  double rbmin[3];
  rbmin[0] = this->StartPosition[0] < this->EndPosition[0] ?
             this->StartPosition[0] : this->EndPosition[0];
  rbmin[1] = this->StartPosition[1] < this->EndPosition[1] ?
             this->StartPosition[1] : this->EndPosition[1];
  rbmin[2] = 0.0;

  this->CurrentRenderer->SetDisplayPoint(rbmin);
  this->CurrentRenderer->DisplayToView();
  this->CurrentRenderer->ViewToWorld();

  double invw;
  double worldRBMin[4];
  this->CurrentRenderer->GetWorldPoint(worldRBMin);
  invw = 1.0 / worldRBMin[3];
  worldRBMin[0] *= invw;
  worldRBMin[1] *= invw;

  // compute world position of upper right corner
  double rbmax[3];
  rbmax[0] = rbmin[0] + width;
  rbmax[1] = rbmin[1] + height;
  rbmax[2] = 0.0;

  this->CurrentRenderer->SetDisplayPoint(rbmax);
  this->CurrentRenderer->DisplayToView();
  this->CurrentRenderer->ViewToWorld();

  double worldRBMax[4];
  this->CurrentRenderer->GetWorldPoint(worldRBMax);
  invw = 1.0 / worldRBMax[3];
  worldRBMax[0] *= invw;
  worldRBMax[1] *= invw;

  double extents[] =
    {
    worldRBMin[0],
    worldRBMax[0],
    worldRBMin[1],
    worldRBMax[1]
    };

  // zoom
  //this->ZoomToExtents(this->CurrentRenderer, extents);
}
