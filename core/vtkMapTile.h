/*=========================================================================

 Program:   Visualization Toolkit

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

#include "vtkFeature.h"
#include "vtkmapcore_export.h"

class vtkStdString;
class vtkPlaneSource;
class vtkActor;
class vtkPolyDataMapper;
class vtkTextureMapToPlane;

class VTKMAPCORE_EXPORT vtkMapTile : public vtkFeature
{
public:
  static vtkMapTile* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkMapTile, vtkFeature)

    // Description:
    // Set path to image on local file system
    void SetFileSystemPath(const std::string& path)
  {
    this->ImageFile = path;
  }

  // Set/Get URL to image on the map tile server
  // Note that, although this class stores the source URL, the *map layer* is
  // responsible for downloading the tile to the local filesystem.
  void SetImageSource(const std::string& imgSrc) { this->ImageSource = imgSrc; }
  std::string GetImageSource() { return this->ImageSource; }

  // Description:
  // Get/Set corners of the tile (lowerleft, upper right)
  vtkGetVector4Macro(Corners, double);
  vtkSetVector4Macro(Corners, double);

  // Description:
  // Get/Set bin of the tile
  vtkGetMacro(Bin, int);
  vtkSetMacro(Bin, int);

  // Description:
  vtkGetMacro(Plane, vtkPlaneSource*) vtkGetMacro(Actor, vtkActor*)
    vtkGetMacro(Mapper, vtkPolyDataMapper*)

    // Description:
    // Get/Set position of the tile
    void SetCenter(double* center);
  void SetCenter(double x, double y, double z);

  void SetVisible(bool val);
  bool IsVisible();

  // Description:
  // Create the geometry and download
  // the image if necessary
  void Init() override;

  // Description:
  // Remove drawables from the renderer and
  // perform any other clean up operations
  void CleanUp() override;

  // Description:
  // Update the map tile
  void Update() override;

protected:
  vtkMapTile();
  ~vtkMapTile();

  // Description:
  // Construct the textured geometry for the tile
  void Build();

  // Description:
  // Storing the remote and local paths
  std::string ImageSource;
  std::string ImageFile;

  vtkPlaneSource* Plane;
  vtkTextureMapToPlane* TexturePlane;
  vtkActor* Actor;
  vtkPolyDataMapper* Mapper;

  int Bin;
  bool VisibleFlag;
  double Corners[4];

private:
  vtkMapTile(const vtkMapTile&);            // Not implemented
  vtkMapTile& operator=(const vtkMapTile&); // Not implemented
};

#endif // __vtkMapTile_h
