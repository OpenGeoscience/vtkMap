/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLayer -
// .SECTION Description
//
// vtkLayer instances are rendered as vtkRenderPasses.  A sequence of render
// passes is defined in vtkMap, which defines the order in which the layers
// are rendered. vtkProps contained in a layer are marked as such through
// a PropertyKey (ID), which is later used by the member vtkGeoMapLayerPass
// at render time to filter these vtkProps from the global vtkRenderer::PropArray.
//

#ifndef __vtkLayer_h
#define __vtkLayer_h

#include "vtkMap.h"
#include "vtkmapcore_export.h"

// VTK Includes
#include <vtkObject.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>


class vtkGeoMapLayerPass;
class vtkInformationIntegerKey;
class vtkRenderPass;

class VTKMAPCORE_EXPORT vtkLayer : public vtkObject
{
public:
  void PrintSelf(ostream &os, vtkIndent indent) override;
  vtkTypeMacro(vtkLayer, vtkObject)

  // Description:
  // Get the renderer for the layer
  vtkGetMacro(Renderer, vtkRenderer*)

  // Description:
  const std::string& GetName() const;
  void SetName(const std::string& Name);

  // Description:
  // Get the unique Layer Id.  The Id is defined internally at
  // construction-time.
  unsigned int GetId();

  // Description:
  vtkGetMacro(Opacity, double)
  vtkSetMacro(Opacity, double)

  // Description:
  vtkBooleanMacro(Visibility, int)
  vtkGetMacro(Visibility, int)
  vtkSetMacro(Visibility, int)

  // Description:
  vtkBooleanMacro(Base, int)
  vtkGetMacro(Base, int)
  vtkSetMacro(Base, int)

  // Description:
  vtkGetObjectMacro(Map, vtkMap)
  void SetMap(vtkMap* map);

  // Description:
  bool IsAsynchronous();

  // Description:
  // Finalize any asynchronous operations.
  // For asynchronous layers, this method should be polled periodically
  // by the vtkMap object.
  virtual vtkMap::AsyncState ResolveAsync();

  // Description:
  virtual void Update() = 0;

  vtkRenderPass* GetRenderPass();

  // Description:
  // Adds actor to vtkRenderer and sets the necessary PropertyKeys in the
  // vtkProp (e.g. Layer Id).  It is imperative to add vtkProp instances to
  // the renderer through this API (and not directly to the vtkRenderer) if
  // a given vtkProp is to be displayed as part of a particular layer. Otherwise
  // they might not be displayed at all,  this is because vtkRenderer actual
  // rendering to vtkGeoMapLayerPass.
  void AddActor(vtkProp* prop);
  void AddActor2D(vtkProp* prop);
  void RemoveActor(vtkProp* prop);

  // Description:
  // Information key containing the Id of this layer. This Id set as a
  // PropertyKey in all the vtkProps (actors) contained within this layer.
  // At render-time, vtkGeoMapLayerPass filters out the vtkProps of its
  // particular layer by comparing Ids.
  static vtkInformationIntegerKey* ID();

protected:
  vtkLayer();
  virtual ~vtkLayer();

  double Opacity;
  int Visibility;
  int Base;

  std::string Name;
  unsigned int Id;
  bool AsyncMode;  // layer has defered (asynchronous) updates

  vtkMap* Map;
  vtkRenderer* Renderer;
  vtkSmartPointer<vtkGeoMapLayerPass> RenderPass;

  static unsigned int GlobalId;

private:
  vtkLayer(const vtkLayer&);  // Not implemented
  vtkLayer& operator=(const vtkLayer&); // Not implemented
};

#endif // __vtkLayer_h

