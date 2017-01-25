/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTeardropSource - generate polygonal teardrop shape
// .SECTION Description
// vtkTeardropSource creates a teardrop-shape geometry.

#ifndef __vtkTeardropSource_h
#define __vtkTeardropSource_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkmap_export.h"

#include "vtkCell.h" // Needed for VTK_CELL_SIZE

class VTKMAP_EXPORT vtkTeardropSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkTeardropSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with default resolution 6, tail-length 0.75, head-radius 0.25.
  // The tail is set to the origin and is symmetrix about the x axis.
  static vtkTeardropSource *New();

  // Description:
  // Set the height of the tail section. This is the height along the cone in
  // its specified direction.
  vtkSetClampMacro(TailHeight,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(TailHeight,double);

  // Description:
  // Set the curvature stregth as the tip end of the tail section, as
  // a fraction of the TailHeight. (Clamped between 0 and 1.)
  // This sets the amount of curvature at that end.
  vtkSetClampMacro(TipStrength,double,0.0,1.0)
  vtkGetMacro(TipStrength,double);

  // Description:
  // Set the curvature stregth as the front end of the tail section, as
  // a fraction of the TailHeight. (Clamped between 0 and 1.)
  // This sets the amount of curvature at that end.
  vtkSetClampMacro(HeadStrength,double,0.0,1.0)
  vtkGetMacro(HeadStrength,double);

  // Description:
  // Set the head radius of the teardrop
  vtkSetClampMacro(HeadRadius,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(HeadRadius,double);

  // Description:
  // Set the number of facets used to represent the teardrop.
  vtkSetClampMacro(Resolution,int,0,VTK_CELL_SIZE)
  vtkGetMacro(Resolution,int);

  // Description:
  // Set/get option to only generate the front-side surfaces
  // (z coordinates >= 0). Intended for use with 2D/map applications.
  // Default is OFF.
  vtkSetMacro(FrontSideOnly,bool);
  vtkGetMacro(FrontSideOnly,bool);
  vtkBooleanMacro(FrontSideOnly, bool);

  // Description:
  // Set/get option to project points onto the xy plane (i.e., set z = 0).
  // Does NOT modify point normals.
  // Intended for use with 2D/map applications. Default is OFF.
  vtkSetMacro(ProjectToXYPlane,bool);
  vtkGetMacro(ProjectToXYPlane,bool);
  vtkBooleanMacro(ProjectToXYPlane, bool);

  // Description:
  // Set/get the desired precision for the output points.
  // vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
  // vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);

protected:
  vtkTeardropSource(int res=12);
  ~vtkTeardropSource() {}

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double TailHeight;
  double TipStrength;
  double HeadStrength;
  double HeadRadius;
  int Resolution;
  bool FrontSideOnly;
  bool ProjectToXYPlane;
  int OutputPointsPrecision;

private:
  vtkTeardropSource(const vtkTeardropSource&);  // Not implemented.
  vtkTeardropSource& operator=(const vtkTeardropSource&);  // Not implemented.

  void ComputeTailPath(vtkPoints *, vtkDoubleArray *);
  void ComputeHeadPath(vtkPoints *, vtkDoubleArray *);
  void ComputePolyData(vtkPoints *, vtkDoubleArray *, vtkPolyData *);
  void ComputeTailCoordinate(double t,
                             double controlPt[4][2],
                             double coordinates[3],
                             double normal[3]);
};

#endif


