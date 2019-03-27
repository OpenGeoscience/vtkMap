/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMap - Map representation using vtk rendering components.
//
// .SECTION Description
//
// Provides an API to manipulate the order in which different vtkLayer instances
// are rendered.
//
// vtkMap uses different vtkInteractorStyle instances to manage different types
// of interaction. The primary style is vtkInteractorStyleGeoMap, which supports
// default interaction (panning/ zooming), single-click selection, rubber-band
// selection and rubber-band zoom. Polygon-draw selection is supported through
// the standard vtkInteractorStyleDrawPolygon.
//
// \note
// Style switching is managed by this class, both style instances are currently
// collaborating with vtkMap in different ways. vtkInteractorStyleGeoMap calls
// vtkMap functions to actually make a selection and invokes an event with the
// result (legacy).  vtkInteractorStyleDrawPolygon is only observed by vtkMap
// and its internal events are caught to handle an actual selection.  The latter
// approach is preferred and vtkInteractorStyleGeoMap will eventually be
// refactored to comply.
//
// \sa vtkInteractorStyleGeoMap
//

#ifndef __vtkMap_h
#define __vtkMap_h
#include <map>
#include <string>
#include <vector>

#include <vtkObject.h>
#include <vtkSmartPointer.h>

#include "vtkMap_typedef.h"
#include "vtkmapcore_export.h"

class vtkCallbackCommand;
class vtkCameraPass;
class vtkCommand;
class vtkFeature;
class vtkGeoMapFeatureSelector;
class vtkGeoMapSelection;
class vtkInteractorStyle;
class vtkInteractorStyleGeoMap;
class vtkInteractorStyleDrawPolygon;
class vtkLayer;
class vtkRenderer;
class vtkRenderPassCollection;
class vtkRenderWindowInteractor;
class vtkSequencePass;

class VTKMAPCORE_EXPORT vtkMap : public vtkObject
{
public:
  using LayerContainer = std::vector<vtkSmartPointer<vtkLayer> >;

  // Description:
  // State of asynchronous layers
  enum AsyncState
  {
    AsyncOff = 0,       // layer is not asynchronous
    AsyncIdle,          // no work scheduled
    AsyncPending,       // work in progress
    AsyncPartialUpdate, // some work completed
    AsyncFullUpdate     // all work completed
  };

  static vtkMap* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkMap, vtkObject)

    // Description:
    // Get/Set the renderer to which map content will be added
    // vtkCxxSetObjectMacro is used so that map takes reference to renderer.
    // This ensures that map can delete its contents before the renderer
    // is deleted.
    void SetRenderer(vtkRenderer* ren);
  vtkGetMacro(Renderer, vtkRenderer*)

    /**
   * Interactor where different InteractorStyles are set.
   */
    void SetInteractor(vtkRenderWindowInteractor* interactor);
  void SetInteractionMode(const vtkMapType::Interaction mode);

  // Description:
  // Get/Set the camera model to use perspective projection.
  // You must set this BEFORE calling Draw() for the first time.
  // The default is off, which uses orthographic (parallel) projection.
  vtkSetMacro(PerspectiveProjection, bool);
  vtkGetMacro(PerspectiveProjection, bool);
  vtkBooleanMacro(PerspectiveProjection, bool);

  // Description:
  // Get/Set the detailing level
  vtkGetMacro(Zoom, int) vtkSetClampMacro(Zoom, int, 0, 19)

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
  void SetStorageDirectory(const char* path);

  // Description:
  // Add / Remove layer from the map.
  void AddLayer(vtkLayer* layer);
  void RemoveLayer(vtkLayer* layer);
  vtkLayer* FindLayer(const char* name);

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
  void FeatureAdded(vtkFeature* feature);

  // Description:
  // Update internal logic when feature is removed.
  // This method should only be called by vtkFeatureLayer
  void ReleaseFeature(vtkFeature* feature);

  // Description:
  // Returns map features at specified display coordinates
  void PickPoint(int displayCoords[2], vtkGeoMapSelection* result);

  // Description:
  // Returns map features at specified area (bounding box
  // in display coordinates)
  void PickArea(int displayCoords[4], vtkGeoMapSelection* selection);

  void OnPolygonSelectionEvent();

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
  void ComputeLatLngCoords(
    double displayCoords[2], double elevation, double latLngCoords[3]);

  // Description:
  // Compute display coordinates for given lat/lon/elevation.
  // For internal debug/test use
  void ComputeDisplayCoords(
    double lanLngCoords[2], double elevation, double displayCoords[3]);

  /**
 * Change the order of layers in the stack. Supports move UP, DOWN,
 * TOP, BOTTOM. Assumes 'layer' is valid.
 * \sa vtkMapType::Move
 */
  void MoveLayer(const vtkLayer* layer, vtkMapType::Move direction);

  vtkSetMacro(DevicePixelRatio, int);
  int GetDevicePixelRatio() const { return this->DevicePixelRatio; }

protected:
  vtkMap();
  ~vtkMap();

  // Description:
  // Clips a number to the specified minimum and maximum values.
  double Clip(double n, double minValue, double maxValue);

  // Computes display-to-world point at specified z coord
  void ComputeWorldCoords(
    double displayCoords[2], double z, double worldCoords[3]);

  // Description:
  // The renderer used to draw the maps
  vtkRenderer* Renderer;

  // Description:
  // The interactor styles used by the map
  vtkSmartPointer<vtkInteractorStyleGeoMap> RubberBandStyle;
  vtkSmartPointer<vtkInteractorStyleDrawPolygon> DrawPolyStyle;

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

  void Initialize();

  void UpdateLayerSequence();

  bool Initialized;

  // Description:
  // Base layer that dictates the coordinate tranformation
  // and navigation
  vtkSmartPointer<vtkLayer> BaseLayer;

  // Description:
  // List of layers attached to the map
  LayerContainer Layers;

  // Description:
  // Helper class for selection
  vtkGeoMapFeatureSelector* FeatureSelector;

  // Description:
  // Callback method for polling timer
  vtkCallbackCommand* PollingCallbackCommand;

  // Description:
  // Current state of asynchronous layers
  AsyncState CurrentAsyncState;

  //@{
  /**
 * Layer infrastructure. LayerCollection defines the order in which layers
 * are rendered.
 */
  vtkSmartPointer<vtkRenderPassCollection> LayerCollection;
  vtkSmartPointer<vtkSequencePass> LayerSequence;
  vtkSmartPointer<vtkCameraPass> CameraPass;
  //@}

  int DevicePixelRatio = 1;

  vtkSmartPointer<vtkCommand> PolygonSelectionObserver;

private:
  vtkMap(const vtkMap&) VTK_DELETE_FUNCTION;
  vtkMap& operator=(const vtkMap&) VTK_DELETE_FUNCTION;

  ///@{
  /**
 * Handlers for MoveLayer.
 * \sa vtkMap::MoveLayer
 */
  void MoveUp(const vtkLayer* layer);
  void MoveDown(const vtkLayer* layer);
  void MoveToTop(const vtkLayer* layer);
  void MoveToBottom(const vtkLayer* layer);
  ///@}

  //@{
  /**
   * Prepare/restore render state for hardware selection.
   */
  void BeginSelection();
  void EndSelection();
  //@}

  int PreviousSwapBuffers = -1;

  vtkRenderWindowInteractor* Interactor = nullptr;
};

#endif // __vtkMap_h
