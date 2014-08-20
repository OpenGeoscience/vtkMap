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
#include <vtkMatrix4x4.h>
#include <vtkPlaneSource.h>

#include <sstream>

vtkStandardNewMacro(vtkMap)

//----------------------------------------------------------------------------
vtkMap::vtkMap()
{
  this->Zoom = 1;
  this->Center[0] = this->Center[1] = 0.0;
  this->Initialized = false;
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
                                                   100.0);
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

  double bottomLeft[2], topRight[2];

  // Obtain world coordinates of viewport corner points
  Renderer->SetWorldPoint(0, 0, 0, 0);
  Renderer->WorldToDisplay();
  Renderer->GetDisplayPoint(bottomLeft);
  vtkWarningMacro( << "Bottom Left Display: " << bottomLeft[0] << " " << bottomLeft[1] << " " << bottomLeft[2]);

  Renderer->SetWorldPoint(1, 1, 0, 0);
  Renderer->WorldToDisplay();
  Renderer->GetDisplayPoint(topRight);
  vtkDebugMacro( << "Top Right Display: " << topRight[0] << " " << topRight[1] << " " << topRight[2]);

  // Obtain window size in terms of world coordinates
  int height = topRight[0] - bottomLeft[0];
  int width = topRight[1] - bottomLeft[1];

  // Obtain dimensions of tile grid
  int ROWS = ymax / height + 1;
  ROWS = ROWS < (2 << (this->Zoom - 1)) ? ROWS : (2 << (this->Zoom - 1));

  int COLS = xmax / width + 1;
  COLS = COLS < (2 << (this->Zoom - 1)) ? COLS : (2 << (this->Zoom - 1));

  // Convert latitude longitude to Grid Coordinates
  int pixX, pixY, tileX, tileY;
  LatLongToPixelXY(this->Center[1], this->Center[0], this->Zoom, pixX, pixY);
  PixelXYToTileXY(pixX, pixY, tileX, tileY);

  int temp_tileX = tileX - ROWS / 2;
  int temp_tileY = tileY - ROWS / 2 > 0 ? tileY - ROWS / 2 : 0;

  for (int i = -ROWS / 2; i < ROWS / 2; i++)
    {
    temp_tileX = tileX - ROWS / 2 > 0 ? tileX - ROWS / 2 : 0;
    for (int j = -COLS / 2; j < COLS / 2; j++)
      {
      // Create a tile
      vtkMapTile *tile = vtkMapTile::New();
      // Set tile position
      tile->SetCenter(j, -i, 0);

      // Set tile texture source
      tile->SetQuadKey(TileXYToQuadKey(temp_tileX, temp_tileY, this->Zoom).c_str());

      // Initialise the tile
      tile->init();

      // Add tile to the renderer
      Renderer->AddActor(tile->GetActor());
      temp_tileX++;
      }
    temp_tileY++;
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
void vtkMap::LatLongToPixelXY(double latitude, double longitude, int levelOfDetail, int &pixelX, int &pixelY)
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
