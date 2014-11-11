/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOsmLayer.h"

#include "vtkMercator.h"
#include "vtkMapTile.h"

#include <vtkObjectFactory.h>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <iomanip>
#include <iterator>
#include <math.h>
#include <sstream>

vtkStandardNewMacro(vtkOsmLayer)

//----------------------------------------------------------------------------
struct sortTiles
{
  inline bool operator() (vtkMapTile* tile1,  vtkMapTile* tile2)
  {
    return (tile1->GetBin() < tile2->GetBin());
  }
};

//----------------------------------------------------------------------------
vtkOsmLayer::vtkOsmLayer() : vtkFeatureLayer()
{
  this->BaseOn();
  this->CacheDirectory = NULL;
}

//----------------------------------------------------------------------------
vtkOsmLayer::~vtkOsmLayer()
{
 //The entries in the CachedTilesMap and also the CachedTiles are also
 //part of a vector held by our derived parent. The Caches are just for quick
 //lookup and the pointers inside of them don't need to be deleted, as our
 //derived parent owns them

  this->CachedTiles.clear();
  this->CachedTilesMap.clear();
}

//----------------------------------------------------------------------------
void vtkOsmLayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOsmLayer::SetCacheSubDirectory(const char *relativePath)
{
  if (!this->Map)
    {
    vtkWarningMacro("Cannot set cache directory before adding layer to vtkMap");
    return;
    }

  if (vtksys::SystemTools::FileIsFullPath(relativePath))
    {
    vtkWarningMacro("Cannot set cache direcotry to relative path");
    return;
    }

  // Do *not* use SystemTools::JoinPath(), because it omits the first slash
  std::string fullPath = this->Map->GetStorageDirectory() + std::string("/")
    + relativePath;

  // Create directory if it doesn't already exist
  if(!vtksys::SystemTools::FileIsDirectory(fullPath.c_str()))
    {
    std::cerr << "Creating osm tile cache " << fullPath << std::endl;
    vtksys::SystemTools::MakeDirectory(fullPath.c_str());
    }
  this->SetCacheDirectory(fullPath.c_str());
}

