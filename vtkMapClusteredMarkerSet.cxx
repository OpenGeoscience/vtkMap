/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapClusteredMarkerSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

   This software is distributed WITHOUT ANY WARRANTY; without even
   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkMapClusteredMarkerSet.h"
#include <vtkIdTypeArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>

#include <algorithm>
#include <set>
#include <vector>

double lat2y(double);
const int NumberOfClusterLevels = 20;

//----------------------------------------------------------------------------
class vtkMapClusteredMarkerSet::MapCluster
{
public:
  //int ClusterId;  // TBD
  double Latitude;
  double Longitude;
  // int ZoomLevel // TBD
  std::set<MapCluster*> ChildrenClusters;
  int NumberOfMarkers;
  int MarkerId;  // only relevant for single-marker clusters
};

//----------------------------------------------------------------------------
class vtkMapClusteredMarkerSet::MapClusteredMarkerSetInternals
{
public:
  bool Changed;
  int ZoomLevel;
  std::vector<MapCluster*> CurrentClusters;  // currently in PolyData
  std::vector<std::set<MapCluster*> > ClusterTable;
  int NumberOfMarkers;
  int NumberOfClusters;
  double ClusterDistance;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkMapClusteredMarkerSet)

//----------------------------------------------------------------------------
vtkMapClusteredMarkerSet::vtkMapClusteredMarkerSet()
{
  this->PolyData = vtkPolyData::New();
  this->Internals = new MapClusteredMarkerSetInternals;
  this->Internals->Changed = false;
  this->Internals->ZoomLevel = -1;
  std::set<MapCluster*> clusterSet;
  std::fill_n(std::back_inserter(this->Internals->ClusterTable),
              NumberOfClusterLevels, clusterSet);
  this->Internals->NumberOfMarkers = 0;
  this->Internals->NumberOfClusters = 0;
  this->Internals->ClusterDistance = 80.0;
}

//----------------------------------------------------------------------------
void vtkMapClusteredMarkerSet::PrintSelf(ostream &os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "vtkMapClusteredMarkerSet" << "\n"
     << indent << "NumberOfClusterLevels: " << NumberOfClusterLevels << "\n"
     << indent << "NumberOfMarkers: "
     << this->Internals->NumberOfMarkers << "\n"
     << indent << "NumberOfClusters: "
     << this->Internals->NumberOfClusters << "\n"
     << std::endl;
}

