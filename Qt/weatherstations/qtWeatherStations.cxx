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

#include "qtWeatherStations.h"
#include "ui_qtWeatherStations.h"
#include "vtkMap.h"
#include <QVTKWidget.h>
#include <vtkCallbackCommand.h>
#include <vtkInteractorStyleImage.h>
#include <vtkNew.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <QVBoxLayout>
#include <curl/curl.h>
#include <sstream>
#include <stdlib.h>  // for sprintf


namespace {
  // ------------------------------------------------------------
  // [static] Handles callbacks from curl_easy_perform()
  // Writes input buffer to stream (last argument)
  size_t handle_curl_input(void *buffer, size_t size,
                           size_t nmemb, void *stream)
  {
    //std::cout << "handle_input() nmemb: " << nmemb << std::endl
    //std::cout << static_cast<char *>(buffer) << std::endl;
    std::stringstream *ss = static_cast<std::stringstream *>(stream);
    *ss << static_cast<char *>(buffer);
    return nmemb;
  }
}  // namespace


// ------------------------------------------------------------
// Callback command for handling mouse events
// In the future, will be replaced by interactor style
class MapCallback : public vtkCallbackCommand
{
public:
  MapCallback(qtWeatherStations *app) : App(app), MouseDown(false) {}

  virtual void Execute(vtkObject *caller, unsigned long eventId, void *callData)
  {
    switch (eventId)
      {
      case vtkCommand::MiddleButtonPressEvent:
        this->MouseDown = true;
        break;
      case vtkCommand::MiddleButtonReleaseEvent:
        this->App->drawMap();
        this->MouseDown = false;
        break;
      case vtkCommand::MouseMoveEvent:
        if (this->MouseDown)
          {
          this->App->drawMap();
          }
      case vtkCommand::MouseWheelForwardEvent:
      case vtkCommand::MouseWheelBackwardEvent:
        this->App->drawMap();
      }
  }

protected:
  qtWeatherStations *App;
  bool MouseDown;
};


// ------------------------------------------------------------
qtWeatherStations::qtWeatherStations(QWidget *parent)
  : QMainWindow(parent)
{
  // Initialize Qt UI (generated from .ui file)
  this->UI = new Ui_qtWeatherStations;
  this->UI->setupUi(this);

  // Manually set label invisible at startup
  this->UI->RetrievingStationsLabel->setVisible(false);

  // Initialize map widget
  this->MapWidget = new QVTKWidget(this->UI->MapFrame);
  this->MapWidget->resize(640, 480);
  this->MapWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  // Initialize Map instance
  this->Map = vtkMap::New();
  vtkNew<vtkRenderer> mapRenderer;
  this->Map->SetRenderer(mapRenderer.GetPointer());
  //this->resetMapCoords();
  this->Map->SetCenter(0, 0);
  this->Map->SetZoom(5);

  vtkNew<vtkRenderWindow> mapRenderWindow;
  mapRenderWindow->AddRenderer(mapRenderer.GetPointer());
  this->MapWidget->SetRenderWindow(mapRenderWindow.GetPointer());

  vtkRenderWindowInteractor *intr = this->MapWidget->GetInteractor();
  intr->SetInteractorStyle(this->Map->GetInteractorStyle());
  intr->Initialize();

  // Pass *all* callbacks to MapCallback instance
  MapCallback *mapCallback = new MapCallback(this);
  intr->AddObserver(vtkCommand::AnyEvent, mapCallback);
  intr->Start();

  // Connect manual map-resize button to corresponding slot
  // Since I cannot figure out how to do this automatically
  QObject::connect(this->UI->ResizeMapButton, SIGNAL(clicked()),
                   this, SLOT(resizeMapWidget()));

  // Other connections
  QObject::connect(this->UI->ResetButton, SIGNAL(clicked()),
                   this, SLOT(resetMapCoords()));
  QObject::connect(this->UI->ShowStationsButton, SIGNAL(clicked()),
                   this, SLOT(showStations()));
}

// ------------------------------------------------------------
// Resets map coordinates
void qtWeatherStations::resetMapCoords()
{
  if (this->Map)
    {
    // Hard coded for now
    this->Map->SetCenter(0, 0);
    this->Map->SetZoom(5);
    this->drawMap();
    }
}

// ------------------------------------------------------------
// Resizes MapWidget to fill its parent frame
void qtWeatherStations::resizeMapWidget()
{
  int margin = 4;
  QSize sz = this->UI->MapFrame->size();
  int w = sz.width() - 2*margin;
  int h = sz.height()- 2*margin;


  this->MapWidget->resize(w, h);
  this->MapWidget->move(margin, margin);

  this->drawMap();
}

// ------------------------------------------------------------
// Retrieves list of weather stations and displays on map
void qtWeatherStations::showStations()
{
  // Todo
  //this->UI->RetrievingStationsLabel->setText("Sorry, not implemented yet");
  //this->UI->RetrievingStationsLabel->show();
  std::stringstream textData;
  textData << "Retrieving station data.\n";
  //this->UI->StationText->setText("Retrieving station info");
  double center[2];
  this->Map->GetCenter(center);
  int zoom = this->Map->GetZoom();
  this->UI->MapCoordinatesWidget->setCoordinates(center, zoom);
  textData << "Map coordinates (lat, lon) are"
           << " (" << center[0] << ", " << center[1] << ")"
           ", zoom " << zoom << "\n";

  // K28 lat-lon coordinates per http://www.latlong.net:
  double lat =  42.849604;
  double lon = -73.758345;
  textData << "Instead using KHQ geo coords (" << lat << ", " << lon << ")\n\n";
  this->UI->StationText->setText(textData.str().c_str());

  // Construct openweathermaps request
  const char *appId = "14cdc51cab181f8848f43497c58f1a96";
  int count = 7;  // number of stations to request
  const char *format = "http://api.openweathermap.org/data/2.5/find"
    "?lat=%f&lon=%f&cnt=%d&units=imperial&APPID=%s";
  char url[256];
  sprintf(url, format, lat, lon, count, appId);
  std::cout << "url " << url << std::endl;

  // Initialize curl & send request
  curl_global_init(CURL_GLOBAL_DEFAULT);
  CURL *curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handle_curl_input);

  std::stringstream ss;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ss);

  //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  //std::cout << "Start request" << std::endl;
  // Use blocking IO for now
  CURLcode res = curl_easy_perform(curl);
  //std::cout << "Request end, return value " << res << std::endl;
  curl_easy_cleanup(curl);
  curl_global_cleanup();


  // Parse input string (json)
  ss.seekp(0L);
  std::string ssData = ss.str();
  textData << ssData.c_str();
  this->UI->StationText->setText(textData.str().c_str());
}

// ------------------------------------------------------------
// Calls map Draw() method
void qtWeatherStations::drawMap()
{
  if (this->Map)
    {
    this->Map->Draw();
    double center[2];
    this->Map->GetCenter(center);
    int zoom = this->Map->GetZoom();
    this->UI->MapCoordinatesWidget->setCoordinates(center, zoom);
    }
}
