/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleMap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInteractorStyleGeoMap - interactor style specifically for map views
// .SECTION Description
//


#ifndef __vtkInteractorStyleGeoMap_h
#define __vtkInteractorStyleGeoMap_h

#include <vtkCommand.h>
#include <vtkInteractorStyleRubberBand2D.h>

#include <vtkmap_export.h>

class vtkActor2D;
class vtkPoints;
class vtkRenderer;

class VTKMAP_EXPORT vtkInteractorStyleGeoMap
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
  static vtkInteractorStyleGeoMap* New();
  vtkTypeMacro(vtkInteractorStyleGeoMap, vtkInteractorStyleRubberBand2D);

  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Constructor / Destructor.
  vtkInteractorStyleGeoMap();
  ~vtkInteractorStyleGeoMap();

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
  vtkInteractorStyleGeoMap(const vtkInteractorStyleGeoMap&);
  void operator=(const vtkInteractorStyleGeoMap&);

  int AllowPanning;
  int RubberBandMode;
  int RubberBandSelectionWithCtrlKey;

  bool LeftButtonIsMiddleButton;

  vtkRenderer* Renderer;

  vtkActor2D* RubberBandActor;
  vtkPoints*  RubberBandPoints;
};

#endif // __vtkInteractorStyleGeoMap_h
