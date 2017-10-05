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

#include "vtkMapTile.h"
#include "vtkMercator.h"

#include <vtkObjectFactory.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtksys/SystemTools.hxx>

#include <curl/curl.h>

#include <algorithm>
#include <cstdio>  // remove()
#include <cstring> // strdup()
#include <iomanip>
#include <iterator>
#include <math.h>
#include <sstream>

vtkStandardNewMacro(vtkOsmLayer)

  //----------------------------------------------------------------------------
  struct sortTiles
{
  inline bool operator()(vtkMapTile* tile1, vtkMapTile* tile2)
  {
    return (tile1->GetBin() < tile2->GetBin());
  }
};

//----------------------------------------------------------------------------
vtkOsmLayer::vtkOsmLayer()
  : vtkFeatureLayer()
{
  this->BaseOn();
  this->MapTileServer = strdup("tile.openstreetmap.org");
  this->MapTileExtension = strdup("png");
  this->MapTileAttribution = strdup("(c) OpenStreetMap contributors");
  this->AttributionActor = NULL;
  this->CacheDirectory = NULL;
}

//----------------------------------------------------------------------------
vtkOsmLayer::~vtkOsmLayer()
{
  if (this->AttributionActor)
  {
    this->AttributionActor->Delete();
  }
  this->RemoveTiles();
  free(this->CacheDirectory);
  free(this->MapTileAttribution);
  free(this->MapTileExtension);
  free(this->MapTileServer);
}

//----------------------------------------------------------------------------
void vtkOsmLayer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkOsmLayer::SetMapTileServer(
  const char* server, const char* attribution, const char* extension)
{
  if (!this->Map)
  {
    vtkWarningMacro("Cannot set map-tile server before adding layer to vtkMap");
    return;
  }

  // Set cache directory
  // Do *not* use SystemTools::JoinPath(), because it omits the first slash
  std::string fullPath =
    this->Map->GetStorageDirectory() + std::string("/") + server;

  // Create directory if it doesn't already exist
  if (!vtksys::SystemTools::FileIsDirectory(fullPath.c_str()))
  {
    if (vtksys::SystemTools::MakeDirectory(fullPath.c_str()))
    {
      std::cerr << "Created map-tile cache directory " << fullPath << std::endl;
    }
    else
    {
      vtkErrorMacro(
        "Unable to create directory for map-tile cache: " << fullPath);
      return;
    }
  }

  // Clear tile cached and update internals
  // Remove tiles from renderer before calling RemoveTiles()
  std::vector<vtkMapTile*>::iterator iter = this->CachedTiles.begin();
  for (; iter != this->CachedTiles.end(); iter++)
  {
    vtkMapTile* tile = *iter;
    this->RemoveActor(tile->GetActor());
  }
  this->RemoveTiles();

  this->MapTileExtension = strdup(extension);
  this->MapTileServer = strdup(server);
  this->MapTileAttribution = strdup(attribution);
  this->CacheDirectory = strdup(fullPath.c_str());

  if (this->AttributionActor)
  {
    this->AttributionActor->SetInput(this->MapTileAttribution);
    this->Modified();
  }
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
    this->SetMapTileServer(
      this->MapTileServer, this->MapTileAttribution, this->MapTileExtension);
  }

  if (!this->AttributionActor && this->MapTileAttribution)
  {
    this->AttributionActor = vtkTextActor::New();
    this->AttributionActor->SetInput(this->MapTileAttribution);
    this->AttributionActor->SetDisplayPosition(10, 0);
    vtkTextProperty* textProperty = this->AttributionActor->GetTextProperty();
    textProperty->SetFontSize(12);
    textProperty->SetFontFamilyToArial();
    textProperty->SetJustificationToLeft();
    textProperty->SetColor(0, 0, 0);
    // Background properties available in vtk 6.2
    //textProperty->SetBackgroundColor(1, 1, 1);
    //textProperty->SetBackgroundOpacity(1.0);
    this->AddActor2D(this->AttributionActor);
  }

  this->AddTiles();

  this->Superclass::Update();
}

//----------------------------------------------------------------------------
void vtkOsmLayer::SetCacheSubDirectory(const char* relativePath)
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
  std::string fullPath =
    this->Map->GetStorageDirectory() + std::string("/") + relativePath;

  // Create directory if it doesn't already exist
  if (!vtksys::SystemTools::FileIsDirectory(fullPath.c_str()))
  {
    std::cerr << "Creating tile cache directory" << fullPath << std::endl;
    vtksys::SystemTools::MakeDirectory(fullPath.c_str());
  }
  this->CacheDirectory = strdup(fullPath.c_str());
}

//----------------------------------------------------------------------------
void vtkOsmLayer::RemoveTiles()
{
  this->CachedTilesMap.clear();
  std::vector<vtkMapTile*>::iterator iter = this->CachedTiles.begin();
  for (; iter != this->CachedTiles.end(); iter++)
  {
    vtkMapTile* tile = *iter;
    tile->Delete();
  }
  this->CachedTiles.clear();
}

