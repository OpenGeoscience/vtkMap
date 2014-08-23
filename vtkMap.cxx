/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMap.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMap.h"
#include "vtkMapTile.h"

// VTK Includes
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkCamera.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkPlaneSource.h>

#include <sstream>
#include <math.h>

vtkStandardNewMacro(vtkMap)

//----------------------------------------------------------------------------
int long2tilex(double lon, int z)
{
  return (int)(floor((lon + 180.0) / 360.0 * pow(2.0, z)));
}

//----------------------------------------------------------------------------
int lat2tiley(double lat, int z)
{
  return (int)(floor((1.0 - log( tan(lat * M_PI/180.0) + 1.0 /
    cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z)));
}

//----------------------------------------------------------------------------
double tilex2long(int x, int z)
{
  return x / pow(2.0, z) * 360.0 - 180;
}

//----------------------------------------------------------------------------
double tiley2lat(int y, int z)
{
  double n = M_PI - 2.0 * M_PI * y / pow(2.0, z);
  return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}

//----------------------------------------------------------------------------
vtkMap::vtkMap()
{
  this->Zoom = 1;
  this->Center[0] = this->Center[1] = 0.0;
  this->Initialized = false;
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
vtkMap::~vtkMap()
{
}

//----------------------------------------------------------------------------
void vtkMap::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << "vtkMap" << std::endl
     << "Zoom Level: " << this->Zoom
     << "Center: " << this->Center[0] << " " << this->Center[1] << std::endl;
}

//----------------------------------------------------------------------------
void vtkMap::Update()
{
  /// Compute the zoom level here
  this->SetZoom(computeZoomLevel(this->Renderer->GetActiveCamera()));

  std::cerr << "Zoom is " << this->Zoom << std::endl;

  RemoveTiles();
  AddTiles();
}

//----------------------------------------------------------------------------
void vtkMap::Draw()
{
  if (!this->Initialized && this->Renderer)
    {
    this->Initialized = true;
    this->Renderer->GetActiveCamera()->SetPosition(this->Center[0],
                                                   this->Center[1],
                                                   10.0);
    }
  this->Update();
  this->Renderer->GetRenderWindow()->Render();
}

//----------------------------------------------------------------------------
void vtkMap::RemoveTiles()
{
  // To do.
}

//----------------------------------------------------------------------------
void vtkMap::AddTiles()
{
  // Obtain Window Size
  double xmax = Renderer->GetSize()[0];
  double ymax = Renderer->GetSize()[1];

  double bottomLeft[3], topRight[3];

  // Obtain world coordinates of viewport corner points
  int width, height, llx, lly;
  this->Renderer->GetTiledSizeAndOrigin(&width, &height, &llx, &lly);

  this->Renderer->SetDisplayPoint(llx, lly, 0.0);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(bottomLeft);
  vtkWarningMacro( << "Bottom Left Display: " << bottomLeft[0] << " " << bottomLeft[1] << " " << bottomLeft[2]);

  this->Renderer->SetDisplayPoint(llx + width, lly + height, 0.0);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(topRight);

  int tile1x = long2tilex(bottomLeft[0], this->Zoom);
  int tile1y = lat2tiley(bottomLeft[1], this->Zoom);
  int tile2x = long2tilex(topRight[0], this->Zoom);
  int tile2y = lat2tiley(topRight[1], this->Zoom);

  int noOfTilesX = pow(2, this->Zoom);
  int noOfTilesY = pow(2, this->Zoom);
  double lonPerTile = 360.0 / noOfTilesX;
  double latPerTile = 360.0 / noOfTilesY;

  int xIndex, yIndex;
  for (int i = tile1x; i <= tile2x; ++i)
    {
    for (int j = tile2y; j <= tile1y; ++j)
      {
      xIndex = i;
      yIndex = pow(2, this->Zoom) - 1 - j;

      vtkMapTile* tile = this->GetCachedTile(this->Zoom, xIndex, yIndex);
      if (!tile)
        {
        std::cerr << "Tile is not cached " << this->Zoom << " " << xIndex << " " << yIndex << std::endl;
        tile = vtkMapTile::New();
        double llx = -180.0 + xIndex * lonPerTile;
        double lly = -180.0 + yIndex * latPerTile;
        double urx = -180.0 + (xIndex + 1) * lonPerTile;
        double ury = -180.0 + (yIndex + 1) * latPerTile;

        tile->SetCorners(llx, lly, urx, ury);

        std::ostringstream oss;
        oss << this->Zoom;
        std::string zoom = oss.str();
        oss.str("");

        oss << i;
        std::string row = oss.str();
        oss.str("");

        oss << (pow(2, this->Zoom) - 1 - yIndex);
        std::string col = oss.str();
        oss.str("");

        // Set tile texture source
        oss << zoom << row << col;
        tile->SetImageKey(oss.str());
        tile->SetImageSource("http://tile.openstreetmap.org/" + zoom + "/" + row +
                             "/" + col + ".png");
        tile->Init();
        tile->SetVisible(true);

        this->AddTileToCache(this->Zoom, xIndex, yIndex, tile);

        // Add tile to the renderer
        Renderer->AddActor(tile->GetActor());
      }
     else
      {
      std::cerr << "Tile is cached" << std::endl;
      tile->SetVisible(true);
      }
    }
  }
}

//----------------------------------------------------------------------------
double vtkMap::Clip(double n, double minValue, double maxValue)
{
  double max = n > minValue ? n : minValue;
  double min = max > maxValue ? maxValue : max;
  return min;
}

//----------------------------------------------------------------------------
void vtkMap::LatLongToPixelXY(double latitude, double longitude,
                              int levelOfDetail, int &pixelX, int &pixelY)
{
  latitude = Clip(latitude, MinLatitude, MaxLatitude);
  longitude = Clip(longitude, MinLongitude, MaxLongitude);

  double x = (longitude + 180) / 360;
  double sinLatitude = sin(latitude * 3.142 / 180);
  double y = 0.5 - log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * 3.142);

  uint mapSize = MapSize(levelOfDetail);
  pixelX = (int) Clip(x * mapSize + 0.5, 0, mapSize - 1);
  pixelY = (int) Clip(y * mapSize + 0.5, 0, mapSize - 1);
}

