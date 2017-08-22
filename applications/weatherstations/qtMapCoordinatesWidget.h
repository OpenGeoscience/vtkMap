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
// .NAME qtMapCoordinatesWdiget - Numerical display of geo coords & zoom level
// .SECTION Description
//

#ifndef __qtMapCoordinatesWidget_h
#define __qtMapCoordinatesWidget_h

#include <QWidget>
class vtkMap;

// Forward Qt class declarations
class Ui_qtMapCoordinatesWidget;

class qtMapCoordinatesWidget : public QWidget
{
  Q_OBJECT
public:
  qtMapCoordinatesWidget(QWidget* parent = 0);

  void setCoordinates(double center[2], int zoom);
  void getCoordinates(double center[2], int& zoom) const;
public slots:
  void refresh();

protected:
  /// Map instance
  vtkMap* Map;

  /// Designer form
  Ui_qtMapCoordinatesWidget* UI;
};

#endif // __qtMapCoordinatesWidget_h
