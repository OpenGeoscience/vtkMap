/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOsmLayer -
// .SECTION Description
//

#ifndef __vtkOsmLayer_h
#define __vtkOsmLayer_h

#include "vtkFeatureLayer.h"
#include "vtkMapTile.h"
#include "vtkMapTileSpecInternal.h"
#include "vtkmap_export.h"

// VTK Includes
#include <vtkObject.h>
#include <vtkRenderer.h>

#include <map>
#include <vector>

class vtkTextActor;

class VTKMAP_EXPORT vtkOsmLayer : public vtkFeatureLayer
{
public:
  static vtkOsmLayer* New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkOsmLayer, vtkFeatureLayer)

  // Set the host URI and attribute text for the tile server
  // The default is tile.openstreetmap.org
  void SetMapTileServer(const char *URI, const char *attribution);

  // Description:
  // The full path to the directory used for caching map-tile files.
  // Set automatically by vtkMap.
  vtkGetStringMacro(CacheDirectory);

  // Description:
  virtual void Update();

  // Description:
  // Set the subdirectory used for caching map files.
  // This method is intended for *testing* use only.
  // The argument is *relative* to vtkMap::StorageDirectory.
  void SetCacheSubDirectory(const char *relativePath);

protected:
  vtkOsmLayer();
  virtual ~vtkOsmLayer();

  vtkSetStringMacro(CacheDirectory);

  virtual void AddTiles();
  void RemoveTiles();

  // Next 3 methods used to add tiles to layer
  void SelectTiles(std::vector<vtkMapTile*>& tiles,
                   std::vector<vtkMapTileSpecInternal>& tileSpecs);
  void InitializeTiles(std::vector<vtkMapTile*>& tiles,
                       std::vector<vtkMapTileSpecInternal>& tileSpecs);
  void RenderTiles(std::vector<vtkMapTile*>& tiles);

  void AddTileToCache(int zoom, int x, int y, vtkMapTile* tile);
  vtkMapTile* GetCachedTile(int zoom, int x, int y);

protected:
  char *MapTileServer;
  char *MapTileAttribution;
  vtkTextActor *AttributionActor;

  char *CacheDirectory;
  std::map< int, std::map< int, std::map <int, vtkMapTile*> > > CachedTilesMap;
  std::vector<vtkMapTile*> CachedTiles;

private:
  vtkOsmLayer(const vtkOsmLayer&);    // Not implemented
  vtkOsmLayer& operator=(const vtkOsmLayer&); // Not implemented
};

#endif // __vtkOsmLayer_h