//----------------------------------------------------------------------------
vtkMapClusteredMarkerSet::~vtkMapClusteredMarkerSet()
{
  this->RemoveMapMarkers();
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkIdType vtkMapClusteredMarkerSet::AddMarker(double latitude,
                                              double longitude)
{
  // Set marker id
  int markerId = this->Internals->NumberOfMarkers++;

  // Create MapCluster for each level of ClusterTable, starting at max level
  double gcsCoords[2];
  double threshold = this->Internals->ClusterDistance;
  for (int i=this->Internals->ClusterTable.size()-1; i>=0; i--)
    {
    // Can the point be clustered at this level?
    gcsCoords[0] = longitude;
    gcsCoords[1] = lat2y(latitude);
    MapCluster *cluster = this->FindClosestCluster(gcsCoords, i, threshold);
    if (cluster)
      {
      // Update cluster
      }
    else
      {
      // Instantiate MapCluster instance
      MapCluster *cluster = new MapCluster;
      cluster->Latitude = latitude;
      cluster->Longitude = longitude;
      cluster->NumberOfMarkers = 1;
      cluster->MarkerId = markerId;
      this->Internals->ClusterTable[i].insert(cluster);
      }
    }

  this->Internals->Changed = true;
  return markerId;
}

//----------------------------------------------------------------------------
void vtkMapClusteredMarkerSet::RemoveMapMarkers()
{
  std::vector<std::set<MapCluster*> >::iterator tableIter =
    this->Internals->ClusterTable.begin();
  for (; tableIter != this->Internals->ClusterTable.end(); tableIter++)
    {
    std::set<MapCluster*> clusterSet = *tableIter;
    std::set<MapCluster*>::iterator clusterIter = clusterSet.begin();
    for (; clusterIter != clusterSet.end(); clusterIter++)
      {
      delete *clusterIter;
      }
    clusterSet.empty();
    }

  this->Internals->CurrentClusters.clear();
  this->Internals->NumberOfMarkers = 0;
  this->Internals->NumberOfClusters = 0;
  this->Internals->Changed = true;
}

//----------------------------------------------------------------------------
void vtkMapClusteredMarkerSet::Update(int zoomLevel)
{
  if (!this->Internals->Changed && (zoomLevel == this->Internals->ZoomLevel))
    {
    return;
    }

  // Setup "MarkerType" point data array (1 component vector)
  vtkUnsignedCharArray *types =
    this->SetupUCharArray(this->PolyData, "MarkerType", 1);

  // Setup "Color" array as point data (vector)
  vtkUnsignedCharArray *colors =
    this->SetupUCharArray(this->PolyData, "Color", 3);
  unsigned char kwBlue[] = {0, 83, 155};
  unsigned char kwGreen[] = {0, 169, 179};

  // Copy cluster info into polydata
  vtkNew<vtkPoints> points;
  double coord[3];
  unsigned char markerType[1];
  markerType[0] = 0;

  this->Internals->CurrentClusters.clear();
  std::set<MapCluster*> clusterSet = this->Internals->ClusterTable[zoomLevel];
  std::set<MapCluster*>::const_iterator iter;
  for (iter = clusterSet.begin(); iter != clusterSet.end(); iter++)
    {
    MapCluster *cluster = *iter;
    coord[0] = cluster->Longitude;
    coord[1] = lat2y(cluster->Latitude);
    coord[2] = 0.0;
    points->InsertNextPoint(coord);
    this->Internals->CurrentClusters.push_back(cluster);
    if (cluster->NumberOfMarkers == 1)  // point marker
      {
      markerType[0] = 0;
      types->InsertNextTupleValue(markerType);
      colors->InsertNextTupleValue(kwBlue);
      }
    else  // cluster marker
      {
      markerType[0] = 1;
      types->InsertNextTupleValue(markerType);
      colors->InsertNextTupleValue(kwGreen);
      }
    }
  this->PolyData->Reset();
  this->PolyData->SetPoints(points.GetPointer());

  //std::cout << "Marker pts " << markerPoints->GetNumberOfPoints() << std::endl;
  //std::cout << "Cluster pts " << clusterPoints->GetNumberOfPoints() << std::endl;

  this->Internals->Changed = false;
  this->Internals->ZoomLevel = zoomLevel;
}

//----------------------------------------------------------------------------
int vtkMapClusteredMarkerSet::GetMarkerType(int glyphNumber)
{
  if ((glyphNumber < 0) ||
      (glyphNumber >= this->Internals->CurrentClusters.size()))
    {
    vtkWarningMacro("Invalid glyph number " << glyphNumber);
    return -1;
    }
  MapCluster *cluster = this->Internals->CurrentClusters[glyphNumber];
  int markerType = cluster->NumberOfMarkers > 1 ? 1 : 0;
  return markerType;
}

//----------------------------------------------------------------------------
int vtkMapClusteredMarkerSet::GetMarkerId(int glyphNumber)
{
  if ((glyphNumber < 0) ||
      (glyphNumber >= this->Internals->CurrentClusters.size()))
    {
    vtkWarningMacro("Invalid glyph number " << glyphNumber);
    return -1;
    }
  MapCluster *cluster = this->Internals->CurrentClusters[glyphNumber];
  return cluster->MarkerId;
}

//----------------------------------------------------------------------------
vtkMapClusteredMarkerSet::MapCluster*
vtkMapClusteredMarkerSet::
FindClosestCluster(double gcsCoords[2], int zoomLevel, double distanceThreshold)
{
  // MapCluster *cluster = new MapCluster;
  // return cluster;
  return NULL;
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray *
vtkMapClusteredMarkerSet::
SetupUCharArray(vtkPolyData *polyData, const char *name, int numberOfComponents)
{
  // Check for array in point data
  if (!polyData->GetPointData()->HasArray(name))
    {
    vtkNew<vtkUnsignedCharArray> newArray;
    newArray->SetName(name);
    newArray->SetNumberOfComponents(numberOfComponents);
    polyData->GetPointData()->AddArray(newArray.GetPointer());
    }
  vtkDataArray *data = polyData->GetPointData()->GetArray(name);
  vtkUnsignedCharArray *array = vtkUnsignedCharArray::SafeDownCast(data);
  array->Reset();
  return array;
}

