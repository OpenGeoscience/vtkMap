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
  this->Clustering = false;

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
     << indent << "Clustering: " << this->Clustering << "\n"
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

  // todo calc initial cluster distance here and divide down
  if (this->Clustering)
    {
    // Populate MarkerTable
    int level = this->Internals->MarkerTable.size() - 1;
    this->Internals->MarkerTable[level].insert(marker);

    level--;
    double threshold = this->Internals->ClusterDistance;
    for (; level >= 0; level--)
      {
      MapMarker *closest =
        this->FindClosestMarker(marker->gcsCoords, level, threshold);
    if (closest)
      {
      // Recursively merge cluster into closest
      this->MergeClusterData(marker, closest, level);
      //vtkDebugMacro("Level " << level << " MERGE, # pts " << cluster->NumberOfMarkers);
      break;
      }
    else
      {
      // Copy cluster and insert at this level
      MapMarker *newCluster = new MapMarker;
      newCluster->Latitude = marker->Latitude;
      newCluster->Longitude = marker->Longitude;
      newCluster->gcsCoords[0] = marker->gcsCoords[0];
      newCluster->gcsCoords[1] = marker->gcsCoords[1];
      newCluster->NumberOfMarkers = marker->NumberOfMarkers;
      newCluster->MarkerId = marker->MarkerId;
      newCluster->Parent = marker->Parent;  // really?
      this->Internals->MarkerTable[level].insert(newCluster);

      marker->Parent = newCluster;
      marker = newCluster;
      vtkDebugMacro("Level " << level << " COPY, # pts " << marker->NumberOfMarkers);
      }
    }


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
    this->Initialized = true;
    }

  // Clip zoom level to size of cluster table
  if (zoomLevel >= NumberOfClusterLevels)
    {
    zoomLevel = NumberOfClusterLevels - 1;
    }

  // If not clustering, only update if markers have changed
  if (!this->Clustering && !this->Internals->MarkersChanged)
    {
    return;
    }

  // If clustering, only update if either zoom or markers changed
  if (this->Clustering && !this->Internals->MarkersChanged &&
      (zoomLevel == this->Internals->ZoomLevel))
    {
    return;
    }

  // In non-clustering mode, markers stored at level 0
  if (!this->Clustering)
    {
    zoomLevel = 0;
    }

  // Copy marker info into polydata
  vtkNew<vtkPoints> points;
  unsigned char markerType;  // 0 == point marker, 1 == cluster marker
  markerType = 0;

  // Get pointers to data arrays
  vtkDataArray *array;
  array = this->PolyData->GetPointData()->GetArray("Color");
  vtkUnsignedCharArray *colors = vtkUnsignedCharArray::SafeDownCast(array);
  colors->Reset();
  array = this->PolyData->GetPointData()->GetArray("MarkerType");
  vtkUnsignedCharArray *types = vtkUnsignedCharArray::SafeDownCast(array);
  types->Reset();
  array = this->PolyData->GetPointData()->GetArray("MarkerCount");
  vtkUnsignedIntArray *counts = vtkUnsignedIntArray::SafeDownCast(array);
  counts->Reset();

  unsigned char kwBlue[] = {0, 83, 155};
  unsigned char kwGreen[] = {0, 169, 179};

  this->Internals->CurrentMarkers.clear();
  std::set<MapMarker*> markerSet = this->Internals->MarkerTable[zoomLevel];
  std::set<MapMarker*>::const_iterator iter;
  for (iter = markerSet.begin(); iter != markerSet.end(); iter++)
    {
    MapMarker *marker = *iter;
    points->InsertNextPoint(marker->gcsCoords);
    this->Internals->CurrentMarkers.push_back(marker);
    if (marker->NumberOfMarkers == 1)  // point marker
      {
      markerType = 0;
      types->InsertNextValue(markerType);
      colors->InsertNextTupleValue(kwBlue);
      counts->InsertNextValue(1);
      }
    else  // cluster marker
      {
      markerType = 1;
      types->InsertNextValue(markerType);
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
vtkMapMarkerSet::MapMarker*
vtkMapMarkerSet::
FindClosestMarker(double gcsCoords[2], int zoomLevel, double distanceThreshold)
{
  // Convert distanceThreshold from image to gcs coords
  double level0Scale = 360.0 / 256.0;  // 360 degress <==> 256 tile pixels
  double scale = level0Scale / static_cast<double>(1<<zoomLevel);
  double gcsThreshold = scale * distanceThreshold;
  double gcsThreshold2 = gcsThreshold * gcsThreshold;

  MapMarker *closestCluster = NULL;
  double closestDistance2 = gcsThreshold2;
  std::set<MapMarker*> clusterSet = this->Internals->MarkerTable[zoomLevel];
  std::set<MapMarker*>::const_iterator setIter = clusterSet.begin();
  for (; setIter != clusterSet.end(); setIter++)
    {
    MapMarker *cluster = *setIter;
    double d2 = 0.0;
    for (int i=0; i<2; i++)
      {
      double d1 = gcsCoords[i] - cluster->gcsCoords[i];
      d2 += d1 * d1;
      }
    if (d2 < closestDistance2)
      {
      closestCluster = cluster;
      closestDistance2 = d2;
      }
    }

  return closestCluster;
}

//----------------------------------------------------------------------------
void
vtkMapMarkerSet::
MergeClusterData(vtkMapMarkerSet::MapMarker* src,
                 vtkMapMarkerSet::MapMarker* dest,
                 int level)
{
  if (!src || !dest)
    {
    return;
    }

  // Recursion also ends when pointers are the same
  if (src == dest)
    {
    return;
    }

  // Update gcsCoords as weighted average
  double denominator = src->NumberOfMarkers + dest->NumberOfMarkers;
  for (int i=0; i<2; i++)
    {
    double numerator = src->gcsCoords[i] * src->NumberOfMarkers +
          dest->gcsCoords[i] * dest->NumberOfMarkers;
    dest->gcsCoords[i] = numerator/denominator;
    }
  dest->Longitude = dest->gcsCoords[0];
  dest->Latitude = y2lat(dest->gcsCoords[1]);
  dest->NumberOfMarkers += src->NumberOfMarkers;
  dest->MarkerId = -1;

  // Check if src in the current level
  std::set<MapMarker*> clusterSet = this->Internals->MarkerTable[level];
  std::set<MapMarker*>::iterator iter = clusterSet.find(src);
  if (iter == clusterSet.end())
    {
    // Merge the point
    this->MergeClusterData(src, dest->Parent, level-1);
    }
  else
    {
    // Merge src into dest's parent cluster
    this->MergeClusterData(src->Parent, dest->Parent, level-1);
    clusterSet.erase(iter);
    delete *iter;
    }

}
