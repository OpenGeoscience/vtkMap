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
// .NAME vtkMultiThreadedOsmLayer -
// .SECTION Description
//

#ifndef __vtkMultiThreadedOsmLayer_h
#define __vtkMultiThreadedOsmLayer_h

#include "vtkOsmLayer.h"
#include "vtkMapTileSpecInternal.h"
#include <vector>

class vtkMapTile;

typedef std::vector<vtkMapTileSpecInternal> TileSpecList;

class vtkMultiThreadedOsmLayer : public vtkOsmLayer
{
public:
  static vtkMultiThreadedOsmLayer *New();
  vtkTypeMacro(vtkMultiThreadedOsmLayer, vtkOsmLayer)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  virtual void Update();

  // Description:
  // Threaded method for managing tile requests, to prevent blocking.
  void BackgroundThreadExecute();

  // Description:
  // Threaded method for concurrent tile requests.
  void RequestThreadExecute(int threadId);

  // Description
  // Periodically check if new tiles have been initialized
  void TimerCallback();
protected:
  vtkMultiThreadedOsmLayer();
  ~vtkMultiThreadedOsmLayer();

  // Description:
  // Update needed tiles to draw current map display
  virtual void AddTiles();

  // Description:
  // Download image file, returns boolean indicating success
  bool DownloadImageFile(std::string url, std::string filename);

  // Description:
  // Instantiate and initialize vtkMapTile
  vtkMapTile *CreateTile(vtkMapTileSpecInternal& spec);

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
