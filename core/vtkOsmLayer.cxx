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

#include "tileNotAvailable_png.h"
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
  vtkOsmLayer::vtkOsmLayer()
  : vtkFeatureLayer()
{
  this->BaseOn();
  this->MapTileServer = strdup("tile.openstreetmap.org");
  this->MapTileExtension = strdup("png");
  this->MapTileAttribution = strdup("(c) OpenStreetMap contributors");
  this->TileNotAvailableImagePath = NULL;
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
void vtkOsmLayer::SetMapTileServer(const char* server,
  const char* attribution,
  const char* extension)
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
  auto iter = this->CachedTiles.begin();
  for (; iter != this->CachedTiles.end(); iter++)
  {
    this->RemoveActor(iter->GetPointer()->GetActor());
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

  // Write the "tile not available" image to the cache directory
  if (!this->TileNotAvailableImagePath)
  {
    std::stringstream ss;
    ss << this->CacheDirectory << "/"
       << "tile-not-available.png";
    this->TileNotAvailableImagePath = strdup(ss.str().c_str());
  }

  if (!vtkOsmLayer::VerifyImageFile(nullptr, TileNotAvailableImagePath))
  {
    FILE* fp = fopen(this->TileNotAvailableImagePath, "wb");
    fwrite(tileNotAvailable_png, 1, tileNotAvailable_png_len, fp);
    fclose(fp);
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

  this->Superclass::Update(); // redundant isn't it ????
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
  this->CachedTiles.clear();
}

//----------------------------------------------------------------------------
void vtkOsmLayer::AddTiles()
{
  if (!this->Renderer)
  {
    return;
  }

  std::vector<vtkSmartPointer<vtkMapTile> > tiles;
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

  // Confirm that the file is a valid image
  if ((res == CURLE_OK) && (!this->VerifyImageFile(fp, filename)))
  {
    res = CURLE_READ_ERROR;
    sprintf(errorBuffer, "map tile contents not a valid image");
  }

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
bool vtkOsmLayer::VerifyImageFile(FILE* fp, std::string filename)
{
  // Confirms that the file is the expected image type.
  // This method is needed because some tile servers return
  // a success status code (200) when in fact returning an html
  // page stating that the image isn't available.
  // This method verifies that the specified file contains image data.

  fp = fopen(filename.c_str(), "rb");
  if (!fp)
  {
    vtkErrorMacro(<< "Could not open file " << filename << " to verify image");
    return false;
  }

  // Current logic supports png and jpeg files.
  // Uses the magic numbers associated with those file types, as listed in
  // https://en.wikipedia.org/wiki/Magic_number_(programming).
  std::string ext = vtksys::SystemTools::GetFilenameLastExtension(filename);

  bool match = false;
  if (ext == ".png")
  {
    match = true;
    unsigned char buffer[8];
    unsigned char pngSignature[] = {
      0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'
    };

    fseek(fp, 0, SEEK_SET);
    std::size_t n = fread(buffer, 1, 8, fp);
    if (n != 8)
    {
      std::cout << "Error, read " << n << " bytes" << std::endl;
    }
    for (int i = 0; i < 8; ++i)
    {
      match &= buffer[i] == pngSignature[i];
#ifndef NDEBUG
      if (buffer[i] != pngSignature[i])
      {
        std::cout << i << ": " << buffer[i] << ", " << pngSignature[i]
                  << std::endl;
      }
#endif
    }
  }
  else if ((ext == ".jpg") || (ext == ".jpeg"))
  {
    match = true;
    unsigned char buffer[2];

    fseek(fp, 0, SEEK_SET);
    fread(buffer, 1, 2, fp);
    match &= buffer[0] == 0xff;
    match &= buffer[1] == 0xd8;

    fseek(fp, -2, SEEK_END);
    fread(buffer, 1, 2, fp);
    match &= buffer[0] == 0xff;
    match &= buffer[1] == 0xd9;
  }

  fclose(fp);
  return match;
}

//----------------------------------------------------------------------------
// Builds two lists based on current viewpoint:
//  * Existing tiles to render
//  * New tile-specs, representing tiles to be instantiated & initialized
void vtkOsmLayer::SelectTiles(std::vector<vtkSmartPointer<vtkMapTile> >& tiles,
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
        tile->VisibilityOn();
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
void vtkOsmLayer::InitializeTiles(
  std::vector<vtkSmartPointer<vtkMapTile> >& tiles,
  std::vector<vtkMapTileSpecInternal>& tileSpecs)
{
  std::stringstream oss;
  std::vector<vtkMapTileSpecInternal>::iterator tileSpecIter =
    tileSpecs.begin();
  std::string filename;
  std::string url;
  for (; tileSpecIter != tileSpecs.end(); tileSpecIter++)
  {
    vtkMapTileSpecInternal spec = *tileSpecIter;

    this->MakeFileSystemPath(spec, oss);
    filename = oss.str();
    this->MakeUrl(spec, oss);
    url = oss.str();

    // Instantiate tile
    vtkSmartPointer<vtkMapTile> tile = vtkSmartPointer<vtkMapTile>::New();
    tile->SetLayer(this);
    tile->SetCorners(spec.Corners);
    tile->SetFileSystemPath(filename);
    tile->SetImageSource(url);
    tiles.push_back(tile);

    // Download image file if needed
    if (!vtksys::SystemTools::FileExists(filename.c_str(), true))
    {
      std::cout << "Downloading " << url << " to " << filename << std::endl;
      if (this->DownloadImageFile(url, filename))
      {
        // Update tile cache
        this->AddTileToCache(
          spec.ZoomXY[0], spec.ZoomXY[1], spec.ZoomXY[2], tile);
      }
      else
      {
        tile->SetFileSystemPath(this->TileNotAvailableImagePath);
      }
    }
    else
    {
      // This is potentially the case when the tile was downloaded in a previous
      // execution of a program using vtkMap and vtkOsmLayer.
      // Update tile cache :
      this->AddTileToCache(
        spec.ZoomXY[0], spec.ZoomXY[1], spec.ZoomXY[2], tile);
    }

    // Initialize tile
    tile->VisibilityOn();
    tile->Init();
  } // for

  //tileSpecs.clear(); // it's not this method job to clear it :)
}

//----------------------------------------------------------------------------
// Updates display to incorporate all new tiles
void vtkOsmLayer::RenderTiles(std::vector<vtkSmartPointer<vtkMapTile> >& tiles)
{
  if (tiles.size() > 0)
  {
    // Remove old tiles
    auto itr = this->CachedTiles.begin();
    for (; itr != this->CachedTiles.end(); ++itr)
    {
      this->RemoveActor(itr->GetPointer()->GetActor());
    }

    // clear the last rendered tiles cache
    CachedTiles.clear();

    // Add new tiles
    for (std::size_t i = 0; i < tiles.size(); ++i)
    {
      this->AddActor(tiles[i]->GetActor());

      // add tiles put on the scene in the proper cache
      CachedTiles.push_back(tiles[i]);
    }

    //tiles.clear(); // it's not this method job to clear it :)
  }
}

//----------------------------------------------------------------------------
void vtkOsmLayer::AddTileToCache(int zoom, int x, int y, vtkMapTile* tile)
{
  this->CachedTilesMap[zoom][x][y] = tile;
  // don't add tiles to CachedTiles here ! as in RenderTiles, CachedTiles will
  // contain old AND new tiles added via AddTileToCache in InitializeTiles,
  // and will be used to remove the last tiles rendered before rendering the new ones.
  //this->CachedTiles.push_back(tile);
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkMapTile> vtkOsmLayer::GetCachedTile(int zoom, int x, int y)
{
  if (this->CachedTilesMap.find(zoom) == this->CachedTilesMap.end() ||
    this->CachedTilesMap[zoom].find(x) == this->CachedTilesMap[zoom].end() ||
    this->CachedTilesMap[zoom][x].find(y) ==
      this->CachedTilesMap[zoom][x].end())
  {
    return nullptr;
  }

  return this->CachedTilesMap[zoom][x][y];
}

//----------------------------------------------------------------------------
void vtkOsmLayer::MakeFileSystemPath(vtkMapTileSpecInternal& tileSpec,
  std::stringstream& ss)
{
  ss.str("");
  ss << this->GetCacheDirectory() << "/" << tileSpec.ZoomRowCol[0] << "-"
     << tileSpec.ZoomRowCol[1] << "-" << tileSpec.ZoomRowCol[2] << "."
     << this->MapTileExtension;
}

//----------------------------------------------------------------------------
void vtkOsmLayer::MakeUrl(vtkMapTileSpecInternal& tileSpec,
  std::stringstream& ss)
{
  ss.str("");
  ss << "http://" << this->MapTileServer << "/" << tileSpec.ZoomRowCol[0] << "/"
     << tileSpec.ZoomRowCol[1] << "/" << tileSpec.ZoomRowCol[2] << "."
     << this->MapTileExtension;
}
