/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMap.h"
#include "vtkGeoMapFeatureSelector.h"
#include "vtkGeoMapSelection.h"
#include "vtkInteractorStyleDrawPolygon.h"
#include "vtkInteractorStyleGeoMap.h"
#include "vtkLayer.h"
#include "vtkMapTile.h"
#include "vtkMap_typedef.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkMercator.h"

// VTK Includes
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCameraPass.h>
#include <vtkCollection.h>
#include <vtkEventForwarderCommand.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkRenderPassCollection.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSequencePass.h>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <math.h>
#include <sstream>

vtkStandardNewMacro(vtkMap) vtkCxxSetObjectMacro(vtkMap, Renderer, vtkRenderer);

//----------------------------------------------------------------------------
double computeCameraDistance(vtkCamera* cam, int zoomLevel)
{
  double deg = 360.0 / std::pow(2.0, zoomLevel);
  return (deg / std::sin(vtkMath::RadiansFromDegrees(cam->GetViewAngle())));
}

//----------------------------------------------------------------------------
int computeZoomLevel(vtkCamera* cam)
{
  double* pos = cam->GetPosition();
  double width = pos[2] * sin(vtkMath::RadiansFromDegrees(cam->GetViewAngle()));

  for (int i = 0; i < 20; ++i)
  {
    if (width >= (360.0 / (std::pow(2.0, i) * 1.001)))
    {
      return i;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
static void StaticPollingCallback(vtkObject* caller,
  long unsigned int vtkNotUsed(eventId), void* clientData,
  void* vtkNotUsed(callData))
{
  vtkMap* self = static_cast<vtkMap*>(clientData);
  self->PollingCallback();
}

//----------------------------------------------------------------------------
vtkMap::vtkMap()
  : LayerCollection(vtkSmartPointer<vtkRenderPassCollection>::New())
  , LayerSequence(vtkSmartPointer<vtkSequencePass>::New())
  , CameraPass(vtkSmartPointer<vtkCameraPass>::New())
  , RubberBandStyle(vtkSmartPointer<vtkInteractorStyleGeoMap>::New())
  , DrawPolyStyle(vtkSmartPointer<vtkInteractorStyleDrawPolygon>::New())
{
  this->StorageDirectory = NULL;
  this->Renderer = NULL;
  this->FeatureSelector = vtkGeoMapFeatureSelector::New();

  this->RubberBandStyle->SetMap(this);

  auto fwd = vtkEventForwarderCommand::New();
  fwd->SetTarget(this);
  this->RubberBandStyle->AddObserver(
    vtkInteractorStyleGeoMap::DisplayClickCompleteEvent, fwd);
  this->RubberBandStyle->AddObserver(
    vtkInteractorStyleGeoMap::DisplayDrawCompleteEvent, fwd);
  this->RubberBandStyle->AddObserver(
    vtkInteractorStyleGeoMap::SelectionCompleteEvent, fwd);
  this->RubberBandStyle->AddObserver(
    vtkInteractorStyleGeoMap::ZoomCompleteEvent, fwd);
  this->RubberBandStyle->AddObserver(
    vtkInteractorStyleGeoMap::RightButtonCompleteEvent, fwd);

  PolygonSelectionObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &vtkMap::OnPolygonSelectionEvent));
  this->DrawPolyStyle->AddObserver(
    vtkCommand::SelectionChangedEvent, PolygonSelectionObserver.GetPointer());

  this->PerspectiveProjection = false;
  this->Zoom = 1;
  this->Center[0] = this->Center[1] = 0.0;
  this->Initialized = false;
  this->BaseLayer = NULL;
  this->PollingCallbackCommand = NULL;
  this->CurrentAsyncState = AsyncOff;

  // Set default storage directory to ~/.vtkmap
  std::string fullPath =
    vtksys::SystemTools::CollapseFullPath(".vtkmap/tiles", "~/");
  this->SetStorageDirectory(fullPath.c_str());
}

//----------------------------------------------------------------------------
vtkMap::~vtkMap()
{
  if (this->FeatureSelector)
  {
    this->FeatureSelector->Delete();
  }
  if (this->PollingCallbackCommand)
  {
    this->PollingCallbackCommand->Delete();
  }
  if (this->StorageDirectory)
  {
    delete[] StorageDirectory;
  }
}

//----------------------------------------------------------------------------
void vtkMap::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkMap" << std::endl
     << "Zoom Level: " << this->Zoom << "Center: " << this->Center[0] << " "
     << this->Center[1] << std::endl
     << "StorageDirectory: " << this->StorageDirectory << std::endl;

  double* camPosition = this->Renderer->GetActiveCamera()->GetPosition();
  double* focalPosition = this->Renderer->GetActiveCamera()->GetFocalPoint();
  os << "  Zoom Level: " << this->Zoom << "\n"
     << "  Center Lat/Lon: " << this->Center[1] << " " << this->Center[0]
     << "\n"
     << "  Camera Position: " << camPosition[0] << " " << camPosition[1] << " "
     << camPosition[2] << "\n"
     << "  Focal Position: " << focalPosition[0] << " " << focalPosition[1]
     << " " << focalPosition[2] << "\n"
     << std::endl;
}

