/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMap -
// .SECTION Description
//

#ifndef __vtkMap_h
#define __vtkMap_h

// VTK Includes
#include <vtkObject.h>
#include <vtkSmartPointer.h>

#include "vtkmap_export.h"

class vtkCallbackCommand;
class vtkCameraPass;
class vtkFeature;
class vtkGeoMapFeatureSelector;
class vtkGeoMapSelection;
class vtkInteractorStyle;
class vtkInteractorStyleGeoMap;
class vtkLayer;
class vtkRenderer;
class vtkRenderPassCollection;
class vtkSequencePass;

#include <map>
#include <string>
#include <vector>

class vtkLayer;

class VTKMAP_EXPORT vtkMap : public vtkObject
{
public:
  // Description:
  // State of asynchronous layers
  enum AsyncState
    {
    AsyncOff = 0,        // layer is not asynchronous
    AsyncIdle,           // no work scheduled
    AsyncPending,        // work in progress
    AsyncPartialUpdate,  // some work completed
    AsyncFullUpdate      // all work completed
    };

  static vtkMap *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkMap, vtkObject)

  // Description
  // Set the GDAL_DATA folder, which is generally needed
  // to display raster features. This methods calls
  // vtkRasterFeature::SetGDALDataDirectory().
  static void SetGDALDataDirectory(char *path);

  // Description:
  // Get/Set the renderer to which map content will be added
  // vtkSetObjectMacro is used so that map takes reference to renderer.
  // This ensures that map can delete its contents before the renderer
  // is deleted.
  vtkGetMacro(Renderer, vtkRenderer*)
  vtkSetObjectMacro(Renderer, vtkRenderer) ///TODO USe def macro in cxx

  // Description:
  // Get/Set the interactor style for the map renderer
  // Note these are asymmetric on purpose
  vtkSetMacro(InteractorStyle, vtkInteractorStyleGeoMap*)
  vtkInteractorStyle *GetInteractorStyle();

  // Description:
  // Get/Set the camera model to use perspective projection.
  // You must set this BEFORE calling Draw() for the first time.
  // The default is off, which uses orthographic (parallel) projection.
  vtkSetMacro(PerspectiveProjection, bool);
  vtkGetMacro(PerspectiveProjection, bool);
  vtkBooleanMacro(PerspectiveProjection, bool);

  // Description:
  // Get/Set the detailing level
  vtkGetMacro(Zoom, int)
  vtkSetMacro(Zoom, int)

  // Description:
  // Get/Set center of the map.
  void GetCenter(double (&latlngPoint)[2]);
  void SetCenter(double latlngPoint[2]);
  void SetCenter(double latitude, double longitude);

  // Description:
  // Set center & zoom level to display area of interest.
  // The 4 coordinates specify a rectangle in lon-lat units:
  // [latitude1, longitude1, latitude2, longitude2]
  void SetVisibleBounds(double latlngCoords[4]);
  void GetVisibleBounds(double latlngCoords[4]);

  // Description:
  // Get/Set the directory used for caching files.
  vtkGetStringMacro(StorageDirectory);
  void SetStorageDirectory(const char *path);

  // Description:
  // Add / Remove layer from the map.
  void AddLayer(vtkLayer* layer);
  void RemoveLayer(vtkLayer* layer);
  vtkLayer *FindLayer(const char *name);

  // TODO Implement this
  //void SetLayerOrder(vtkLaye* layer, int offsetFromCurrent);

  // Description:
  // Update the map contents for the current view
  void Update();

  // Description:
  // Update the renderer with relevant map content
  void Draw();

  // Description:
  // Update internal logic when feature is added.
  // This method should only be called by vtkFeatureLayer
  void FeatureAdded(vtkFeature *feature);

  // Description:
  // Update internal logic when feature is removed.
  // This method should only be called by vtkFeatureLayer
  void ReleaseFeature(vtkFeature *feature);

  // Description:
  // Returns map features at specified display coordinates
  void PickPoint(int displayCoords[2], vtkGeoMapSelection* result);

  // Description:
  // Returns map features at specified area (bounding box
  // in display coordinates)
  void PickArea(int displayCoords[4], vtkGeoMapSelection *selection);

  // Description:
  // Periodically poll asynchronous layers
  void PollingCallback();

  // Description:
  // Current state of asynchronous layers
  enum AsyncState GetAsyncState();

  // Description:
  // Compute lat-lon coordinates for given display coordinates
  // and elevation. The latLngCoords[] is updated with
  // [latitude, longitude, elevation].
  void ComputeLatLngCoords(double displayCoords[2], double elevation,
                           double latLngCoords[3]);

  // Description:
  // Compute display coordinates for given lat/lon/elevation.
  // For internal debug/test use
  void ComputeDisplayCoords(double lanLngCoords[2], double elevation,
                            double displayCoords[3]);

  ///@{
  /**
   * Change order of layers in the stack.
   */
  void MoveUp(vtkLayer* layer);
  void MoveDown(vtkLayer* layer);
  void MoveToTop(vtkLayer* layer);
  void MoveToBottom(vtkLayer* layer);
  ///@}

protected:
  vtkMap();
  ~vtkMap();

  // Description:
  // Clips a number to the specified minimum and maximum values.
  double Clip(double n, double minValue, double maxValue);

  // Computes display-to-world point at specified z coord
  void ComputeWorldCoords(double displayCoords[2], double z,
                          double worldCoords[3]);

  // Description:
  // The renderer used to draw the maps
  vtkRenderer* Renderer;

  // Description:
  // The interactor style used by the map
  vtkInteractorStyleGeoMap* InteractorStyle;

  // Description:
  bool PerspectiveProjection;

  // Description:
  // Set Zoom level, which determines the level of detailing
  int Zoom;

  // Description:
  // Center of the map
  double Center[2];

  // Description:
  // Directory for caching map files
  char* StorageDirectory;

protected:
  void Initialize();

  void UpdateLayerSequence();

  bool Initialized;

  // Description:
  // Base layer that dictates the coordinate tranformation
  // and navigation
  vtkLayer* BaseLayer;

  // Description:
  // List of layers attached to the map
  std::vector<vtkLayer*> Layers;

  // Description:
  // Helper class for selection
  vtkGeoMapFeatureSelector *FeatureSelector;

  // Description:
  // Callback method for polling timer
  vtkCallbackCommand *PollingCallbackCommand;

  // Description:
  // Current state of asynchronous layers
  AsyncState CurrentAsyncState;

  vtkSmartPointer<vtkRenderPassCollection> LayerCollection;
  vtkSmartPointer<vtkSequencePass> LayerSequence;
  vtkSmartPointer<vtkCameraPass> CameraPass;
  
private:
  vtkMap(const vtkMap&);  // Not implemented
  vtkMap& operator=(const vtkMap&); // Not implemented
};

#endif // __vtkMap_h
