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
 * @brief   Extract points based on 3 different criteria; points within
 * bounds, not occluded and not masked.
 *
 * Adds functionality to vtkSelectVisiblePoints to further filter points
 * through a masking array, in addition to z-buffer, viewport bounds, etc.
 *
 * @warning
 * You must carefully synchronize the execution of this filter. The
 * filter refers to a renderer, which is modified every time a render
 * occurs. Therefore, the filter is always out of date, and always
 * executes. You may have to perform two rendering passes, or if you
 * are using this filter in conjunction with vtkLabeledDataMapper,
 * things work out because 2D rendering occurs after the 3D rendering.
 *
 * @sa vtkSelectVisiblePoints::GetMTime
*/

#ifndef vtkMapPointSelection_h
#define vtkMapPointSelection_h
#include <array>
#include <string>

#include "vtkSelectVisiblePoints.h"
#include "vtkmapcore_export.h"

class vtkBitArray;
class vtkMatrix4x4;
class vtkRenderer;

class VTKMAPCORE_EXPORT vtkMapPointSelection : public vtkSelectVisiblePoints
{
public:
  vtkTypeMacro(vtkMapPointSelection, vtkSelectVisiblePoints);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkMapPointSelection* New();

  enum ArrayIndices
  {
    MASK = 0
  };

  enum Coordinates
  {
    WORLD = 0,
    DISPLAY = 1
  };

  //@{
  /**
   * Enables point filtering through a user defined mask array (filters
   * out points with false values in the mask array). Similar behavior
   * to vtkGlyph3DMapper, except that if the array is not found and this
   * mode is enabled the filter throws an error and returns.
   * @sa vtkGlyph3DMapper
   */
  vtkSetMacro(FilterMasked, bool);
  vtkGetMacro(FilterMasked, bool);
  vtkBooleanMacro(FilterMasked, bool);

  /**
   * Set the name of the point array to use as a mask for point selection.
   */
  void SetMaskArray(const std::string& name);
  //@}

  //@{
  /**
   * Enables point filtering through z-occlusion. It compares the point's
   * z-coordinate with its counterpart in the depth-buffer. Unlike its parent
   * class this mode can be disabled.
   */
  vtkSetMacro(FilterOccluded, bool);
  vtkGetMacro(FilterOccluded, bool);
  //@}

  //@{
  /**
   * Set the coordinate system in which the output is. The output datasets
   * may have point coordinates reported in world or screen coordinates.
   */
  vtkGetMacro(CoordinateSystem, int);
  vtkSetClampMacro(CoordinateSystem, int, WORLD, DISPLAY);
  //@}

  //@{
  /**
   * Set a point offset to be applied to all of the output points. The offset
   * is assumed to be in vtkMapPointSelection::CoordinateSystem coordinates.
   */
  vtkSetVector3Macro(PointOffset, double);
  vtkGetVector3Macro(PointOffset, double);
  //@}

protected:
  vtkMapPointSelection() = default;
  ~vtkMapPointSelection() override = default;

  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool InitializeMasking();

  /**
   * Transform point from world to display coordinates.
   */
  bool WorldToDisplay(
    const std::array<double, 4>& pointWorld, std::array<double, 4>& pointDispl);

  //@{
  /**
   * Selection predicates.
   */
  bool IsPointVisible(
    const std::array<double, 4>& point, const vtkIdType& pointId);
  bool IsMasked(const vtkIdType& id) const;
  bool IsWithinBounds(const std::array<double, 4>& point) const;
  bool IsOccluded(const std::array<double, 4>& point) const;
  //@}

  //////////////////////////////////////////////////////////////////////////////

  bool FilterMasked = false;
  bool FilterOccluded = false;
  float* DepthBuffer = nullptr;
  vtkBitArray* MaskArray = nullptr;

  int CoordinateSystem = WORLD;
  double PointOffset[3] = { 0.0, 0.0, 0.0 };

private:
  vtkMapPointSelection(const vtkMapPointSelection&) = delete;
  void operator=(const vtkMapPointSelection&) = delete;
};

#endif // vtkMapPointSelection_h
