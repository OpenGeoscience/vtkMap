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

#include <vtkObject.h>
#include <vtkCollection.h>
#include "vtkmap_export.h"

//class vtkCollection;
class vtkFeature;
class vtkIdList;
class vtkMap;
class vtkPolydataFeature;

class VTKMAP_EXPORT vtkGeoMapSelection : public vtkObject
{
  friend class vtkMap;
 public:
  static vtkGeoMapSelection* New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkGeoMapSelection, vtkObject);

  bool IsEmpty();
  void Clear();
  void AddFeature(vtkFeature *feature);
  void AddFeature(vtkFeature *feature, vtkIdList *ids);

  // Description:
  // Returns the set of currently-selected map features
  vtkGetObjectMacro(SelectedFeatures, vtkCollection);

  // Description:
  // Returns the set of currently-selected cell ids for
  // a specified polydata map feature
  vtkIdList *GetSelectedCellIds(vtkPolydataFeature *feature) const;

 protected:
  vtkGeoMapSelection();
  ~vtkGeoMapSelection();

  vtkCollection *SelectedFeatures;

 private:
  vtkGeoMapSelection(const vtkGeoMapSelection&);  // not implemented
  vtkGeoMapSelection& operator=(const vtkGeoMapSelection&);  // not implemented

  class vtkGeoMapSelectionInternal;
  vtkGeoMapSelectionInternal *Internal;
};

#endif   // __vtkGeoMapSelection_h
