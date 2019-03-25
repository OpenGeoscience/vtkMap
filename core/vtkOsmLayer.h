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
#include "vtkmapcore_export.h"

// VTK Includes
#include <vtkObject.h>
#include <vtkRenderer.h>

#include <map>
#include <sstream>
#include <vector>

class vtkTextActor;

class VTKMAPCORE_EXPORT vtkOsmLayer : public vtkFeatureLayer
{
public:
  static vtkOsmLayer* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkOsmLayer, vtkFeatureLayer)

    // Set the map tile server and corresponding attribute text.
    // The default server is tile.openstreetmap.org.
    // The attribution will be displayed at the bottom of the window.
    // The file extension is typically "png" or "jpg".
    void SetMapTileServer(
      const char* server, const char* attribution, const char* extension);

  // Description:
  // The full path to the directory used for caching map-tile files.
  // Set automatically by vtkMap.
  vtkGetStringMacro(CacheDirectory)

  // Description:
  void Update() override;

  // Description:
  // Set the subdirectory used for caching map files.
  // This method is intended for *testing* use only.
  // The argument is *relative* to vtkMap::StorageDirectory.
  void SetCacheSubDirectory(const char* relativePath);

protected:
  vtkOsmLayer();
  ~vtkOsmLayer() override;

  vtkSetStringMacro(CacheDirectory)

  virtual void AddTiles();
  bool DownloadImageFile(std::string url, std::string filename);
  bool VerifyImageFile(FILE* fp, std::string filename);
  void RemoveTiles();

  // Next 3 methods used to add tiles to layer
  void SelectTiles(std::vector<vtkSmartPointer<vtkMapTile>>& tiles,
    std::vector<vtkMapTileSpecInternal>& tileSpecs);
  void InitializeTiles(std::vector<vtkSmartPointer<vtkMapTile>>& tiles,
    std::vector<vtkMapTileSpecInternal>& tileSpecs);
  void RenderTiles(std::vector<vtkSmartPointer<vtkMapTile>>& tiles);

  void AddTileToCache(int zoom, int x, int y, vtkMapTile* tile);
  vtkSmartPointer<vtkMapTile> GetCachedTile(int zoom, int x, int y);

  // Construct paths for local & remote tile access
  // A stringstream is passed in for performance reasons
  void MakeFileSystemPath(
    vtkMapTileSpecInternal& tileSpec, std::stringstream& ss);
  void MakeUrl(vtkMapTileSpecInternal& tileSpec, std::stringstream& ss);

protected:
  char* MapTileExtension;
  char* MapTileServer;
  char* MapTileAttribution;
  char* TileNotAvailableImagePath;
  vtkTextActor* AttributionActor;

  char* CacheDirectory;
  // CachedTilesMap contains already built tiles
  std::map<int, std::map<int, std::map<int, vtkSmartPointer<vtkMapTile>> > > CachedTilesMap;
  // CachedTiles is intended to retrieve tiles put on the scene
  std::vector<vtkSmartPointer<vtkMapTile>> CachedTiles;

private:
  vtkOsmLayer(const vtkOsmLayer&);            // Not implemented
  vtkOsmLayer& operator=(const vtkOsmLayer&); // Not implemented
};

#endif // __vtkOsmLayer_h
