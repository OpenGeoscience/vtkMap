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
// .NAME qtWSMapMainWindow - Main window for weather station map application
// .SECTION Description
//


#ifndef __qtWSMapMainWindow_h
#define __qtWSMapMainWindow_h

#include "vtkMap.h"
#include <qmainwindow.h>

class QWidget;


class qtWSMapMainWindow : public QMainWindow
{
 public:
  qtWSMapMainWindow();

  void drawMap();
 protected:
  QWidget *createCoordinatesDisplay();

  vtkMap *Map;
};

#endif  // __qtWSMapMainWindow_h

