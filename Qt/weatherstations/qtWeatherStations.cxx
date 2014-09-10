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
#include <cJSON/cJSON.h>
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
// Callback command for handling interactor mouse events
// In the future, will be replaced by interactor style
class MapCallback : public vtkCallbackCommand
{
public:
  MapCallback(qtWeatherStations *app) : App(app) {}

  virtual void Execute(vtkObject *caller, unsigned long eventId, void *callData)
  {
    switch (eventId)
      {
      case vtkCommand::MouseWheelForwardEvent:
      case vtkCommand::MouseWheelBackwardEvent:
        // Redraw for zoom events
        this->App->drawMap();
        break;

      case vtkCommand::ModifiedEvent:
        // Update markers for interactor-modified events
        this->App->updateMap();
        break;

      default:
        //std::cout << "Mouse event " << vtkCommand::GetStringFromEventId(eventId) << std::endl;
        break;
      }
  }

protected:
  qtWeatherStations *App;
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
  //this->Map->SetCenter(32.2, -90.9);  // approx ERDC coords
  this->Map->SetCenter(42.849604, -73.758345);  // KHQ coords
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

  // Initialize curl
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

// ------------------------------------------------------------
qtWeatherStations::~qtWeatherStations()
{
  curl_global_cleanup();
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
  this->UI->StationText->setFontFamily("Courier New");
  this->UI->StationText->clear();
  this->UI->StationText->setText("Retrieving station data");
  // Todo is there any way to update the display *now* ???

  std::stringstream textData;
  double center[2];
  this->Map->GetCenter(center);
  int zoom = this->Map->GetZoom();
  this->UI->MapCoordinatesWidget->setCoordinates(center, zoom);
  double lat = center[0];
  double lon = center[1];
  textData << "Map coordinates (lat, lon) are"
           << " (" << lat << ", " << lon << ")"
           ", zoom " << zoom << "\n";

  // Construct openweathermaps request
  int count = this->UI->StationCountSpinBox->value();
  const char *appId = "14cdc51cab181f8848f43497c58f1a96";
  const char *format = "http://api.openweathermap.org/data/2.5/find"
    "?lat=%f&lon=%f&cnt=%d&units=imperial&APPID=%s";
  char url[256];
  sprintf(url, format, lat, lon, count, appId);
  std::cout << "url " << url << std::endl;

  // Initialize curl & send request
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


  // Parse input string (json)
  ss.seekp(0L);
  std::string ssData = ss.str();

  cJSON *json = cJSON_Parse(ssData.c_str());
  if (!json)
    {
    std::cerr << "Error before: [" << cJSON_GetErrorPtr() << "]" << std::endl;
    textData << ssData.c_str();
    this->UI->StationText->setText(textData.str().c_str());
    return;
    }

  // char *out = cJSON_Print(json);
  // printf("%s\n",out);
  // free(out);

  cJSON *stationList = cJSON_GetObjectItem(json, "list");
  if (!stationList)
    {
    std::cerr << "No list element found" << std::endl;
    textData << ssData.c_str();
    this->UI->StationText->setText(textData.str().c_str());
    return;
    }
  int stationListSize = cJSON_GetArraySize(stationList);
  textData << "Station list size " << stationListSize << "\n";

  this->DisplayStationMarkers(stationList);

  // Todo: refactor following code into DisplayStationData() method

  // Dump info for returned stations
  for (int i=0; i<stationListSize; ++i)
    {
    // Station ID & name
    cJSON *station = cJSON_GetArrayItem(stationList, i);
    cJSON *idNode = cJSON_GetObjectItem(station, "id");
    cJSON *nameNode = cJSON_GetObjectItem(station, "name");
    textData << std::setw(3) << i+1
             << ". " << idNode->valueint
             << "  " << std::setw(20) << nameNode->valuestring;

    // Current temp
    cJSON *mainNode = cJSON_GetObjectItem(station, "main");
    cJSON *tempNode = cJSON_GetObjectItem(mainNode, "temp");
    textData << "  " << std::setiosflags(std::ios_base::fixed)
             << std::setprecision(1) << tempNode->valuedouble << "F";

    // Geo coords
    cJSON *coordNode = cJSON_GetObjectItem(station, "coord");
    cJSON *latNode = cJSON_GetObjectItem(coordNode, "lat");
    cJSON *lonNode = cJSON_GetObjectItem(coordNode, "lon");
    textData << "  (" << std::setiosflags(std::ios_base::fixed)
             << std::setprecision(6) << latNode->valuedouble
             << ", " << std::setiosflags(std::ios_base::fixed)
             << std::setprecision(6) << lonNode->valuedouble << ")";

    // Datetime
    cJSON *dtNode = cJSON_GetObjectItem(station, "dt");
    time_t dt(dtNode->valueint);  // dt units are seconds
    textData << "  " << ctime(&dt);  // includes newline
    //textData  << "\n";
    }
  this->UI->StationText->setText(textData.str().c_str());

  cJSON_Delete(json);
}

// ------------------------------------------------------------
void qtWeatherStations::DisplayStationMarkers(cJSON *stationList)
{
  this->Map->RemoveMapMarkers();

  int stationListSize = cJSON_GetArraySize(stationList);

  // Create map markers for each station
  for (int i=0; i<stationListSize; ++i)
    {
    cJSON *station = cJSON_GetArrayItem(stationList, i);
    cJSON *coordNode = cJSON_GetObjectItem(station, "coord");
    cJSON *latNode = cJSON_GetObjectItem(coordNode, "lat");
    cJSON *lonNode = cJSON_GetObjectItem(coordNode, "lon");

    double lat = latNode->valuedouble;
    double lon = lonNode->valuedouble;
    // std::cout << "Adding marker at lat/lon "
    //           << lat << ", " << lon << std::endl;
    this->Map->AddMarker(lat, lon);
    }
 }


// ------------------------------------------------------------
// Calls map Draw() method
void qtWeatherStations::drawMap()
{
  if (this->Map)
    {
    this->Map->Draw();
    }
}


// ------------------------------------------------------------
// Calls map Update() method
void qtWeatherStations::updateMap()
{
  if (this->Map)
    {
    // Call Map->Update to update marker display positions
    this->Map->Update();

    // Also update displayed lat/lon coords
    double center[2];
    this->Map->GetCenter(center);
    //std::cout << "GetCenter " << center[0] << ", " << center[1] << std::endl;
    int zoom = this->Map->GetZoom();
    this->UI->MapCoordinatesWidget->setCoordinates(center, zoom);
    }
}
