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
/**
 * @class   vtkMapPointSelection
 * @brief   Extract points that are visible.
 *
 * Adds functionality to vtkSelectVisiblePoints to further filter points
 * through a masking array, in addition to  z-buffer, viewport, etc.
 *
 * @warning
 * You must carefully synchronize the execution of this filter. The
 * filter refers to a renderer, which is modified every time a render
 * occurs. Therefore, the filter is always out of date, and always
 * executes. You may have to perform two rendering passes, or if you
 * are using this filter in conjunction with vtkLabeledDataMapper,
 * things work out because 2D rendering occurs after the 3D rendering.
 *
 * @sa
 * vtkSelectVisiblePoints
*/

#ifndef vtkMapPointSelection_h
#define vtkMapPointSelection_h
#include <array>
#include <string>

#include "vtkmapcore_export.h"
#include "vtkSelectVisiblePoints.h"

class vtkBitArray;
class vtkMatrix4x4;
class vtkRenderer;

class VTKMAPCORE_EXPORT vtkMapPointSelection :
  public vtkSelectVisiblePoints
{
public:
  vtkTypeMacro(vtkMapPointSelection, vtkSelectVisiblePoints);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkMapPointSelection *New();

  enum ArrayIndices { MASK = 0 };

  //@{
  /**
   * Tells the mapper to filter input points that have false values
   * in the mask array. If there is no mask array (id access mode is set
   * and there is no such id, or array name access mode is set and
   * the there is no such name), masking is silently ignored.
   * A mask array is a vtkBitArray with only one component.
   * Initial value is false.
   */
  vtkSetMacro(FilterMasked, bool);
  vtkGetMacro(FilterMasked, bool);
  vtkBooleanMacro(FilterMasked, bool);

  vtkSetMacro(FilterOccluded, bool);
  vtkGetMacro(FilterOccluded, bool);
  //@}

  /**
   * Set the name of the point array to use as a mask for point selection.
   */
  void SetMaskArray(const std::string& name);

  vtkMTimeType GetMTime() override;

protected:
  vtkMapPointSelection() = default;
  ~vtkMapPointSelection() override = default;

  int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *) override;

  bool InitializeMasking();

  bool IsPointVisible(const std::array<double, 4>& point,
    const vtkIdType& pointId);
  bool IsMasked(const vtkIdType& id) const;
  bool IsWithinBounds(const std::array<double, 3>& point) const;
  bool IsOccluded(const std::array<double, 3>& point) const;

  //////////////////////////////////////////////////////////////////////////////
  bool FilterMasked = false;
  bool FilterOccluded = false;
  float* DepthBuffer = nullptr;
  vtkBitArray* MaskArray = nullptr;

private:
  vtkMapPointSelection(const vtkMapPointSelection&) = delete;
  void operator=(const vtkMapPointSelection&) = delete;
};

#endif // vtkMapPointSelection_h
