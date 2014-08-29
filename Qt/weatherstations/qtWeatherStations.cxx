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

#include "qtWeatherStations.h"
#include "ui_qtWeatherStations.h"
#include "vtkMap.h"
#include <QVTKWidget.h>
#include <vtkCallbackCommand.h>
#include <vtkInteractorStyleImage.h>
#include <vtkNew.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <QVBoxLayout>


// ------------------------------------------------------------
// Callback command for handling mouse events
// In the future, will be replaced by interactor style
class MapCallback : public vtkCallbackCommand
{
public:
  MapCallback(vtkMap *map) : Map(map), MouseDown(false) {}

  virtual void Execute(vtkObject *caller, unsigned long eventId, void *callData)
  {
    switch (eventId)
      {
      case vtkCommand::MiddleButtonPressEvent:
        this->MouseDown = true;
        break;
      case vtkCommand::MiddleButtonReleaseEvent:
        this->MouseDown = false;
        break;
      case vtkCommand::MouseMoveEvent:
        if (this->MouseDown)
          {
          this->Map->Draw();
          }
      case vtkCommand::MouseWheelForwardEvent:
      case vtkCommand::MouseWheelBackwardEvent:
        this->Map->Draw();
      }
  }

protected:
  vtkMap *Map;
  bool MouseDown;
};


// ------------------------------------------------------------
qtWeatherStations::qtWeatherStations(QWidget *parent)
  : QMainWindow(parent)
{
  // Initialize Qt UI (generated from .ui file)
  this->UI = new Ui_qtWeatherStations;
  this->UI->setupUi(this);

//QVBoxLayout *frameLayout = new QVBoxLayout;

  QObject::connect(this->UI->ResizeMapButton, SIGNAL(clicked()),
                   this, SLOT(resizeMap()));

  // Initialize map widget
  this->MapWidget = new QVTKWidget(this->UI->MapFrame);
  this->MapWidget->resize(640, 480);
  this->MapWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  // Initialize Map instance
  this->Map = vtkMap::New();
  vtkNew<vtkRenderer> mapRenderer;
  this->Map->SetRenderer(mapRenderer.GetPointer());
  this->Map->SetCenter(0, 0);
  this->Map->SetZoom(5);

  vtkNew<vtkRenderWindow> mapRenderWindow;
  mapRenderWindow->AddRenderer(mapRenderer.GetPointer());
  this->MapWidget->SetRenderWindow(mapRenderWindow.GetPointer());

  vtkRenderWindowInteractor *intr = this->MapWidget->GetInteractor();
  intr->SetInteractorStyle(this->Map->GetInteractorStyle());
  intr->Initialize();

  // Pass *all* callbacks to MapCallback instance
  MapCallback *mapCallback = new MapCallback(this->Map);
  intr->AddObserver(vtkCommand::AnyEvent, mapCallback);
  intr->Start();
}

// ------------------------------------------------------------
// Resizes MapWidget to fill its parent frame
// Since I cannot figure out how to do this automatically
void qtWeatherStations::resizeMap()
{
  int margin = 4;
  QSize sz = this->UI->MapFrame->size();
  int w = sz.width() - 2*margin;
  int h = sz.height()- 2*margin;


  this->MapWidget->resize(w, h);
  this->MapWidget->move(margin, margin);

  this->drawMap();
}

// ------------------------------------------------------------
// Retrieves list of weather stations and displays on map
void qtWeatherStations::showStations()
{
  // Todo
}

// ------------------------------------------------------------
// Calls map's Draw() method
void qtWeatherStations::drawMap()
{
  if (this->Map)
    {
    this->Map->Draw();
    }
}
