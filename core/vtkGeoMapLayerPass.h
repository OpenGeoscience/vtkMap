/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMap

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkGeoMapLayerPass
 * @brief Rendering infrastructure of a vtkLayer.
 *
 * Renders the vtkProps related to a vtkLayer instance. At render-time, a
 * pass receives the global PropArray held by the vtkRenderer through a
 * vtkRenderState instance.  This global array is filtered by the class
 * by comparing layer ids. The filtered vector of vtkProps is then rendered.
 *
 * A sequence of these passes can be defined in order to render a set of
 * stacked layers.
 *
 */
#ifndef vtkGeoMapLayerPass_h
#define vtkGeoMapLayerPass_h
#include <vector>             // For std::vector
#include "vtkmapcore_export.h"
#include <vtkRenderPass.h>


class vtkProp;

class VTKMAPCORE_EXPORT vtkGeoMapLayerPass : public vtkRenderPass
{
public:
  static vtkGeoMapLayerPass* New();
  vtkTypeMacro(vtkGeoMapLayerPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void Render(const vtkRenderState* s) VTK_OVERRIDE;

  void ReleaseGraphicsResources(vtkWindow* win) VTK_OVERRIDE;

  vtkSetMacro(LayerId, int);

protected:
  vtkGeoMapLayerPass();
  ~vtkGeoMapLayerPass() VTK_OVERRIDE;

private:
  vtkGeoMapLayerPass(const vtkGeoMapLayerPass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoMapLayerPass&) VTK_DELETE_FUNCTION;

/**
 * Builds the vector of vtkProps related to this vtkLayer.
 */
  void FilterLayerProps(const vtkRenderState* state);

  void RenderOpaqueGeometry(const vtkRenderState* state);
  void RenderTranslucentGeometry(const vtkRenderState* state);
  void RenderOverlay(const vtkRenderState* state);

  std::vector<vtkProp*> LayerProps;
  int LayerId = -1;
};

#endif // vtkGeoMapLayerPass_h
