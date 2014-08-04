/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMap -
// .SECTION Description
//

#ifndef __vtkMap_h
#define __vtkMap_h

// VTK Includes
#include <vtkObject.h>

class vtkRenderer;

class vtkMap : public vtkObject
{
public:
  static vtkMap *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkMap, vtkObject)

  // Description:
  // Bing Maps specific constants to be used during conversions
  static const double EarthRadius = 6378137;
  static const double MinLatitude = -85.05112878;
  static const double MaxLatitude = 85.05112878;
  static const double MinLongitude = -180;
  static const double MaxLongitude = 180;

  // Description:
  // Get/Set the renderer in which map tiles will be added
  vtkGetMacro(Renderer, vtkRenderer *)
  vtkSetMacro(Renderer, vtkRenderer *)

  // Description:
  // Get/Set the detailing level
  vtkGetMacro(Zoom, int)
  vtkSetMacro(Zoom, int)

  // Description:
  // Get/Set latitude of the center of the map
  vtkGetMacro(latitude, double)
  vtkSetMacro(latitude, double)

  // Description:
  // Get/Set Longitude of the center of the map
  vtkGetMacro(longitude, double)
  vtkSetMacro(longitude, double)

  // Description:
  // Update the renderer with relevant tiles to draw the Map
  void Update();

protected:
  vtkMap();
  ~vtkMap();

  // Description:
  // Add visible tiles to the renderer
  void AddTiles();

  // Description:
  // Remove hidden/invisible tiles from the renderer
  void RemoveTiles();

  // Description:
  // Converts tile XY coordinates into a QuadKey at a specified level of detail.
  std::string TileXYToQuadKey(int tileX, int tileY, int levelOfDetail);

  // Description:
  // Converts pixel XY coordinates into tile XY coordinates of the
  // tile containing the specified pixel.
  void PixelXYToTileXY(int pixelX, int pixelY, int &tileX, int &tileY);

  // Description:
  // Converts a point from latitude/longitude into pixel XY coordinates
  // at a specified level of detail
  void LatLongToPixelXY(double latitude, double longitude, int levelOfDetail, int &pixelX, int &pixelY);

  // Description:
  // Clips a number to the specified minimum and maximum values.
  double Clip(double n, double minValue, double maxValue);

  // Description:
  // Returns the Map Height/Width depending on the Zoom level
  uint MapSize(int levelOfDetail);

  // Description:
  // Set the renderer to draww the maps
  vtkRenderer *Renderer;

  // Description:
  // Set Zoom level, which determines the level of detailing
  int Zoom;

  // Description:
  //
  double latitude;
  double longitude;

private:
  vtkMap(const vtkMap&);  // Not implemented
  void operator=(const vtkMap&); // Not implemented
};

#endif // __vtkMap_h
