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
// .NAME qtWeatherStations - QMainWindow for weather stations map/app
// .SECTION Description
//

#ifndef __qtWeatherStations_h
#define __qtWeatherStations_h

#include "StationReport.h"
#include <QMainWindow>
#include <vtkType.h>
#include <map>
class cJSON;
class vtkMap;
class vtkRenderer;
class QVTKWidget;


// Forward Qt class declarations
class Ui_qtWeatherStations;

class qtWeatherStations : public QMainWindow
{
  Q_OBJECT
 public:
  qtWeatherStations(QWidget *parent = 0);
  virtual ~qtWeatherStations();

  void drawMap();
  void updateMap();
  vtkRenderer *getRenderer() const;
  void pickMarker(int displayCoords[2]);
 public slots:

 protected:
  cJSON *RequestStationData();
  std::vector<StationReport> ParseStationData(cJSON *data);
  void DisplayStationData(std::vector<StationReport> statonList);
  void DisplayStationMarkers(std::vector<StationReport> statonList);

  vtkMap *Map;               /// Map instance
  vtkRenderer *Renderer;     /// vtk renderer
  QVTKWidget *MapWidget;     /// Map widget
  Ui_qtWeatherStations *UI;  /// Qt Designer form
  std::map<vtkIdType, StationReport> StationMap;

 protected slots:
   void resetMapCoords();
   void resizeMapWidget();
   void showStations();
};

#endif  // __qtWeatherStations_h
