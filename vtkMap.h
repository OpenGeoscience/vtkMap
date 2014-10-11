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

class vtkActor;
class vtkInteractorStyle;
class vtkInteractorStyleMap;
class vtkMapTile;
class vtkMapMarkerSet;
class vtkMapPickResult;
class vtkPicker;
class vtkPoints;
class vtkRenderer;

#include <map>
#include <string>
#include <vector>

class vtkLayer;

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
  // Get/Set the interactor style in which map tiles will be added
  // Note these are asymmetric on purpose
  vtkSetMacro(InteractorStyle, vtkInteractorStyleMap*)
  vtkInteractorStyle *GetInteractorStyle();

  // Description:
  // Get the map marker layer
  vtkGetMacro(MapMarkerSet, vtkMapMarkerSet*);

  // Description:
  // Get/Set the picker used for picking map markers
  vtkGetMacro(Picker, vtkPicker*)
  vtkSetMacro(Picker, vtkPicker*)

  // Description:
  // Get/Set the detailing level
  vtkGetMacro(Zoom, int)
  vtkSetMacro(Zoom, int)

  // Description:
  // Get/Set the tile zoom level
  vtkGetMacro(TileZoom, int)
  vtkSetMacro(TileZoom, int)

  // Description:
  // Get/Set center of the map
  void GetCenter(double (&latlngPoint)[2]);
  vtkSetVector2Macro(Center, double);

  // Description:
  // Get/Set the directory used for caching map tiles
  vtkGetStringMacro(CacheDirectory);
  vtkSetStringMacro(CacheDirectory);

  // Description:
  // Add / Remove layer from the map.
  void AddLayer(vtkLayer* layer);
  void RemoveLayer(vtkLayer* layer);

  // Description:
  // Recompute view angle when window size changes
  void OnResize();

  // Description:
  // Update the renderer with relevant tiles to draw the Map
  void Update();

  // Description:
  // Update the renderer with relevant tiles to draw the Map
  void Draw();

  // Description:
  // Returns info at specified display coordinates
  void PickPoint(int displayCoords[2], vtkMapPickResult* result);

  // Description:
  // Transform from map coordiantes to display coordinates
  // gcsToDisplay(points, "EPSG:3882")
  // This method assumes plate carree projection if the source projection is
  // empty. In case of plate carree projection, the point is supposed to be in
  // [latitude, longitude, elevation] format.
  vtkPoints* gcsToDisplay(vtkPoints* points, std::string srcProjection="");

  // Description:
  // Transform from display coordinates to map coordinates
  // If the map projection is EPSG 4326 or EPSG 3857, then returned
  // points will have the following format: [latitude, longitude, elevation]
  vtkPoints* displayToGcs(vtkPoints* points);

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
  // The renderer used to draw the maps
  vtkRenderer* Renderer;

  // Description:
  // The interactor style used by the map
  vtkInteractorStyleMap* InteractorStyle;

  // Description:
  // The picker used for picking map markers
  vtkPicker* Picker;

  // Description:
  // Set Zoom level, which determines the level of detailing
  int Zoom;

  // Description:
  // Center of the map
  double Center[2];

  // Directory for caching map tiles
  char* CacheDirectory;

  // Description:
  // The map marker manager
  vtkMapMarkerSet *MapMarkerSet;

protected:
  bool Initialized;

  // Description:
  // Effective zoom used by the tiles
  int TileZoom;

  // Description:
  // Base layer that dictates the coordinate tranformation
  // and navigation
  vtkLayer* BaseLayer;

  // Description:
  // List of layers attached to the map
  std::vector<vtkLayer*> Layers;

  int WindowHeight;

private:
  vtkMap(const vtkMap&);  // Not implemented
  void operator=(const vtkMap&); // Not implemented
};

#endif // __vtkMap_h