//----------------------------------------------------------------------------
void vtkMap::SetVisibleBounds(double latLngCoords[4])
{
  // Clip input coords to max lat/lon supported by web mercator (for now)
  double validCoords[4];
  validCoords[0] = vtkMercator::validLatitude(latLngCoords[0]);
  validCoords[1] = vtkMercator::validLongitude(latLngCoords[1]);
  validCoords[2] = vtkMercator::validLatitude(latLngCoords[2]);
  validCoords[3] = vtkMercator::validLongitude(latLngCoords[3]);

  // Convert to gcs coordinates
  double worldCoords[4];
  worldCoords[0] = validCoords[1];
  worldCoords[1] = vtkMercator::lat2y(validCoords[0]);
  worldCoords[2] = validCoords[3];
  worldCoords[3] = vtkMercator::lat2y(validCoords[2]);

  // Compute size as the larger of delta lon/lat
  double dx = fabs(worldCoords[2] - worldCoords[0]);
  if (dx > 180.0)
  {
    // If > 180, then points wrap around the 180th meridian
    // Adjust things to be > 0
    dx = 360.0 - dx;
    worldCoords[0] += worldCoords[0] < 0.0 ? 360.0 : 0;
    worldCoords[2] += worldCoords[2] < 0.0 ? 360.0 : 0;
  }
  double dy = fabs(worldCoords[3] - worldCoords[1]);
  double delta = dx > dy ? dx : dy;

  // Compute zoom level
  double maxDelta = 360.0;
  double maxZoom = 20; // NB: from 0 to 19
  int zoom = 0;
  double scaledDelta = delta;
  for (zoom = 0; scaledDelta < maxDelta && zoom < maxZoom; zoom++)
  {
    scaledDelta *= 2.0;
  }
  // Adjust for perspective vs. orthographic
  if (zoom > 0)
  {
    zoom -= this->PerspectiveProjection ? 1 : 0;
  }

  // Update center and zoom
  double center[2];
  center[0] = 0.5 * (validCoords[0] + validCoords[2]);
  center[1] = 0.5 * (validCoords[1] + validCoords[3]);
  this->SetZoom(zoom);
  this->SetCenter(center);
}

//----------------------------------------------------------------------------
void vtkMap::GetVisibleBounds(double latLngCoords[4])
{
  if (!this->Initialized)
  {
    return;
  }

  double displayCoords[2];
  double worldCoords[3];
  double latitude;
  double longitude;

  // Convert origin to world coords
  displayCoords[0] = 0.0;
  displayCoords[1] = 0.0;
  this->ComputeWorldCoords(displayCoords, 0.0, worldCoords);
  latitude = vtkMercator::y2lat(worldCoords[1]);
  longitude = worldCoords[0];
  latLngCoords[0] = vtkMercator::validLatitude(latitude);
  latLngCoords[1] = vtkMercator::validLongitude(longitude);

  // Convert opposite corner to world coords
  int* sizeCoords = this->Renderer->GetRenderWindow()->GetSize();
  displayCoords[0] = sizeCoords[0];
  displayCoords[1] = sizeCoords[1];
  this->ComputeWorldCoords(displayCoords, 0.0, worldCoords);
  latitude = vtkMercator::y2lat(worldCoords[1]);
  longitude = worldCoords[0];
  latLngCoords[2] = vtkMercator::validLatitude(latitude);
  latLngCoords[3] = vtkMercator::validLongitude(longitude);
}

