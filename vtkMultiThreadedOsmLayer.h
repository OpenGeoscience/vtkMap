/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiThreadedOsmLayer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiThreadedOsmLayer - Multithreaded OSM layer
// .SECTION Description
// A multithreaded subclass of vtkOsmLayer.
// It performs concurrent map-tile requests in background threads,
// in order to circumvent I/O blocking. On initialization, the class
// starts a single background thread that supervises the overall logic
// for both: (i) requesting files from the map tile server and
// (ii) constructing new vtkMapTile instances. This supervisory thread
// in turn executes a set of additional threads concurrently to perform
// file I/O and http I/O actions. Because map tiles are generated
// asynchronously, the class overrides the vtkLayer::ResolveAsync()
// method; if new tiles have been created by the background threads,
// they are added to the layer's map-tile cache in ResolveAsync().

#ifndef __vtkMultiThreadedOsmLayer_h
#define __vtkMultiThreadedOsmLayer_h

#include "vtkOsmLayer.h"
#include "vtkMapTileSpecInternal.h"
#include <string>
#include <vector>

class vtkMapTile;

typedef std::vector<vtkMapTileSpecInternal> TileSpecList;

class VTKMAP_EXPORT vtkMultiThreadedOsmLayer : public vtkOsmLayer
{
public:
  static vtkMultiThreadedOsmLayer *New();
  vtkTypeMacro(vtkMultiThreadedOsmLayer, vtkOsmLayer)
  void PrintSelf(ostream &os, vtkIndent indent) override;

  // Description:
  void Update() override;

  // Description:
  // Threaded method for managing tile requests, to prevent blocking.
  void BackgroundThreadExecute();

  // Description:
  // Threaded method for concurrent tile requests.
  void RequestThreadExecute(int threadId);

  // Description:
  // Override vtkLayer::ResolveAsync()
  // Update tile cache with any new tiles
  vtkMap::AsyncState ResolveAsync() override;

protected:
  vtkMultiThreadedOsmLayer();
  ~vtkMultiThreadedOsmLayer() override;

  // Description:
  // Update needed tiles to draw current map display
  void AddTiles() override;

  // Description:
  // Download image file, returns boolean indicating success
  bool DownloadImageFile(std::string url, std::string filename);

  // Description:
  // Instantiate and initialize vtkMapTile
  vtkMapTile *CreateTile(
    vtkMapTileSpecInternal& spec,
    const std::string& localPath,
    const std::string& remoteUrl);

  // Description:
  // Assign tile specs evenly across request threads
  void AssignTileSpecsToThreads(TileSpecList& specs);

  // Description:
  // Assign 1 tile spec to each thread
  void AssignOneTileSpecPerThread(TileSpecList& specs);

  // Description:
  // Separate tile specs from threads into two lists,
  // depending on whether they have initialized tile or not.
  // Does not clear lists, but appends data to them.
  void CollateThreadResults(TileSpecList& newTiles,
                            TileSpecList& tileSpecs);

  // Description:
  // Copies new tiles to shared list.
  void UpdateNewTiles(TileSpecList& newTiles);

  class vtkMultiThreadedOsmLayerInternals;
  vtkMultiThreadedOsmLayerInternals *Internals;
private:
  vtkMultiThreadedOsmLayer(const vtkMultiThreadedOsmLayer&);  // Not implemented
  void operator=(const vtkMultiThreadedOsmLayer&); // Not implemented
};

#endif // __vtkMultiThreadedOsmLayer_h
