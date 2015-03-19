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
class vtkMap;
class vtkPoints;

class VTKMAP_EXPORT vtkInteractorStyleGeoMap
  : public vtkInteractorStyleRubberBand2D
{
public:
  enum Commands
    {
    SelectionCompleteEvent = vtkCommand::UserEvent + 1,
    DisplayCompleteEvent,
    ZoomCompleteEvent
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

  vtkActor2D* RubberBandActor;
  vtkPoints*  RubberBandPoints;
};

#endif // __vtkInteractorStyleGeoMap_h