//----------------------------------------------------------------------------
void vtkOsmLayer::Update()
{
  if (!this->Map)
    {
    return;
    }

  if (!this->CacheDirectory)
    {
    // Note this calls the public "Sub" directory method
    this->SetCacheSubDirectory("osm");
    }

  this->AddTiles();

  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkOsmLayer::RemoveTiles()
{
  // TODO
  if (!this->Renderer)
    {
    return;
    }
}

//----------------------------------------------------------------------------
void vtkOsmLayer::AddTiles()
{
  if (!this->Renderer)
    {
    return;
    }

  double focusDisplayPoint[3], bottomLeft[4], topRight[4];
  int width, height, tile_llx, tile_lly;

  this->Renderer->SetWorldPoint(0.0, 0.0, 0.0, 1.0);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(focusDisplayPoint);

  this->Renderer->GetTiledSizeAndOrigin(&width, &height, &tile_llx, &tile_lly);
  this->Renderer->SetDisplayPoint(tile_llx, tile_lly, focusDisplayPoint[2]);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(bottomLeft);

  if (bottomLeft[3] != 0.0)
    {
    bottomLeft[0] /= bottomLeft[3];
    bottomLeft[1] /= bottomLeft[3];
    bottomLeft[2] /= bottomLeft[3];
    }

  //std::cerr << "Before bottomLeft " << bottomLeft[0] << " " << bottomLeft[1] << std::endl;

  bottomLeft[0] = std::max(bottomLeft[0], -180.0);
  bottomLeft[0] = std::min(bottomLeft[0],  180.0);
  bottomLeft[1] = std::max(bottomLeft[1], -180.0);
  bottomLeft[1] = std::min(bottomLeft[1],  180.0);

  this->Renderer->SetDisplayPoint(tile_llx + width,
                                  tile_lly + height,
                                  focusDisplayPoint[2]);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(topRight);

  if (topRight[3] != 0.0)
    {
    topRight[0] /= topRight[3];
    topRight[1] /= topRight[3];
    topRight[2] /= topRight[3];
    }

  topRight[0] = std::max(topRight[0], -180.0);
  topRight[0] = std::min(topRight[0],  180.0);
  topRight[1] = std::max(topRight[1], -180.0);
  topRight[1] = std::min(topRight[1],  180.0);

  int zoomLevel = this->Map->GetZoom() + 1;

  int tile1x = vtkMercator::long2tilex(bottomLeft[0], zoomLevel);
  int tile2x = vtkMercator::long2tilex(topRight[0], zoomLevel);

  int tile1y = vtkMercator::lat2tiley(
                 vtkMercator::y2lat(bottomLeft[1]), zoomLevel);
  int tile2y = vtkMercator::lat2tiley(
                 vtkMercator::y2lat(topRight[1]), zoomLevel);

  //std::cerr << "tile1y " << tile1y << " " << tile2y << std::endl;

  if (tile2y > tile1y)
    {
    int temp = tile1y;
    tile1y = tile2y;
    tile2y = temp;
    }

  //std::cerr << "Before bottomLeft " << bottomLeft[0] << " " << bottomLeft[1] << std::endl;
  //std::cerr << "Before topRight " << topRight[0] << " " << topRight[1] << std::endl;

  /// Clamp tilex and tiley
  tile1x = std::max(tile1x, 0);
  tile1x = std::min(static_cast<int>(std::pow(2.0, zoomLevel)) - 1, tile1x);
  tile2x = std::max(tile2x, 0);
  tile2x = std::min(static_cast<int>(std::pow(2.0, zoomLevel)) - 1, tile2x);

  tile1y = std::max(tile1y, 0);
  tile1y = std::min(static_cast<int>(std::pow(2.0, zoomLevel)) - 1, tile1y);
  tile2y = std::max(tile2y, 0);
  tile2y = std::min(static_cast<int>(std::pow(2.0, zoomLevel)) - 1, tile2y);

  int noOfTilesX = std::max(1, static_cast<int>(std::pow(2.0, zoomLevel)));
  int noOfTilesY = std::max(1, static_cast<int>(std::pow(2.0, zoomLevel)));

  double lonPerTile = 360.0 / noOfTilesX;
  double latPerTile = 360.0 / noOfTilesY;

  //std::cerr << "llx " << llx << " lly " << lly << " " << height << std::endl;
  //std::cerr << "tile1y " << tile1y << " " << tile2y << std::endl;

  //std::cerr << "tile1x " << tile1x << " tile2x " << tile2x << std::endl;
  //std::cerr << "tile1y " << tile1y << " tile2y " << tile2y << std::endl;

  std::vector<vtkMapTile*> pendingTiles;
  int xIndex, yIndex;
  for (int i = tile1x; i <= tile2x; ++i)
    {
    for (int j = tile2y; j <= tile1y; ++j)
      {
      xIndex = i;
      yIndex = static_cast<int>(std::pow(2.0, zoomLevel)) - 1 - j;

      vtkMapTile* tile = this->GetCachedTile(zoomLevel, xIndex, yIndex);
      if (!tile)
        {
        tile = vtkMapTile::New();
        const double llx = -180.0 + xIndex * lonPerTile;
        const double lly = -180.0 + yIndex * latPerTile;
        const double urx = -180.0 + (xIndex + 1) * lonPerTile;
        const double ury = -180.0 + (yIndex + 1) * latPerTile;

        tile->SetCorners(llx, lly, urx, ury);

        std::ostringstream oss;
        oss << zoomLevel;
        std::string zoom = oss.str();
        oss.str("");

        oss << i;
        std::string row = oss.str();
        oss.str("");

        oss << (static_cast<int>(std::pow(2.0, zoomLevel)) - 1 - yIndex);
        std::string col = oss.str();
        oss.str("");

        // Set tile texture source
        oss << zoom << row << col;
        tile->SetImageKey(oss.str());
        tile->SetImageSource("http://tile.openstreetmap.org/" + zoom + "/" + row +
                             "/" + col + ".png");
        this->AddTileToCache(zoomLevel, xIndex, yIndex, tile);
      }
    pendingTiles.push_back(tile);
    tile->SetVisible(true);
    }
  }

  if (pendingTiles.size() > 0)
    {
    // Remove the old tiles first
    std::vector<vtkMapTile*>::iterator itr = this->CachedTiles.begin();
    for (; itr != this->CachedTiles.end(); ++itr)
      {
      this->Renderer->RemoveActor((*itr)->GetActor());
      }

    vtkPropCollection* props = this->Renderer->GetViewProps();

    props->InitTraversal();
    vtkProp* prop = props->GetNextProp();
    std::vector<vtkProp*> otherProps;
    while (prop)
      {
      otherProps.push_back(prop);
      prop = props->GetNextProp();
      }

    this->Renderer->RemoveAllViewProps();

    std::sort(pendingTiles.begin(),
              pendingTiles.end(),
              sortTiles());

    for (int i = 0; i < pendingTiles.size(); ++i)
      {
      // Add tile to the renderer
      this->AddFeature(pendingTiles[i]);
      }

    std::vector<vtkProp*>::iterator itr2 = otherProps.begin();
    while (itr2 != otherProps.end())
      {
      this->Renderer->AddViewProp(*itr2);
      ++itr2;
      }
    }
}

//----------------------------------------------------------------------------
void vtkOsmLayer::AddTileToCache(int zoom, int x, int y, vtkMapTile* tile)
{
  this->CachedTilesMap[zoom][x][y] = tile;
  this->CachedTiles.push_back(tile);
}

//----------------------------------------------------------------------------
vtkMapTile *vtkOsmLayer::GetCachedTile(int zoom, int x, int y)
{
  if (this->CachedTilesMap.find(zoom) == this->CachedTilesMap.end() &&
      this->CachedTilesMap[zoom].find(x) == this->CachedTilesMap[zoom].end() &&
      this->CachedTilesMap[zoom][x].find(y) == this->CachedTilesMap[zoom][x].end())
    {
    return NULL;
    }

  return this->CachedTilesMap[zoom][x][y];
}
