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
#include "vtkmapgeojson_export.h"

class PolyData;

class VTKMAPGEOJSON_EXPORT vtkGeoJSONMapFeature : public vtkPolydataFeature
{
public:
  static vtkGeoJSONMapFeature* New();
  void PrintSelf(ostream &os, vtkIndent indent) override;
  vtkTypeMacro(vtkGeoJSONMapFeature, vtkPolydataFeature)

  // Description:
  // The GeoJSON input string
  // Note that no streaming option is available
  vtkSetStringMacro(InputString);
  vtkGetStringMacro(InputString);

  // Description:
  // Override
  void Init() override;

protected:
  vtkGeoJSONMapFeature();
  ~vtkGeoJSONMapFeature();

  char *InputString;
  vtkPolyData *PolyData;

private:
  vtkGeoJSONMapFeature(const vtkGeoJSONMapFeature&); // Not implemented
  vtkGeoJSONMapFeature& operator=(const vtkGeoJSONMapFeature&); // Not implemented
};


#endif // __vtkGeoJSONMapFeature_h
