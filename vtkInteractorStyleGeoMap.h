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
// .NAME vtkInteractorStyleGeoMap - interactor style specifically for map views
// .SECTION Description
// The legacy implementation used RubberBandActor to render a colored selection
// rectangle.  This approach does not fit directly the layer-per-renderpass
// paradigm since only actors in a layer would be rendered (RubberBandActor is
// never added to any vtkGeoMapLayerPass). To achieve rendering the rectangle
// through an actor, a top-layer (containing RubberBandActor only) would need to
// be added to the sequence while doing rubber band selection.
//
// Performance note: both of these approaches (legacy--through Interactor->Render()
// and potentially-future-- rendering RubberBandActor through vtkRenderPass) force
// re-rendering the entire scene while manipulating/growing the selection rectangle
// , which might not scale well with the number of actors.
//
// Because of this, InteractorStyleGeoMap defaults for now to its base class
// approach which does not render anything through the vtkRenderer OnMouseMove()
// but rather just does it in the CPU caching the current frame to restore it after
// selection is finished.
#ifndef __vtkInteractorStyleGeoMap_h
#define __vtkInteractorStyleGeoMap_h

#include <vtkCommand.h>
#include <vtkInteractorStyleRubberBand2D.h>

#include <vtkmap_export.h>

class vtkActor2D;
class vtkMap;
class vtkPoints;

class VTKMAP_EXPORT vtkInteractorStyleGeoMap
  : public vtkInteractorStyleRubberBand2D
{
public:
  enum Commands
    {
    SelectionCompleteEvent = vtkCommand::UserEvent + 1,
    DisplayClickCompleteEvent,   // DisplayOnlyMode && mouse click
    DisplayDrawCompleteEvent,    // DisplayOnlyMode && rectangle draw
    ZoomCompleteEvent,
    RightButtonCompleteEvent     // for application-context menus
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

  vtkSetVector4Macro(OverlayColor, double);
  vtkSetVector4Macro(EdgeColor, double);

  // Description:
  // Overriding these functions to implement custom
  // interactions.
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();
  virtual void OnRightButtonUp();
  virtual void OnMouseMove();
  virtual void OnMouseWheelForward();
  virtual void OnMouseWheelBackward();

  using vtkInteractorStyleRubberBand2D::GetStartPosition;
  using vtkInteractorStyleRubberBand2D::GetEndPosition;

  int* GetStartPosition() { return this->StartPosition; }
  int* GetEndPosition()   { return this->EndPosition; }

  // Description:
  // Modes for RubberBandMode
  enum enumRubberBandMode
    {
    DisabledMode = 0,   // standard map interaction (select/pan)
    SelectionMode,
    ZoomMode,
    DisplayOnlyMode
    };

  // Description:
  // Control whether we do zoom, selection, or nothing special if rubberband
  vtkSetClampMacro(RubberBandMode, int, DisabledMode, DisplayOnlyMode);
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
  // For potential/future use cases:
  // Enable rubberband selection (while in zoom mode) via Ctrl-key modifier
  vtkSetMacro(RubberBandSelectionWithCtrlKey, int);
  vtkGetMacro(RubberBandSelectionWithCtrlKey, int);
  vtkBooleanMacro(RubberBandSelectionWithCtrlKey, int);

  //void ZoomToExtents(vtkRenderer* renderer, double extents[4]);

  // Map
  void SetMap(vtkMap* map);

protected:
  void Pan();
  void Zoom();

  double OverlayColor[4];
  double EdgeColor[4];

private:
// Not implemented.
  vtkInteractorStyleGeoMap(const vtkInteractorStyleGeoMap&);
  void operator=(const vtkInteractorStyleGeoMap&);

  vtkMap *Map;

  int RubberBandMode;
  int RubberBandSelectionWithCtrlKey;

/**
  * vtkInteractorStyleGeoMap defaults for now to its base class
  * approach which does not render anything through the vtkRenderer. This
  * is a temporary fix and there is no API to change it given that the
  * layer-renderPass paradigm is not compatible with rendering through
  * vtkRenderer in the interactor style.
  */
  bool UseDefaultRenderingMode = true;

  vtkActor2D* RubberBandActor;
  vtkPoints*  RubberBandPoints;
};

#endif // __vtkInteractorStyleGeoMap_h
