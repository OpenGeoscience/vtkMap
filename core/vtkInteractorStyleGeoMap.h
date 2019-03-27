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
//
// This interactor style supports panning, zooming (mouse-wheel or double-click)
// and picking (single-click or rubber-band).
//
// InteractorStyleGeoMap defaults for now to its base class approach which does
// not render anything through the vtkRenderer OnMouseMove() but rather just
// does it in the CPU caching the current frame to restore it after selection
// is finished.
//
#ifndef __vtkInteractorStyleGeoMap_h
#define __vtkInteractorStyleGeoMap_h
#include <memory>

#include <vtkCommand.h>
#include <vtkInteractorStyleRubberBand2D.h>

#include "vtkmapcore_export.h"

class vtkActor2D;
class vtkMap;
class vtkPoints;

namespace vtkMapType
{
class Timer;
}

class VTKMAPCORE_EXPORT vtkInteractorStyleGeoMap
  : public vtkInteractorStyleRubberBand2D
{
public:
  enum Commands
  {
    SelectionCompleteEvent = vtkCommand::UserEvent + 1,
    DisplayClickCompleteEvent, // DisplayOnlyMode && mouse click
    DisplayDrawCompleteEvent,  // DisplayOnlyMode && rectangle draw
    ZoomCompleteEvent,
    RightButtonCompleteEvent // for application-context menus
  };

public:
  // Description:
  // Standard VTK functions.
  static vtkInteractorStyleGeoMap* New();
  vtkTypeMacro(vtkInteractorStyleGeoMap, vtkInteractorStyleRubberBand2D);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Constructor / Destructor.
  vtkInteractorStyleGeoMap();
  ~vtkInteractorStyleGeoMap();

  // Description:
  // Overriding these functions to implement custom
  // interactions.
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnRightButtonDown() override;
  void OnRightButtonUp() override;
  void OnMouseMove() override;
  void OnMouseWheelForward() override;
  void OnMouseWheelBackward() override;

  using vtkInteractorStyleRubberBand2D::GetStartPosition;
  using vtkInteractorStyleRubberBand2D::GetEndPosition;

  int* GetStartPosition() override { return this->StartPosition; }
  int* GetEndPosition() override { return this->EndPosition; }

  // Description:
  // Modes for RubberBandMode
  enum enumRubberBandMode
  {
    DisabledMode = 0, // standard map interaction (select/pan)
    SelectionMode,
    ZoomMode,
    DisplayOnlyMode
  };

  // Description:
  // Control whether we do zoom, selection, or nothing special if rubberband
  vtkSetClampMacro(RubberBandMode, int, DisabledMode, DisplayOnlyMode);
  vtkGetMacro(RubberBandMode, int);
  void SetRubberBandModeToZoom() { this->SetRubberBandMode(ZoomMode); }
  void SetRubberBandModeToSelection()
  {
    this->SetRubberBandMode(SelectionMode);
  }
  void SetRubberBandModeToDisplayOnly()
  {
    this->SetRubberBandMode(DisplayOnlyMode);
  }
  void SetRubberBandModeToDisabled() { this->SetRubberBandMode(DisabledMode); }

  vtkSetMacro(DoubleClickDelay, size_t);

  // Map
  void SetMap(vtkMap* map);

protected:
  void Pan() override;

private:
  vtkInteractorStyleGeoMap(const vtkInteractorStyleGeoMap&) = delete;
  void operator=(const vtkInteractorStyleGeoMap&) = delete;

  bool IsDoubleClick();

  /**
 * Zoom handlers.
 */
  void ZoomIn(int levels);
  void ZoomOut(int levels);

  vtkMap* Map;
  int RubberBandMode;
  vtkPoints* RubberBandPoints = nullptr;

  std::unique_ptr<vtkMapType::Timer> Timer;
  size_t DoubleClickDelay = 500;
  unsigned char MouseClicks = 0;

  /**
 * vtkInteractorStyleGeoMap::Pan() can be called through a mouse movement
 * or through vtkInteractorStyle::OnTimer(). Since vtkMap makes use of
 * vtkCommand::TimerEvent for polling it is necessary to ensure panning does
 * not take place if the mouse is static (trackball camera style). This flag
 * is used to ensure vtkInteractorStyleGeoMap::Pan() calls due to TimerEvents
 * are blocked.
 * \sa vtkMap::Initialize vtkInteractorStyle::OnTimer
 */
  bool MouseMoved = false;
};

#endif // __vtkInteractorStyleGeoMap_h
