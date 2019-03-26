/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTeardropSource.h"

#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
//#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataWriter.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkTransform.h>

#include <iomanip>
#include <iostream>

#include <math.h>

vtkStandardNewMacro(vtkTeardropSource);

//----------------------------------------------------------------------------
// Construct with default resolution 6, height 1.0, radius 0.5, and capping
// on.
vtkTeardropSource::vtkTeardropSource(int res)
{
  res = (res < 0 ? 0 : res);
  this->Resolution = res;
  this->TailHeight = 0.75;
  this->TipStrength = 0.25;
  this->HeadStrength = 0.25;
  this->HeadRadius = 0.25;
  this->FrontSideOnly = false;
  this->ProjectToXYPlane = false;

  this->OutputPointsPrecision = DOUBLE_PRECISION;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
int vtkTeardropSource::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector),
  vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkPolyData* output =
    vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Todo handle multiple pieces
  vtkDebugMacro("TeardropSource Executing");

  vtkPoints* tailPath = vtkPoints::New(VTK_DOUBLE);
  vtkNew<vtkDoubleArray> tailNormals;
  this->ComputeTailPath(tailPath, tailNormals.GetPointer());
  vtkPoints* headPath = vtkPoints::New(VTK_DOUBLE);
  vtkNew<vtkDoubleArray> headNormals;
  this->ComputeHeadPath(headPath, headNormals.GetPointer());

  // Combine tail & head into one path
  vtkPoints* path = vtkPoints::New(VTK_DOUBLE);
  vtkNew<vtkDoubleArray> pathNormals;
  pathNormals->SetNumberOfComponents(3);

  int i;
  double* coordinates;
  double normal[3];
  // Skip last tail point (overlaps first head point)
  for (i = 0; i < tailPath->GetNumberOfPoints() - 1; i++)
  {
    coordinates = tailPath->GetPoint(i);
    path->InsertNextPoint(coordinates);
    tailNormals->GetTuple(i, normal);
    pathNormals->InsertNextTuple(normal);
  }
  for (i = 0; i < headPath->GetNumberOfPoints(); i++)
  {
    coordinates = headPath->GetPoint(i);
    path->InsertNextPoint(coordinates);
    headNormals->GetTuple(i, normal);
    pathNormals->InsertNextTuple(normal);
  }
  vtkDebugMacro(
    "Teardrop curve has " << path->GetNumberOfPoints() << " points");

  // std::cout << "Profile coordinates:" << "\n";
  // for (i=0; i<path->GetNumberOfPoints(); i++)
  //   {
  //   coordinates = path->GetPoint(i);
  //   std::cout << std::setw(3) << i << "  "
  //             << std::fixed << std::setprecision(3)
  //             << coordinates[0] << " " << coordinates[1]
  //             << "\n";
  //   }
  // std::cout << "Profile normals" << "\n";
  // for (i=0; i<path->GetNumberOfPoints(); i++)
  //   {
  //   double *normal = pathNormals->GetTuple(i);
  //   vtkMath::Normalize(normal);
  //   std::cout << std::setw(3) << i << "  "
  //             << std::fixed << std::setprecision(3)
  //             << normal[0] << " " << normal[1] << " " << normal[2]
  //             << "\n";
  //   }

  this->ComputePolyData(path, pathNormals.GetPointer(), output);

  tailPath->Delete();
  headPath->Delete();
  path->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkTeardropSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Resolution: " << this->Resolution << "\n";
  os << indent << "TailHeight: " << this->TailHeight << "\n";
  os << indent << "TipStrength: " << this->TipStrength << "\n";
  os << indent << "HeadStrength: " << this->HeadStrength << "\n";
  os << indent << "HeadRadius: " << this->HeadRadius << "\n";
  os << indent << "FrontSideOnly: " << this->FrontSideOnly << "\n";
  os << indent << "ProjectToXYPlane: " << this->ProjectToXYPlane << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision
     << "\n";
}

