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

#include "qtMapCoordinatesWidget.h"
#include "ui_qtMapCoordinatesWidget.h"
#include "vtkMap.h"
#include <QString>


// ------------------------------------------------------------
qtMapCoordinatesWidget::qtMapCoordinatesWidget(QWidget *parent)
  : QWidget(parent), Map(0)
{
  this->UI = new Ui_qtMapCoordinatesWidget;
  this->UI->setupUi(this);

}

// ------------------------------------------------------------
void qtMapCoordinatesWidget::setCoordinates(double center[2], int zoom)
{
  QString latText;
  latText.sprintf("%.6f", center[0]);
  this->UI->LatitudeEdit->setText(latText);

  QString lonText;
  lonText.sprintf("%.6f", center[1]);
  this->UI->LongitudeEdit->setText(lonText);

  this->UI->ZoomEdit->setValue(zoom);
}

// ------------------------------------------------------------
void qtMapCoordinatesWidget::getCoordinates(double center[2], int& zoom) const
{
  QString latText = this->UI->LatitudeEdit->text();
  center[0] = latText.toDouble();

  QString lonText = this->UI->LongitudeEdit->text();
  center[1] = lonText.toDouble();

  zoom = this->UI->ZoomEdit->value();
}

// ------------------------------------------------------------
void qtMapCoordinatesWidget::refresh()
{
}
