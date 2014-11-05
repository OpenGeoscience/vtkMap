/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGeoJSONMapFeature -
// .SECTION Description
//

#ifndef __vtkGeoJSONMapFeature_h
#define __vtkGeoJSONMapFeature_h

#include "vtkPolydataFeature.h"
#include "vtkmap_export.h"

class PolyData;

class VTKMAP_EXPORT vtkGeoJSONMapFeature : public vtkPolydataFeature
{
public:
  static vtkGeoJSONMapFeature* New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkGeoJSONMapFeature, vtkPolydataFeature)

  // Description:
  // The GeoJSON input string
  // Note that no streaming option is available
  vtkSetStringMacro(InputString);
  vtkGetStringMacro(InputString);

  // Description:
  // Override
  virtual void Init();

  // Description:
  // Override
  //virtual void CleanUp();

  // Description:
  // Override
  //virtual void Update();

protected:
  vtkGeoJSONMapFeature();
  ~vtkGeoJSONMapFeature();

  char *InputString;
  vtkPolyData *PolyData;

private:
  vtkGeoJSONMapFeature(const vtkGeoJSONMapFeature&); // Not implemented
  void operator=(const vtkGeoJSONMapFeature&); // Not implemented
};


#endif // __vtkGeoJSONMapFeature_h