//----------------------------------------------------------------------------
void vtkTeardropSource::ComputeTailPath(vtkPoints* tailPath,
  vtkDoubleArray* tailNormals)
{
  tailPath->Reset();
  tailNormals->Reset();
  tailNormals->SetNumberOfComponents(3);

  // Set up control points
  double controlPt[4][2];
  controlPt[0][0] = 0.0;
  controlPt[0][1] = 0.0;

  controlPt[1][0] = this->TipStrength * this->TailHeight;
  controlPt[1][1] = controlPt[0][1];

  controlPt[2][0] = (1.0 - this->HeadStrength) * this->TailHeight;
  controlPt[2][1] = this->HeadRadius;

  controlPt[3][0] = this->TailHeight;
  controlPt[3][1] = this->HeadRadius;

  // First implementation computes evenly-spaced points based on Resolution,
  // HeadRadius, and TailLength.
  double circumference = 2.0 * vtkMath::Pi() * this->HeadRadius;
  double headEdge = circumference / this->Resolution;
  double tailEdge = (this->TailHeight + this->HeadRadius) / headEdge;
  int tailResolution = vtkMath::Ceil(tailEdge);
  vtkDebugMacro("tail resolution: " << tailResolution);

  int numPoints = tailResolution + 1;
  double t;
  double coordinates[3];
  double normal[3];
  for (int i = 0; i < numPoints; i++)
  {
    t = static_cast<double>(i) / tailResolution;
    this->ComputeTailCoordinate(t, controlPt, coordinates, normal);
    tailPath->InsertNextPoint(coordinates);
    tailNormals->InsertNextTuple(normal);
  }
}

//----------------------------------------------------------------------------
void vtkTeardropSource::ComputeHeadPath(vtkPoints* headPath,
  vtkDoubleArray* headNormals)
{
  headPath->Reset();
  headNormals->Reset();
  headNormals->SetNumberOfComponents(3);

  double coordinates[3];
  double normal[3];

  // First point is hard-coded
  coordinates[0] = this->TailHeight;
  coordinates[1] = this->HeadRadius;
  coordinates[2] = 0.0;
  headPath->InsertNextPoint(coordinates);

  normal[0] = 0.0;
  normal[1] = 1.0;
  normal[2] = 0.0;
  headNormals->InsertNextTuple(normal);

  for (int i = 1; i < this->Resolution; i++)
  {
    const double theta =
      0.5 * vtkMath::Pi() * (1.0 - static_cast<double>(i) / this->Resolution);
    const double cosTheta = cos(theta);
    const double sinTheta = sin(theta);

    coordinates[0] = cosTheta * this->HeadRadius + this->TailHeight;
    coordinates[1] = sinTheta * this->HeadRadius;
    headPath->InsertNextPoint(coordinates);

    normal[0] = cosTheta;
    normal[1] = sinTheta;
    headNormals->InsertNextTuple(normal);
  }

  // Last point is hard-coded
  coordinates[0] = this->TailHeight + this->HeadRadius;
  coordinates[1] = 0.0;
  headPath->InsertNextPoint(coordinates);
  normal[0] = 1.0;
  normal[1] = 0.0;
  headNormals->InsertNextTuple(normal);
}

