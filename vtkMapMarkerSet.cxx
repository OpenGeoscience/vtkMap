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
#include "vtkMapPickResult.h"
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
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>

#include <algorithm>
#include <set>
#include <vector>

double lat2y(double);
double y2lat(double);
const int NumberOfClusterLevels = 20;

//----------------------------------------------------------------------------
class vtkMapMarkerSet::MapMarker
{
public:
  //int ClusterId;  // TBD
  double Latitude;
  double Longitude;
  double gcsCoords[2];
  // int ZoomLevel // TBD
  MapMarker *Parent;
  int NumberOfMarkers;  // 1 for single-point markers, >1 for clusters
  int MarkerId;  // only relevant for single-point markers (not clusters)
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMapMarkerSet)

//----------------------------------------------------------------------------
class vtkMapMarkerSet::MapMarkerSetInternals
{
public:
  bool MarkersChanged;
  std::vector<MapMarker*> CurrentMarkers;  // currently in PolyData

  // Used for marker clustering:
  int ZoomLevel;
  std::vector<std::set<MapMarker*> > MarkerTable;
  int NumberOfMarkers;
  int NumberOfClusters;
  double ClusterDistance;
};

//----------------------------------------------------------------------------
vtkMapMarkerSet::vtkMapMarkerSet()
{
  this->Initialized = false;
  this->Renderer = NULL;
  this->PolyData = vtkPolyData::New();
  this->Mapper = NULL;
  this->Actor = NULL;
  this->MarkerClustering = false;

  this->Internals = new MapMarkerSetInternals;
  this->Internals->MarkersChanged = false;
  this->Internals->ZoomLevel = -1;
  std::set<MapMarker*> clusterSet;
  std::fill_n(std::back_inserter(this->Internals->MarkerTable),
              NumberOfClusterLevels, clusterSet);
  this->Internals->NumberOfMarkers = 0;
  this->Internals->NumberOfClusters = 0;
  this->Internals->ClusterDistance = 80.0;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << this->GetClassName() << "\n"
     << indent << "Initialized: " << this->Initialized << "\n"
     << indent << "MarkerClustering: " << this->MarkerClustering << "\n"
    //<< indent << ": " << << "\n"
     << std::endl;
}

