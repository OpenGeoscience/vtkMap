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

#include "qtWSMapMainWindow.h"
#include "qtMapCoordinatesWidget.h"
#include <QVTKWidget.h>
#include <vtkCallbackCommand.h>
#include <vtkInteractorStyleImage.h>
#include <vtkNew.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <QFrame>
#include <QPushButton>



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
qtWSMapMainWindow::qtWSMapMainWindow()
{
  QFrame *frame = new QFrame();
  this->setCentralWidget(frame);

  // Setup map-coords widget
  qtMapCoordinatesWidget *mapCoordsWidget = new qtMapCoordinatesWidget(frame);
  mapCoordsWidget->move(50, 50);


  // Setup push button (plain Qt widget)
  QPushButton *button = new QPushButton("Test", frame);
  button->move(50, 650);


  // Setup map (QVTKWwidget)
  QVTKWidget *mapWidget = new QVTKWidget(frame);
  mapWidget->resize(900, 500);
  mapWidget->move(50, 100);

  this->Map = vtkMap::New();
  vtkNew<vtkRenderer> mapRenderer;
  this->Map->SetRenderer(mapRenderer.GetPointer());
  this->Map->SetCenter(0, 0);
  this->Map->SetZoom(5);

  vtkNew<vtkRenderWindow> mapRenderWindow;
  mapRenderWindow->AddRenderer(mapRenderer.GetPointer());
  mapWidget->SetRenderWindow(mapRenderWindow.GetPointer());

  vtkRenderWindowInteractor *intr = mapWidget->GetInteractor();
  intr->SetInteractorStyle(this->Map->GetInteractorStyle());
  intr->Initialize();


  // Pass *all* callbacks to MapCallback instance
  MapCallback *mapCallback = new MapCallback(this->Map);
  intr->AddObserver(vtkCommand::AnyEvent, mapCallback);
  intr->Start();
}

// ------------------------------------------------------------
// Calls map's Draw() method
void qtWSMapMainWindow::drawMap()
{
  this->Map->Draw();
}
