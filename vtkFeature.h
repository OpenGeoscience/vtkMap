/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFeature -
// .SECTION Description
//

#ifndef __vtkFeature_h
#define __vtkFeature_h

#include <vtkObject.h>
#include <vtkWeakPointer.h>

#include "vtkmap_export.h"
#include "vtkFeatureLayer.h"

class vtkProp;

#include <string>

class VTKMAP_EXPORT vtkFeature : public vtkObject
{
public:
  enum Bins
    {
    Hidden = 99,
    Visible = 100,
    };

  void PrintSelf(ostream &os, vtkIndent indent) override;
  vtkTypeMacro(vtkFeature, vtkObject);

  // Expecting it to EPSG codes (EPSG4326)
  vtkGetStringMacro(Gcs);
  vtkSetStringMacro(Gcs);

  vtkGetMacro(Visibility, int);
  vtkSetMacro(Visibility, int);
  vtkBooleanMacro(Visibility, int);

  vtkGetMacro(Bin, int);
  vtkSetMacro(Bin, int);

  //we hold onto a weak pointer to the current feature layer that
  //we are part of
  void SetLayer(vtkFeatureLayer* layer);

  // Description:
  // Build feature when added to the feature layer.
  // Init should be called by the feature layer and
  // not directly by the application code.
  virtual void Init() = 0;

  // Description:
  // Perform any clean up operation related to rendering.
  // CleanUp should be called by the feature layer and
  // not directly by the application code.
  virtual void CleanUp() = 0;

  // Description:
  // Update feature and prepare it for rendering.
  // Update should be called by the feature layer and
  // not directly by the application code.
  virtual void Update() = 0;

  // Description:
  // Return boolean indicating if the feature is to be displayed,
  // which is the boolean product of the feature's visibiltiy
  // and the layer's visibility flags.
  bool IsVisible();

  // Description:
  // Return vtkProp that represents feature in picking operations.
  // Default is none.
  virtual vtkProp *PickProp();

  // Description:
  // Pick all feature items at designated display coordinates.
  // For external features to override.
  virtual void PickItems(
    vtkRenderer *renderer,
    int displayCoords[4],
    vtkGeoMapSelection *selection);

protected:
  vtkFeature();
  ~vtkFeature();

  unsigned int Id;
  int Visibility;
  int Bin;

  char* Gcs;

  vtkTimeStamp BuildTime;
  vtkTimeStamp UpdateTime;

  vtkWeakPointer<vtkFeatureLayer> Layer;

private:
  vtkFeature(const vtkFeature&);  // Not implemented
  vtkFeature& operator=(const vtkFeature&); // Not implemented
};


#endif // __vtkFeature_h
