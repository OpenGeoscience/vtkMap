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
#include "vtkInteractorStyleGeoMap.h"
#include "vtkLayer.h"
#include "vtkMapTile.h"
#include "vtkMercator.h"

// VTK Includes
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCollection.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <math.h>
#include <sstream>

vtkStandardNewMacro(vtkMap)

//----------------------------------------------------------------------------
double computeCameraDistance(vtkCamera* cam, int zoomLevel)
{
  double deg = 360.0 / std::pow( 2.0, zoomLevel);
  // Offset by 0.1% to reduce numerical issues w/computeZoomLevel()
  return (1.001 * deg / std::sin(vtkMath::RadiansFromDegrees(cam->GetViewAngle())));
}

//----------------------------------------------------------------------------
int computeZoomLevel(vtkCamera* cam)
{
  double* pos = cam->GetPosition();
  double width = pos[2] * sin(vtkMath::RadiansFromDegrees(cam->GetViewAngle()));

  for (int i = 0; i < 20; ++i) {
    if (width >= (360.0 / std::pow( 2.0, i))) {
      /// We are forcing the minimum zoom level to 2 so that we can get
      /// high res imagery even at the zoom level 0 distance
      return i;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
static void StaticPollingCallback(
  vtkObject* caller, long unsigned int vtkNotUsed(eventId),
    void* clientData, void* vtkNotUsed(callData))
{
  vtkMap *self = static_cast<vtkMap*>(clientData);
  self->PollingCallback();
}

//----------------------------------------------------------------------------
vtkMap::vtkMap()
{
  this->StorageDirectory = NULL;
  this->Renderer = NULL;
  this->FeatureSelector = vtkGeoMapFeatureSelector::New();
  this->InteractorStyle = vtkInteractorStyleGeoMap::New();
  this->InteractorStyle->SetMap(this);
  this->Zoom = 1;
  this->Center[0] = this->Center[1] = 0.0;
  this->Initialized = false;
  this->BaseLayer = NULL;
  this->PollingCallbackCommand = NULL;
  this->CurrentAsyncState = AsyncOff;


  // Set default storage directory to ~/.vtkmap
  std::string fullPath =
    vtksys::SystemTools::CollapseFullPath(".vtkmap", "~/");
  this->SetStorageDirectory(fullPath.c_str());
}

//----------------------------------------------------------------------------
vtkMap::~vtkMap()
{
  if (this->FeatureSelector)
    {
    this->FeatureSelector->Delete();
    }
  if (this->InteractorStyle)
    {
    this->InteractorStyle->Delete();
    }
  if (this->PollingCallbackCommand)
    {
    this->PollingCallbackCommand->Delete();
    }
  if ( this->StorageDirectory )
    {
    delete[] StorageDirectory;
    }

  const std::size_t layer_size = this->Layers.size();
  for(std::size_t i=0; i < layer_size; ++i)
    { //invoke delete on each vtk class in the vector
    vtkLayer* layer = this->Layers[i];
    if(layer)
      {
      layer->Delete();
      }
    }

  if(this->BaseLayer)
    {
    this->BaseLayer->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkMap::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkMap" << std::endl
     << "Zoom Level: " << this->Zoom
     << "Center: " << this->Center[0] << " " << this->Center[1] << std::endl
     << "StorageDirectory: " << this->StorageDirectory << std::endl;

  double *camPosition = this->Renderer->GetActiveCamera()->GetPosition();
  double *focalPosition = this->Renderer->GetActiveCamera()->GetFocalPoint();
  os << "  Zoom Level: " << this->Zoom << "\n"
     << "  Center Lat/Lon: " << this->Center[1] << " "
     << this->Center[0] << "\n"
     << "  Camera Position: " << camPosition[0] << " "
     << camPosition[1] << " " << camPosition[2] << "\n"
     << "  Focal Position: " << focalPosition[0] << " "
     << focalPosition[1] << " " << focalPosition[2] << "\n"
     << std::endl;
}

//----------------------------------------------------------------------------
vtkInteractorStyle *vtkMap::GetInteractorStyle()
{
  return this->InteractorStyle;
}

//----------------------------------------------------------------------------
void vtkMap::SetVisibleBounds(double latLngCoords[4])
{
  // Convert to gcs coordinates
  double worldCoords[4];
  worldCoords[0] = latLngCoords[1];
  worldCoords[1] = vtkMercator::lat2y(latLngCoords[0]);
  worldCoords[2] = latLngCoords[3];
  worldCoords[3] = vtkMercator::lat2y(latLngCoords[2]);

  // Compute size as the larger of delta lon/lat
  double deltaLon = fabs(worldCoords[2] - worldCoords[0]);
  if (deltaLon > 180.0)
    {
    // If > 180, then points wrap around the 180th meridian
    // Adjust things to be > 0
    deltaLon = 360.0 - deltaLon;
    worldCoords[0] += worldCoords[0] < 0.0 ? 360.0 : 0;
    worldCoords[2] += worldCoords[2] < 0.0 ? 360.0 : 0;
    }
  double deltaLat = fabs(worldCoords[3] - worldCoords[1]);
  double delta = deltaLon > deltaLat ? deltaLon : deltaLat;

  // Compute zoom level
  double maxDelta = 360.0;
  double maxZoom = 20;
  int zoom = 0;
  for (zoom=0; delta < maxDelta && zoom < maxZoom; zoom++)
    {
    delta *= 2.0;
    }
  zoom--;

  // Compute center
  double center[2];
  center[0] = 0.5 * (latLngCoords[0] + latLngCoords[2]);
  if (center[0] > 180.0)
    {
    center[0] -= 360.0;
    }
  center[1] = 0.5 * (latLngCoords[1] + latLngCoords[3]);

  this->SetCenter(center);
  this->SetZoom(zoom);

  // Update camera if initialized
  if (this->Renderer)
    {
    double x = this->Center[1];
    double y = vtkMercator::lat2y(this->Center[0]);
    double distance =
      computeCameraDistance(this->Renderer->GetActiveCamera(), this->Zoom);
    this->Renderer->GetActiveCamera()->SetPosition(x, y, distance);
    this->Renderer->GetActiveCamera()->SetFocalPoint(x, y, 0.0);
    }
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

  // Convert origin to world coords
  displayCoords[0] = 0.0;
  displayCoords[1] = 0.0;
  this->ComputeWorldCoords(displayCoords, 0.0, worldCoords);
  latLngCoords[0] = vtkMercator::y2lat(worldCoords[1]);
  latLngCoords[1] = worldCoords[0];

  // Convert opposite corner to world coords
  int *sizeCoords = this->Renderer->GetRenderWindow()->GetSize();
  displayCoords[0] = sizeCoords[0];
  displayCoords[1] = sizeCoords[1];
  this->ComputeWorldCoords(displayCoords, 0.0, worldCoords);
  latLngCoords[2] = vtkMercator::y2lat(worldCoords[1]);
  latLngCoords[3] = worldCoords[0];
}

//----------------------------------------------------------------------------
void vtkMap::GetCenter(double (&latlngPoint)[2])
{
  double* center = this->Renderer->GetCenter();
  //std::cerr << "center is " << center[0] << " " << center[1] << std::endl;
  this->Renderer->SetDisplayPoint(center[0], center[1], 0.0);
  this->Renderer->DisplayToWorld();
  double* worldPoint = this->Renderer->GetWorldPoint();

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
void vtkMap::SetStorageDirectory(const char *path)
{
  if(!path)
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
  if(!vtksys::SystemTools::FileIsDirectory(fullPath.c_str()))
    {
    std::cerr << "Creating storage directory " << fullPath << std::endl;
    vtksys::SystemTools::MakeDirectory(fullPath.c_str());
    }

  // Copy path to StorageDirectory
  delete [] this->StorageDirectory;
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
                    <<" Must set map's renderer *before* adding layers.");
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
    layer->Register(this);
    }
  else
    {
    std::vector<vtkLayer*>::iterator it =
        std::find(this->Layers.begin(), this->Layers.end(), layer);
    if (it == this->Layers.end())
      {
      // TODO Use bin numbers to sort layer and its actors
      this->Layers.push_back(layer);
      layer->Register(this);
      }
    }

  layer->SetMap(this);
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
  vtkFeatureLayer *featureLayer = vtkFeatureLayer::SafeDownCast(layer);
  if (featureLayer)
    {
    vtkCollection *features = featureLayer->GetFeatures();
    for (int i=0; i<features->GetNumberOfItems(); i++)
      {
      vtkObject *item = features->GetItemAsObject(i);
      vtkFeature *feature = vtkFeature::SafeDownCast(item);
      if (feature)
        {
        this->FeatureSelector->RemoveFeature(feature);
        }
      }
    }

  this->Layers.erase(std::remove(this->Layers.begin(),
                                 this->Layers.end(), layer));
}

//----------------------------------------------------------------------------
vtkLayer *vtkMap::FindLayer(const char *name)
{
  vtkLayer *result = NULL;  // return value

  std::vector<vtkLayer*>::iterator it = this->Layers.begin();
  for (; it != this->Layers.end(); it++)
    {
    vtkLayer *layer = *it;
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
  this->SetZoom(computeZoomLevel(this->Renderer->GetActiveCamera()));

  // Update the base layer first
  this->BaseLayer->Update();

  for (size_t i = 0; i < this->Layers.size(); ++i)
    {
    this->Layers[i]->Update();
    }
}

//----------------------------------------------------------------------------
void vtkMap::Draw()
{
  if (!this->Initialized && this->Renderer)
    {
    this->Initialized = true;

    // Make sure storage directory specified
    if (!this->StorageDirectory ||
        std::strlen(this->StorageDirectory) == 0)
      {
      std::string fullPath =
        vtksys::SystemTools::CollapseFullPath(".vtkmap", "~/");
      this->SetStorageDirectory(fullPath.c_str());
      std::cerr << "Set map-tile storage directory to "
                << this->StorageDirectory << std::endl;
      }

    // Make sure storage directory specified with unix separators
    std::string strStorageDir(this->StorageDirectory);  // for convenience
    vtksys::SystemTools::ConvertToUnixSlashes(strStorageDir);
    // If trailing slash char, strip it off
    if (*strStorageDir.rbegin() == '/')
       {
       strStorageDir.erase(strStorageDir.end()-1);
       this->SetStorageDirectory(strStorageDir.c_str());
       }

    // Make sure storage directory exists
    if(!vtksys::SystemTools::FileIsDirectory(this->StorageDirectory))
      {
      std::cerr << "Create map-tile storage directory "
                << this->StorageDirectory << std::endl;
      vtksys::SystemTools::MakeDirectory(this->StorageDirectory);
      }

    // Initialize polling timer if there are any asynchronous layers
    std::vector<vtkLayer*> allLayers(this->Layers);
    allLayers.push_back(this->BaseLayer);
    for (size_t i = 0; i < allLayers.size(); ++i)
      {
      if (allLayers[i]->IsAsynchronous())
        {
        this->PollingCallbackCommand = vtkCallbackCommand::New();
        this->PollingCallbackCommand->SetClientData(this);
        this->PollingCallbackCommand->SetCallback(StaticPollingCallback);

        vtkRenderWindowInteractor *interactor
          = this->Renderer->GetRenderWindow()->GetInteractor();
        interactor->CreateRepeatingTimer(31);  // prime number > 30 fps
        interactor->AddObserver(vtkCommand::TimerEvent,
                                this->PollingCallbackCommand);

        break;
        }
      }

    // Initialize graphics
    double x = this->Center[1];
    double y = vtkMercator::lat2y(this->Center[0]);
    double distance =
      computeCameraDistance(this->Renderer->GetActiveCamera(), this->Zoom);
    this->Renderer->GetActiveCamera()->SetPosition(x, y, distance);
    this->Renderer->GetActiveCamera()->SetFocalPoint(x, y, 0.0);
    this->Renderer->GetRenderWindow()->Render();
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
void vtkMap::FeatureAdded(vtkFeature *feature)
{
  this->FeatureSelector->AddFeature(feature);
}

//----------------------------------------------------------------------------
void vtkMap::FeatureRemoved(vtkFeature *feature)
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
void vtkMap::ComputeLatLngCoords(double displayCoords[2], double elevation,
                                 double latLngCoords[3])
{
  // Compute GCS coordinates
  double worldCoords[3] = {0.0, 0.0, 0.0};
  this->ComputeWorldCoords(displayCoords, elevation, worldCoords);

  // Convert to lat-lon
  double latitude = vtkMercator::y2lat(worldCoords[1]);
  double longitude = worldCoords[0];

  // Set output
  latLngCoords[0] = latitude;
  latLngCoords[1] = longitude;
  latLngCoords[3] = elevation;
}

//----------------------------------------------------------------------------
void vtkMap::PickPoint(int displayCoords[2], vtkGeoMapSelection* result)
{
  this->FeatureSelector->PickPoint(this->Renderer, displayCoords, result);

}

//----------------------------------------------------------------------------
void vtkMap::PickArea(int displayCoords[4], vtkGeoMapSelection* result)
{
  this->FeatureSelector->PickArea(this->Renderer, displayCoords, result);
}

//----------------------------------------------------------------------------
void vtkMap::ComputeWorldCoords(double displayCoords[2], double z,
                                double worldCoords[3])
{
  // Get renderer's DisplayToWorld point
  double rendererCoords[4];
  this->Renderer->SetDisplayPoint(displayCoords[0], displayCoords[1], 0.0);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(rendererCoords);
  if (rendererCoords[3] != 0.0)
    {
    rendererCoords[0] /= rendererCoords[3];
    rendererCoords[1] /= rendererCoords[3];
    rendererCoords[2] /= rendererCoords[3];
    }

  // Get camera point
  double cameraCoords[3];
  vtkCamera *camera = this->Renderer->GetActiveCamera();
  camera->GetPosition(cameraCoords);

  // Compute line-of-sight vector from camera to renderer point
  double losVector[3];
  vtkMath::Subtract(rendererCoords, cameraCoords, losVector);

  // Set magnitude of vector's z coordinate to 1.0
  vtkMath::MultiplyScalar(losVector, fabs(1.0/losVector[2]));

  // Project line-of-sight vector from camera to specified z
  double deltaZ = cameraCoords[2] - z;
  vtkMath::MultiplyScalar(losVector, deltaZ);
  worldCoords[0] = cameraCoords[0] + losVector[0];
  worldCoords[1] = cameraCoords[1] + losVector[1];
  worldCoords[2] = z;
}

//----------------------------------------------------------------------------
void vtkMap::ComputeDisplayCoords(double latLngCoords[2], double elevation,
                                  double displayCoords[3])
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
  std::vector<vtkLayer*> allLayers(this->Layers);
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
