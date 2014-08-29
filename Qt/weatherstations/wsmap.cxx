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
// .NAME wsmap.cxx - Weather station map demo program
// .SECTION Description
//


#include "qtWSMapMainWindow.h"
#include <qapplication.h>
#include <iostream>


int main(int argc, char *argv[])
{
  std::cout << "Hello from wsmap" << std::endl;

  QApplication app(argc, argv);
  qtWSMapMainWindow win;
  win.show();
  win.resize(1000, 800);
  win.drawMap();

  return app.exec();
}
