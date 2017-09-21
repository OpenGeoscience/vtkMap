/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapPointSelection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMapPointSelection.h"

#include <vtkBitArray.h>
#include <vtkCamera.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>

vtkStandardNewMacro(vtkMapPointSelection)

  void vtkMapPointSelection::SetMaskArray(const std::string& name)
{
  this->SetInputArrayToProcess(vtkMapPointSelection::MASK, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_POINTS, name.c_str());
  this->FilterMasked = true;
  this->Modified();
}

int vtkMapPointSelection::RequestData(vtkInformation*,
  vtkInformationVector** inputVec, vtkInformationVector* outputVec)
{
  // Get input
  auto inInfo = inputVec[0]->GetInformationObject(0);
  auto input =
    vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    return 1;
  }

  if (!this->Renderer || !this->Renderer->GetRenderWindow())
  {
    vtkErrorMacro(<< "Invalid vtkRenderer or RenderWindow!");
    return 0;
  }

  // This will trigger if you do something like ResetCamera before the Renderer
  // or RenderWindow have allocated their appropriate system resources (like
  // creating an OpenGL context)." Resource allocation must occur before the
  // depth buffer could be accessed.
  if (this->Renderer->GetRenderWindow()->GetNeverRendered())
  {
    vtkDebugMacro(<< "RenderWindow is not initialized!");
    return 1;
  }

  vtkCamera* cam = this->Renderer->GetActiveCamera();
  if (!cam)
  {
    return 1;
  }

  // Get output
  auto outInfo = outputVec->GetInformationObject(0);
  auto output =
    vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  auto inPD = input->GetPointData();
  auto outPD = output->GetPointData();
  vtkNew<vtkPoints> outPts;
  outPts->Allocate(numPts / 2 + 1);
  outPD->CopyAllocate(inPD);

  auto outputVertices = vtkCellArray::New();
  output->SetVerts(outputVertices);
  outputVertices->Delete();

  // Initialize viewport bounds and depth buffer (if needed), masking, etc.
  this->DepthBuffer = this->Initialize(this->FilterOccluded);
  if (this->FilterMasked)
    if (!this->InitializeMasking())
      return 0;

  int abort = 0;
  std::array<double, 4> point = { 0.0, 0.0, 0.0, 1.0 };
  vtkIdType progressInterval = numPts / 20 + 1;
  for (vtkIdType cellId = -1, ptId = 0; ptId < numPts && !abort; ptId++)
  {
    // Update progress
    if (!(ptId % progressInterval))
    {
      this->UpdateProgress(static_cast<double>(ptId) / numPts);
      abort = this->GetAbortExecute();
    }

    input->GetPoint(ptId, point.data());

    int visible = 0;
    std::array<double, 4> pointDisplay = { 0.0, 0.0, 0.0, 1.0 };
    if (this->WorldToDisplay(point, pointDisplay))
      visible = this->IsPointVisible(pointDisplay, ptId);

    if ((visible && !this->SelectInvisible) ||
      (!visible && this->SelectInvisible))
    {
      // Add point to output
      std::array<double, 4> outputPoint = { 0.0, 0.0, 0.0, 1.0 };
      switch (this->CoordinateSystem)
      {
        case DISPLAY:
          outputPoint = std::move(pointDisplay);
          break;
        case WORLD:
          outputPoint = std::move(point);
          break;
      }

      // Apply offset
      outputPoint[0] += PointOffset[0];
      outputPoint[1] += PointOffset[1];
      outputPoint[1] += PointOffset[2];

      cellId = outPts->InsertNextPoint(outputPoint.data());
      output->InsertNextCell(VTK_VERTEX, 1, &cellId);
      outPD->CopyData(inPD, ptId, cellId);
    }
  }

  if (this->FilterOccluded)
  {
    delete[] this->DepthBuffer;
    this->DepthBuffer = nullptr;
  }

  output->SetPoints(outPts.GetPointer());
  output->Squeeze();
  return 1;
}

bool vtkMapPointSelection::InitializeMasking()
{
  auto dataObj = this->GetInputDataObject(0, 0);
  int assoc = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  auto arr =
    this->GetInputArrayToProcess(vtkMapPointSelection::MASK, dataObj, assoc);

  this->MaskArray = vtkArrayDownCast<vtkBitArray>(arr);
  if (!this->MaskArray)
  {
    vtkErrorMacro(<< "Masking is enabled but there is no mask array.");
    return false;
  }
  else
  {
    if (this->MaskArray->GetNumberOfComponents() != 1)
    {
      vtkErrorMacro("Expecting a mask array with one component, getting "
        << this->MaskArray->GetNumberOfComponents() << " components.");
      return false;
    }
  }
  return true;
}

bool vtkMapPointSelection::WorldToDisplay(
  const std::array<double, 4>& pointWorld, std::array<double, 4>& pointDispl)
{
  std::array<double, 4> pointView;
  this->CompositePerspectiveTransform->MultiplyPoint(
    pointWorld.data(), pointView.data());

  // Perspective division
  if (pointView[3] == 0.0)
    return false;

  this->Renderer->SetViewPoint(pointView[0] / pointView[3],
    pointView[1] / pointView[3], pointView[2] / pointView[3]);
  this->Renderer->ViewToDisplay();

  this->Renderer->GetDisplayPoint(pointDispl.data());
  return true;
}

bool vtkMapPointSelection::IsPointVisible(
  const std::array<double, 4>& point, const vtkIdType& pointId)
{
  bool success = true;
  success &= IsWithinBounds(point);

  if (this->FilterMasked)
    success &= !IsMasked(pointId);

  if (this->FilterOccluded)
    success &= !IsOccluded(point);

  return success;
}

bool vtkMapPointSelection::IsWithinBounds(
  const std::array<double, 4>& point) const
{
  return (point[0] >= this->InternalSelection[0] &&
    point[0] <= this->InternalSelection[1] &&
    point[1] >= this->InternalSelection[2] &&
    point[1] <= this->InternalSelection[3]);
}

bool vtkMapPointSelection::IsMasked(const vtkIdType& id) const
{
  return (this->MaskArray->GetValue(id) == 0);
}

bool vtkMapPointSelection::IsOccluded(const std::array<double, 4>& point) const
{
  double depth;
  if (this->DepthBuffer != nullptr)
  {
    // Access the value from the captured zbuffer.  Note, we only
    // captured a portion of the zbuffer, so we need to offset dx by
    // the selection window.
    const size_t index = static_cast<size_t>(static_cast<int>(point[0]) -
      this->InternalSelection[0] +
      (static_cast<int>(point[1]) - this->InternalSelection[2]) *
        (this->InternalSelection[1] - this->InternalSelection[0] + 1));
    depth = this->DepthBuffer[index];
  }
  else
  {
    depth = this->Renderer->GetZ(
      static_cast<int>(point[0]), static_cast<int>(point[1]));
  }

  if (point[2] < (depth + this->Tolerance))
  {
    return true;
  }
  return false;
}

void vtkMapPointSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