//----------------------------------------------------------------------------
uint vtkMap::MapSize(int levelOfDetail)
{
  return (uint) 256 << levelOfDetail;
}

//----------------------------------------------------------------------------
void vtkMap::AddTileToCache(int zoom, int x, int y, vtkMapTile* tile)
{
  this->CachedTiles[zoom][x][y] = tile;
}

//----------------------------------------------------------------------------
vtkMapTile *vtkMap::GetCachedTile(int zoom, int x, int y)
{
  if (this->CachedTiles.find(zoom) == this->CachedTiles.end() &&
      this->CachedTiles[zoom].find(x) == this->CachedTiles[zoom].end() &&
      this->CachedTiles[zoom][x].find(y) == this->CachedTiles[zoom][x].end())
    {
    return NULL;
    }

  return this->CachedTiles[zoom][x][y];
}

//----------------------------------------------------------------------------
void vtkMap::PixelXYToTileXY(int pixelX, int pixelY, int &tileX, int &tileY)
{
  tileX = pixelX / 256;
  tileY = pixelY / 256;
}

//----------------------------------------------------------------------------
std::string vtkMap::TileXYToQuadKey(int tileX, int tileY, int levelOfDetail)
{
  std::stringstream quadKey;
  for (int i = levelOfDetail; i > 0; i--)
    {
    char digit = '0';
    int mask = 1 << (i - 1);
    if ( (tileX & mask) != 0 )
      {
      digit++;
      }
    if ( (tileY & mask) != 0 )
      {
      digit++;
      digit++;
      }
    quadKey << digit;
    }
  return quadKey.str();
}