//----------------------------------------------------------------------------
void vtkOsmLayer::AddTiles()
{
  if (!this->Renderer)
  {
    return;
  }

  std::vector<vtkMapTile*> tiles;
  std::vector<vtkMapTileSpecInternal> tileSpecs;

  this->SelectTiles(tiles, tileSpecs);
  if (tileSpecs.size() > 0)
  {
    this->InitializeTiles(tiles, tileSpecs);
  }
  this->RenderTiles(tiles);
}

//----------------------------------------------------------------------------
bool vtkOsmLayer::DownloadImageFile(std::string url, std::string filename)
{
  //std::cout << "Downloading " << filename << std::endl;
  CURL* curl;
  FILE* fp;
  CURLcode res;
  char errorBuffer[CURL_ERROR_SIZE]; // for debug
  long httpStatus = 0;               // for debug
  curl = curl_easy_init();
  if (!curl)
  {
    vtkErrorMacro(<< "curl_easy_init() failed");
    return false;
  }

  fp = fopen(filename.c_str(), "wb");
  if (!fp)
  {
    vtkErrorMacro(<< "Cannot open file " << filename.c_str());
    return false;
  }
#ifdef DISABLE_CURL_SIGNALS
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
#endif
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
  res = curl_easy_perform(curl);

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
  vtkDebugMacro("Download " << url.c_str() << " status: " << httpStatus);
  curl_easy_cleanup(curl);
  fclose(fp);

  // If there was an error, remove invalid image file
  if (res != CURLE_OK)
  {
    //std::cerr << "Removing invalid file: " << filename << std::endl;
    remove(filename.c_str());
    vtkErrorMacro(<< errorBuffer);
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
// Builds two lists based on current viewpoint:
//  * Existing tiles to render
//  * New tile-specs, representing tiles to be instantiated & initialized
void vtkOsmLayer::SelectTiles(std::vector<vtkMapTile*>& tiles,
  std::vector<vtkMapTileSpecInternal>& tileSpecs)
{
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

  if (this->Map->GetPerspectiveProjection())
  {
    bottomLeft[0] = std::max(bottomLeft[0], -180.0);
    bottomLeft[0] = std::min(bottomLeft[0], 180.0);
    bottomLeft[1] = std::max(bottomLeft[1], -180.0);
    bottomLeft[1] = std::min(bottomLeft[1], 180.0);
  }

  this->Renderer->SetDisplayPoint(
    tile_llx + width, tile_lly + height, focusDisplayPoint[2]);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(topRight);

  if (topRight[3] != 0.0)
  {
    topRight[0] /= topRight[3];
    topRight[1] /= topRight[3];
    topRight[2] /= topRight[3];
  }

  if (this->Map->GetPerspectiveProjection())
  {
    topRight[0] = std::max(topRight[0], -180.0);
    topRight[0] = std::min(topRight[0], 180.0);
    topRight[1] = std::max(topRight[1], -180.0);
    topRight[1] = std::min(topRight[1], 180.0);
  }

  int zoomLevel = this->Map->GetZoom();
  zoomLevel += this->Map->GetPerspectiveProjection() ? 1 : 0;
  int zoomLevelFactor =
    1 << zoomLevel; // Zoom levels are interpreted as powers of two.

  int tile1x = vtkMercator::long2tilex(bottomLeft[0], zoomLevel);
  int tile2x = vtkMercator::long2tilex(topRight[0], zoomLevel);

  int tile1y =
    vtkMercator::lat2tiley(vtkMercator::y2lat(bottomLeft[1]), zoomLevel);
  int tile2y =
    vtkMercator::lat2tiley(vtkMercator::y2lat(topRight[1]), zoomLevel);

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
  tile1x = std::min(zoomLevelFactor - 1, tile1x);
  tile2x = std::max(tile2x, 0);
  tile2x = std::min(zoomLevelFactor - 1, tile2x);

  tile1y = std::max(tile1y, 0);
  tile1y = std::min(zoomLevelFactor - 1, tile1y);
  tile2y = std::max(tile2y, 0);
  tile2y = std::min(zoomLevelFactor - 1, tile2y);

  int noOfTilesX = std::max(1, zoomLevelFactor);
  int noOfTilesY = std::max(1, zoomLevelFactor);

  double lonPerTile = 360.0 / noOfTilesX;
  double latPerTile = 360.0 / noOfTilesY;

  //std::cerr << "llx " << llx << " lly " << lly << " " << height << std::endl;
  //std::cerr << "tile1y " << tile1y << " " << tile2y << std::endl;

  //std::cerr << "tile1x " << tile1x << " tile2x " << tile2x << std::endl;
  //std::cerr << "tile1y " << tile1y << " tile2y " << tile2y << std::endl;

  std::ostringstream ossKey, ossImageSource;
  std::vector<vtkMapTile*> pendingTiles;
  int xIndex, yIndex;
  for (int i = tile1x; i <= tile2x; ++i)
  {
    for (int j = tile2y; j <= tile1y; ++j)
    {
      xIndex = i;
      yIndex = zoomLevelFactor - 1 - j;

      vtkMapTile* tile = this->GetCachedTile(zoomLevel, xIndex, yIndex);
      if (tile)
      {
        tiles.push_back(tile);
        tile->SetVisible(true);
      }
      else
      {
        vtkMapTileSpecInternal tileSpec;

        tileSpec.Corners[0] = -180.0 + xIndex * lonPerTile;       // llx
        tileSpec.Corners[1] = -180.0 + yIndex * latPerTile;       // lly
        tileSpec.Corners[2] = -180.0 + (xIndex + 1) * lonPerTile; // urx
        tileSpec.Corners[3] = -180.0 + (yIndex + 1) * latPerTile; // ury

        tileSpec.ZoomRowCol[0] = zoomLevel;
        tileSpec.ZoomRowCol[1] = i;
        tileSpec.ZoomRowCol[2] = zoomLevelFactor - 1 - yIndex;

        tileSpec.ZoomXY[0] = zoomLevel;
        tileSpec.ZoomXY[1] = xIndex;
        tileSpec.ZoomXY[2] = yIndex;

        tileSpecs.push_back(tileSpec);
      }
    }
  }
}

//----------------------------------------------------------------------------
// Instantiates and initializes tiles from spec objects
void vtkOsmLayer::InitializeTiles(std::vector<vtkMapTile*>& tiles,
  std::vector<vtkMapTileSpecInternal>& tileSpecs)
{
  std::stringstream oss;
  std::vector<vtkMapTileSpecInternal>::iterator tileSpecIter =
    tileSpecs.begin();
  for (; tileSpecIter != tileSpecs.end(); tileSpecIter++)
  {
    vtkMapTileSpecInternal spec = *tileSpecIter;

    vtkMapTile* tile = vtkMapTile::New();
    tile->SetLayer(this);
    tile->SetCorners(spec.Corners);

    // Set the local & remote paths
    this->MakeFileSystemPath(spec, oss);
    tile->SetFileSystemPath(oss.str());
    this->MakeUrl(spec, oss);
    tile->SetImageSource(oss.str());

    // Initialize the tile and add to the cache
    tile->Init();
    int zoom = spec.ZoomXY[0];
    int x = spec.ZoomXY[1];
    int y = spec.ZoomXY[2];
    this->AddTileToCache(zoom, x, y, tile);
    tiles.push_back(tile);
    tile->SetVisible(true);
  }
  tileSpecs.clear();
}

//----------------------------------------------------------------------------
// Updates display to incorporate all new tiles
void vtkOsmLayer::RenderTiles(std::vector<vtkMapTile*>& tiles)
{
  if (tiles.size() > 0)
  {
    // Remove old tiles
    std::vector<vtkMapTile*>::iterator itr = this->CachedTiles.begin();
    for (; itr != this->CachedTiles.end(); ++itr)
    {
      this->RemoveActor((*itr)->GetActor());
    }

    // Add new tiles
    for (std::size_t i = 0; i < tiles.size(); ++i)
    {
      this->AddActor(tiles[i]->GetActor());
    }

    tiles.clear();
  }
}

//----------------------------------------------------------------------------
void vtkOsmLayer::AddTileToCache(int zoom, int x, int y, vtkMapTile* tile)
{
  this->CachedTilesMap[zoom][x][y] = tile;
  this->CachedTiles.push_back(tile);
}

//----------------------------------------------------------------------------
vtkMapTile* vtkOsmLayer::GetCachedTile(int zoom, int x, int y)
{
  if (this->CachedTilesMap.find(zoom) == this->CachedTilesMap.end() &&
    this->CachedTilesMap[zoom].find(x) == this->CachedTilesMap[zoom].end() &&
    this->CachedTilesMap[zoom][x].find(y) ==
      this->CachedTilesMap[zoom][x].end())
  {
    return NULL;
  }

  return this->CachedTilesMap[zoom][x][y];
}

//----------------------------------------------------------------------------
void vtkOsmLayer::MakeFileSystemPath(
  vtkMapTileSpecInternal& tileSpec, std::stringstream& ss)
{
  ss.str("");
  ss << this->GetCacheDirectory() << "/" << tileSpec.ZoomRowCol[0] << "-"
     << tileSpec.ZoomRowCol[1] << "-" << tileSpec.ZoomRowCol[2] << "."
     << this->MapTileExtension;
}

//----------------------------------------------------------------------------
void vtkOsmLayer::MakeUrl(
  vtkMapTileSpecInternal& tileSpec, std::stringstream& ss)
{
  ss.str("");
  ss << "http://" << this->MapTileServer << "/" << tileSpec.ZoomRowCol[0] << "/"
     << tileSpec.ZoomRowCol[1] << "/" << tileSpec.ZoomRowCol[2] << "."
     << this->MapTileExtension;
  ;
}
