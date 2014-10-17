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
// .NAME vtkOsmLayer -
// .SECTION Description
//

#ifndef __vtkOsmLayer_h
#define __vtkOsmLayer_h

#include "vtkFeatureLayer.h"

#include "vtkMapTile.h"

// VTK Includes
#include <vtkObject.h>
#include <vtkRenderer.h>

#include <map>
#include <vector>

class vtkOsmLayer : public vtkFeatureLayer
{
public:
  static vtkOsmLayer* New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkOsmLayer, vtkFeatureLayer)

  // Description:
  virtual void Update();

protected:
  vtkOsmLayer();
  virtual ~vtkOsmLayer();

  void AddTiles();
  void RemoveTiles();

  void AddTileToCache(int zoom, int x, int y, vtkMapTile* tile);
  vtkMapTile* GetCachedTile(int zoom, int x, int y);

protected:
  std::map< int, std::map< int, std::map <int, vtkMapTile*> > > CachedTiles;
  std::vector<vtkActor*> CachedActors;
  std::vector<vtkMapTile*> NewPendingTiles;

private:
  vtkOsmLayer(const vtkOsmLayer&);    // Not implemented
  void operator=(const vtkOsmLayer&); // Not implemented
};

#endif // __vtkOsmLayer_h

