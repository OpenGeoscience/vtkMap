/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapClusteredMarkerSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMapClusteredMarkerSource - points source for map markers
// .SECTION Description
// Computes 2 vtkPolyData for (i) individual map markers, and (ii) map
// marker clusters. The computed polydata contains point sets and
// point data (i.e., no cells).


#ifndef __vtkMapClusteredMarkerSource_h
#define __vtkMapClusteredMarkerSource_h

#include <vtkPolyDataAlgorithm.h>
#include <vtkType.h>

class vtkInformation;
class vtkInformationVector;
class vtkPolyData;
class vtkUnsignedCharArray;

class vtkMapClusteredMarkerSource : public vtkPolyDataAlgorithm
{
public:
  static vtkMapClusteredMarkerSource *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro(vtkMapClusteredMarkerSource, vtkObject);

  // Description:
  // Add marker coordinates
  vtkIdType AddMarker(double latitude, double longitude);

  // Description:
  // Removes all map markers
  void RemoveMapMarkers();

 protected:
  vtkMapClusteredMarkerSource();
  ~vtkMapClusteredMarkerSource();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkUnsignedCharArray *SetupUCharArray(vtkPolyData *polyData, const char *name,
                                        int numberOfComponents=3);

  class MapClusteredMarkerSourceInternals;
  MapClusteredMarkerSourceInternals* Internals;
private:
  vtkMapClusteredMarkerSource(const vtkMapClusteredMarkerSource&);  // not implemented
  void operator=(const vtkMapClusteredMarkerSource);  // not implemented
};

#endif // __vtkMapClusteredMarkerSource_h
