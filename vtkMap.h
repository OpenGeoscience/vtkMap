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
class vtkMapTile;

#include <map>
#include <vector>

class vtkMap : public vtkObject
{
public:
  static vtkMap *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkMap, vtkObject)

  // Description:
  // Get/Set the renderer in which map tiles will be added
  vtkGetMacro(Renderer, vtkRenderer*)
  vtkSetMacro(Renderer, vtkRenderer*)

  // Description:
  // Get/Set the detailing level
  vtkGetMacro(Zoom, int)
  vtkSetMacro(Zoom, int)

  // Description:
  // Get/Set center of the map
  vtkGetVector2Macro(Center, double);
  vtkSetVector2Macro(Center, double);

  // Description:
  // Get/Set Destination directory path
  void  SetDestinationPath(const std::string& destdir) {this->Destination = destdir;}
  std::string GetDestinationDir() {return this->Destination;}

  // Description:
  // Update the renderer with relevant tiles to draw the Map
  void Update();

  // Description:
  // Update the renderer with relevant tiles to draw the Map
  void Draw();

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
  // Clips a number to the specified minimum and maximum values.
  double Clip(double n, double minValue, double maxValue);

  // Description:
  // Add tile to the cache
  void AddTileToCache(int zoom, int x, int y, vtkMapTile* tile);

  // Description:
  // Return cached tile givena zoom level and indices x and y
  vtkMapTile* GetCachedTile(int zoom, int x, int y);

  // Description:
  // Set the renderer to draww the maps
  vtkRenderer* Renderer;

  // Description:
  // Set Zoom level, which determines the level of detailing
  int Zoom;

  // Description:
  // Center of the map
  double Center[2];

  // Description:
  // Cached tiles
  std::map< int, std::map< int, std::map <int, vtkMapTile*> > > CachedTiles;

  std::vector<vtkMapTile*> NewPendingTiles;

  bool Initialized;
  std::string Destination;

private:
  vtkMap(const vtkMap&);  // Not implemented
  void operator=(const vtkMap&); // Not implemented
};

#endif // __vtkMap_h
