/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "vtkVgInteractorStyleRubberBand2D.h"

#include "vtkVgRendererUtils.h"

// VTK includes.
#include <vtkActor2D.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCommand.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkPoints.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkUnsignedCharArray.h>

vtkStandardNewMacro(vtkVgInteractorStyleRubberBand2D);
vtkCxxSetObjectMacro(vtkVgInteractorStyleRubberBand2D, Renderer, vtkRenderer);

//-----------------------------------------------------------------------------
vtkVgInteractorStyleRubberBand2D::vtkVgInteractorStyleRubberBand2D() :
  vtkInteractorStyleRubberBand2D()
{
  this->AllowPanning = 1;
  this->RubberBandMode = ZoomMode;
  this->RubberBandSelectionWithCtrlKey = 0;
  this->LeftButtonIsMiddleButton = false;
  this->Renderer = 0;
  this->RubberBandActor = 0;
  this->RubberBandPoints = 0;
}

//-----------------------------------------------------------------------------
vtkVgInteractorStyleRubberBand2D::~vtkVgInteractorStyleRubberBand2D()
{
  if (this->Renderer)
    {
    this->Renderer->UnRegister(this);
    }
  if (this->RubberBandActor)
    {
    this->RubberBandActor->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkVgInteractorStyleRubberBand2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkVgInteractorStyleRubberBand2D::OnKeyPress()
{
  switch (this->Interactor->GetKeyCode())
    {
    case 'a':
    case 'A':
      this->InvokeEvent(KeyPressEvent_A);
      break;
    case 'r':
    case 'R':
      this->InvokeEvent(KeyPressEvent_R);
      break;
    case 'z':
    case 'Z':
      this->InvokeEvent(KeyPressEvent_Z);
      break;
    case 's':
    case 'S':
      this->InvokeEvent(KeyPressEvent_S);
      break;
    case 'p':
    case 'P':
      this->InvokeEvent(KeyPressEvent_P);
      break;
    default:
      if (strcmp(this->Interactor->GetKeySym(), "Delete") == 0)
        {
        this->InvokeEvent(KeyPressEvent_Delete);
        break;
        }
    }
}

//-----------------------------------------------------------------------------
void vtkVgInteractorStyleRubberBand2D::OnChar()
{
  // FIXME - may break vsPlay
  // hide ResetCamera
  switch (this->Interactor->GetKeyCode())
    {
    case 'r' :
    case 'R' :
      return;
    default:
      this->Superclass::OnChar();
    }
}

//-----------------------------------------------------------------------------
void vtkVgInteractorStyleRubberBand2D::OnLeftButtonDown()
{
  if (this->Interactor->GetShiftKey())
    {
    this->LeftButtonIsMiddleButton = true;
    this->Superclass::OnMiddleButtonDown();
    return;
    }
  this->LeftButtonIsMiddleButton = false;

  if (this->RubberBandMode == vtkVgInteractorStyleRubberBand2D::DisabledMode &&
      !this->Interactor->GetControlKey())
    {
    return;
    }

  // fall back to built-in rubberband drawing if no renderer was given
  if (!this->Renderer)
    {
    this->Superclass::OnLeftButtonDown();
    return;
    }

  if (this->Interaction != NONE)
    {
    return;
    }

  this->Interaction = SELECTING;

  // initialize the rubberband actor on first use
  if (!this->RubberBandActor)
    {
    this->RubberBandActor = vtkActor2D::New();
    this->RubberBandPoints = vtkPoints::New();
    vtkPolyData* PD  = vtkPolyData::New();
    vtkCellArray* CA = vtkCellArray::New();
    vtkCellArray* CA2 = vtkCellArray::New();
    vtkPolyDataMapper2D* PDM = vtkPolyDataMapper2D::New();
    vtkUnsignedCharArray* UCA = vtkUnsignedCharArray::New();

    this->RubberBandPoints->SetNumberOfPoints(4);

    vtkIdType ids[] = { 0, 1, 2, 3, 0 };
    CA2->InsertNextCell(5, ids);
    CA->InsertNextCell(4, ids);

    UCA->SetNumberOfComponents(4);
    UCA->SetName("Colors");

    unsigned char color[]     = { 200, 230, 250,  50 };
    unsigned char edgeColor[] = {  60, 173, 255, 255 };
    UCA->InsertNextTupleValue(edgeColor);
    UCA->InsertNextTupleValue(color);

    PD->GetCellData()->SetScalars(UCA);
    PD->SetPoints(this->RubberBandPoints);
    PD->SetPolys(CA);
    PD->SetLines(CA2);
    PDM->SetInputData(PD);

    this->RubberBandActor->SetMapper(PDM);

    this->Renderer->AddViewProp(this->RubberBandActor);

    CA->FastDelete();
    CA2->FastDelete();
    UCA->FastDelete();
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
    this->Renderer->AddViewProp(this->RubberBandActor);
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

  this->SetCurrentRenderer(this->Renderer);
  this->InvokeEvent(vtkCommand::StartInteractionEvent);
  this->GetInteractor()->Render();
}

//-----------------------------------------------------------------------------
void vtkVgInteractorStyleRubberBand2D::OnLeftButtonUp()
{
  if (this->LeftButtonIsMiddleButton)
    {
    this->LeftButtonIsMiddleButton = false;
    this->Superclass::OnMiddleButtonUp();
    return;
    }

  if (!this->RubberBandActor)
    {
    this->Superclass::OnLeftButtonUp();
    return;
    }

  if (this->Interaction != SELECTING)
    {
    return;
    }

  this->RubberBandActor->VisibilityOff();

  int area = (this->EndPosition[0] - this->StartPosition[0]) *
             (this->EndPosition[1] - this->StartPosition[1]);

  // just a left click?
  if (this->RubberBandMode == DisabledMode ||
      this->RubberBandMode == DisplayOnlyMode ||
      area == 0)
    {
    this->InvokeEvent(LeftClickEvent);
    }
  // don't zoom or select for small rubberband; probably unintentional
  else if (abs(area) > 25)
    {
    // ZoomMode and NOT selection instead because of Ctrl modifier
    if (this->RubberBandMode == ZoomMode &&
        !(this->RubberBandSelectionWithCtrlKey &&
          this->Interactor->GetControlKey()))
      {
      this->Zoom();
      this->InvokeEvent(vtkCommand::EndInteractionEvent);
      }
    else
      {
      this->InvokeEvent(SelectionCompleteEvent);
      }
    }

  this->GetInteractor()->Render();
  this->Interaction = NONE;
}

//--------------------------------------------------------------------------
void vtkVgInteractorStyleRubberBand2D::OnRightButtonDown()
{
  this->StartPosition[0] = this->Interactor->GetEventPosition()[0];
  this->StartPosition[1] = this->Interactor->GetEventPosition()[1];
  this->Superclass::OnRightButtonDown();
}

//--------------------------------------------------------------------------
void vtkVgInteractorStyleRubberBand2D::OnRightButtonUp()
{
  int pos[2];
  this->Interactor->GetEventPosition(pos);

  if (abs(pos[0] - this->StartPosition[0]) < 5 &&
      abs(pos[1] - this->StartPosition[1]) < 5)
    {
    this->InvokeEvent(RightClickEvent);
    }

  this->Superclass::OnRightButtonUp();
}

//--------------------------------------------------------------------------
void vtkVgInteractorStyleRubberBand2D::OnMiddleButtonDown()
{
  if (!this->AllowPanning)
    {
    // Do nothing.
    }
  else
    {
    this->Superclass::OnMiddleButtonDown();
    }
}

//--------------------------------------------------------------------------
void vtkVgInteractorStyleRubberBand2D::OnMiddleButtonUp()
{
  if (!this->AllowPanning)
    {
    // Do nothing.
    }
  else
    {
    this->Superclass::OnMiddleButtonUp();
    }
}

//--------------------------------------------------------------------------
void vtkVgInteractorStyleRubberBand2D::OnMouseMove()
{
  if (this->RubberBandMode == DisabledMode)
    {
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

//-----------------------------------------------------------------------------
void vtkVgInteractorStyleRubberBand2D::ZoomToExtents(vtkRenderer* ren,
    double extents[4])
{
  vtkVgRendererUtils::ZoomToExtents2D(ren, extents);
  this->InvokeEvent(ZoomCompleteEvent);
}

//-----------------------------------------------------------------------------
void vtkVgInteractorStyleRubberBand2D::Zoom()
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
  this->ZoomToExtents(this->CurrentRenderer, extents);
}
