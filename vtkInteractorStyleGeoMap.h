/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef __vtkVgInteractorStyleRubberBand2D_h
#define __vtkVgInteractorStyleRubberBand2D_h

#include <vtkCommand.h>
#include <vtkInteractorStyleRubberBand2D.h>

#include <vgExport.h>

class vtkActor2D;
class vtkPoints;
class vtkRenderer;

class VTKVG_CORE_EXPORT vtkVgInteractorStyleRubberBand2D
  : public vtkInteractorStyleRubberBand2D
{
public:
  enum Commands
    {
    LeftClickEvent = vtkCommand::UserEvent + 1,
    RightClickEvent,
    KeyPressEvent_A,
    KeyPressEvent_R,
    KeyPressEvent_Z,
    KeyPressEvent_S,
    KeyPressEvent_P,
    KeyPressEvent_Delete,
    ZoomCompleteEvent,
    SelectionCompleteEvent
    };

public:
  // Description:
  // Standard VTK functions.
  static vtkVgInteractorStyleRubberBand2D* New();
  vtkTypeMacro(vtkVgInteractorStyleRubberBand2D, vtkInteractorStyleRubberBand2D);

  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Constructor / Destructor.
  vtkVgInteractorStyleRubberBand2D();
  ~vtkVgInteractorStyleRubberBand2D();

  // Description:
  virtual void OnKeyPress();

  virtual void OnChar();

  // Description:
  // Overriding these functions to implement custom
  // interactions.
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();
  virtual void OnRightButtonDown();
  virtual void OnRightButtonUp();
  virtual void OnMiddleButtonDown();
  virtual void OnMiddleButtonUp();
  virtual void OnMouseMove();

  using vtkInteractorStyleRubberBand2D::GetStartPosition;
  using vtkInteractorStyleRubberBand2D::GetEndPosition;

  int* GetStartPosition() { return this->StartPosition; }
  int* GetEndPosition()   { return this->EndPosition; }

  // Description:
  // Enable/disable panning.
  vtkSetMacro(AllowPanning, int);
  vtkGetMacro(AllowPanning, int);
  vtkBooleanMacro(AllowPanning, int);

  // Description:
  // Modes for RubberBandMode
  enum enumRubberBandMode
    {
    ZoomMode = 0,
    SelectionMode,
    DisplayOnlyMode,
    DisabledMode
    };

  // Description:
  // Control whether we do zoom, selection, or nothing special if rubberband
  vtkSetClampMacro(RubberBandMode, int, ZoomMode, DisabledMode);
  vtkGetMacro(RubberBandMode, int);
  void SetRubberBandModeToZoom()
    {this->SetRubberBandMode(ZoomMode);}
  void SetRubberBandModeToSelection()
    {this->SetRubberBandMode(SelectionMode);}
  void SetRubberBandModeToDisplayOnly()
    {this->SetRubberBandMode(DisplayOnlyMode);}
  void SetRubberBandModeToDisabled()
    {this->SetRubberBandMode(DisabledMode);}

  // Description:
  // Enable rubberband selection (while in zoom mode) via Ctrl-key modifier
  vtkSetMacro(RubberBandSelectionWithCtrlKey, int);
  vtkGetMacro(RubberBandSelectionWithCtrlKey, int);
  vtkBooleanMacro(RubberBandSelectionWithCtrlKey, int);

  void ZoomToExtents(vtkRenderer* renderer, double extents[4]);

  // Renderer to use for rubberband drawing.
  void SetRenderer(vtkRenderer* renderer);

protected:
  void Zoom();

private:
// Not implemented.
  vtkVgInteractorStyleRubberBand2D(const vtkVgInteractorStyleRubberBand2D&);
  void operator=(const vtkVgInteractorStyleRubberBand2D&);

  int AllowPanning;
  int RubberBandMode;
  int RubberBandSelectionWithCtrlKey;

  bool LeftButtonIsMiddleButton;

  vtkRenderer* Renderer;

  vtkActor2D* RubberBandActor;
  vtkPoints*  RubberBandPoints;
};

#endif // __vtkVgInteractorStyleRubberBand2D_h
