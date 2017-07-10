/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkValuePass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGeoMapLayerPass
 *
 * Renders a vtkLayer instance. A sequence of these passes can be defined
 * in order to render a set of stacked layers.
 */
#ifndef vtkGeoMapLayerPass_h
#define vtkGeoMapLayerPass_h
#include "vtkmap_export.h"
#include "vtkDefaultPass.h"


class vtkDefaultPass;

class VTKMAP_EXPORT vtkGeoMapLayerPass : public vtkDefaultPass
{
public:
  static vtkGeoMapLayerPass* New();
  vtkTypeMacro(vtkGeoMapLayerPass, vtkDefaultPass);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void Render(const vtkRenderState* s) VTK_OVERRIDE;

  void ReleaseGraphicsResources(vtkWindow* win) VTK_OVERRIDE;

protected:
  vtkGeoMapLayerPass();
  ~vtkGeoMapLayerPass() VTK_OVERRIDE;

private:
  vtkGeoMapLayerPass(const vtkGeoMapLayerPass&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoMapLayerPass&) VTK_DELETE_FUNCTION;
};

#endif // vtkGeoMapLayerPass_h
