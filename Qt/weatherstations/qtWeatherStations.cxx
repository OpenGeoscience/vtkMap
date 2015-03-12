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
#include "vtkMapMarkerSet.h"
//#include "vtkMapPickResult.h"
#include "vtkOsmLayer.h"
#include <QVTKWidget.h>
#include <vtkCallbackCommand.h>
#include <vtkInteractorStyleImage.h>
#include <vtkNew.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

#include <QMessageBox>
#include <QString>
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
// Callback command for handling interactor mouse events
// In the future, will be replaced by interactor style
class MapCallback : public vtkCallbackCommand
{
public:
  MapCallback(qtWeatherStations *app) : App(app) {}

  virtual void Execute(vtkObject *caller, unsigned long eventId, void *callData)
  {
    vtkRenderWindowInteractor *interactor =
      vtkRenderWindowInteractor::SafeDownCast(caller);

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

      // case vtkCommand::LeftButtonPressEvent:
      //   {
      //   int *pos = interactor->GetEventPosition();
      //   this->App->pickMarker(pos);
      //   }
      //   break;

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
  // Always set map's renderer *before* adding layers
  this->Renderer = vtkRenderer::New();
  this->Map->SetRenderer(this->Renderer);

  vtkNew<vtkOsmLayer> osmLayer;
  this->Map->AddLayer(osmLayer.GetPointer());
  //this->resetMapCoords();
  //this->Map->SetCenter(32.2, -90.9);  // approx ERDC coords
  this->Map->SetCenter(42.849604, -73.758345);  // KHQ coords
  this->Map->SetZoom(5);

  this->Map->GetMapMarkerSet()->ClusteringOn();

  vtkNew<vtkRenderWindow> mapRenderWindow;
  mapRenderWindow->AddRenderer(this->Renderer);
  this->MapWidget->SetRenderWindow(mapRenderWindow.GetPointer());

  vtkRenderWindowInteractor *intr = this->MapWidget->GetInteractor();
  intr->SetInteractorStyle(this->Map->GetInteractorStyle());
  intr->Initialize();

  // Pass *all* callbacks to MapCallback instance
  this->InteractorCallback = new MapCallback(this);
  intr->AddObserver(vtkCommand::AnyEvent, this->InteractorCallback);
  intr->Start();

  // Connect UI controls
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
  if (this->Map)
    {
    this->Map->Delete();
    }
  if (this->Renderer)
    {
    this->Renderer->Delete();
    }
  if (this->InteractorCallback)
    {
    this->InteractorCallback->Delete();
    }
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
// Retrieves list of weather stations and displays on map
void qtWeatherStations::showStations()
{
  this->UI->StationText->setFontFamily("Courier New");

  // Clear out current station data
  this->UI->StationText->clear();
  this->UI->StationText->setText("Retrieving station data.");
  // Todo is there any way to update StationText (QTextEdit) *now* ???

  this->Map->GetMapMarkerSet()->RemoveMarkers();
  this->StationMap.clear();

  // Request weather station data
  Json::Value json = this->RequestStationData();
  if (json.isNull())
    {
    return;
    }

  std::vector<StationReport> stationList = this->ParseStationData(json);
  this->DisplayStationData(stationList);
  this->DisplayStationMarkers(stationList);
}

// ------------------------------------------------------------
Json::Value qtWeatherStations::RequestStationData()
{
  Json::Value json;  // return value
  std::stringstream ss;

  // Get current map coordinates
  double center[2];
  this->Map->GetCenter(center);
  int zoom = this->Map->GetZoom();
  this->UI->MapCoordinatesWidget->setCoordinates(center, zoom);
  double lat = center[0];
  double lon = center[1];
  ss << "Map coordinates (lat, lon) are"
     << " (" << lat << ", " << lon << ")"
    ", zoom " << zoom;

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

  std::stringstream curlStream;
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlStream);

  //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  //std::cout << "Start request" << std::endl;
  // Use blocking IO for now
  CURLcode res = curl_easy_perform(curl);
  //std::cout << "Request end, return value " << res << std::endl;
  curl_easy_cleanup(curl);

  // Parse input string (json)
  curlStream.seekp(0L);
  std::string curlData = curlStream.str();
  //std::cout << curlData << std::endl;

  Json::Reader reader;
  if (!reader.parse(curlStream, json, false))
    {
    ss << "\n" << "Error parsing input data - see console for more info.";
    std::cerr << reader.getFormattedErrorMessages() << std::endl;
    }
  this->UI->StationText->append(QString::fromStdString(ss.str()));
  //std::cout << "json.type() " << json.type() << std::endl;
  //std::cout << json.asString() << std::endl;

  return json;
}

// ------------------------------------------------------------
// Parses json object and returns list of station reports
std::vector<StationReport>
qtWeatherStations::ParseStationData(Json::Value json)
{
  std::vector<StationReport> stationList;  // return value

  Json::Value stationListNode = json["list"];
  if (stationListNode.isNull())
    {
    return stationList;
    }

  for (int i=0; i<stationListNode.size(); ++i)
    {
    StationReport station;

    // Station ID & name
    Json::Value stationNode = stationListNode[i];

    Json::Value idNode = stationNode["id"];
    station.id = idNode.asInt();

    Json::Value nameNode = stationNode["name"];
    station.name = nameNode.asString();

    // Geo coords
    Json::Value coordNode = stationNode["coord"];
    Json::Value latNode = coordNode["lat"];
    Json::Value lonNode = coordNode["lon"];
    station.latitude = latNode.asDouble();
    station.longitude = lonNode.asDouble();

    // Datetime
    Json::Value dtNode = stationNode["dt"];
    station.datetime = time_t(dtNode.asInt());  // dt units are seconds

    // Current temp
    Json::Value mainNode = stationNode["main"];
    Json::Value tempNode = mainNode["temp"];
    station.temperature = tempNode.asDouble();

    stationList.push_back(station);
    }

  return stationList;
}

// ------------------------------------------------------------
// Writes station info to StationText (QTextEdit)
void qtWeatherStations::
DisplayStationData(std::vector<StationReport> stationList)
{
  std::stringstream ss;
  for (int i=0; i<stationList.size(); ++i)
    {
    StationReport station = stationList[i];
    ss << std::setw(3) << i+1
       << ". " << station.id
       << "  " << std::setw(20) << station.name

       << "  " << std::setiosflags(std::ios_base::fixed)
       << std::setprecision(1) << station.temperature << "F"

       << "  (" << std::setiosflags(std::ios_base::fixed)
       << std::setprecision(6) << station.latitude
       << "  " << std::setiosflags(std::ios_base::fixed)
       << std::setprecision(6) << station.longitude << ")"

       << "  " << ctime(&station.datetime);
    }

  this->UI->StationText->append(QString::fromStdString(ss.str()));
}

// ------------------------------------------------------------
// Updates display to show marker for each station in input list
// Updates internal Stations dictionary
void qtWeatherStations::
DisplayStationMarkers(std::vector<StationReport> stationList)
{
  // Create map markers for each station
  vtkMapMarkerSet *markerLayer = this->Map->GetMapMarkerSet();
  for (int i=0; i<stationList.size(); ++i)
    {
    StationReport station = stationList[i];
    vtkIdType id = markerLayer->AddMarker(station.latitude, station.longitude);
    if (id >= 0)
      this->StationMap[id] = station;
    }
  this->drawMap();
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

// ------------------------------------------------------------
// Returns vtkRenderer
vtkRenderer *qtWeatherStations::getRenderer() const
{
  return this->Renderer;
}

// ------------------------------------------------------------
// Handles left-click event
void qtWeatherStations::pickMarker(int displayCoords[2])
{
  QMessageBox::information(this->MapWidget, "Work In Progress",
                           "Sorry - picking is currently disabled");
  // std::stringstream ss;
  // vtkNew<vtkMapPickResult> pickResult;
  // this->Map->PickPoint(displayCoords, pickResult.GetPointer());

  // switch (pickResult->GetMapFeatureType())
  //   {
  //   case VTK_MAP_FEATURE_MARKER:
  //     {
  //     std::map<vtkIdType, StationReport>::iterator stationIter =
  //       this->StationMap.find(pickResult->GetMapFeatureId());
  //     if (stationIter != this->StationMap.end())
  //       {
  //       StationReport station = stationIter->second;
  //       ss << "Station: " << station.name << "\n"
  //          << "Current Temp: " << std::setiosflags(std::ios_base::fixed)
  //          << std::setprecision(1) << station.temperature << "F";
  //       QMessageBox::information(this->MapWidget, "Marker clicked",
  //                                QString::fromStdString(ss.str()));
  //       }
  //     }
  //     break;

  //   case VTK_MAP_FEATURE_CLUSTER:
  //     ss << "Cluster of " << pickResult->GetNumberOfMarkers() << " stations.";
  //     QMessageBox::information(this->MapWidget, "Cluster clicked",
  //                              QString::fromStdString(ss.str()));
  //     break;
  //   }
}

// ------------------------------------------------------------
// Overrides base class method
void qtWeatherStations::resizeEvent(QResizeEvent *event)
{
  QMainWindow::resizeEvent(event);

  // Resize map widget if it's been initialized
  if (this->MapWidget)
    {
    int margin = 4;
    QSize sz = this->UI->MapFrame->size();
    int w = sz.width() - 2*margin;
    int h = sz.height()- 2*margin;

    this->MapWidget->resize(w, h);
    this->MapWidget->move(margin, margin);

    this->drawMap();
    }
}
