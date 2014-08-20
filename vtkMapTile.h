/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkMapTile.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMapTile -
// .SECTION Description
//

#ifndef __vtkMapTile_h
#define __vtkMapTile_h

// VTK Includes
#include "vtkObject.h"

class vtkStdString;
class vtkPlaneSource;
class vtkActor;
class vtkPolyDataMapper;
class vtkTextureMapToPlane;

class vtkMapTile : public vtkObject
{
public:
  static vtkMapTile* New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro (vtkMapTile, vtkObject)

  // Description:
  // Get/Set Bing Maps QuadKey corresponding to the tile
  void SetImageKey(const std::string& key)
    {
    this->ImageKey = key;
    }

  void  SetImageSource(const std::string& imgSrc) {this->ImageSource= imgSrc;}
  std::string GetImageSource() {return this->ImageSource;}

  void SetCorners(double llx, double lly, double urx, double ury)
    {
    this->Corners[0] = llx;
    this->Corners[1] = lly;
    this->Corners[2] = urx;
    this->Corners[3] = ury;
    }

  // Description:
  //
  vtkGetMacro(Plane, vtkPlaneSource*)
  vtkGetMacro(Actor, vtkActor*)
  vtkGetMacro(Mapper, vtkPolyDataMapper*)

  // Description:
  // Get/Set position of the tile
  void SetCenter(double* center);
  void SetCenter(double x, double y, double z);

  // Description:
  // Initialise the geometry and texture of the tile
  void init();

protected:
  vtkMapTile();
  ~vtkMapTile();

  // Description:
  // Check if the corresponding texture is downloaded
  bool IsTextureDownloaded(const char* outfile);

  // Description:
  // Download the texture corresponding to the Bing Maps QuadKey
  void DownloadTexture(const char* url, const char* outfilename);

  // Description:
  // Generate url of tile and output file from QuadKey, and download the texture
  // if not already downloaded.
  void InitializeTexture();

  // Description:
  // Storing the Quadkey
  std::string ImageSource;
  std::string ImageFile;
  std::string ImageKey;

  vtkPlaneSource* Plane;
  vtkTextureMapToPlane* texturePlane;
  vtkActor* Actor;
  vtkPolyDataMapper* Mapper;
  double Corners[4];

private:
  vtkMapTile(const vtkMapTile&);  // Not implemented
  void operator=(const vtkMapTile&); // Not implemented
};

#endif // __vtkMapTile_h
