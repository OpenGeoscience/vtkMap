/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapMarkerSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMapMarkerSet.h"
#include "vtkMapClusteredMarkerSet.h"
#include "vtkTeardropSource.h"
#include <vtkActor.h>
#include <vtkDataArray.h>
#include <vtkDistanceToCamera.h>
#include <vtkIdTypeArray.h>
#include <vtkGlyph3D.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPointPicker.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

double lat2y(double);

vtkStandardNewMacro(vtkMapMarkerSet)

//----------------------------------------------------------------------------
vtkMapMarkerSet::vtkMapMarkerSet()
{
  this->Renderer = NULL;
  this->MarkerSet = vtkMapClusteredMarkerSet::New();
  //this->MarkerSet->DebugOn();
  this->Mapper = NULL;
  this->Actor = NULL;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkMapMarkerSet::~vtkMapMarkerSet()
{
  if (this->MarkerSet)
    {
    MarkerSet->Delete();
    }
  if (this->Mapper)
    {
    this->Mapper->Delete();
    }
  if (this->Actor)
    {
    this->Actor->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::SetRenderer(vtkRenderer *renderer)
{
  this->Renderer = renderer;

  //
  // Setup the marker-glyph pipeline
  //

  // Use DistanceToCamera filter to scale markers to constant screen size
  vtkNew<vtkDistanceToCamera> dFilter;
  dFilter->SetScreenSize(50.0);
  dFilter->SetRenderer(this->Renderer);
  dFilter->SetInputData(this->MarkerSet->GetPolyData());

  // Use teardrop shape for individual markers
  vtkNew<vtkTeardropSource> markerGlyphSource;
  // Rotate to point downward (parallel to y axis)
  vtkNew<vtkTransformFilter> rotateMarker;
  rotateMarker->SetInputConnection(markerGlyphSource->GetOutputPort());
  vtkNew<vtkTransform> transform;
  transform->RotateZ(90.0);
  rotateMarker->SetTransform(transform.GetPointer());

  // Use sphere for cluster markers
  vtkNew<vtkSphereSource> clusterGlyphSource;
  clusterGlyphSource->SetPhiResolution(12);
  clusterGlyphSource->SetThetaResolution(12);
  clusterGlyphSource->SetRadius(0.25);

  // Setup glyph
  vtkNew<vtkGlyph3D> glyph;
  glyph->SetSourceConnection(0, rotateMarker->GetOutputPort());
  glyph->SetSourceConnection(1, clusterGlyphSource->GetOutputPort());
  glyph->SetInputConnection(dFilter->GetOutputPort());
  glyph->SetIndexModeToVector();
  glyph->ScalingOn();
  glyph->SetScaleFactor(1.0);
  glyph->SetScaleModeToScaleByScalar();
  glyph->SetColorModeToColorByScalar();
  // Just gotta know this:
  glyph->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "DistanceToCamera");
  glyph->SetInputArrayToProcess(
    1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "MarkerType");
  glyph->SetInputArrayToProcess(
    3, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Color");
  glyph->GeneratePointIdsOn();


  // Setup mapper and actor
  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(glyph->GetOutputPort());
  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Renderer->AddActor(this->Actor);
}

//----------------------------------------------------------------------------
vtkIdType vtkMapMarkerSet::AddMarker(double Latitude, double Longitude)
{
  vtkIdType id = this->MarkerSet->AddMarker(Latitude, Longitude);
  return id;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::RemoveMapMarkers()
{
  this->MarkerSet->RemoveMapMarkers();
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::Update(int zoomLevel)
{
  this->MarkerSet->Update(zoomLevel);
}

//----------------------------------------------------------------------------
vtkIdType vtkMapMarkerSet::
PickMarker(vtkRenderer *renderer, vtkPicker *picker, int displayCoords[2])
{
  vtkIdType markerId = -1;  // return value

  vtkPointPicker *pointPicker = vtkPointPicker::SafeDownCast(picker);
  if (pointPicker &&
      (pointPicker->Pick(displayCoords[0], displayCoords[1], 0.0, renderer)))
    {
    // std::cout << "Using tolerance " << pointPicker->GetTolerance() << std::endl;
    // vtkPoints *pickPoints = pointPicker->GetPickedPositions();
    // std::cout << "Picked " << pickPoints->GetNumberOfPoints() << " points "
    //           << std::endl;

    vtkIdType pointId = pointPicker->GetPointId();
    if (pointId > 0)
      {
      vtkDataArray *dataArray =
        pointPicker->GetDataSet()->GetPointData()->GetArray("InputPointIds");
      vtkIdTypeArray *glyphIdArray = vtkIdTypeArray::SafeDownCast(dataArray);
      if (glyphIdArray)
        {
        int glyphId = glyphIdArray->GetValue(pointId);
        // std::cout << "Point id " << pointId
        //           << " - Data " << glyphId << std::endl;
        //int markerType = this->MarkerSet->GetMarkerType(glyphId);
        markerId = this->MarkerSet->GetMarkerId(glyphId);
        // std::cout << "Marker type " << markerType
        //           << ", MarkerId " << markerId
        //           << std::endl;
        }
      }
    }

  return markerId;
}
