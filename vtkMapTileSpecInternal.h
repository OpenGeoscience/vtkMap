/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapTileSpecInternal.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMapTileSpecInternal - wrapper for map tile plus data
// .SECTION Description
// Used internally by vtkOsmLayer and subclasses

#ifndef __vtkMapTileSpecInternal_h
#define __vtkMapTileSpecInternal_h

class vtkMapTile;

class vtkMapTileSpecInternal
{
public:
  double Corners[4];  // world coordinates
  int ZoomRowCol[3];  // OSM tile indices
  int ZoomXY[3];      // local cache indices
};

#endif // __vtkMapTileSpecInternal_h