//----------------------------------------------------------------------------
void vtkMap::GetCenter(double (&latlngPoint)[2])
{
  double* center = this->Renderer->GetCenter();
  //std::cerr << "center is " << center[0] << " " << center[1] << std::endl;
  this->Renderer->SetDisplayPoint(center[0], center[1], 0.0);
  this->Renderer->DisplayToWorld();

  double worldPoint[4];
  this->Renderer->GetWorldPoint(worldPoint);

  if (worldPoint[3] != 0.0)
  {
    worldPoint[0] /= worldPoint[3];
    worldPoint[1] /= worldPoint[3];
    worldPoint[2] /= worldPoint[3];
  }

  worldPoint[1] = vtkMercator::y2lat(worldPoint[1]);
  latlngPoint[0] = worldPoint[1];
  latlngPoint[1] = worldPoint[0];
}

//----------------------------------------------------------------------------
void vtkMap::SetCenter(double latLonPoint[2])
{
  this->SetCenter(latLonPoint[0], latLonPoint[1]);
}

//----------------------------------------------------------------------------
void vtkMap::SetCenter(double latitude, double longitude)
{
  this->Center[0] = latitude;
  this->Center[1] = longitude;

  // If initialized, update camera distance
  if (this->Initialized)
  {
    double x = longitude;
    double y = vtkMercator::lat2y(latitude);

    double cameraCoords[3] = { 0.0, 0.0, 1.0 };
    this->Renderer->GetActiveCamera()->GetPosition(cameraCoords);
    double z = cameraCoords[2];

    if (this->PerspectiveProjection)
    {
      z = computeCameraDistance(this->Renderer->GetActiveCamera(), this->Zoom);
    }
    this->Renderer->GetActiveCamera()->SetPosition(x, y, z);
    this->Renderer->GetActiveCamera()->SetFocalPoint(x, y, 0.0);
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkMap::SetStorageDirectory(const char* path)
{
  if (!path)
  {
    return;
  }

  std::string fullPath;
  if (vtksys::SystemTools::FileIsFullPath(path))
  {
    fullPath = path;
  }
  else
  {
    fullPath = vtksys::SystemTools::CollapseFullPath(path);
    vtkWarningMacro("Relative path specified, using " << fullPath);
  }

  // Create directory if it doesn't already exist
  if (!vtksys::SystemTools::FileIsDirectory(fullPath.c_str()))
  {
    std::cerr << "Creating storage directory " << fullPath << std::endl;
    vtksys::SystemTools::MakeDirectory(fullPath.c_str());
  }

  // Copy path to StorageDirectory
  delete[] this->StorageDirectory;
  const size_t n = fullPath.size() + 1;
  this->StorageDirectory = new char[n];
  strcpy(this->StorageDirectory, fullPath.c_str());
}

//----------------------------------------------------------------------------
void vtkMap::AddLayer(vtkLayer* layer)
{
  if (!this->Renderer)
  {
    vtkWarningMacro("Cannot add layer to vtkMap."
      << " Must set map's renderer *before* adding layers.");
    return;
  }

  if (layer->GetBase())
  {
    if (layer == this->BaseLayer)
    {
      return;
    }

    if (this->BaseLayer)
    {
      this->Layers.push_back(this->BaseLayer);
    }
    this->BaseLayer = layer;
  }
  else
  {
    LayerContainer::iterator it =
      std::find(this->Layers.begin(), this->Layers.end(), layer);
    if (it == this->Layers.end())
    {
      // TODO Use bin numbers to sort layer and its actors
      this->Layers.push_back(layer);
    }
  }

  layer->SetMap(this);
  this->UpdateLayerSequence();
}

//----------------------------------------------------------------------------
void vtkMap::RemoveLayer(vtkLayer* layer)
{
  if (layer == this->BaseLayer)
  {
    vtkErrorMacro("[error] Cannot remove base layer");
    return;
  }

  // Remove any features from feature selector
  vtkFeatureLayer* featureLayer = vtkFeatureLayer::SafeDownCast(layer);
  if (featureLayer)
  {
    vtkCollection* features = featureLayer->GetFeatures();
    for (int i = 0; i < features->GetNumberOfItems(); i++)
    {
      vtkObject* item = features->GetItemAsObject(i);
      vtkFeature* feature = vtkFeature::SafeDownCast(item);
      if (feature)
      {
        this->FeatureSelector->RemoveFeature(feature);
      }
    }
    features->RemoveAllItems();
  }

  auto itLayer = std::find(this->Layers.begin(), this->Layers.end(), layer);
  if (itLayer != this->Layers.end())
  {
    this->Layers.erase(itLayer);
  }
  this->UpdateLayerSequence();
}

//----------------------------------------------------------------------------
vtkLayer* vtkMap::FindLayer(const char* name)
{
  vtkLayer* result = NULL; // return value

  if (this->BaseLayer && this->BaseLayer->GetName() == name)
  {
    return this->BaseLayer;
  }

  LayerContainer::iterator it = this->Layers.begin();
  for (; it != this->Layers.end(); it++)
  {
    vtkLayer* layer = *it;
    if (layer->GetName() == name)
    {
      result = layer;
      break;
    }
  }

  return result;
}

//----------------------------------------------------------------------------
void vtkMap::Update()
{
  if (!this->BaseLayer)
  {
    return;
  }

  // Compute the zoom level here
  if (this->PerspectiveProjection)
  {
    this->SetZoom(computeZoomLevel(this->Renderer->GetActiveCamera()));
    //std::cout << "vtkMap::Update() set Zoom to " << this->Zoom << std::endl;
  }
  else
  {
    vtkCamera* camera = this->Renderer->GetActiveCamera();
    camera->ParallelProjectionOn();

    // Camera parallel scale == 1/2 the viewport height in world coords.
    // Each tile is 360 / 2**zoom in world coords
    // Each tile is 256 (pixels) in display coords
    int* renSize = this->Renderer->GetSize();
    //std::cout << "renSize " << renSize[0] << ", " << renSize[1] << std::endl;
    int zoomLevelFactor = 1 << this->Zoom;
    const double displayScaling = 1.0 / this->DevicePixelRatio;
    double parallelScale =
      displayScaling * 0.5 * (renSize[1] * 360.0 / zoomLevelFactor) / 256.0;
    //std::cout << "SetParallelScale " << parallelScale << std::endl;
    camera->SetParallelScale(parallelScale);
  }

  // Update the base layer first
  this->BaseLayer->Update();

  for (size_t i = 0; i < this->Layers.size(); ++i)
  {
    this->Layers[i]->Update();
  }
}

//----------------------------------------------------------------------------
void vtkMap::Initialize()
{
  // Set camera projection
  bool parallel = !this->PerspectiveProjection;
  this->Renderer->GetActiveCamera()->SetParallelProjection(parallel);

  // Make sure storage directory specified
  if (!this->StorageDirectory || std::strlen(this->StorageDirectory) == 0)
  {
    std::string fullPath =
      vtksys::SystemTools::CollapseFullPath(".vtkmap", "~/");
    this->SetStorageDirectory(fullPath.c_str());
    std::cerr << "Set map-tile storage directory to " << this->StorageDirectory
              << std::endl;
  }

  // Make sure storage directory specified with unix separators
  std::string strStorageDir(this->StorageDirectory); // for convenience
  vtksys::SystemTools::ConvertToUnixSlashes(strStorageDir);
  // If trailing slash char, strip it off
  if (*strStorageDir.rbegin() == '/')
  {
    strStorageDir.erase(strStorageDir.end() - 1);
    this->SetStorageDirectory(strStorageDir.c_str());
  }

  // Make sure storage directory exists
  if (!vtksys::SystemTools::FileIsDirectory(this->StorageDirectory))
  {
    std::cerr << "Create map-tile storage directory " << this->StorageDirectory
              << std::endl;
    vtksys::SystemTools::MakeDirectory(this->StorageDirectory);
  }

  // Initialize polling timer if there are any asynchronous layers
  LayerContainer allLayers(this->Layers);
  allLayers.push_back(this->BaseLayer);
  for (size_t i = 0; i < allLayers.size(); ++i)
  {
    if (allLayers[i]->IsAsynchronous())
    {
      this->PollingCallbackCommand = vtkCallbackCommand::New();
      this->PollingCallbackCommand->SetClientData(this);
      this->PollingCallbackCommand->SetCallback(StaticPollingCallback);

      vtkRenderWindowInteractor* interactor =
        this->Renderer->GetRenderWindow()->GetInteractor();
      interactor->CreateRepeatingTimer(31); // prime number > 30 fps
      interactor->AddObserver(
        vtkCommand::TimerEvent, this->PollingCallbackCommand);

      break;
    }
  }

  // Initialize graphics
  double x = this->Center[1];
  double y = vtkMercator::lat2y(this->Center[0]);

  // from setCenter
  double cameraCoords[3] = { 0.0, 0.0, 1.0 };
  this->Renderer->GetActiveCamera()->GetPosition(cameraCoords);

  double distance;
  if (this->PerspectiveProjection)
  {
    distance =
      computeCameraDistance(this->Renderer->GetActiveCamera(), this->Zoom);
  }
  else
  {
    distance = cameraCoords[2];
  }

  this->Renderer->GetActiveCamera()->SetPosition(x, y, distance);
  this->Renderer->GetActiveCamera()->SetFocalPoint(x, y, 0.0);
  this->Renderer->SetBackground(1.0, 1.0, 1.0);
  this->Renderer->GetRenderWindow()->Render();

  this->UpdateLayerSequence();
  this->LayerSequence->SetPasses(LayerCollection);
  CameraPass->SetDelegatePass(LayerSequence);
  this->Renderer->SetPass(CameraPass);

  this->Initialized = true;
}

//----------------------------------------------------------------------------
void vtkMap::UpdateLayerSequence()
{
  this->LayerCollection->RemoveAllItems();
  this->LayerCollection->AddItem(this->BaseLayer->GetRenderPass());
  for (auto& layer : this->Layers)
  {
    this->LayerCollection->AddItem(layer->GetRenderPass());
  }
}

//----------------------------------------------------------------------------
void vtkMap::Draw()
{
  if (!this->Initialized && this->Renderer)
  {
    this->Initialize();
  }

  this->Update();
  this->Renderer->GetRenderWindow()->Render();
}

//----------------------------------------------------------------------------
vtkMap::AsyncState vtkMap::GetAsyncState()
{
  return this->CurrentAsyncState;
}

//----------------------------------------------------------------------------
void vtkMap::FeatureAdded(vtkFeature* feature)
{
  this->FeatureSelector->AddFeature(feature);
}

//----------------------------------------------------------------------------
void vtkMap::ReleaseFeature(vtkFeature* feature)
{
  this->FeatureSelector->RemoveFeature(feature);
}

//----------------------------------------------------------------------------
double vtkMap::Clip(double n, double minValue, double maxValue)
{
  double max = n > minValue ? n : minValue;
  double min = max > maxValue ? maxValue : max;
  return min;
}

//----------------------------------------------------------------------------
void vtkMap::ComputeLatLngCoords(
  double displayCoords[2], double elevation, double latLngCoords[3])
{
  // Compute GCS coordinates
  double worldCoords[3] = { 0.0, 0.0, 0.0 };
  this->ComputeWorldCoords(displayCoords, elevation, worldCoords);

  // Convert to lat-lon
  double latitude = vtkMercator::y2lat(worldCoords[1]);
  double longitude = worldCoords[0];

  // Clip to "valid" coords
  latLngCoords[0] = vtkMercator::validLatitude(latitude);
  latLngCoords[1] = vtkMercator::validLongitude(longitude);
  latLngCoords[2] = elevation;
}

//----------------------------------------------------------------------------
void vtkMap::PickPoint(int displayCoords[2], vtkGeoMapSelection* result)
{
  this->BeginSelection();
  this->FeatureSelector->PickPoint(this->Renderer, displayCoords, result);
  this->EndSelection();
}

//----------------------------------------------------------------------------
void vtkMap::PickArea(int displayCoords[4], vtkGeoMapSelection* result)
{
  this->BeginSelection();
  this->FeatureSelector->PickArea(this->Renderer, displayCoords, result);
  this->EndSelection();
}

//----------------------------------------------------------------------------
void vtkMap::OnPolygonSelectionEvent()
{
  std::vector<vtkVector2i> points = this->DrawPolyStyle->GetPolygonPoints();
  vtkNew<vtkGeoMapSelection> result;

  this->BeginSelection();
  this->FeatureSelector->PickPolygon(
    this->Renderer, points, result.GetPointer());
  this->EndSelection();

  this->InvokeEvent(
    vtkInteractorStyleGeoMap::SelectionCompleteEvent, result.GetPointer());
}

//----------------------------------------------------------------------------
void vtkMap::BeginSelection()
{
  auto renWin = this->Renderer->GetRenderWindow();
  this->PreviousSwapBuffers = renWin->GetSwapBuffers();
  renWin->SwapBuffersOff();
  this->Renderer->SetPass(nullptr);
}

//----------------------------------------------------------------------------
void vtkMap::EndSelection()
{
  this->Renderer->SetPass(this->CameraPass);
  auto renWin = this->Renderer->GetRenderWindow();
  renWin->SetSwapBuffers(this->PreviousSwapBuffers);
}

//----------------------------------------------------------------------------
void vtkMap::ComputeWorldCoords(
  double displayCoords[2], double z, double worldCoords[3])
{
  // Get renderer's DisplayToWorld point
  double rendererCoords[4] = { 0.0, 0.0, 0.0, 1.0 };
  this->Renderer->SetDisplayPoint(displayCoords[0], displayCoords[1], 0.0);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(rendererCoords);
  if (rendererCoords[3] != 0.0)
  {
    rendererCoords[0] /= rendererCoords[3];
    rendererCoords[1] /= rendererCoords[3];
    rendererCoords[2] /= rendererCoords[3];
  }

  if (this->PerspectiveProjection)
  {
    // Project to z

    // Get camera point
    double cameraCoords[3];
    vtkCamera* camera = this->Renderer->GetActiveCamera();
    camera->GetPosition(cameraCoords);

    // Compute line-of-sight vector from camera to renderer point
    double losVector[3];
    vtkMath::Subtract(rendererCoords, cameraCoords, losVector);

    // Set magnitude of vector's z coordinate to 1.0
    vtkMath::MultiplyScalar(losVector, fabs(1.0 / losVector[2]));

    // Project line-of-sight vector from camera to specified z
    double deltaZ = cameraCoords[2] - z;
    vtkMath::MultiplyScalar(losVector, deltaZ);
    worldCoords[0] = cameraCoords[0] + losVector[0];
    worldCoords[1] = cameraCoords[1] + losVector[1];
    worldCoords[2] = z;
  }
  else
  {
    worldCoords[0] = rendererCoords[0];
    worldCoords[1] = rendererCoords[1];
    worldCoords[2] = z;
  }
}

//----------------------------------------------------------------------------
void vtkMap::ComputeDisplayCoords(
  double latLngCoords[2], double elevation, double displayCoords[3])
{
  double x = latLngCoords[1];
  double y = vtkMercator::lat2y(latLngCoords[0]);
  this->Renderer->SetWorldPoint(x, y, elevation, 1.0);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(displayCoords);
}

//----------------------------------------------------------------------------
void vtkMap::PollingCallback()
{
  AsyncState result;
  AsyncState newState = AsyncIdle;

  // Compute highest "state" of async layers
  LayerContainer allLayers(this->Layers);
  allLayers.push_back(this->BaseLayer);
  for (size_t i = 0; i < allLayers.size(); ++i)
  {
    if (allLayers[i]->IsAsynchronous())
    {
      result = allLayers[i]->ResolveAsync();
      newState = newState >= result ? newState : result;
    }
  }
  this->CurrentAsyncState = newState;

  // Current strawman is to redraw on partial or full update
  if (newState >= AsyncPartialUpdate)
  {
    this->Draw();
  }
}

//----------------------------------------------------------------------------
void vtkMap::MoveLayer(const vtkLayer* layer, vtkMapType::Move direction)
{
  switch (direction)
  {
    case vtkMapType::Move::UP:
      this->MoveUp(layer);
      break;
    case vtkMapType::Move::DOWN:
      this->MoveDown(layer);
      break;
    case vtkMapType::Move::TOP:
      this->MoveToTop(layer);
      break;
    case vtkMapType::Move::BOTTOM:
      this->MoveToBottom(layer);
      break;
    default:
      vtkErrorMacro(<< "Move direction not supported!");
  }

  this->Draw();
}

//----------------------------------------------------------------------------
void vtkMap::MoveUp(const vtkLayer* layer)
{
  auto result = std::find(this->Layers.begin(), this->Layers.end(), layer);
  auto nextIt = result;
  nextIt++;
  if (result == this->Layers.cend() || nextIt == this->Layers.cend())
  {
    return;
  }

  std::iter_swap(result, nextIt);
  this->UpdateLayerSequence();
}

//----------------------------------------------------------------------------
void vtkMap::MoveDown(const vtkLayer* layer)
{
  auto result = std::find(this->Layers.begin(), this->Layers.end(), layer);
  if (result == this->Layers.cend() || result == this->Layers.cbegin())
  {
    return;
  }

  auto prevIt = result;
  prevIt--;
  std::iter_swap(result, prevIt);
  this->UpdateLayerSequence();
}

//----------------------------------------------------------------------------
void vtkMap::MoveToTop(const vtkLayer* layer)
{
  auto result = std::find(this->Layers.begin(), this->Layers.end(), layer);
  if (result == this->Layers.cend())
  {
    return;
  }

  // rotation ccw
  std::rotate(result, result + 1, this->Layers.end());
  this->UpdateLayerSequence();
}

//----------------------------------------------------------------------------
void vtkMap::MoveToBottom(const vtkLayer* layer)
{
  auto result = std::find(this->Layers.begin(), this->Layers.end(), layer);
  if (result == this->Layers.cend())
  {
    return;
  }

  // rotation cw
  // Note: when an iterator is reversed, the reversed version points
  // to the one preceeding it, so it is decremented to point to the original
  // element.
  auto rresult = LayerContainer::reverse_iterator(result) - 1;
  std::rotate(rresult, rresult + 1, this->Layers.rend());
  this->UpdateLayerSequence();
}

//----------------------------------------------------------------------------
void vtkMap::SetInteractionMode(const vtkMapType::Interaction mode)
{
  vtkInteractorStyle* style = nullptr;
  switch (mode)
  {
    case vtkMapType::Interaction::Default:
      this->RubberBandStyle->SetRubberBandModeToDisabled();
      style = this->RubberBandStyle;
      break;
    case vtkMapType::Interaction::RubberBandSelection:
      this->RubberBandStyle->SetRubberBandModeToSelection();
      style = this->RubberBandStyle;
      break;
    case vtkMapType::Interaction::RubberBandZoom:
      this->RubberBandStyle->SetRubberBandModeToZoom();
      style = this->RubberBandStyle;
      break;
    case vtkMapType::Interaction::RubberBandDisplayOnly:
      this->RubberBandStyle->SetRubberBandModeToDisplayOnly();
      style = this->RubberBandStyle;
      break;
    case vtkMapType::Interaction::PolygonSelection:
      style = this->DrawPolyStyle;
      break;
  }

  if (!this->Interactor)
    return;

  this->Interactor->SetInteractorStyle(style);
}

//----------------------------------------------------------------------------
void vtkMap::SetInteractor(vtkRenderWindowInteractor* inter)
{
  if (this->Interactor != inter)
  {
    if (this->Interactor)
      this->Interactor->UnRegister(this);

    this->Interactor = inter;
    if (this->Interactor)
    {
      this->Interactor->Register(this);
      this->SetInteractionMode(vtkMapType::Interaction::Default);
    }

    this->Modified();
  }
}
