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
// InteractorStyleGeoMap defaults for now to its base class approach which does
// not render anything through the vtkRenderer OnMouseMove() but rather just
// does it in the CPU caching the current frame to restore it after selection
// is finished.
//
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

  // Map
  void SetMap(vtkMap* map);

protected:
  void Pan() override;

private:
  vtkInteractorStyleGeoMap(const vtkInteractorStyleGeoMap&) = delete;
  void operator=(const vtkInteractorStyleGeoMap&) = delete;

  vtkMap *Map;
  int RubberBandMode;
  vtkPoints*  RubberBandPoints;
};

#endif // __vtkInteractorStyleGeoMap_h
