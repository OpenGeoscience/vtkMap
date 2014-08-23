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

  enum Bins
    {
    Hidden = 99,
    Visible = 100,
    };

  // Description:
  // Get/Set Bing Maps QuadKey corresponding to the tile
  void SetImageKey(const std::string& key)
    {
    this->ImageKey = key;
    }

  void  SetImageSource(const std::string& imgSrc) {this->ImageSource= imgSrc;}
  std::string GetImageSource() {return this->ImageSource;}

  // Description:
  // Get/Set corners of the tile (lowerleft, upper right)
  vtkGetVector4Macro(Corners, double);
  vtkSetVector4Macro(Corners, double);

  // Description:
  // Get/Set bin of the tile
  vtkGetMacro(Bin, int);
  vtkSetMacro(Bin, int);

  // Description:
  vtkGetMacro(Plane, vtkPlaneSource*)
  vtkGetMacro(Actor, vtkActor*)
  vtkGetMacro(Mapper, vtkPolyDataMapper*)

  // Description:
  // Get/Set position of the tile
  void SetCenter(double* center);
  void SetCenter(double x, double y, double z);

  // Description:
  // Create the geometry and download the image if necessary
  void Init();

  void SetVisible(bool val);
  bool IsVisible();

protected:
  vtkMapTile();
  ~vtkMapTile();

  // Description:
  // Check if the corresponding image is downloaded
  bool IsImageDownloaded(const char* outfile);

  // Description:
  // Download the image corresponding to the Bing Maps QuadKey
  void DownloadImage(const char* url, const char* outfilename);

  // Description:
  // Generate url of tile and output file from QuadKey, and download the texture
  // if not already downloaded.
  void InitializeDownload();

  // Description:
  // Storing the Quadkey
  std::string ImageSource;
  std::string ImageFile;
  std::string ImageKey;

  vtkPlaneSource* Plane;
  vtkTextureMapToPlane* TexturePlane;
  vtkActor* Actor;
  vtkPolyDataMapper* Mapper;

  int Bin;
  bool VisibleFlag;
  double Corners[4];

private:
  vtkMapTile(const vtkMapTile&);  // Not implemented
  void operator=(const vtkMapTile&); // Not implemented
};

#endif // __vtkMapTile_h
