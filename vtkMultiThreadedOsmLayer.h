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

typedef std::vector<vtkMapTileSpecInternal> TileSpecList;

class vtkMultiThreadedOsmLayer : public vtkOsmLayer
{
public:
  static vtkMultiThreadedOsmLayer *New();
  vtkTypeMacro(vtkMultiThreadedOsmLayer, vtkOsmLayer)
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // (Thread Safe) Request map tile from server
  void RequestThreadExecute(int threadId);

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
  // Assign 1 tile spec to each thread
  void AssignOneTileSpecPerThread(TileSpecList& specs);

  // Description:
  // Separate tile specs from threads into two lists,
  // depending on whether they have initialized tile or not.
  // Does not clear lists, but appends data to them.
  void CollateThreadResults(std::vector<vtkMapTile*> newTiles,
                            TileSpecList& tileSpecs);

  class vtkMultiThreadedOsmLayerInternals;
  vtkMultiThreadedOsmLayerInternals *Internals;
private:
  vtkMultiThreadedOsmLayer(const vtkMultiThreadedOsmLayer&);  // Not implemented
  void operator=(const vtkMultiThreadedOsmLayer&); // Not implemented
};

#endif // __vtkMultiThreadedOsmLayer_h
