/*
Weather station map using vtkMap
*/


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