//----------------------------------------------------------------------------
void vtkTeardropSource::ComputePolyData(vtkPoints* path,
  vtkDoubleArray* pathNormals,
  vtkPolyData* output)
{
  int numPathPts = path->GetNumberOfPoints();
  int numOutputPts = 2 + (numPathPts - 2) * this->Resolution;
  int numOutputPolys = 2 * this->Resolution // triangles
    + (numPathPts - 3) * this->Resolution;  // quads

  vtkPoints* outputPts = vtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    outputPts->SetDataType(VTK_DOUBLE);
  }
  else
  {
    outputPts->SetDataType(VTK_FLOAT);
  }

  outputPts->SetNumberOfPoints(numOutputPts);
  vtkDoubleArray* outputNormals = vtkDoubleArray::New();
  outputNormals->SetNumberOfComponents(3);
  outputNormals->SetNumberOfTuples(numOutputPts);

  vtkCellArray* outputPolys = vtkCellArray::New();
  outputPolys->Allocate(outputPolys->EstimateSize(numOutputPolys, 4));

  // Set 1st and last point
  double pathCoords[3];
  double outputCoords[3];
  double pathNormal[3];
  double outputNormal[3];
  vtkIdType ids[4];
  path->GetPoint(0, pathCoords);
  outputPts->SetPoint(0, pathCoords);
  outputNormal[0] = -1.0;
  outputNormal[1] = 0.0;
  outputNormal[2] = 0.0;
  outputNormals->SetTuple(0, outputNormal);

  int lastId = numOutputPts - 1;
  path->GetPoint(path->GetNumberOfPoints() - 1, pathCoords);
  outputPts->SetPoint(lastId, pathCoords);
  //outputPts->Print(std::cout);
  outputNormal[0] = 1.0;
  outputNormals->SetTuple(lastId, outputNormal);

  // Copy points and generate tri-quads-tri
  double theta = 0.0;
  int pointId = 1;
  int firstId = 1;
  int deltaPointIds = path->GetNumberOfPoints() - 2;
  //std::cout << "deltaPointIds: " << deltaPointIds << std::endl;
  double maxAngle = 2.0 * vtkMath::Pi();
  if (this->FrontSideOnly)
  {
    // For FrontSideOnly, increase angle so that 180 degrees is covered
    // by 1 more step than Resolution, because last profile connects to first,
    // which we omit for FrontSideOnly.
    maxAngle = vtkMath::Pi() * static_cast<double>(this->Resolution + 1) /
      static_cast<double>(this->Resolution);
  }

  for (int i = 0; i < this->Resolution; i++)
  {
    theta = i * maxAngle / this->Resolution;
    double cosTheta = cos(theta);
    double sinTheta = sin(theta);

    // Copy points
    for (int j = 1; j <= deltaPointIds; j++)
    {
      path->GetPoint(j, pathCoords);
      outputCoords[0] = pathCoords[0];            // x
      outputCoords[1] = pathCoords[1] * cosTheta; // y
      outputCoords[2] = this->ProjectToXYPlane ?  // z
        0.0
                                               : pathCoords[1] * sinTheta;
      outputPts->SetPoint(pointId, outputCoords);

      pathNormals->GetTuple(j, pathNormal);
      outputNormal[0] = pathNormal[0];
      outputNormal[1] = pathNormal[1] * cosTheta;
      outputNormal[2] = pathNormal[1] * sinTheta;
      vtkMath::Normalize(outputNormal);
      outputNormals->SetTuple(pointId, outputNormal);

      pointId++;
    }

    if (this->FrontSideOnly && i == (this->Resolution - 1))
    {
      // For FrontSideOnly, do not connect last set of points back
      // to first set. Instead break here.
      break;
    }

    // Triangle at bottom
    ids[0] = 0;
    ids[1] = (firstId + deltaPointIds) % (lastId - 1);
    ids[2] = firstId;
    outputPolys->InsertNextCell(3, ids);

    // Quads
    for (int j = 0; j < deltaPointIds - 1; j++)
    {
      ids[0] = firstId + j;
      ids[1] = (ids[0] + deltaPointIds) % (lastId - 1);
      ids[2] = ids[1] + 1;
      ids[3] = ids[0] + 1;
      outputPolys->InsertNextCell(4, ids);
    }

    // Triangle at top
    ids[0] = firstId + deltaPointIds - 1;
    ids[1] = ids[0] + deltaPointIds;
    if (ids[1] > lastId)
    {
      // Need modulus to get last triangle right
      // Need conditional to get next-to-last triangle right
      ids[1] = (ids[0] + deltaPointIds) % (lastId - 1);
    }
    ids[2] = lastId;
    outputPolys->InsertNextCell(3, ids);

    // Update vars
    firstId += deltaPointIds;
  }

  outputPts->Squeeze();
  //outputPts->Print(std::cout);
  output->SetPoints(outputPts);
  outputPts->Delete();

  outputNormals->Squeeze();
  //outputNormals->Print(std::cout);
  output->GetPointData()->SetNormals(outputNormals);
  outputNormals->Delete();

  outputPolys->Squeeze();
  //outputPolys->Print(std::cout);
  output->SetPolys(outputPolys);
  outputPolys->Delete();

  //outputPolys->Print(std::cout);

  // vtkNew<vtkPolyDataWriter> writer;
  // const char *filename = "teardrop.vtk";
  // writer->SetFileName(filename);
  // writer->SetInputData(output);
  // writer->Write();
  // std::cout << "Wrote " << filename << std::endl;

  //std::cout << std::endl;
}

//----------------------------------------------------------------------------
void vtkTeardropSource::ComputeTailCoordinate(double t,
  double controlPt[4][2],
  double coordinates[3],
  double normal[3])
{
  double tm1 = 1.0 - t;
  double tm13 = tm1 * tm1 * tm1;
  double t3 = t * t * t;

  double velocity[2];
  for (int i = 0; i < 2; i++)
  {
    coordinates[i] = tm13 * controlPt[0][i] +
      3 * t * tm1 * tm1 * controlPt[1][i] +
      3.0 * t * t * tm1 * controlPt[2][i] + t3 * controlPt[3][i];

    velocity[i] = 3.0 * tm1 * tm1 * (controlPt[1][i] - controlPt[0][i]) +
      6.0 * t * tm1 * (controlPt[2][i] - controlPt[1][i]) +
      t3 * (controlPt[3][i] - controlPt[2][i]);
  }
  coordinates[2] = 0.0;
  normal[0] = -velocity[1];
  normal[1] = velocity[0];
  normal[2] = 0.0;
}
