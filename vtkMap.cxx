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

#include "vtkInteractorStyleMap.h"
#include "vtkLayer.h"
#include "vtkMapMarkerSet.h"
#include "vtkMapTile.h"
#include "vtkMercator.h"

// VTK Includes
#include <vtkActor2D.h>
#include <vtkImageInPlaceFilter.h>
#include <vtkObjectFactory.h>
#include <vtkPointPicker.h>
#include <vtkPoints.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkCamera.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkPoints.h>
#include <vtkPlaneSource.h>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <iomanip>
#include <iterator>
#include <math.h>
#include <sstream>

vtkStandardNewMacro(vtkMap)

//----------------------------------------------------------------------------
double computeCameraDistance(vtkCamera* cam, int zoomLevel)
{
  double deg = 360.0 / std::pow(2, zoomLevel);
  return (deg / std::sin(vtkMath::RadiansFromDegrees(cam->GetViewAngle())));
}

//----------------------------------------------------------------------------
int computeZoomLevel(vtkCamera* cam)
{
  int i;
  double* pos = cam->GetPosition();
  double width = pos[2] * sin(vtkMath::RadiansFromDegrees(cam->GetViewAngle()));

  for (i = 0; i < 20; i += 1) {
    if (width >= (360.0 / pow(2, i))) {
      /// We are forcing the minimum zoom level to 2 so that we can get
      /// high res imagery even at the zoom level 0 distance
      return i;
    }
  }
}

//----------------------------------------------------------------------------
vtkMap::vtkMap()
{
  this->StorageDirectory = NULL;
  this->Renderer = NULL;
  this->InteractorStyle = vtkInteractorStyleMap::New();
  this->InteractorStyle->SetMap(this);
  //this->InteractorStyle->DebugOn();
  this->Picker = vtkPointPicker::New();
  this->Zoom = 1;
  this->Center[0] = this->Center[1] = 0.0;
  this->MapMarkerSet = vtkMapMarkerSet::New();
  this->Initialized = false;
  this->BaseLayer = NULL;
}