//----------------------------------------------------------------------------
vtkMapMarkerSet::~vtkMapMarkerSet()
{
  if (this->PolyData)
    {
    this->PolyData->Delete();
    }
  if (this->Mapper)
    {
    this->Mapper->Delete();
    }
  if (this->Actor)
    {
    this->Actor->Delete();
    }
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkIdType vtkMapMarkerSet::AddMarker(double latitude, double longitude)
{
  // Set marker id
  int markerId = this->Internals->NumberOfMarkers++;
  vtkDebugMacro("Adding marker " << markerId);

  // Instantiate MapMarker
  MapMarker *marker = new MapMarker;
  marker->Latitude = latitude;
  marker->Longitude = longitude;
  marker->gcsCoords[0] = longitude;
  marker->gcsCoords[1] = lat2y(latitude);
  marker->NumberOfMarkers = 1;
  marker->Parent = 0;
  marker->MarkerId = markerId;

  if (this->MarkerClustering)
    {
    int level = this->Internals->MarkerTable.size() - 1;
    this->Internals->MarkerTable[level].insert(marker);
    }
  else
    {
    // When not clustering, use table row 0
    this->Internals->MarkerTable[0].insert(marker);
    }

  this->Internals->MarkersChanged = true;
  return markerId;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::RemoveMarkers()
{
  // Explicitly delete MapMarker instances in the table
  std::vector<std::set<MapMarker*> >::iterator tableIter =
    this->Internals->MarkerTable.begin();
  for (; tableIter != this->Internals->MarkerTable.end(); tableIter++)
    {
    std::set<MapMarker*> markerSet = *tableIter;
    std::set<MapMarker*>::iterator markerIter = markerSet.begin();
    for (; markerIter != markerSet.end(); markerIter++)
      {
      delete *markerIter;
      }
    markerSet.clear();
    tableIter->operator=(markerSet);
    }

  this->Internals->CurrentMarkers.clear();
  this->Internals->NumberOfMarkers = 0;
  this->Internals->NumberOfClusters = 0;
  this->Internals->MarkersChanged = true;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::Update(int zoomLevel)
{
  // Make sure everything is initialized
  if (!this->Initialized && this->Renderer)
    {
    this->InitializeRenderingPipeline();
    if (this->MarkerClustering)
      {
      this->InitializeMarkerClustering();
      }
    this->Initialized = true;
    }

  if (!this->MarkerClustering)
    {
    zoomLevel = 0;
    }

  // Update polydata
  unsigned char kwBlue[] = {0, 83, 155};
  unsigned char kwGreen[] = {0, 169, 179};

  // Copy marker info into polydata
  vtkNew<vtkPoints> points;
  double coord[3];
  unsigned char markerType[1];
  markerType[0] = 0;

  // Get pointers to data arrays
  vtkDataArray *array;
  array = this->PolyData->GetPointData()->GetArray("Color");
  vtkUnsignedCharArray *colors = vtkUnsignedCharArray::SafeDownCast(array);
  array = this->PolyData->GetPointData()->GetArray("MarkerType");
  vtkUnsignedCharArray *types = vtkUnsignedCharArray::SafeDownCast(array);
  array = this->PolyData->GetPointData()->GetArray("MarkerCount");
  vtkUnsignedIntArray *counts = vtkUnsignedIntArray::SafeDownCast(array);

  this->Internals->CurrentMarkers.clear();
  std::set<MapMarker*> markerSet = this->Internals->MarkerTable[zoomLevel];
  std::set<MapMarker*>::const_iterator iter;
  for (iter = markerSet.begin(); iter != markerSet.end(); iter++)
    {
    MapMarker *marker = *iter;
    coord[0] = marker->Longitude;
    coord[1] = lat2y(marker->Latitude);
    coord[2] = 0.0;
    points->InsertNextPoint(coord);
    this->Internals->CurrentMarkers.push_back(marker);
    if (marker->NumberOfMarkers == 1)  // point marker
      {
      markerType[0] = 0;
      types->InsertNextTupleValue(markerType);
      colors->InsertNextTupleValue(kwBlue);
      counts->InsertNextValue(1);
      }
    else  // cluster marker
      {
      markerType[0] = 1;
      types->InsertNextTupleValue(markerType);
      colors->InsertNextTupleValue(kwGreen);
      counts->InsertNextValue(marker->NumberOfMarkers);
      }
    }
  this->PolyData->Reset();
  this->PolyData->SetPoints(points.GetPointer());

  this->Internals->MarkersChanged = false;
  this->Internals->ZoomLevel = zoomLevel;
}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::
PickPoint(vtkRenderer *renderer, vtkPicker *picker, int displayCoords[2],
    vtkMapPickResult *result)
{
  result->SetDisplayCoordinates(displayCoords);
  result->SetMapLayer(0);
  // TODO Need display <--> gcs coords
  result->SetMapFeatureType(VTK_MAP_FEATURE_NONE);
  result->SetNumberOfMarkers(0);
  result->SetMapFeatureId(-1);

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

        MapMarker *marker = this->Internals->CurrentMarkers[glyphId];
        // std::cout << "Marker id " << marker->MarkerId
        //           << ", Count " << marker->NumberOfMarkers
        //           << ", at " << marker->Latitude << ", " << marker->Longitude
        //           << std::endl;
        result->SetNumberOfMarkers(marker->NumberOfMarkers);
        if (marker->NumberOfMarkers == 1)
          {
          result->SetMapFeatureType(VTK_MAP_FEATURE_MARKER);
          result->SetMapFeatureId(marker->MarkerId);
          }
        else if (marker->NumberOfMarkers > 1)
          {
          result->SetMapFeatureType(VTK_MAP_FEATURE_CLUSTER);
          }
        //result->Print(std::cout);
        }
      }
    }

}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::InitializeRenderingPipeline()
{

  // Add "Color" data array to polydata
  const char *colorName = "Color";
  vtkNew<vtkUnsignedCharArray> colors;
  colors->SetName(colorName);
  colors->SetNumberOfComponents(3);  // for RGB
  this->PolyData->GetPointData()->AddArray(colors.GetPointer());

  // Add "MarkerType" array to polydata - to select glyph
  const char *typeName = "MarkerType";
  vtkNew<vtkUnsignedCharArray> types;
  types->SetName(typeName);
  types->SetNumberOfComponents(1);
  this->PolyData->GetPointData()->AddArray(types.GetPointer());

  // Add "MarkerCount" array to polydata - to scale cluster glyph size
  const char *countName = "MarkerCount";
  vtkNew<vtkUnsignedIntArray> counts;
  counts->SetName(countName);
  counts->SetNumberOfComponents(1);
  this->PolyData->GetPointData()->AddArray(counts.GetPointer());

  // Use DistanceToCamera filter to scale markers to constant screen size
  vtkNew<vtkDistanceToCamera> dFilter;
  dFilter->SetScreenSize(50.0);
  dFilter->SetRenderer(this->Renderer);
  dFilter->SetInputData(this->PolyData);

  // Todo Use ProgrammableAttributeDataFilter to scale cluster glyphs

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
void vtkMapMarkerSet::InitializeMarkerClustering()
{

}

//----------------------------------------------------------------------------
void vtkMapMarkerSet::UpdateMarkerClustering(int zoomLevel)
{

}
