/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMapMarker -
// .SECTION Description
//

#ifndef __vtkMapMarker_h
#define __vtkMapMarker_h

// VTK Includes
#include "vtkObject.h"
//#include "vtkImageData.h"

class vtkActor2D;
class vtkImageData;
class vtkImageMapper;

class vtkMapMarker : public vtkObject
{
public:
  static vtkMapMarker* New();
  static void LoadMarkerImage(const char *PngFilename);
  //virtual void PrintSelf(ostream &os, vtkIndent indent);
  vtkTypeMacro (vtkMapMarker, vtkObject)

  void SetCoordinates(double Latitude, double Longitude);
  vtkGetMacro(Actor, vtkActor2D*);

 protected:
  vtkMapMarker();
  ~vtkMapMarker();

  static vtkImageData *ImageData;
  vtkImageMapper *Mapper;
  vtkActor2D *Actor;


 private:
  vtkMapMarker(const vtkMapMarker&);  // Not implemented
  void operator=(const vtkMapMarker&); // Not implemented

};

#endif  // __vtkMapMarker_h
