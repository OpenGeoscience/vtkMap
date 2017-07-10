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

#ifndef __vtkLayer_h
#define __vtkLayer_h

#include "vtkMap.h"
#include "vtkmap_export.h"

// VTK Includes
#include <vtkObject.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>


class vtkGeoMapLayerPass;
//class vtkInformat
class vtkRenderPass;

class VTKMAP_EXPORT vtkLayer : public vtkObject
{
public:
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkLayer, vtkObject)

  // Description:
  // Get the renderer for the layer
  vtkGetMacro(Renderer, vtkRenderer*)

  // Description:
  const std::string& GetName();
  void SetName(const std::string& Name);

  // Description:
  unsigned int GetId();
  void SetId(const unsigned int& id);

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

