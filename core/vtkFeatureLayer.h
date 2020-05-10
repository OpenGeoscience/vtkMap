/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFeatureLayer
// .SECTION Description
//

#ifndef __vtkFeatureLayer_h
#define __vtkFeatureLayer_h

#include "vtkLayer.h"
#include "vtkmapcore_export.h"

class vtkCollection;
class vtkFeature;

class VTKMAPCORE_EXPORT vtkFeatureLayer : public vtkLayer
{
public:
  static vtkFeatureLayer* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkFeatureLayer, vtkLayer)

    // Description:
    // Override vtkObjectBase::UnRegister(vtkObjectBase*)
    // Needed in order to delete features, which use a weak pointer to
    // to their vtkFeatureLayer instance,
    void UnRegister(vtkObjectBase* o) override;

  // Description:
  // Add a new feature to the layer
  // Note: layer must be added to a vtkMap *before* features can be added.
  void AddFeature(vtkSmartPointer<vtkFeature> feature);

  // Description:
  // Remove a feature from the layer
  void RemoveFeature(vtkSmartPointer<vtkFeature> feature);

  // Description:
  // Return all features contained here
  vtkCollection* GetFeatures();

  // Description:
  // Update features and prepare them for rendering
  void Update() override;

protected:
  vtkFeatureLayer();
  ~vtkFeatureLayer() override;

protected:
  class vtkInternal;
  vtkInternal* Impl;

private:
  vtkFeatureLayer(const vtkFeatureLayer&);            // Not implemented
  vtkFeatureLayer& operator=(const vtkFeatureLayer&); // Not implemented
};

#endif // __vtkFeatureLayer_h
