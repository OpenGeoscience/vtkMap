/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGeoMapSelection - set of selected entities
// .SECTION Description
//

#ifndef __vtkGeoMapSelection_h
#define __vtkGeoMapSelection_h

#include "vtkmapcore_export.h"
#include <vtkCollection.h>
#include <vtkObject.h>

//class vtkCollection;
class vtkFeature;
class vtkIdList;
class vtkMap;
class vtkFeature;

class VTKMAPCORE_EXPORT vtkGeoMapSelection : public vtkObject
{
public:
  static vtkGeoMapSelection* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkGeoMapSelection, vtkObject);

  bool IsEmpty();
  void Clear();
  void AddFeature(vtkFeature* feature);
  void AddFeature(vtkFeature* feature, vtkIdList* cellIds);
  void AddFeature(vtkFeature* feature,
    vtkIdList* markerIds,
    vtkIdList* clusterIds);

  vtkSetVector4Macro(LatLngBounds, double);
  vtkGetVector4Macro(LatLngBounds, double);

  // Description:
  // Returns the set of currently-selected map features
  vtkGetObjectMacro(SelectedFeatures, vtkCollection);

  // Description:
  // Retrieves the selected cell ids for a polydata feature.
  // Returns true if input feature is vtkPolydataFeature (only)
  bool GetPolyDataCellIds(vtkFeature* feature, vtkIdList* idList) const;

  // Description:
  // Returns the selected marker ids and cluster ids for marker set
  // Note that cluster ids are internally generated.
  // Returns true if input feature is vtkMapMarkerSet
  bool GetMapMarkerIds(vtkFeature* feature,
    vtkIdList* markerIdList,
    vtkIdList* clusterIdList) const;

protected:
  vtkGeoMapSelection();
  ~vtkGeoMapSelection();

  double LatLngBounds[4] = { 0., 0., 0., 0. };
  vtkCollection* SelectedFeatures;

private:
  vtkGeoMapSelection(const vtkGeoMapSelection&);            // not implemented
  vtkGeoMapSelection& operator=(const vtkGeoMapSelection&); // not implemented

  class vtkGeoMapSelectionInternal;
  vtkGeoMapSelectionInternal* Internal;
};

#endif // __vtkGeoMapSelection_h