//----------------------------------------------------------------------------
vtkMap::~vtkMap()
{
  if (this->InteractorStyle)
    {
    this->InteractorStyle->Delete();
    }
  if (this->MapMarkerSet)
    {
    this->MapMarkerSet->Delete();
    }
  if (this->Picker)
    {
    this->Picker->Delete();
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
     << "  Center Lat/Lon: " << vtkMercator::y2lat(this->Center[0]) << " "
     << this->Center[1] << "\n"
     << "  Camera Lat/Lon/Z: " << vtkMercator::y2lat(camPosition[1]) << " "
     << camPosition[0] << " " << camPosition[2] << "\n"
     << "  Focal Lat/Lon/Z: " << vtkMercator::y2lat(focalPosition[1]) << " "
     << focalPosition[0] << " " << focalPosition[2] << "\n"
     << std::endl;
}

//----------------------------------------------------------------------------
vtkInteractorStyle *vtkMap::GetInteractorStyle()
{
  return this->InteractorStyle;
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
  size_t n = fullPath.size();
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
    }
  else
    {
    std::vector<vtkLayer*>::iterator it =
        std::find(this->Layers.begin(), this->Layers.end(), layer);
    if (it == this->Layers.end())
      {
      // TODO Use bin numbers to sort layer and its actors
      this->Layers.push_back(layer);
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

  this->Layers.erase(std::remove(this->Layers.begin(),
                                 this->Layers.end(), layer));
}

//----------------------------------------------------------------------------
vtkLayer *vtkMap::FindLayer(char *name)
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

  this->MapMarkerSet->Update(this->Zoom);
}

//----------------------------------------------------------------------------
void vtkMap::Draw()
{
  if (!this->Initialized && this->Renderer)
    {
    this->Initialized = true;
    this->MapMarkerSet->SetRenderer(this->Renderer);

    // Make sure storage directory specified
    if (!this->StorageDirectory || "" == this->StorageDirectory)
      {
      // Use vtksys::SplitPath() to expand home directory
      std::vector<std::string> storagePath;
      vtksys::SystemTools::SplitPath("~/", storagePath);
      storagePath.push_back(".vtkmap");
      std::string fullPath = vtksys::SystemTools::JoinPath(storagePath);
      this->SetStorageDirectory(fullPath.c_str());
      std::cerr << "Set storage directory to " << this->StorageDirectory << std::endl;
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
      std::cerr << "Create tile storage directory " << this->StorageDirectory
                << std::endl;
      vtksys::SystemTools::MakeDirectory(this->StorageDirectory);
      }

    // Initialize graphics
    this->Center[0] = vtkMercator::lat2y(this->Center[0]);
    this->Renderer->GetActiveCamera()->SetPosition(
      this->Center[1],
      this->Center[0],
      computeCameraDistance(this->Renderer->GetActiveCamera(), this->Zoom));
    this->Renderer->GetActiveCamera()->SetFocalPoint(this->Center[1],
                                                     this->Center[0],
                                                     0.0);
    this->Renderer->GetRenderWindow()->Render();
    }
  this->Update();
  this->Renderer->GetRenderWindow()->Render();
}

//----------------------------------------------------------------------------
double vtkMap::Clip(double n, double minValue, double maxValue)
{
  double max = n > minValue ? n : minValue;
  double min = max > maxValue ? maxValue : max;
  return min;
}

//----------------------------------------------------------------------------
void vtkMap::PickPoint(int displayCoords[2], vtkMapPickResult* result)
{
  this->MapMarkerSet->PickPoint(this->Renderer, this->Picker, displayCoords,
      result);
}

//----------------------------------------------------------------------------
vtkPoints* vtkMap::gcsToDisplay(vtkPoints* points, std::string srcProjection)
{
  if (!srcProjection.empty())
   {
   vtkErrorMacro("Does not handle projections other than latlon");
   }
  int noOfPoints = static_cast<int>(points->GetNumberOfPoints());
  double inPoint[3];
  double outPoint[3];
  vtkPoints* newPoints = vtkPoints::New(VTK_DOUBLE);
  newPoints->SetNumberOfPoints(noOfPoints);
  double latitude, longitude;
  double x, y;
  for (int i = 0; i < noOfPoints; ++i)
    {
    points->GetPoint(i, inPoint);
    latitude = inPoint[0];
    longitude = inPoint[1];
    x = longitude;
    y = vtkMercator::lat2y(latitude);

    inPoint[0] = vtkMercator::lat2y(inPoint[0]);
    //this->Renderer->SetWorldPoint(inPoint[1], inPoint[0], inPoint[2], 1.0);
    this->Renderer->SetWorldPoint(x, y, inPoint[2], 1.0);
    this->Renderer->WorldToDisplay();
    this->Renderer->GetDisplayPoint(outPoint);
    newPoints->SetPoint(i, outPoint);
    }

  return newPoints;
}

//----------------------------------------------------------------------------
vtkPoints* vtkMap::displayToGcs(vtkPoints* points)
{
  double inPoint[3];
  double outPoint[3];
  int noOfPoints = static_cast<int>(points->GetNumberOfPoints());
  vtkPoints* newPoints = vtkPoints::New(VTK_DOUBLE);
  newPoints->SetNumberOfPoints(noOfPoints);
  double wCoords[4];
  for (int i = 0; i < noOfPoints; ++i)
    {
    points->GetPoint(i, inPoint);
    this->Renderer->SetDisplayPoint(inPoint[0], inPoint[1], inPoint[2]);
    this->Renderer->DisplayToWorld();
    this->Renderer->GetWorldPoint(wCoords);

    if (wCoords[3] != 0.0)
      {
      wCoords[0] /= wCoords[3];
      wCoords[1] /= wCoords[3];
      wCoords[2] /= wCoords[3];
      wCoords[3] = 1.0;
      }

    double latitude = vtkMercator::y2lat(wCoords[1]);
    double longitude = wCoords[0];

    outPoint[0] = latitude;
    outPoint[1] = longitude;
    outPoint[2] = 0.0;

    newPoints->SetPoint(i, outPoint);
    }

  return newPoints;
}
