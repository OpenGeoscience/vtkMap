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
#include <map>
#include <vtkType.h>
#include <vtk_jsoncpp.h>
class vtkGeoMapSelection;
class vtkMap;
class vtkMapMarkerSet;
class vtkRenderer;
class QVTKWidget;
class MapCallback;
class QWidget;

// Forward Qt class declarations
class Ui_qtWeatherStations;

class qtWeatherStations : public QMainWindow
{
  Q_OBJECT
public:
  qtWeatherStations(QWidget* parent = 0);
  ~qtWeatherStations() override;

  void drawMap();
  void updateMap();
  vtkRenderer* getRenderer() const;
  void displaySelectionInfo(vtkGeoMapSelection* selection) const;
  QWidget* mapWidget() const;
public slots:

protected:
  Json::Value RequestStationData();
  std::vector<StationReport> ParseStationData(Json::Value data);
  void DisplayStationData(std::vector<StationReport> statonList);
  void DisplayStationMarkers(std::vector<StationReport> statonList);
  void resizeEvent(QResizeEvent* event) override;

  vtkMap* Map;                 /// Map instance
  vtkMapMarkerSet* MapMarkers; // Map markers
  vtkRenderer* Renderer;       /// vtk renderer
  QVTKWidget* MapWidget;       /// Map widget
  Ui_qtWeatherStations* UI;    /// Qt Designer form
  std::map<vtkIdType, StationReport> StationMap;
  MapCallback* InteractorCallback;

protected slots:
  void moveToCoords();
  void showStations();
  void toggleClustering(int state);
  void onClusterDistanceChanged(int value);
  void onUpdateCoordsWidget();
  void recomputeClusters();
};

#endif // __qtWeatherStations_h
