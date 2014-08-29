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

#include "vtkMap.h"
#include "qtMapCoordinatesWidget.h"
#include "ui_qtMapCoordinatesWidget.h"


// ------------------------------------------------------------
qtMapCoordinatesWidget::qtMapCoordinatesWidget(QWidget *parent)
  : QWidget(parent), Map(0)
{
  this->UI = new Ui_qtMapCoordinatesWidget;
  this->UI->setupUi(this);

}

// ------------------------------------------------------------
void qtMapCoordinatesWidget::refresh()
{
}
