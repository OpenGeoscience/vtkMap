/*
Main window for weather station map application
*/


#ifndef _qtWSMapMainWindow_h_
#define _qtWSMapMainWindow_h_

#include "vtkMap.h"
#include <qmainwindow.h>


class qtWSMapMainWindow : public QMainWindow
{
 public:
  qtWSMapMainWindow();

  void drawMap();
 protected:
  vtkMap *Map;
};

#endif

